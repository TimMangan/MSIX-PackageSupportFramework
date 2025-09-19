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

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilesection


// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#if _DEBUG
//#define MOREDEBUG 1
#endif

#define WRAPPER_GETPRIVATEPROFILESECTION(theDestinationFilename) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            auto wideString = std::make_unique<wchar_t[]>(stringLength); \
            retfinal = impl::GetPrivateProfileSectionW(widen_argument(appName).c_str(), wideString.get(), stringLength, LongDestinationFilename.c_str()); \
            if (_doserrno != ENOENT) \
            { \
                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr); \
                Log(LogLevel_DebugBasic, L"[%s%d] GetPrivateProfileSectionFixup returns %x characters from %s.", g_MfrModuleName, dllInstance, retfinal, LongDestinationFilename.c_str()); \
                if (retfinal != 0) \
                { \
                    Log(LogLevel_DebugBasic, L"[%s%d] GetPrivateProfileSectionFixup data %s.", g_MfrModuleName, dllInstance, string); \
                } \
                return retfinal; \
            } \
        } \
        else \
        { \
            retfinal = impl::GetPrivateProfileSectionW(appName, string, stringLength, LongDestinationFilename.c_str()); \
            Log(LogLevel_DebugBasic, L"[%s%d] GetPrivateProfileSectionFixup returns %x characters from %s.", g_MfrModuleName, dllInstance, retfinal, LongDestinationFilename.c_str()); \
            if (retfinal != 0) \
            { \
                Log(LogLevel_DebugBasic, L"[%s%d] GetPrivateProfileSectionFixup data %s.", g_MfrModuleName, dllInstance, string); \
            } \
            return retfinal; \
        } \
    }

template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
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
    DWORD retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            if (fileName != NULL)
            {
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"GetPrivateProfileSectionFixup for fileName", widen(fileName, CP_ACP).c_str());

                // This get is inherently a read-only operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);
                wfileName = AdjustSlashes(wfileName, dllInstance);
                wfileName = AdjustBadUNC(wfileName, dllInstance, L"GetPrivateProfileSectionFixup");

                Cohorts cohorts;
                DetermineCohorts(LogLevel_DebugIntermediate, wfileName, &cohorts, dllInstance, L"GetPrivateProfileSectionFixup");

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
                                // try the request path (which must be the local redirected version by definition), and then a package equivalent, then return 0 characters as the fail.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package, then native,  then return 0 characters as the fail.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage);
                                }
                                else if (cohorts.NativeIsValidOptionInScenario &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsNative);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested);
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
                                //// try the redirected path, then package, then don't need native and return 0 characters as the fail.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested);
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
                                // try the redirected path, then package path, then return 0 characters as the fail.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage);
                                }
                                else
                                {

                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package, then native, then return 0 characters as the fail.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage);
                                }
                                else if (cohorts.NativeIsValidOptionInScenario &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsNative);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested);
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
                                // try the redirected path, then package, then possibly native, then return 0 characters as the fail.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                                    PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage);
                                }
                                else if (cohorts.NativeIsValidOptionInScenario &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsNative);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested);
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
                    std::wstring UseFile = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohorts, dllInstance);
                    // In a redirect to local scenario, we are responsible for determining if source is local or in package
                    UseFile = SelectLocalOrPackageForRead(UseFile, cohorts.WsPackage);

                    WRAPPER_GETPRIVATEPROFILESECTION(UseFile);
                }
            }
            else
            {
            Log(LogLevel_DebugBasic, L"[%s%d]  GetPrivateProfileSectionFixup: null filename, don't redirect as may be registry based or default.", g_MfrModuleName, dllInstance);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"GetPrivateProfileSectionFixup")

    UINT uVal = impl::GetPrivateProfileSection(appName, string, stringLength, fileName);
    Log(LogLevel_DebugBasic, L"[%s%d] GetPrivateProfileSectionFixup Returning 0x%x from unfixed call.", g_MfrModuleName, dllInstance, uVal);
    return uVal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSection, GetPrivateProfileSectionFixup);
