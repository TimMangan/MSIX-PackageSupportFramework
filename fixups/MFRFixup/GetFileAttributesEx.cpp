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

#if _DEBUG
//#define DEBUGPATHTESTING 1
#include "DebugPathTesting.h"
#endif

#include "FunctionImplementations.h"
#include <psf_logging.h>


void LogAttributesEx(DWORD DllInstance, LPVOID fileInformation)
{
    if (fileInformation != NULL)
    {
        Log(L"[%d] GetFileAttributesEx         Attributes 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->dwFileAttributes);
        Log(L"[%d] GetFileAttributesEx         Creation 0x%x 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwHighDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwLowDateTime);
        Log(L"[%d] GetFileAttributesEx         Access   0x%x 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwHighDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwLowDateTime);
        Log(L"[%d] GetFileAttributesEx         Write    0x%x 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwHighDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwLowDateTime);
        Log(L"[%d] GetFileAttributesEx         Size     0x%I64x 0x%I64x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeHigh,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeLow);
    }
}




#define WRAPPER_GETFILEATTRIBUTESEX(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::GetFileAttributesEx(LongDestinationFilename.c_str(), infoLevelId, fileInformation); \
        if (retfinal != 0) \
        { \
            if (debug) \
            { \
                Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, LongDestinationFilename.c_str(), retfinal); \
                LogAttributesEx(DllInstance, fileInformation); \
            } \
            return retfinal; \
        } \
    }


template <typename CharT>
BOOL __stdcall GetFileAttributesExFixup(
    _In_ const CharT* fileName,
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    DWORD retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            Log(L"[%d] GetFileAttributesExFixup for fileName '%s' ", DllInstance, wfileName.c_str());
#endif
            mfr::mfr_path file_mfr = mfr::create_mfr_path(wfileName);
            mfr::mfr_folder_mapping map;
            std::wstring testWsRequested = file_mfr.Request_NormalizedPath.c_str();
            std::wstring testWsNative;
            std::wstring testWsPackage;
            std::wstring testWsRedirected;
            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent = 
                    testWsRedirected = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTESEX(testWsRedirected, debug);

                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsPackage, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsRedirected, debug);

                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsPackage, debug);

                    testWsNative = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTESEX(testWsNative, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsNative.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package, then don't need native.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsRedirected, debug);

                    testWsPackage = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTESEX(testWsPackage, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
                    LogAttributesEx(DllInstance, fileInformation);
#endif
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsRedirected, debug);

                    testWsPackage = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTESEX(testWsPackage, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsRedirected, debug);

                    testWsPackage = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTESEX(testWsPackage, debug);

                    testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsNative, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsNative.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] GetFileAttributesEx    in_redirection_area_writablepackageroot", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWsRedirected = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTESEX(testWsRedirected, debug);

                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    WRAPPER_GETFILEATTRIBUTESEX(testWsPackage, debug);

                    if (map.DoesRuntimeMapNativeToVFS)
                    {
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.NativePathBase);
                        
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsNative.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] GetFileAttributesEx    in_redirection_area_other", DllInstance);
#endif
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
    LOGGED_CATCHHANDLER(DllInstance, L"GetFileAttributesEx")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributesEx Exception=0x%x", DllInstance, GetLastError());
    }
#endif

    SetLastError(0);
    retfinal = impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
#if _DEBUG
    Log(L"[%d] GetFileAttributesEx: returns retfinal=%d", DllInstance, retfinal);
    if (retfinal == 0)
    {
        Log(L"[%d] GetFileAttributesEx: returns GetLastError=0x%x", DllInstance, GetLastError());
    }
    else
    {
        if (fileInformation != NULL)
        {
            Log(L"[%d] GetFileAttributesEx         Attributes 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->dwFileAttributes);
            Log(L"[%d] GetFileAttributesEx         Creation 0x%x 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwHighDateTime,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwLowDateTime);
            Log(L"[%d] GetFileAttributesEx         Access   0x%x 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwHighDateTime,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwLowDateTime);
            Log(L"[%d] GetFileAttributesEx         Write    0x%x 0x%x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwHighDateTime,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwLowDateTime);
            Log(L"[%d] GetFileAttributesEx         Size     0x%I64x 0x%I64x", DllInstance, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeHigh,
                ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeLow);
        }
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributesEx, GetFileAttributesExFixup);

