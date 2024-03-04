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

#define WRAPPER_GETPRIVATEPROFILESECTION(theDestinationFilename, debug, moredebug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            auto wideString = std::make_unique<wchar_t[]>(stringLength); \
            retfinal = impl::GetPrivateProfileSectionW(widen_argument(appName).c_str(), wideString.get(), stringLength, LongDestinationFilename.c_str()); \
            if (_doserrno != ENOENT) \
            { \
                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr); \
                if (debug) \
                { \
                    Log(L"[%d] GetPriviateProfileSectionFixup returns %x characters from %s.", dllInstance, retfinal, LongDestinationFilename.c_str()); \
                } \
                if (retfinal != 0 && moredebug) \
                { \
                    Log(L"[%d] GetPriviateProfileSectionFixup data %s.", dllInstance, string); \
                } \
                return retfinal; \
            } \
        } \
        else \
        { \
            retfinal = impl::GetPrivateProfileSectionW(appName, string, stringLength, LongDestinationFilename.c_str()); \
            if (debug) \
            { \
                Log(L"[%d] GetPriviateProfileSectionFixup returns %x characters from %s.", dllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
            if (retfinal != 0 && moredebug) \
            { \
                Log(L"[%d] GetPriviateProfileSectionFixup data %s.", dllInstance, string); \
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
#if _DEBUG
                LogString(dllInstance, L"GetPrivateProfileSectionFixup for fileName", widen(fileName, CP_ACP).c_str());
#endif
                // This get is inheirently a read-only operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);
                wfileName = AdjustSlashes(wfileName);
                wfileName = AdjustBadUNC(wfileName, dllInstance, L"GetPrivateProfileSectioniFixup");

                Cohorts cohorts;
                DetermineCohorts(wfileName, &cohorts, moredebug, dllInstance, L"GetPrivateProfileSectionFixup");

                if (!MFRConfiguration.Ilv_Aware)
                {
                    switch (cohorts.file_mfr.Request_MfrPathType)
                    {
                    case mfr::mfr_path_types::in_native_area:
                        if (cohorts.map.Valid_mapping)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // try the request path (which must be the local redirected version by definition), and then a package equivalent, then return 0 characters as the fail.
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected, debug, moredebug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage, debug, moredebug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested, debug, moredebug);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package, then native,  then return 0 characters as the fail.
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected, debug, moredebug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage, debug, moredebug);
                                }
                                else if (cohorts.UsingNative &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsNative, debug, moredebug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested, debug, moredebug);
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
                        if (cohorts.map.Valid_mapping)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // not possible, fall through
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                //// try the redirected path, then package, then don't need native and return 0 characters as the fail.
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected, debug, moredebug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage, debug, moredebug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested, debug, moredebug);
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
                        if (cohorts.map.Valid_mapping)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // try the redirected path, then package path, then return 0 characters as the fail.
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected, debug, moredebug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage, debug, moredebug);
                                }
                                else
                                {

                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested, debug, moredebug);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package, then native, then return 0 characters as the fail.
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected, debug, moredebug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage, debug, moredebug);
                                }
                                else if (cohorts.UsingNative &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsNative, debug, moredebug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested, debug, moredebug);
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
                        if (cohorts.map.Valid_mapping)
                        {
                            switch (cohorts.map.RedirectionFlags)
                            {
                            case mfr::mfr_redirect_flags::prefer_redirection_local:
                                // not possible
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package, then possibly native, then return 0 characters as the fail.
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRedirected, debug, moredebug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsPackage, debug, moredebug);
                                }
                                else if (cohorts.UsingNative &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsNative, debug, moredebug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESECTION(cohorts.WsRequested, debug, moredebug);
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
                    std::wstring UseFile = DetermineIlvPathForReadOperations(cohorts, dllInstance, moredebug);
                    // In a redirect to local scenario, we are responsible for determing if source is local or in package
                    UseFile = SelectLocalOrPackageForRead(UseFile, cohorts.WsPackage);

                    WRAPPER_GETPRIVATEPROFILESECTION(UseFile, debug, moredebug);
                }
            }
            else
            {
#if _DEBUG
            Log(L"[%d]  GetPrivateProfileSectionFixup: null filename, don't redirect as may be registry based or default.", dllInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"GetPrivateProfileSectionFixup")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileSectionFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    UINT uVal = impl::GetPrivateProfileSection(appName, string, stringLength, fileName);
#if MOREDEBUG
    Log(L"[%d] GetPrivateProfileSectionFixup Returning 0x%x from unfixed call.", dllInstance, uVal);
#endif
    return uVal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSection, GetPrivateProfileSectionFixup);
