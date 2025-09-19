//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew


#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"
#include "Detect_Pipe.h"
#include "..\CommonSrc\findStringIC.h"

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:CreateFileFixupAnsi_Fixup=impl::_CreateFileW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CreateFileFixupWide_Fixup=impl::_CreateFileW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:CreateFileFixupAnsi_Fixup=impl::CreateFileW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CreateFileFixupWide_Fixup=impl::CreateFileW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

HANDLE  WRAPPER_CREATEFILE(Json_Debug_Levels debugRequestLevel,  
                            std::wstring theDestinationFile,
                            _In_ DWORD desiredAccess,
                            _In_ DWORD shareMode,
                            _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
                            _In_ DWORD creationDisposition,
                            _In_ DWORD flagsAndAttributes,
                            _In_opt_ HANDLE templateFile, 
                            DWORD dllInstance)
{
    HANDLE retfinal;
    std::wstring LongDestinationFile = MakeLongPath(theDestinationFile);

    retfinal = impl::CreateFileW(LongDestinationFile.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);

    if (retfinal == INVALID_HANDLE_VALUE)
    {
        Log(debugRequestLevel, L"[%s%d] WRAPPER_CREATEFILE returns FAILURE 0x%x on file '%s'", g_MfrModuleName, dllInstance, GetLastError(), LongDestinationFile.c_str());
    }
    else
    {
        Log(debugRequestLevel, L"[%s%d] WRAPPER_CREATEFILE returns handle 0x%x on file '%s'", g_MfrModuleName, dllInstance, retfinal, LongDestinationFile.c_str());
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

    auto guard = g_reentrancyGuard.enter();
    HANDLE retfinal;

    try
    {
        if (guard)
        {
            dllInstance = ++g_InterceptInstance;
            std::wstring wPathName = widen(pathName);
            wPathName = AdjustSlashes(wPathName, dllInstance);
            wPathName = AdjustLocalPipeName(wPathName);

            if (wPathName._Starts_with(L"\\\\?\\UNC"))
            {
                wPathName = L"\\" + wPathName.substr(7);
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateFile adjustment to fileName", wPathName.c_str());
            }

            if (wPathName._Starts_with(L"STORAGE#") ||
                wPathName._Starts_with(L"\\\\?\\STORAGE#"))
            {
                Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup Storage Namespace", g_MfrModuleName, dllInstance);
            }
            else if (wPathName.size() == 3)
            {
                if (wPathName.compare(L"C:\\") ||
                    wPathName.compare(L"c:\\"))
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup for native equivelent of AppVPackageDrive", g_MfrModuleName, dllInstance);
                }
            }
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateFileFixup for path", pathName);
            Log(LogLevel_DebugBasic, L"[%s%d]        DesiredAccess %s", g_MfrModuleName, dllInstance, Log_DesiredAccess(desiredAccess).c_str());
            Log(LogLevel_DebugBasic, L"[%s%d]        ShareMode %s", g_MfrModuleName, dllInstance, Log_ShareMode(shareMode).c_str());
            Log(LogLevel_DebugBasic, L"[%s%d]        creationDisposition %s", g_MfrModuleName, dllInstance, Log_CreationDisposition(creationDisposition).c_str());
            Log(LogLevel_DebugBasic, L"[%s%d]        flagsAndAttributes %s", g_MfrModuleName, dllInstance,  Log_FlagsAndAttributes(LogLevel_DebugBasic, flagsAndAttributes).c_str());

            bool IsAWriteCase = IsCreateForChange(desiredAccess, creationDisposition, flagsAndAttributes);
            bool IsPossibleDirectoryCase = IsPossibleCreateForDirectory(desiredAccess, creationDisposition, flagsAndAttributes);

            wPathName = AdjustBadUNC(wPathName, dllInstance, L"CreateFile");
            

#if NOTOBSOLETE
            if (!IsAWriteCase && !IsPossibleDirectoryCase)
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

            Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Could be a write operation=%d", g_MfrModuleName, dllInstance, IsAWriteCase);
            Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Is possibly a directory operation=%d", g_MfrModuleName, dllInstance, IsPossibleDirectoryCase);

            // This get is may or may not be a write operation.
            // There may be a need to COW, and may need to create parent folders in redirection area first.
            Cohorts cohorts;
            DetermineCohorts(LogLevel_DebugMaximum, wPathName, &cohorts, dllInstance, L"CreateFileFixup");
            
            LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CreateFileFixup: Cohort requested", cohorts.WsRequested.c_str());
            LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CreateFileFixup: Cohort redirection", cohorts.WsRedirected.c_str());
            LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CreateFileFixup: Cohort package", cohorts.WsPackage.c_str());
            LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CreateFileFixup: Cohort native", cohorts.WsNative.c_str());
            Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: MfrPathType=%s", g_MfrModuleName, dllInstance, MfrFlagTypesString(cohorts.file_mfr.Request_MfrPathType));

            if (!MFRConfiguration.Ilv_Aware)
            {
                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (!IsPossibleDirectoryCase)
                    {
                        if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled  &&
                            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                        {
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (IsAWriteCase)
                                {
                                    // COW is applicable first.
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                    {
                                        //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                            return retfinal;
                        }
                        if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                            (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                                cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: traditional redirection mapping.", g_MfrModuleName, dllInstance);
                            // try the redirected path, then package (via COW), then native (possibly via COW).
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                PathExists(cohorts.WsRedirected.c_str()))
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: use redirected.", g_MfrModuleName, dllInstance);
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: use package.", g_MfrModuleName, dllInstance);
                                if (IsAWriteCase)
                                {
                                    // COW is applicable first.
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                    {
                                        //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            if (cohorts.NativeIsValidOptionInScenario &&
                                PathExists(cohorts.WsNative.c_str()))
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: use native.", g_MfrModuleName, dllInstance);
                                if (IsAWriteCase)
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                    {
                                        ///PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: no such file exists, use redirected path to fail.", g_MfrModuleName, dllInstance);

                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                            return retfinal;
                        }
                    }
                    else
                    {
                        // 5/7/2025:  If this is a directory request in the native area, let's just return the native directory if it exists
                        // Any attempt to perform a subsequent action will result in redirection based on the cohorts anyway.
                        if (PathExists(cohorts.WsRequested.c_str()))
                        {
                            Log(LogLevel_DebugBasic, "[%s%d] Native Directory requested that exists, use that directory.", g_MfrModuleName, dllInstance);
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRequested, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                            return retfinal;
                        }


                        if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                        {
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (IsAWriteCase)
                                {
                                    // COW is applicable first.
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        // This condition should not happen unless there is no such file locally or in package and it was required.  So make a call to get a reasonable failure code
                                        //retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        //return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                            return retfinal;
                        }
                        else if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                            (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                                cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                        {
                            // try the redirected path, then package (via COW), then native (possibly via COW).
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (IsAWriteCase)
                                {
                                    // COW is applicable first.
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        //retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        //return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            if (cohorts.NativeIsValidOptionInScenario &&
                                PathExists(cohorts.WsNative.c_str()))
                            {
                                if (IsAWriteCase)
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                                    {
                                        ///PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (MFRConfiguration.Ilv_Aware)
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            else
                            {
                                //// try the redirected path, then package (COW), then don't need native.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }

                                if (IsAWriteCase)
                                {
                                    // COW is applicable first.
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                    {
                                        //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }

                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                        }
                        else
                        {
                            if (MFRConfiguration.Ilv_Aware)
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            else
                            {
                                //// try the redirected path, then package (COW), then don't need native.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }

                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Package VFS with local redirection case.", g_MfrModuleName, dllInstance);
                        // try the redirection path, then the package (COW).
                        if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohorts.WsRedirected.c_str()))
                        {
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                            return retfinal;
                        }
                        if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (IsAWriteCase)
                            {
                                // COW is applicable first.
                                if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                        }
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                        return retfinal;
                    }
                    else if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                        (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Package VFS with traditional redirection case.", g_MfrModuleName, dllInstance);
                        DWORD ohCrap = GetFileAttributes(cohorts.WsPackage.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: OhCrap package att = 0x%x", g_MfrModuleName, dllInstance, ohCrap);
                        if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: package file exists case.", g_MfrModuleName, dllInstance);
                            if (MFRConfiguration.Ilv_Aware)
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: ilvAware case.", g_MfrModuleName, dllInstance);
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            else
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: NOT ilvAware case.", g_MfrModuleName, dllInstance);
                                if (!IsPossibleDirectoryCase)
                                {
                                    Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: NOT directory case.", g_MfrModuleName, dllInstance);
                                    // try the redirection path, then the package (COW), then native (possibly COW)
                                    if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                        PathExists(cohorts.WsRedirected.c_str()))
                                    {
                                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Read only Package VFS but exists in redir, ready to create in redirected area", g_MfrModuleName, dllInstance);

                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }

                                    ///#if MOREDEBUG
                                     ///                        Log(L"[%s%d] CreateFileFixup: Package VFS exists", g_MfrModuleName, dllInstance);
                                     ///#endif
                                    if (IsAWriteCase)
                                    {
                                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Write case.", g_MfrModuleName, dllInstance);
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: Cow PkgVfs-->Redirected", g_MfrModuleName, dllInstance);
                                                                       
                                        // COW is applicable first.
                                        if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                        {
                                            Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: Cow OK, ready to create", g_MfrModuleName, dllInstance);
                                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                            return retfinal;
                                        }
                                        else
                                        {
                                            Log(LogLevel_DebugIntermediate, L"[% s % d] CreateFileFixup: Cow Bad, ready to create", g_MfrModuleName, dllInstance);
                                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                            return retfinal;
                                        }
                                    }
                                    else
                                    {
                                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: NOT write case.", g_MfrModuleName, dllInstance);
                                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Read only Package VFS but isn't in redirection area, ready to create in package path", g_MfrModuleName, dllInstance);
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        int eCode = GetLastError();
                                        Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Handle=0x%x eCode=0x%x", g_MfrModuleName, dllInstance, retfinal, eCode);
                                        if (retfinal == INVALID_HANDLE_VALUE &&
                                            eCode == ERROR_PATH_NOT_FOUND)
                                        {
                                            if (LogLevel_DebugIntermediate <= g_JsonDebugLevel)
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: was path not found in package area.", g_MfrModuleName, dllInstance);
                                                if (PathParentExists(cohorts.WsRedirected.c_str()))
                                                {
                                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: redirection parent found.", g_MfrModuleName, dllInstance);
                                                }
                                                else
                                                {
                                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: redirection parent not found.", g_MfrModuleName, dllInstance);
                                                }
                                                if (PathExists(cohorts.WsRedirected.c_str()))
                                                {
                                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: redirection file found.", g_MfrModuleName, dllInstance);
                                                }
                                                else
                                                {
                                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: redirection file not found.", g_MfrModuleName, dllInstance);
                                                }
                                            }

                                            // Return the most appropriate error code
                                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                                PathParentExists(cohorts.WsRedirected.c_str()) && !PathExists(cohorts.WsRedirected.c_str()))
                                            {
                                                Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: Reset error to File not found.", g_MfrModuleName, dllInstance);
                                                SetLastError(ERROR_FILE_NOT_FOUND);
                                            }
                                        }
                                        return retfinal;
                                    }

                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: Package VFS wasn't present.", g_MfrModuleName, dllInstance);
                                    if (cohorts.NativeIsValidOptionInScenario)
                                    {
                                        if (IsAWriteCase)
                                        {
                                            // COW is applicable first.
                                            if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                            {
                                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                                return retfinal;
                                            }
                                            else
                                            {
                                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                                return retfinal;
                                            }
                                        }
                                        else
                                        {
                                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                            return retfinal;
                                        }
                                    }
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                                else
                                {
                                    // Is a directory, so we probably want to use the native location, if it exists, since that will layer in the package
                                    if (cohorts.NativeIsValidOptionInScenario &&
                                        PathExists(cohorts.WsNative.c_str()))
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                }
                            }
                        }
                        else
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: package file does NOT exists case.", g_MfrModuleName, dllInstance);
                            if (MFRConfiguration.Ilv_Aware)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: ilvAware case.", g_MfrModuleName, dllInstance);
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                if (retfinal == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: 1920, so maybe try native path?", g_MfrModuleName, dllInstance);
                                    if (cohorts.NativeIsValidOptionInScenario)
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        if (retfinal == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND && creationDisposition == OPEN_EXISTING)
                                        {
                                            Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: 1921, so maybe try redirected path?", g_MfrModuleName, dllInstance);
                                            if (cohorts.NativeIsValidOptionInScenario)
                                            {
                                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                            }
                                        }
                                    }
                                }
                                return retfinal;
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: NOT ilvAware case.", g_MfrModuleName, dllInstance);
                                if (!IsPossibleDirectoryCase)
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: NOT Directory case.", g_MfrModuleName, dllInstance);
                                    if (IsAWriteCase)
                                    {
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: write case.", g_MfrModuleName, dllInstance);
                                        // The file wasn't in the package, so precreate folders and let it rip!
                                        PreCreateFolders(cohorts.WsRedirected, dllInstance, L"CreateFileFixup");
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: NOT write case, but since file not in package try redirection area.", g_MfrModuleName, dllInstance);
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        int eCode = GetLastError();
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: Handle=0x%x eCode=0x%x", g_MfrModuleName, dllInstance, retfinal, eCode);

                                        if (retfinal == INVALID_HANDLE_VALUE &&
                                            eCode == ERROR_PATH_NOT_FOUND)
                                        {
                                            Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: was path not found in package area.", g_MfrModuleName, dllInstance);
                                            if (PathParentExists(cohorts.WsPackage.c_str()))
                                            {
                                                // Return the most appropriate error code
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup:  package parent found, Reset error to File not found.", g_MfrModuleName, dllInstance);
                                                SetLastError(ERROR_FILE_NOT_FOUND);
                                            }
                                            else
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: package parent not found.", g_MfrModuleName, dllInstance);
                                            }
                                        }
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CreateFileFixup: Directory case.", g_MfrModuleName, dllInstance);
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirected path, then package (COW), then possibly native (Possibly COW).
                        if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohorts.WsRedirected.c_str()))
                        {
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                            return retfinal;
                        }
                        if (cohorts.WsPackage.compare(cohorts.WsRedirected) != 0 &&
                            PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (MFRConfiguration.Ilv_Aware)
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                            else
                            {
                                if (IsAWriteCase)
                                {
                                    // COW is applicable first.
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                        }
                        if (cohorts.NativeIsValidOptionInScenario &&
                            PathExists(cohorts.WsNative.c_str()))
                        {
                            if (IsAWriteCase)
                            {
                                // COW is applicable first.
                                if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                                return retfinal;
                            }
                        }
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                        retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
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
                    std::wstring usePath = L"";
                    // 5/7/2025 change to make directories that are native use native
                    if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_native_area &&
                        IsPossibleDirectoryCase)
                    {
                        // Test if the native path exists and it is actually a directory, if so then use the native path (avoid for Draw.IO config issue)
                        DWORD att = ::GetFileAttributes(cohorts.WsNative.c_str());
                        if (att != INVALID_FILE_ATTRIBUTES &&
                            (att & FILE_ATTRIBUTE_DIRECTORY) != 0)
                        {
                            usePath = cohorts.WsRequested;
                            Log(LogLevel_DebugBasic, "[%s%d] Native Directory requested that exists, use that directory.", g_MfrModuleName, dllInstance);
                        }
                    }
                    if (usePath.length() == 0)
                    {
                        if (IsAWriteCase)
                        {
                            usePath = DetermineIlvPathForWriteOperations(LogLevel_DebugIntermediate, cohorts, dllInstance);
                            // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                            // if-and-only-if they are present in the package.
                            PreCreateLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, usePath, cohorts.WsPackage, dllInstance, L"CreateFileFixup");
                            // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                            CowLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, usePath, cohorts.WsPackage, dllInstance, L"CreateFileFixup");
                            // In a write to package scenario, folders may be needed.
                            PreCreatePackageFoldersIfIlvNeededForWrite(LogLevel_DebugBasic, usePath, dllInstance, L"CreateFileFixup");
                        }
                        else
                        {
                            usePath = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohorts, dllInstance);
                            // In a redirect to local scenario, we are responsible for determining if source is local or in package
                            usePath = SelectLocalOrPackageForRead(usePath, cohorts.WsPackage);
                        }
                    }

                    retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, usePath, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);

                    // Special case to keep app from getting confused by giving them VFS\AppVPackageRoot instead of C:\.
                    // We still want to pre-create that folder in case they are going to add to it.
                    if (retfinal != INVALID_HANDLE_VALUE &&
                        pathName != nullptr)
                    {
                        std::wstring wpath = widen(pathName);
                        if (wStringToLower(wpath) == L"\\?\\c:" ||
                            wStringToLower(wpath) == L"\\?\\c:\\" ||
                            wStringToLower(wpath) == L"c:" ||
                            wStringToLower(wpath) == L"c:\\")
                        {
                            CloseHandle(retfinal);
                            retfinal = WRAPPER_CREATEFILE(LogLevel_DebugBasic, wpath, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance);
                        }
                    }
                    return retfinal;
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup: IsUnsupportedForInterceptsNow", g_MfrModuleName, dllInstance);
                    // else fall through
                }
                
            }
        }
        else
        {
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateFileFixup [unguarded] for path", pathName);
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateFileFixup")

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
    Log(LogLevel_DebugBasic, L"[%s%d] CreateFileFixup (unguarded) returns with handle 0x%x and error=0x%x", g_MfrModuleName, dllInstance, retfinal, GetLastError());
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateFile, CreateFileFixup);
