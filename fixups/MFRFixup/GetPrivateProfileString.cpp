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

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilestring

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#define WRAPPER_GETPRIVATEPROFILESTRING(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(LongDestinationFilename.c_str()).c_str()); \
            if (debug) \
            { \
                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x from %s", dllInstance, retfinal, LongDestinationFilename.c_str()); \
                if (retfinal > 0) \
                { \
                    LogString(dllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string); \
                } \
            } \
            return retfinal; \
        } \
        else \
        { \
            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, LongDestinationFilename.c_str()); \
            if (debug) \
            { \
                if (retfinal > 0) \
                { \
                    Log(L"[%d] GetPrivateProfileStringFixup: Wide Returned length=0x%x from %s", dllInstance, retfinal, LongDestinationFilename.c_str()); \
                    LogString(dllInstance, L"GetPrivateProfileStringFixup: Returned string", string); \
                } \
                else \
                { \
                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length from %s", dllInstance, LongDestinationFilename.c_str()); \
                } \
            } \
            return retfinal; \
        } \
    }


template <typename CharT>
DWORD __stdcall GetPrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* defaultString,
    _Out_writes_to_opt_(returnStringSizeInChars, return +1) CharT* string,
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
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (fileName != NULL)
                    {
                        LogString(dllInstance, L"GetPrivateProfileStringFixup (A) for fileName", widen(fileName, CP_ACP).c_str());
                    }
                    else
                    {
                        Log(L"[%d] GetPrivateProfileStringFixup for null file.", dllInstance);
                    }
                    if (appName != NULL)
                    {

                        LogString(dllInstance, L" Section", widen_argument(appName).c_str());
                    }
                    if (keyName != NULL)
                    {
                        LogString(dllInstance, L" Key", widen_argument(keyName).c_str());
                    }
                }
                else
                {
                    if (fileName != NULL)
                    {
                        LogString(dllInstance, L"GetPrivateProfileStringFixup (W) for fileName", widen(fileName, CP_ACP).c_str());
                    }
                    else
                    {
                        Log(L"[%d] GetPrivateProfileStringFixup for null file.", dllInstance);
                    }
                    if (appName != NULL)
                    {

                        LogString(dllInstance, L" Section", appName);
                    }
                    if (keyName != NULL)
                    {
                        LogString(dllInstance, L" Key", keyName);
                    }
                }
#endif
                // This get is inheirently a read-only operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);

                Cohorts cohorts;
                DetermineCohorts(wfileName, &cohorts, moredebug, dllInstance, L"GetPrivateProfileStringFixup");

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
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRequested, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            // try the redirected path, then package, then native, then default
                           if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRequested, debug);
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
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRequested, debug);
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
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRequested, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            // try the redirected path, then package, then native, then default
                           if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                            }
                            else
                            {
                               // No file, calling allows for default value or to get from registry.
                               WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRequested, debug);
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
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                            }
                            else
                            {
                                // No file, calling allows for default value or to get from registry.
                                WRAPPER_GETPRIVATEPROFILESTRING(cohorts.WsRequested, debug);
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
                Log(L"[%d] GetPrivateProfileStringFixup: null fileName, don't redirect", dllInstance);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"GetPrivateProfileString")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileString: Exception=0x%x", dllInstance, GetLastError());
    }
#endif 


    retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if MOREDEBUG
    LogString(dllInstance, L" Returning from unfixed call.", string);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileString, GetPrivateProfileStringFixup);

