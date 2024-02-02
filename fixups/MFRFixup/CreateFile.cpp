//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew

#if _DEBUG
//#define MOREDEBUG 1
//#define EVENMOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"
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
            if (wPathName._Starts_with(L"STORAGE#") ||
                wPathName._Starts_with(L"\\\\?\\STORAGE#"))
            {
                Log(L"[%d] CreateFileFixup Storage Namespace", dllInstance);
            }
            else if (wPathName.size() == 3)
            {
                if (wPathName.compare(L"C:\\") ||
                    wPathName.compare(L"c:\\"))
                {
                    Log(L"[%d] CreateFileFixup AppVPackageDrive", dllInstance);
                }
            }
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
            if (!MFRConfiguration.Ilv_Aware)
            {
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
                        if (cohorts.map.Valid_mapping &&
                            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                        {
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsPackage.c_str()))
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
                                        // This condition should not happen unless there is no such file locally or in package and it was required.  So make a call to get a reasonable failure code
                                        //retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        //return retfinal;
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
                            // try the redirected path, then package (via COW), then native (possibly via COW).
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (IsAWriteCase)
                                {
                                    // COW is applicable first.
                                    if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                                    {
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        //retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        //return retfinal;
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
                                if (IsAWriteCase)
                                {
                                    if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
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
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
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
#if EVENMOREDEBUG
                        Log(L"[%d] CreateFileFixup: Package VFS with local redirection case.", dllInstance);
#endif
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
#if EVENMOREDEBUG
                        Log(L"[%d] CreateFileFixup: Package VFS with traditional redirection case.", dllInstance);
                        DWORD ohCrap = GetFileAttributes(cohorts.WsPackage.c_str());
                        Log(L"[%d] CreateFileFixup: OhCrap package att = 0x%x", dllInstance, ohCrap);
#endif
                        if (PathExists(cohorts.WsPackage.c_str()))
                        {
#if EVENMOREDEBUG
                            Log(L"[%d] CreateFileFixup: package file exists case.", dllInstance);
#endif
                            if (MFRConfiguration.Ilv_Aware)
                            {
#if EVENMOREDEBUG
                                Log(L"[%d] CreateFileFixup: ilvAware case.", dllInstance);
#endif
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
#if EVENMOREDEBUG
                                Log(L"[%d] CreateFileFixup: NOT ilvAware case.", dllInstance);
#endif
                                if (!IsADirectoryCase)
                                {
#if EVENMOREDEBUG
                                    Log(L"[%d] CreateFileFixup: NOT directory case.", dllInstance);
#endif
                                    // try the redirection path, then the package (COW), then native (possibly COW)
                                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                    {
#if MOREDEBUG
                                        Log(L"[%d] CreateFileFixup: Read only Package VFS but exists in redir, ready to create in redirected area", dllInstance);
#endif
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        return retfinal;
                                    }

                                    ///#if MOREDEBUG
                                     ///                        Log(L"[%d] CreateFileFixup: Package VFS exists", dllInstance);
                                     ///#endif
                                    if (IsAWriteCase)
                                    {
#if EVENMOREDEBUG
                                        Log(L"[%d] CreateFileFixup: Write case.", dllInstance);
#endif
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
#if EVENMOREDEBUG
                                        Log(L"[%d] CreateFileFixup: NOT write case.", dllInstance);
#endif
#if MOREDEBUG
                                        Log(L"[%d] CreateFileFixup: Read only Package VFS but isn't in redirection area, ready to create in package path", dllInstance);
#endif
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        int eCode = GetLastError();
#if _DEBUG
                                        Log(L"[%d] CreateFileFixup: Handle=0x%x eCode=0x%x", dllInstance, retfinal, eCode);
#endif
                                        if (retfinal == INVALID_HANDLE_VALUE &&
                                            eCode == ERROR_PATH_NOT_FOUND)
                                        {
#if _DEBUG
                                            Log(L"[%d] CreateFileFixup: was path not found in package area.", dllInstance);
                                            if (PathParentExists(cohorts.WsRedirected.c_str()))
                                            {
                                                Log(L"[%d] CreateFileFixup: redirection parent found.", dllInstance);
                                            }
                                            else
                                            {
                                                Log(L"[%d] CreateFileFixup: redirection parent not found.", dllInstance);
                                            }
                                            if (PathExists(cohorts.WsRedirected.c_str()))
                                            {
                                                Log(L"[%d] CreateFileFixup: redirection file found.", dllInstance);
                                            }
                                            else
                                            {
                                                Log(L"[%d] CreateFileFixup: redirection file not found.", dllInstance);
                                            }
#endif
                                            // Return the most appropriate error code
                                            if (!cohorts.map.IsAnExclusionToRedirect && PathParentExists(cohorts.WsRedirected.c_str()) && !PathExists(cohorts.WsRedirected.c_str()))
                                            {
#if _DEBUG
                                                Log(L"[%d] CreateFileFixup: Reset error to File not found.", dllInstance);
#endif
                                                SetLastError(ERROR_FILE_NOT_FOUND);
                                            }
                                        }
                                        return retfinal;
                                    }

                                    ///#if MOREDEBUG
                                    ///                    Log(L"[%d] CreateFileFixup: Package VFS wasn't present.", dllInstance);
                                    ///#endif
                                    if (cohorts.UsingNative)
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
#if EVENMOREDEBUG
                            Log(L"[%d] CreateFileFixup: package file does NOT exists case.", dllInstance);
#endif
                            if (MFRConfiguration.Ilv_Aware)
                            {
#if EVENMOREDEBUG
                                Log(L"[%d] CreateFileFixup: ilvAware case.", dllInstance);
#endif
                                retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                if (retfinal == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                {
#if EVENMOREDEBUG
                                    Log(L"[%d] CreateFileFixup: 1920, somaybe try native path?", dllInstance);
#endif
                                    if (cohorts.UsingNative)
                                    {
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        if (retfinal == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND && creationDisposition == OPEN_EXISTING)
                                        {
#if EVENMOREDEBUG
                                            Log(L"[%d] CreateFileFixup: 1920, somaybe try redirected path?", dllInstance);
#endif
                                            if (cohorts.UsingNative)
                                            {
                                                retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                            }
                                        }
                                    }
                                }
                                return retfinal;
                            }
                            else
                            {
#if EVENMOREDEBUG
                                Log(L"[%d] CreateFileFixup: NOT ilvAware case.", dllInstance);
#endif
                                if (!IsADirectoryCase)
                                {
#if EVENMOREDEBUG
                                    Log(L"[%d] CreateFileFixup: NOT Directory case.", dllInstance);
#endif
                                    if (IsAWriteCase)
                                    {
#if EVENMOREDEBUG
                                        Log(L"[%d] CreateFileFixup: write case.", dllInstance);
#endif
                                        // The file wasn't in the package, so precreate folders and let it rip!
                                        PreCreateFolders(cohorts.WsRedirected, dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        return retfinal;
                                    }
                                    else
                                    {
#if EVENMOREDEBUG
                                        Log(L"[%d] CreateFileFixup: NOT write case, but since file not in package try redirection area.", dllInstance);
#endif
                                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                                        int eCode = GetLastError();
#if EVENMOREDEBUG
                                        Log(L"[%d] CreateFileFixup: Handle=0x%x eCode=0x%x", dllInstance, retfinal, eCode);
#endif
                                        if (retfinal == INVALID_HANDLE_VALUE &&
                                            eCode == ERROR_PATH_NOT_FOUND)
                                        {
#if EVENMOREDEBUG
                                            Log(L"[%d] CreateFileFixup: was path not found in package area.", dllInstance);
#endif
                                            if (PathParentExists(cohorts.WsPackage.c_str()))
                                            {
                                                // Return the most appropriate error code
                                                Log(L"[%d] CreateFileFixup:  package parent found, Reset error to File not found.", dllInstance);
                                                SetLastError(ERROR_FILE_NOT_FOUND);
                                            }
                                            else
                                            {
#if EVENMOREDEBUG
                                                Log(L"[%d] CreateFileFixup: package parent not found.", dllInstance);
#endif
                                            }
                                        }
                                        return retfinal;
                                    }
                                }
                                else
                                {
#if EVENMOREDEBUG
                                    Log(L"[%d] CreateFileFixup: Directory case.", dllInstance);
#endif
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
                // ILV in use
                if (!IsThisUnsupportedForInterceptsNow(cohorts.WsRequested))
                {
                    std::wstring usePath;
                    if (IsAWriteCase)
                    {
                        usePath = DetermineIlvPathForWriteOperations(cohorts, dllInstance, moredebug);
                        // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                        // if-and-only-if they are present in the package.
                        PreCreateLocalFoldersIfNeededForWrite(usePath, cohorts.WsPackage, dllInstance, debug, L"CreateFileFixup");
                        // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                        CowLocalFoldersIfNeededForWrite(usePath, cohorts.WsPackage, dllInstance, debug, L"CreateFileFixup");
                        // In a write to package scenario, folders may be needed.
                        PreCreatePackageFoldersIfIlvNeededForWrite(usePath, dllInstance, debug, L"CreateFileFixup");
                    }
                    else
                    {
                        usePath = DetermineIlvPathForReadOperations(cohorts, dllInstance, moredebug);
                        // In a redirect to local scenario, we are responsible for determing if source is local or in package
                        usePath = SelectLocalOrPackageForRead(usePath, cohorts.WsPackage);
                    }
                    retfinal = WRAPPER_CREATEFILE(usePath, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;
                }
                // else fall through
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
        if (LongDirectory.length() != widen(pathName).length())
        {
            retfinal = impl::CreateFileW(LongDirectory.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
        }
        else
        {
            retfinal = impl::CreateFile(pathName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = INVALID_HANDLE_VALUE; //impl::CreateFile(pathName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
    }
#if _DEBUG
    Log(L"[%d] CreateFileFixup (unguarded) returns with handle 0x%x and error=0x%x", dllInstance, retfinal, GetLastError());
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateFile, CreateFileFixup);
