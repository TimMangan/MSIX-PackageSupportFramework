//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

#if _DEBUG
//#define DEBUGPATHTESTING 1
#include "DebugPathTesting.h"
#endif




#define WRAPPER_GETFILEATTRIBUTES(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::GetFileAttributesW(LongDestinationFilename.c_str()); \
        if (retfinal != INVALID_FILE_ATTRIBUTES) \
        { \
            if (debug) \
            { \
                Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, LongDestinationFilename.c_str(), retfinal); \
            } \
            return retfinal; \
        } \
    }

template <typename CharT>
DWORD __stdcall GetFileAttributesFixup(_In_ const CharT* fileName) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
    bool moreDebug = false;
#if _DEBUG
    debug = true;
#endif
#if MOREDEBUG
    moreDebug = true;
#endif
    DWORD retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            if constexpr (psf::is_ansi<CharT>)
            {
                LogString(DllInstance, L"GetFileAttributesFixupA for fileName", wfileName.c_str());
            }
            else
            {
                LogString(DllInstance, L"GetFileAttributesFixupW for fileName", wfileName.c_str());
            }
#endif

#if DEBUGPATHTESTING
            if (wfileName.compare(L"C:\\Program Files\\PlaceholderTest\\Placeholder.txt") == 0)
            {
                DebugPathTesting(DllInstance);
            }
#endif
            // This get is inheirently a read-only operation in all cases.
            // We prefer to use the redirecton case, if present.
            Cohorts cohorts;
            DetermineCohorts(wfileName, &cohorts, moreDebug, DllInstance, L"GetAttributesFixup");

            switch (cohorts.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohorts.map.Valid_mapping)
                {
                    switch (cohorts.map.RedirectionFlags)
                    {
                    case mfr::mfr_redirect_flags::prefer_redirection_local:
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent = 
                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug);  // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug);  // returns if successful.
                        // Both failed if here
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", DllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        // try the redirected path, then package, then native.
                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug);   // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug);   // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsNative, debug);   // returns if successful.
                        // All failed if here
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", DllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
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
                        //// try the redirected path, then package, then don't need native.
                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug);   // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug);   // returns if successful.
                        // Both failed if here
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", DllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
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

                        // try the request path, which must be the local redirected version by definition, and then a package equivalent.
                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug);   // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug);   // returns if successful.
                        // Both failed if here
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", DllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        // try the redirected path, then package, then native.
                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug);  // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug);  // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsNative, debug);  // returns if successful.
                        // All failed if here
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", DllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
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
                    // try the redirected path, then package, then native if relevant.
                    WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug);  // returns if successful.

                    WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug);  // returns if successful.

                    if (cohorts.UsingNative)
                    {
                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsNative, debug);  // returns if successful.
                    }
#if _DEBUG
                    Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", DllInstance, retfinal, GetLastError());
#endif
                    return retfinal;
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
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"GetFileAttributesTest")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributes Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    std::wstring LongFileName = MakeLongPath(widen(fileName));
    retfinal = impl::GetFileAttributes(LongFileName.c_str());
#if _DEBUG
    Log(L"[%d] GetFileAttributes: returns retfinal=%d", DllInstance, retfinal);
    if (retfinal == INVALID_FILE_ATTRIBUTES)
    {
        Log(L"[%d] GetFileAttributes: No Redirect returns GetLastError=0x%x", DllInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributes, GetFileAttributesFixup);

