// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfile2

#if _DEBUG
//#define MOREDEBUG 1 
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"



#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:CopyFile2FixupAnsi_Fixup=impl::_CopyFile2Fixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CopyFile2FixupWide_Fixup=impl::_CopyFile2Fixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:CopyFile2FixupAnsi_Fixup=impl::CopyFile2Fixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CopyFile2FixupWide_Fixup=impl::CopyFile2Fixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

#define  WRAPPER_COPYFILE2(existingFileWs, newFileWs, extendedParameters) \
    { \
        std::wstring LongExistingFileWs = MakeLongPath(existingFileWs); \
        std::wstring LongNewFileWs = MakeLongPath(newFileWs); \
        retfinal = impl::CopyFile2(LongExistingFileWs.c_str(), LongNewFileWs.c_str(), extendedParameters); \
        LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFile2Fixup: Actual From", LongExistingFileWs.c_str()); \
        LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFile2Fixup: Actual To", LongNewFileWs.c_str()); \
        if (retfinal == ERROR_SUCCESS) \
        { \
            Log(LogLevel_DebugBasic, L"[%s%d] CopyFile2Fixup: return SUCCESS", g_MfrModuleName, dllInstance); \
        } \
        else \
        { \
            Log(LogLevel_DebugBasic, L"[%s%d] CopyFile2Fixup: return FAILURE err=0x%x", g_MfrModuleName, dllInstance, GetLastError()); \
        } \
        return (retfinal); \
    }


HRESULT __stdcall CopyFile2Fixup(
    _In_ PCWSTR existingFileName,
    _In_ PCWSTR newFileName,
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] HRESULT retfinal;

    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyFile2Fixup from", existingFileName);
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyFile2Fixup to", newFileName);

            std::wstring wExistingFileName = widen(existingFileName);
            std::wstring wNewFileName = widen(newFileName);
            wExistingFileName = AdjustSlashes(wExistingFileName, dllInstance);
            wNewFileName = AdjustSlashes(wNewFileName, dllInstance);

            wExistingFileName = AdjustBadUNC(wExistingFileName, dllInstance, L"CopyFile2Fixup (existing)");
            wNewFileName = AdjustBadUNC(wNewFileName, dllInstance, L"CopyFile2Fixup (new)");



            // This get is inherently a write operation in all cases.
            // We will always want the redirected location for the new file name.
            Cohorts cohortsExisting;
            DetermineCohorts(LogLevel_DebugIntermediate, wExistingFileName, &cohortsExisting, dllInstance, L"CopyFile2Fixup (existing)");

            Cohorts cohortsNew;
            DetermineCohorts(LogLevel_DebugIntermediate, wNewFileName, &cohortsNew, dllInstance, L"CopyFile2Fixup (new)");
            
            
            if (!MFRConfiguration.Ilv_Aware)
            {
                std::wstring newFileWsRedirected;

                switch (cohortsNew.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && 
                        cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                        // TODO: CopyFile precreates folders here, why not CopyFIle2? and other cases
                        //PreCreateFolders(newFileWsRedirected, dllInstance, L"CopyFile2Fixup");
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && 
                        cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && 
                        cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && 
                        cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    newFileWsRedirected = cohortsNew.WsRequested;
                    break;
                case mfr::mfr_path_types::is_Protocol:
                case mfr::mfr_path_types::is_DosSpecial:
                case mfr::mfr_path_types::is_Shell:
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    newFileWsRedirected = cohortsNew.WsRequested;
                    break;
                }
                Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFile2Fixup: redirected destination=%s", g_MfrModuleName, dllInstance, newFileWsRedirected.c_str());


                switch (cohortsExisting.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent, or make original call to fail.
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters);
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters);
                        }
                        else
                        {
                            // There isn't such a file anywhere.  So the call will fail.
                            WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters);
                        }
                    }
                    else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirected path, then package, then native, or let fail using original.
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters);
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters);
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsNative, newFileWsRedirected, extendedParameters);
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)  
                    {
                        //// try the redirected path, then package (COW), then don't need native.
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters);
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters);
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirection path, then the package (COW).
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters);
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters);
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters);
                        }
                    }
                    else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirection path, then the package (COW), then native (possibly COW)
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters);
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters);
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsNative, newFileWsRedirected, extendedParameters);
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirected path, then package (COW), then possibly native (Possibly COW).
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters);
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters);
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                            WRAPPER_COPYFILE2(cohortsExisting.WsNative, newFileWsRedirected, extendedParameters);
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters);
                        }
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
                // ILV
                std::wstring usePathNew = DetermineIlvPathForWriteOperations(LogLevel_DebugIntermediate, cohortsNew, dllInstance);
                LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFile2Fixup ILV UseTo", usePathNew.c_str());

                // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                // if-and-only-if they are present in the package.
                PreCreateLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, usePathNew, cohortsNew.WsPackage, dllInstance, L"CopyFile2Fixup");
                // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                CowLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, usePathNew, cohortsNew.WsPackage, dllInstance, L"CopyFile2Fixup");
                // In a write to package scenario, folders may be needed.
                PreCreatePackageFoldersIfIlvNeededForWrite(LogLevel_DebugBasic, usePathNew, dllInstance, L"CopyFile2Fixup");

                std::wstring usePathExisting = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohortsExisting, dllInstance);
                // In a redirect to local scenario, we are responsible for determining if source is local or in package
                usePathExisting = SelectLocalOrPackageForRead(usePathExisting, cohortsExisting.WsPackage);
                LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFile2Fixup ILV UseFrom", usePathExisting.c_str());

                WRAPPER_COPYFILE2(usePathExisting, usePathNew, extendedParameters);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyFile2Fixup")

    if (existingFileName != nullptr && newFileName != nullptr)
    {
        std::wstring LongFileName1 = MakeLongPath(widen(existingFileName));
        std::wstring LongFileName2 = MakeLongPath(widen(newFileName));
        retfinal = impl::CopyFile2(LongFileName1.c_str(), LongFileName2.c_str(), extendedParameters);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER); //impl::CopyFile2(existingFileName, newFileName, extendedParameters);
    }
    Log(LogLevel_DebugBasic, L"[%s%d] CopyFile2Fixup returns 0x%x", g_MfrModuleName, dllInstance, retfinal);
    return retfinal;
}
DECLARE_FIXUP(impl::CopyFile2, CopyFile2Fixup);
