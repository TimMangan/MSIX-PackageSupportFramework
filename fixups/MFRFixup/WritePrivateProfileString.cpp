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

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringa

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#define WRAPPER_WRITEPRIVATEPROFILESTRING(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            retfinal = impl::WritePrivateProfileString(appName, keyName, string, narrow(LongDestinationFilename).c_str()); \
            if (debug) \
            { \
                Log(L"[%d] WritePrivateProfileString(A) returns %d on file %s", dllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
            return retfinal; \
        } \
        else \
        { \
            retfinal = impl::WritePrivateProfileString(appName, keyName, string, LongDestinationFilename.c_str()); \
            if (debug) \
            { \
                Log(L"[%d] WritePrivateProfileString(W) returns %d on file %s", dllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
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
#if _DEBUG
                LogString(dllInstance, L"WritePrivateProfileStringFixup for fileName", fileName);
#endif
                // This get is inheirently a write operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);

                Cohorts cohorts;
                DetermineCohorts(wfileName, &cohorts, moredebug, dllInstance, L"WritePrivateProfileStringFixup");

                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohorts.map.Valid_mapping)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            // try the request path (which must be the local redirected version by definition), and then a package equivalent with COW if needed.
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                // no special acction, just write to redirected area
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            // try the redirected path, then package (COW), then native (possibly via COW).
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                                }
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                // TODO: This might not be the best way to decide is COW is appropriate.  
                                //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                                //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                                if (cohorts.map.DoesRuntimeMapNativeToVFS)
                                {
                                    if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                    }
                                    else
                                    {
                                        WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                                    }
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
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
                            //// try the redirected path, then package with COW, then don't need native and create in redirected.
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                                }
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
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
                            // try the redirected path, then package path (COW), then create redirected
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            // try the redirected path, then package (COW), then native (COW), then just create new in redirected.
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                                }
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
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
                            // try the redirected path, then package (COW), then possibly native (COW), then create new in redirected.
                            if (PathExists(cohorts.WsRedirected.c_str()))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsPackage, debug);
                                }
                            }
                            else if (cohorts.UsingNative &&
                                     PathExists(cohorts.WsNative.c_str()))
                            {
                                if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"WritePrivateProfileStringFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsNative, debug);
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"WritePrivateProfileStringFixup");
                                WRAPPER_WRITEPRIVATEPROFILESTRING(cohorts.WsRedirected, debug);
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
                Log(L"[%d] WritePrivateProfileStringFixup: null fileName, don't redirect", dllInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"WritePrivateProfileStringFixup")
#else
    catch (...)
    {
        Log(L"[%d] WritePrivateProfileStringFixup: Exception=0x%x", dllInstance, GetLastError());
    }
#endif 


    return impl::WritePrivateProfileString(appName, keyName, string, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileString, WritePrivateProfileStringFixup);
