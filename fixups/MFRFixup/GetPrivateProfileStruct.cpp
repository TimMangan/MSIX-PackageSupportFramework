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

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilestruct

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#if _DEBUG
//#define MOREDEBUG 1
#endif

#define WRAPPER_GETPRIVATEPROFILESTRUCT(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            retfinal = impl::GetPrivateProfileStructW(widen_argument(sectionName).c_str(), widen_argument(key).c_str(), structArea, uSizeStruct, LongDestinationFilename.c_str()); \
        } \
        else \
        { \
            retfinal = impl::GetPrivateProfileStructW(sectionName, key, structArea, uSizeStruct, LongDestinationFilename.c_str()); \
        } \
        if (debug) \
        { \
            Log(L"[%d] GetPrivateProfileStructFixup Returned is: %d from '%s' ", dllInstance, retfinal, LongDestinationFilename.c_str()); \
        } \
        return retfinal; \
    }


template <typename CharT>
BOOL __stdcall GetPrivateProfileStructFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _Out_writes_to_opt_(uSizeStruct, return) LPVOID structArea,
    _In_ UINT uSizeStruct,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    BOOL retfinal;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {

            if (fileName != NULL)
            {
#if DEBUG
                LogString(dllInstance, L"GetPrivateProfileStructFixup for fileName", fileName);
#endif

                // This get is inheirently a read-only operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);
                wfileName = AdjustSlashes(wfileName);
                wfileName = AdjustBadUNC(wfileName, dllInstance, L"GetPrivateProfileStructFixup");

                Cohorts cohorts;
                DetermineCohorts(wfileName, &cohorts, moredebug, dllInstance, L"GetPrivateProfileStructFixup");

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
                                // try the request path, which must be the local redirected version by definition, and then a package equivalent, then default 
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRedirected, debug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsPackage, debug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRequested, debug);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package, then native, then default
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRedirected, debug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsPackage, debug);
                                }
                                else if (cohorts.UsingNative &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsNative, debug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRequested, debug);
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
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRedirected, debug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsPackage, debug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRequested, debug);
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
                                // try the redirected path, then package path,  then default.
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRedirected, debug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsPackage, debug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRequested, debug);
                                }
                                break;
                            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                // try the redirected path, then package, then native, then default
                                if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRedirected, debug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsPackage, debug);
                                }
                                else if (cohorts.UsingNative &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsNative, debug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRequested, debug);
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
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRedirected, debug);
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsPackage, debug);
                                }
                                else if (cohorts.UsingNative &&
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsNative, debug);
                                }
                                else
                                {
                                    // No file, calling allows for default value or to get from registry.
                                    WRAPPER_GETPRIVATEPROFILESTRUCT(cohorts.WsRequested, debug);
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

                    WRAPPER_GETPRIVATEPROFILESTRUCT(UseFile, debug);
                }
            }
            else
            {
                Log(L"[%d]GetPrivateProfileStructFixup: null fileName, don't redirect", dllInstance);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"GetPrivateProfileStruct")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileStructFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif 


    retfinal =  impl::GetPrivateProfileStruct(sectionName, key, structArea, uSizeStruct, fileName);
#if MOREDEBUG
    Log(L"[%d] GetPrivateProfileStructFixup Returned %d from unfixed call.", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileStruct, GetPrivateProfileStructFixup);

