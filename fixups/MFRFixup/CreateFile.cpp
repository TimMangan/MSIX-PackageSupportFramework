//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "Detect_Pipe.h"


HANDLE  WRAPPER_CREATEFILE(std::wstring theDestinationFile,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile, 
    DWORD dllInstance, bool debug)
{
    HANDLE retfinal;
    std::wstring LongDestinationFile = MakeLongPath(theDestinationFile);
    retfinal = impl::CreateFileW(LongDestinationFile.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);

    if (debug)
    {
        if (retfinal == INVALID_HANDLE_VALUE)
        {
            Log(L"[%d] WRAPPER_CREATEFILE returns FAILURE 0x%x on file '%s'", dllInstance, GetLastError(), LongDestinationFile.c_str());
        }
        else
        {
            Log(L"[%d] WRAPPER_CREATEFILE returns handle 0x%x on file '%s'", dllInstance, retfinal, LongDestinationFile.c_str());
        }
    }
    return retfinal;  
}


template <typename CharT>
HANDLE __stdcall CreateFileFixup(_In_ const CharT* pathName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    HANDLE retfinal;

    try
    {
        if (guard)
        {
            dllInstance = ++g_InterceptInstance;
            std::wstring wPathName = widen(pathName);
            wPathName = AdjustSlashes(wPathName);
            wPathName = AdjustLocalPipeName(wPathName);

#if _DEBUG
            LogString(dllInstance, L"CreateFileFixup for path", pathName);
#if MOREDEBUG
            Log(L"[%d]        DesiredAccess %s", dllInstance, Log_DesiredAccess(desiredAccess).c_str());
            Log(L"[%d]        ShareMode %s", dllInstance, Log_ShareMode(shareMode).c_str());
            Log(L"[%d]        creationDisposition %s", dllInstance, Log_CreationDisposition(creationDisposition).c_str());
            Log(L"[%d]        flagsAndAttributes %s", dllInstance,  Log_FlagsAndAttributes(flagsAndAttributes).c_str());
#endif
#endif
            bool IsAWriteCase = IsCreateForChange(desiredAccess, creationDisposition, flagsAndAttributes);
            bool IsADirectoryCase = IsCreateForDirectory(desiredAccess, creationDisposition, flagsAndAttributes);

#if NOTOBSOLETE
            if (!IsAWriteCase && !IsADirectoryCase)
            {
                // Windows Forms apps can use System.Configuration to store settings in their exe.Config file.  The Save method ends up making calls to
                // System.Security.AccessControl.FileSecurity to change the file attributes and if this is a package file it will cause an exception.
                // An example of this is the application mRemoteNG.  We can avoid this by detecting the file at opening and make it do a copy to start with.
                if (IsSpecialCaseforChange(wPathName))
                {
                    IsAWriteCase = true;
                }
            }
#endif

#if MOREDEBUG
            Log(L"[%d] CreateFileFixup: Could be a write operation=%d", dllInstance, IsAWriteCase);
            Log(L"[%d] CreateFileFixup: Is a directory operation=%d", dllInstance, IsADirectoryCase);
#endif

            // This get is may or may not be a write operation.
            // There may be a need to COW, and may need to create parent folders in redirection area first.
            Cohorts cohorts;
            DetermineCohorts(wPathName, &cohorts, moredebug, dllInstance, L"CreateFileFixup");
#if MOREDEBUG
            //LogString(dllInstance, L"CreateFileFixup: Cohort redirection", cohorts.WsRedirected.c_str());
            //LogString(dllInstance, L"CreateFileFixup: Cohort package", cohorts.WsPackage.c_str());
            //LogString(dllInstance, L"CreateFileFixup: Cohort native", cohorts.WsNative.c_str());
            Log(L"[%d] CreateFileFixup: MfrPathType=%s", dllInstance, MfrFlagTypesString(cohorts.file_mfr.Request_MfrPathType));
#endif

            switch (cohorts.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (!IsADirectoryCase)
                {
                    if (cohorts.map.Valid_mapping &&
                        cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (IsAWriteCase)
                            {
                                // COW is applicable first.
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                {
                                    //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (cohorts.map.Valid_mapping &&
                        (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
#if MOREDEBUG
                        Log(L"[%d] CreateFileFixup: traditional redirection mapping.", dllInstance);
#endif
                        // try the redirected path, then package (via COW), then native (possibly via COW).
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d] CreateFileFixup: use redirected.", dllInstance);
#endif
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        if (PathExists(cohorts.WsPackage.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d] CreateFileFixup: use package.", dllInstance);
#endif
                            if (IsAWriteCase)
                            {
                                // COW is applicable first.
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                {
                                    //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        if (cohorts.UsingNative &&
                            PathExists(cohorts.WsNative.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d] CreateFileFixup: use native.", dllInstance);
#endif
                            if (IsAWriteCase)
                            {
                                if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                {
                                    ///PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                        }
#if MOREDEBUG
                        Log(L"[%d] CreateFileFixup: no such file exists, use redirected path to fail.", dllInstance);
#endif
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                }
                else
                {
                retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                if (cohorts.map.Valid_mapping)
                {
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (MFRConfiguration.Ilv_Aware)
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            //// try the redirected path, then package (COW), then don't need native.
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            
                            if (IsAWriteCase)
                            {
                                // COW is applicable first.
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                {
                                    //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    else
                    {
                        if (MFRConfiguration.Ilv_Aware)
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            //// try the redirected path, then package (COW), then don't need native.
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }

                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                if (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    cohorts.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;
                }
                else if (cohorts.map.Valid_mapping &&
                         (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (MFRConfiguration.Ilv_Aware)
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            if (!IsADirectoryCase)
                            {
                                // try the redirection path, then the package (COW), then native (possibly COW)
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                                
                                ///#if MOREDEBUG
                                 ///                        Log(L"[%d] CreateFileFixup: Package VFS exists", dllInstance);
                                 ///#endif
                                 if (IsAWriteCase)
                                    {
                                        ///#if MOREDEBUG
                                        ///                            Log(L"[%d] CreateFileFixup: Cow PkgVfs-->Redirected", dllInstance);
                                        ///#endif
                                                                    // COW is applicable first.
                                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                        {
                                            ///#if MOREDEBUG
                                            ///                                Log(L"[%d] CreateFileFixup: Cow OK, ready to create", dllInstance);
                                            ///#endif
                                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                            return retfinal;
                                        }
                                        else
                                        {
                                            ///#if MOREDEBUG
                                            ///                                Log(L"[%d] CreateFileFixup: Cow Bad, ready to create", dllInstance);
                                            ///#endif
                                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                            return retfinal;
                                        }
                                    }
                                 else
                                 {
                                     ///#if MOREDEBUG
                                     ///                            Log(L"[%d] CreateFileFixup: Read only Package VFS, ready to create", dllInstance);
                                     ///#endif
                                     retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                     return retfinal;
                                 }
                                
                                ///#if MOREDEBUG
                                ///                    Log(L"[%d] CreateFileFixup: Package VFS wasn't present.", dllInstance);
                                ///#endif
                                if (cohorts.UsingNative )
                                {
                                    if (IsAWriteCase)
                                    {
                                        // COW is applicable first.
                                        if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                        {
                                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                            return retfinal;
                                        }
                                        else
                                        {
                                            retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                            return retfinal;
                                        }
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        return retfinal;
                                    }
                                }
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                // Is a directory, so we probably want to use the native location, if it exists, since that will layer in the package
                                if (cohorts.UsingNative &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                                else
                                {
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                            }
                        }
                    }
                    else
                    {
                        if (MFRConfiguration.Ilv_Aware)
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            if (!IsADirectoryCase)
                            {
                                if (IsAWriteCase)
                                {
                                    if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                    {
                                        ///#if MOREDEBUG
                                        ///                                Log(L"[%d] CreateFileFixup: Cow OK, ready to create", dllInstance);
                                        ///#endif
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        ///#if MOREDEBUG
                                        ///                                Log(L"[%d] CreateFileFixup: Cow Bad, ready to create", dllInstance);
                                        ///#endif
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                        }
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohorts.map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (MFRConfiguration.Ilv_Aware)
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            if (IsAWriteCase)
                            {
                                // COW is applicable first.
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                        }
                    }
                    if (cohorts.UsingNative &&
                        PathExists(cohorts.WsNative.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;
                    }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                break;
            case mfr::mfr_path_types::is_Protocol:
            case mfr::mfr_path_types::is_DosSpecial:
            case mfr::mfr_path_types::is_Shell:
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                break;
            }

        }
        else
        {
#if _DEBUG
            LogString(dllInstance, L"CreateFileFixup [unguarded] for path", pathName);
#endif
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"CreateFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (pathName != nullptr)
    {
        std::wstring LongDirectory = MakeLongPath(widen(pathName));
        retfinal = impl::CreateFile(LongDirectory.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = INVALID_HANDLE_VALUE; //impl::CreateFile(pathName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
    }
#if _DEBUG
    Log(L"[%d] CreateFileFixup returns with handle 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateFile, CreateFileFixup);
