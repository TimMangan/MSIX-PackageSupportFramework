//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringa

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#if _DEBUG
//#define MOREDEBUG 1
#endif

#define WRAPPER_WRITEPRIVATEPROFILESTRING(theDestinationFilename) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            retfinal = impl::WritePrivateProfileString(appName, keyName, string, narrow(LongDestinationFilename).c_str()); \
            Log(LogLevel_DebugBasic, L"[%s%d] WritePrivateProfileString(A) returns %d on file %s", g_MfrModuleName, dllInstance, retfinal, LongDestinationFilename.c_str()); \
            return retfinal; \
        } \
        else \
        { \
            retfinal = impl::WritePrivateProfileString(appName, keyName, string, LongDestinationFilename.c_str()); \
            Log(LogLevel_DebugBasic, L"[%s%d] WritePrivateProfileString(W) returns %d on file %s", g_MfrModuleName, dllInstance, retfinal, LongDestinationFilename.c_str()); \
            return retfinal; \
        } \
    }

template <typename CharT>
BOOL __stdcall WritePrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {

            if (fileName != NULL)
            {
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"WritePrivateProfileStringFixup for fileName", fileName);

                // This get is inherently a write operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);
                wfileName = AdjustSlashes(wfileName, dllInstance);
                wfileName = AdjustBadUNC(wfileName, dllInstance, L"WritePrivateProfileStringFixup");

                Cohorts cohorts;
                DetermineCohorts(LogLevel_DebugIntermediate, wfileName, &cohorts, dllInstance, L"WritePrivateProfileStringFixup");

                if (!MFRConfiguration.Ilv_Aware)
                {
                    switch (cohorts.file_mfr.Request_MfrPathType)
                    {
                    case mfr::mfr_path_types::in_native_area:
                        if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // try the request path (which must be the local redirected version by definition), and then a package equivalent with COW if needed.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    // no special acction, just write to redirected area
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage);
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package (COW), then native (possibly via COW).
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage);
                                    }
                                }
                                else if (cohorts.NativeIsValidOptionInScenario &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    // TODO: This might not be the best way to decide is COW is appropriate.  
                                    //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                                    //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                                    if (cohorts.map.DoesRuntimeMapNativeToVFS)
                                    {
                                        if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                        {
                                            WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                        }
                                        else
                                        {
                                            WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative);
                                        }
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative);
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_none:
                            case mfr::mfr_redirect_flags::disabled:
                            default:
                                // just fall through to unguarded code
                                break;
                            }
                        }
                        break;
                    case mfr::mfr_path_types::in_package_pvad_area:
                        if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // not possible, fall through
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                //// try the redirected path, then package with COW, then don't need native and create in redirected.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage);
                                    }
                                }
                                else if (cohorts.NativeIsValidOptionInScenario &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative);
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_none:
                            case mfr::mfr_redirect_flags::disabled:
                            default:
                                // just fall through to unguarded code
                                break;
                            }
                        }
                        break;
                    case mfr::mfr_path_types::in_package_vfs_area:
                        if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // try the redirected path, then package path (COW), then create redirected
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage);
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package (COW), then native (COW), then just create new in redirected.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage);
                                    }
                                }
                                else if (cohorts.NativeIsValidOptionInScenario &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative);
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_none:
                            case mfr::mfr_redirect_flags::disabled:
                            default:
                                // just fall through to unguarded code
                                break;
                            }
                        }
                        break;
                    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                        if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // not possible
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package (COW), then possibly native (COW), then create new in redirected.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage);
                                    }
                                }
                                else if (cohorts.NativeIsValidOptionInScenario &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative);
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_none:
                            case mfr::mfr_redirect_flags::disabled:
                            default:
                                // just fall through to unguarded code
                                break;
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
                    std::wstring UseFile = DetermineIlvPathForWriteOperations(LogLevel_DebugIntermediate, cohorts, dllInstance);
                    // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                    // if-and-only-if they are present in the package.
                    PreCreateLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, UseFile, cohorts.WsPackage, dllInstance, L"WritePrivateProfileStringFixup");
                    // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                    CowLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, UseFile, cohorts.WsPackage, dllInstance, L"WritePrivateProfileStringFixup");
                    // In a write to package scenario, folders may be needed.
                    PreCreatePackageFoldersIfIlvNeededForWrite(LogLevel_DebugBasic, UseFile, dllInstance, L"WritePrivateProfileStringFixup");

                    WRAPPER_WRITEPRIVATEPROFILESTRING(UseFile);
                }

            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] WritePrivateProfileStringFixup: null fileName, don't redirect", g_MfrModuleName, dllInstance);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"WritePrivateProfileStringFixup")

    return impl::WritePrivateProfileString(appName, keyName, string, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileString, WritePrivateProfileStringFixup);
