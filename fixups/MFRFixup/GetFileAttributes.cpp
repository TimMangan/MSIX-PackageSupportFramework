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




#define WRAPPER_GETFILEATTRIBUTES(theDestinationFilename, debug, moredebug, wsWhich) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::GetFileAttributesW(LongDestinationFilename.c_str()); \
        DWORD error = GetLastError(); \
        if (retfinal != INVALID_FILE_ATTRIBUTES) \
        { \
            if (debug) \
            { \
                Log(L"[%d] GetFileAttributes returns SUCCESS 0x%x and file '%s'", dllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
            return retfinal; \
        } \
        if (error == ERROR_FILE_NOT_FOUND) \
        { \
            anyFileNotFound = true; \
        } \
        else if (error == ERROR_PATH_NOT_FOUND) \
        { \
            anyPathNotFound = true; \
        } \
        if (moredebug) \
        { \
           Log(L"[%d] GetFileAttributesFixup FAILED 0x%x for %s.", dllInstance, error, wsWhich); \
        } \
    }

template <typename CharT>
DWORD __stdcall GetFileAttributesFixup(_In_ const CharT* fileName) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
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
            dllInstance = ++g_InterceptInstance;
            std::wstring wfileName = widen(fileName);
            wfileName = AdjustSlashes(wfileName);

#if _DEBUG
            if constexpr (psf::is_ansi<CharT>)
            {
                LogString(dllInstance, L"GetFileAttributesFixupA for fileName", wfileName.c_str());
            }
            else
            {
                LogString(dllInstance, L"GetFileAttributesFixupW for fileName", wfileName.c_str());
            }
#endif

#if DEBUGPATHTESTING
            if (wfileName.compare(L"C:\\Program Files\\PlaceholderTest\\Placeholder.txt") == 0)
            {
                DebugPathTesting(dllInstance);
            }
#endif
            // This get is inheirently a read-only operation in all cases.
            // We prefer to use the redirecton case, if present.
            Cohorts cohorts;
            DetermineCohorts(wfileName, &cohorts, moreDebug, dllInstance, L"GetAttributesFixup");

            bool anyFileNotFound = false;
            bool anyPathNotFound = false;

            switch (cohorts.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohorts.map.Valid_mapping)
                {
                    switch (cohorts.map.RedirectionFlags)
                    {
                    case mfr::mfr_redirect_flags::prefer_redirection_local:
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent = 
                            WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");  // returns if successful.
                        }
                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug, moreDebug, L"WsPackage");  // returns if successful.

                        // Both failed if here
                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            // try the redirected path, then package, then native.
                            WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");   // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug, moreDebug, L"WsPackage");   // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsNative, debug, moreDebug, L"WsNative");   // returns if successful.

                        // All failed if here
                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
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
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            //// try the redirected path, then package, then don't need native.
                            WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");   // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug, moreDebug, L"WsPackage");   // returns if successful.

                        // Both failed if here
                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
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
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent.
                            WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");   // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug, moreDebug, L"WsPackage");   // returns if successful.

                        // Both failed if here
                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            // try the redirected path, then package, then native.
                            WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");  // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug, moreDebug, L"WsPackage");  // returns if successful.

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsNative, debug, moreDebug, L"WsNative");  // returns if successful.

                        // All failed if here
                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
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
                    switch (cohorts.map.RedirectionFlags)
                    {
                    case mfr::mfr_redirect_flags::prefer_redirection_local:
                        // not possible
                        break;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            // try the redirected path, then package, then native if relevant.
                            WRAPPER_GETFILEATTRIBUTES(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");  // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTES(cohorts.WsPackage, debug, moreDebug, L"WsPackage");  // returns if successful.

                        if (cohorts.UsingNative)
                        {
                            WRAPPER_GETFILEATTRIBUTES(cohorts.WsNative, debug, moreDebug, L"WsNative");  // returns if successful.
                        }


                        // all failed if still here
                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
                    case mfr::mfr_redirect_flags::prefer_redirection_none:
                    case mfr::mfr_redirect_flags::disabled:
                    default:
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
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"GetFileAttributesTest")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributes Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (fileName != nullptr)
    {
        std::wstring LongFileName = MakeLongPath(widen(fileName));
        retfinal = impl::GetFileAttributes(LongFileName.c_str());
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = INVALID_FILE_ATTRIBUTES; //impl::GetFileAttributes(fileName);
    }
#if _DEBUG
    Log(L"[%d] GetFileAttributes: returns retfinal=%d", dllInstance, retfinal);
    if (retfinal == INVALID_FILE_ATTRIBUTES)
    {
        Log(L"[%d] GetFileAttributes: No Redirect returns GetLastError=0x%x", dllInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributes, GetFileAttributesFixup);

