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


void LogAttributesEx(DWORD dllInstance, LPVOID fileInformation)
{
    if (fileInformation != NULL)
    {
        Log(L"[%d] GetFileAttributesEx         Attributes 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->dwFileAttributes);
        Log(L"[%d] GetFileAttributesEx         Creation 0x%x 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwHighDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwLowDateTime);
        Log(L"[%d] GetFileAttributesEx         Access   0x%x 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwHighDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwLowDateTime);
        Log(L"[%d] GetFileAttributesEx         Write    0x%x 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwHighDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwLowDateTime);
        Log(L"[%d] GetFileAttributesEx         Size     0x%x 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeHigh,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeLow);
    }
}




#define WRAPPER_GETFILEATTRIBUTESEX(theDestinationFilename, debug, moredebug, wsWhich) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::GetFileAttributesEx(LongDestinationFilename.c_str(), infoLevelId, fileInformation); \
        DWORD error = GetLastError(); \
        if (retfinal != 0) \
        { \
            if (debug) \
            { \
                Log(L"[%d] GetFileAttributesEx returns result SUCCESS and file '%s'", dllInstance, LongDestinationFilename.c_str()); \
                LogAttributesEx(dllInstance, fileInformation); \
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
           Log(L"[%d] GetFileAttributesExFixup FAILED 0x%x for %s.", dllInstance, error, wsWhich); \
        } \
    }


template <typename CharT>
BOOL __stdcall GetFileAttributesExFixup(
    _In_ const CharT* fileName,
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation) noexcept
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
            Log(L"[%d] GetFileAttributesExFixup level 0x%x for fileName '%s' ", dllInstance, infoLevelId, wfileName.c_str());
#endif
            Cohorts cohorts;
            DetermineCohorts(wfileName, &cohorts, moreDebug, dllInstance, L"GetFileAttributesExFixup");
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
                            // try the request path, which must be the local redirected version by definition, and then a package equivalent  
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");   // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, debug, moreDebug, L"WsPackage");   // returns if successful.

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
                        Log(L"[%d] GetFileAttributesEx returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            // try the redirected path, then package, then native.
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");   // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, debug, moreDebug, L"WsPackage");   // returns if successful.

                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, debug, moreDebug, L"WsNative");   // returns if successful.

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
                        Log(L"[%d] GetFileAttributesEx returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
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
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");   // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, debug, moreDebug, "WsPackage");   // returns if successful.

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
                        Log(L"[%d] GetFileAttributesEx returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
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
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");   // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, debug, moreDebug, L"WsPackage");   // returns if successful.

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
                        Log(L"[%d] GetFileAttributesEx returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
#endif
                        return retfinal;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        if (!cohorts.map.IsAnExclusionToRedirect)
                        {
                            // try the redirected path, then package, then native.
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");  // returns if successful.
                        }

                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, debug, moreDebug, L"WsPackage");  // returns if successful.

                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, debug, moreDebug, L"WsNative");  // returns if successful.

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
                        Log(L"[%d] GetFileAttributesEx returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
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
                    if (!cohorts.map.IsAnExclusionToRedirect)
                    {
                        // try the redirected path, then package, then native if relevant.
                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, debug, moreDebug, L"WsRedirected");  // returns if successful.
                    }

                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, debug, moreDebug, L"WsPackage");  // returns if successful.

                    if (cohorts.UsingNative)
                    {
                        WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, debug, moreDebug, L"WsNative");  // returns if successful.
                    }

                    if (anyFileNotFound)
                    {
                        SetLastError(ERROR_FILE_NOT_FOUND);
                    }
                    else if (anyPathNotFound)
                    {
                        SetLastError(ERROR_PATH_NOT_FOUND);
                    }
#if _DEBUG
                    Log(L"[%d] GetFileAttributesEx returns with result 0x%x and error =0x%x", dllInstance, retfinal, GetLastError());
#endif
                    return retfinal;
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
#if _DEBUG
                Log(L"[%d] GetFileAttributesEx has mfr_path_type 0x%x", dllInstance, cohorts.file_mfr.Request_MfrPathType);
#endif
                break;
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"GetFileAttributesEx")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributesEx Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    SetLastError(0);
    if (fileName != nullptr)
    {
        std::wstring LongFileName = MakeLongPath(widen(fileName));
#if MOREDEBUG
        Log(L"[%d] GetFileAttributesEx: unfixed versus %s", dllInstance, LongFileName.c_str());
#endif
        retfinal = impl::GetFileAttributesEx(LongFileName.c_str(), infoLevelId, fileInformation);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = INVALID_FILE_ATTRIBUTES; //impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
    }
#if _DEBUG
    Log(L"[%d] GetFileAttributesEx: returns retfinal=%d", dllInstance, retfinal);
    if (retfinal == 0)
    {
        Log(L"[%d] GetFileAttributesEx: returns GetLastError=0x%x", dllInstance, GetLastError());
        if (GetLastError() == 2)
        {
            retfinal = impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
            Log(L"[%d] GetFileAttributesEx: returns retry retfinal=%d", dllInstance, retfinal);
        }
    }
    else
    {
        if (fileInformation != NULL)
        {
            Log(L"[%d] GetFileAttributesEx         Attributes %s", dllInstance, Log_FlagsAndAttributes(((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->dwFileAttributes).c_str());
            Log(L"[%d] GetFileAttributesEx         Creation 0x%x 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwHighDateTime,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwLowDateTime);
            Log(L"[%d] GetFileAttributesEx         Access   0x%x 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwHighDateTime,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwLowDateTime);
            Log(L"[%d] GetFileAttributesEx         Write    0x%x 0x%x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwHighDateTime,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwLowDateTime);
            Log(L"[%d] GetFileAttributesEx         Size     0x%I64x 0x%I64x", dllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeHigh,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeLow);
        }
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributesEx, GetFileAttributesExFixup);

