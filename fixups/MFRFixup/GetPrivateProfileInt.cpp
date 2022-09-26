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

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofileint 

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#define WRAPPER_GETPRIVATEPROFILEINT(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::GetPrivateProfileIntW(widen_argument(sectionName).c_str(), widen_argument(key).c_str(), nDefault, LongDestinationFilename.c_str()); \
        if (debug) \
        { \
        Log(L"[%d] GetPrivateProfileIntFixup Returned uint: %d from '%s' ", dllInstance, retfinal, LongDestinationFilename.c_str()); \
        } \
        return retfinal; \
    }


template <typename CharT>
UINT __stdcall GetPrivateProfileIntFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _In_opt_ const INT nDefault,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif
    UINT retfinal = nDefault;
    try
    {
        if (guard)
        {
#if _DEBUG
            if constexpr (psf::is_ansi<CharT>)
            {
                if (fileName != NULL)
                {
                    LogString(dllInstance, L"GetPrivateProfileIntFixup for fileName", widen_argument(fileName).c_str());
                }
                else
                {
                    Log(L"[%d] GetPrivateProfileIntFixup for null file.", dllInstance);
                }
                if (sectionName != NULL)
                {
                    LogString(dllInstance, L"       Section", widen_argument(sectionName).c_str());
                }
                if (key != NULL)
                {
                    LogString(dllInstance, L"       Key", widen_argument(key).c_str());
                }
            }
            else
            {
                if (fileName != NULL)
                {
                    LogString(dllInstance, L"GetPrivateProfileIntFixup for fileName", fileName);
                }
                else
                {
                    Log(L"[%d] GetPrivateProfileIntFixup for null file.", dllInstance);
                }
                if (sectionName != NULL)
                {
                    LogString(dllInstance, L"       Section", sectionName);
                }
                if (key != NULL)
                {
                    LogString(dllInstance, L"       Key", key);
                }
            }
#endif
            if (fileName != NULL)
            {
                // This get is inheirently a read-only operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);
                wfileName = AdjustSlashes(wfileName);

                Cohorts cohorts;
                DetermineCohorts(wfileName, &cohorts, moredebug, dllInstance, L"GetPrivateProfileIntFixup");

                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohorts.map.Valid_mapping)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent, then default 
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRedirected, debug);  // Returns always
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRequested, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRequested, debug);
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
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, so calling requested provides default value or gets from registry.
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRequested, debug);
                            }
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
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, so calling requested provides default value or gets from registry.
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRequested, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            // try the redirected path, then package, then native, then default
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, so calling requested provides default value or gets from registry.
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRequested, debug);
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
                            if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, so calling requested provides default value or gets from registry.
                                WRAPPER_GETPRIVATEPROFILEINT(cohorts.WsRequested, debug);
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
                Log(L"[%d] GetPrivateProfileIntFixup: null filename, don't redirect as may be registry based or default.", dllInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"GetPrivateProfileInt")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileIntFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    UINT uVal = impl::GetPrivateProfileInt(sectionName, key, nDefault, fileName);
#if MOREDEBUG
    Log(L"[%d] GetPrivateProfileIntFixup Returning 0x%x from unfixed call.", dllInstance, uVal);
#endif 
    return uVal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileInt, GetPrivateProfileIntFixup);
