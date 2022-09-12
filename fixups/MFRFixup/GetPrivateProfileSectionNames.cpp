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

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilesectionnames


// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#define WRAPPER_GETPRIVATEPROFILESECTIONNAME(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            auto wideString = std::make_unique<wchar_t[]>(stringLength); \
            retfinal = impl::GetPrivateProfileSectionNamesW(wideString.get(), stringLength, LongDestinationFilename.c_str()); \
            if (_doserrno != ENOENT) \
            { \
                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr); \
                if (debug) \
                { \
                    Log(L"[%d] GetPrivateProfileSectionsNames returns length 0x%x from %s", dllInstance, retfinal, LongDestinationFilename.c_str()); \
                } \
                return retfinal; \
            } \
        } \
        else \
        { \
            retfinal = impl::GetPrivateProfileSectionNamesW(string, stringLength, LongDestinationFilename.c_str()); \
            if (debug) \
            { \
                Log(L"[%d] GetPrivateProfileSectionsNames returns length 0x%x from %x", dllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
        } \
    }

template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionNamesFixup(
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
                LogString(dllInstance, L"GetPrivateProfileSectionNamesFixup for fileName", widen(fileName, CP_ACP).c_str());
#endif
                // This get is inheirently a read-only operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);

                Cohorts cohorts;
                DetermineCohorts(wfileName, &cohorts, moredebug, dllInstance, L"GetPrivateProfileNamesFixup");

                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohorts.map.Valid_mapping)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent, then default 
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRequested, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            // try the redirected path, then package, then native, then default
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsPackage, debug);
                            }
                            else if (PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRequested, debug);
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
                            //// try the redirected path, then package, then don't need native, so default
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRequested, debug);
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
                            // try the redirected path, then package path, then default.
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRequested, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            // try the redirected path, then package, then native, then default
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRequested, debug);
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
                            // try the redirected path, then package, then possibly native, then default.
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESECTIONNAME(cohorts.WsRequested, debug);
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
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_protocol_path:
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
                Log(L"[%d] GetPrivateProfileNamesFixup: null fileName, don't redirect as may be registry based or default.", dllInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"GetPrivateProfileSectionNamesFixup")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileSectionNamesFixup: Exception=0x%x", dllInstance, GetLastError());
    }
#endif


    retfinal = impl::GetPrivateProfileSectionNames(string, stringLength, fileName);
#if MOREDEBUG
    Log(L"[%d] GetPrivateProfileSectionNamesFixup Returned uint: %d from unfixed call.", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSectionNames, GetPrivateProfileSectionNamesFixup);
