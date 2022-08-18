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

#if _DEBUG
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
#endif



template <typename CharT>
DWORD __stdcall GetFileAttributesFixup(_In_ const CharT* fileName) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
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
            if (wfileName.compare(L"C:\\Program Files\\PlaceholderTest\\Placeholder.txt")==0)
            {
                DebugPathTesting(DllInstance);
            } 
#endif
            // This get is inheirently a read-only operation in all cases.
            // We prefer to use the redirecton case, if present.
            mfr::mfr_path file_mfr = mfr::create_mfr_path(wfileName);
            mfr::mfr_folder_mapping map;
            std::wstring testWs;
            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent = 
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package, then don't need native.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] GetFileAttributes    in_redirection_area_writablepackageroot", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
#if MOREDEBUG
                    Log(L"[%d] GetFileAttributes    INVALID_FILE_ATTRIBUTES on '%s' err=0x%x", DllInstance, testWs.c_str(), GetLastError());
#endif
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    retfinal = impl::GetFileAttributesW(testWs.c_str());
                    if (retfinal != INVALID_FILE_ATTRIBUTES)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
#if MOREDEBUG
                        Log(L"[%d] GetFileAttributes    INVALID_FILE_ATTRIBUTES on '%s' err=0x%x", DllInstance, testWs.c_str(), GetLastError());
#endif
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.NativePathBase);
                                retfinal = impl::GetFileAttributesW(testWs.c_str());
                            if (retfinal != INVALID_FILE_ATTRIBUTES)
                            {
#if _DEBUG  
                                Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                                return retfinal;
                            }
                            else
                            {
#if _DEBUG
                                Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                                return retfinal;
                            }
                        }
                        else
                        {
#if _DEBUG
                            Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                            return retfinal;
                        }
                    }
                }      
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] GetFileAttributes    in_redirection_area_other", DllInstance);
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
    LOGGED_CATCHHANDLER(DllInstance, L"GetFileAttributesTest")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributes Exception=0x%x", DllInstance, GetLastError());
    }
#endif

    retfinal = impl::GetFileAttributes(fileName);
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





template <typename CharT>
BOOL __stdcall GetFileAttributesExFixup(
    _In_ const CharT* fileName,
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
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
            std::wstring testWs;
            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent = 
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package, then don't need native.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
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
                    testWs = file_mfr.Request_NormalizedPath.c_str();
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
#if MOREDEBUG
                    Log(L"[%d] GetFileAttributesEx    INVALID_FILE_ATTRIBUTES on '%s' err=0x%x", DllInstance, testWs.c_str(), GetLastError());
#endif
                    testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                    if (retfinal != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                        LogAttributesEx(DllInstance, fileInformation);
#endif
                        return retfinal;
                    }
                    else
                    {
#if MOREDEBUG
                        Log(L"[%d] GetFileAttributesEx    INVALID_FILE_ATTRIBUTES on '%s' err=0x%x", DllInstance, testWs.c_str(), GetLastError());
#endif
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            testWs = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.NativePathBase);
                            retfinal = impl::GetFileAttributesEx(testWs.c_str(), infoLevelId, fileInformation);
                            if (retfinal != 0)
                            {
#if _DEBUG  
                                Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x", DllInstance, testWs.c_str(), retfinal);
                                LogAttributesEx(DllInstance, fileInformation);
#endif
                                return retfinal;
                            }
                            else
                            {
#if _DEBUG
                                Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                                return retfinal;
                            }
                        }
                        else
                        {
#if _DEBUG
                            Log(L"[%d] GetFileAttributesEx returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWs.c_str(), retfinal, GetLastError());
#endif
                            return retfinal;
                        }
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



template <typename CharT>
BOOL __stdcall SetFileAttributesFixup(_In_ const CharT* fileName, _In_ DWORD fileAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD DllInstance = ++g_InterceptInstance;
    BOOL retfinal;
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            LogString(DllInstance, L"SetFileAttributesFixup for fileName", wfileName.c_str());
#endif
            // This get is inheirently a write operation in all cases.
            // We may need to copy the file first.
            mfr::mfr_path file_mfr = mfr::create_mfr_path(wfileName);
            mfr::mfr_folder_mapping map;
            std::wstring testWsNative;
            std::wstring testWsPackage;
            std::wstring testWsRedirected;
            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_native_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] SetFileAttributes    match on LocalRedirMapping", DllInstance);
#endif
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent using COW
                    testWsRedirected = file_mfr.Request_NormalizedPath.c_str();
                    testWsPackage = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(),fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes");
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
                        if (PathParentExists(testWsPackage.c_str()))
                        {
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsRedirected.c_str(), retfinal, GetLastError());
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsRedirected.c_str(), retfinal, GetLastError());
#endif
                            return retfinal;
                        }
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] SetFileAttributes    match on TraditionalRedirMapping", DllInstance);
#endif
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    testWsPackage = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                    testWsNative = file_mfr.Request_NormalizedPath.c_str();
#if MOREDEBUG
                    Log(L"[%d] SetFileAttributes      RedirPath=%s", DllInstance, testWsRedirected.c_str());
                    Log(L"[%d] SetFileAttributes    PackagePath=%s", DllInstance, testWsPackage.c_str());
                    Log(L"[%d] SetFileAttributes     NativePath=%s", DllInstance, testWsNative.c_str());
#endif
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(),fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        [[maybe_unused]] bool bRet = Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes");
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        // TODO: This might not be the best way to decide is COW is appropriate.  
                        //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                        //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            Cow(testWsNative, testWsRedirected, DllInstance, L"SetFileAttributes");
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                            return retfinal;
                        }
                        else
                        {
                            retfinal = impl::SetFileAttributesW(testWsNative.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsNative.c_str(), retfinal);
#endif
                            return retfinal;
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  If any of the parent folders exist, we want to create the redirection parent folder and let this call create the redirected file.
                        if (PathParentExists(testWsRedirected.c_str()))
                        {
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsRedirected.c_str(), retfinal, GetLastError());
#endif
                            return retfinal;
                        }
                        else if (PathParentExists(testWsPackage.c_str()) ||
                                 PathParentExists(testWsNative.c_str()))
                        {
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsRedirected.c_str(), retfinal, GetLastError());
#endif
                            return retfinal;
                        }
                        else
                        {
                            retfinal = impl::SetFileAttributesW(testWsNative.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsNative.c_str(), retfinal, GetLastError());
#endif
                            return retfinal;
                        }
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_package_pvad_area", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    testWsPackage = file_mfr.Request_NormalizedPath;
                    testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] setFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes");
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_package_vfs_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    testWsPackage = file_mfr.Request_NormalizedPath.c_str();
                    testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    //testWsNative ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes");
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    testWsPackage = file_mfr.Request_NormalizedPath.c_str();
                    testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    testWsNative = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes");
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        // TODO: This might not be the best way to decide is COW is appropriate.  
                        //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                        //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            Cow(testWsNative, testWsRedirected, DllInstance, L"SetFileAttributes");
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                            return retfinal;
                        }
                        else
                        {
                            retfinal = impl::SetFileAttributesW(testWsNative.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsNative.c_str(), retfinal);
#endif
                            return retfinal;
                        }
                    }
                    else
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_redirection_area_writablepackageroot", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    testWsRedirected = file_mfr.Request_NormalizedPath.c_str();
                    testWsPackage = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    testWsNative = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.NativePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes");
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        // TODO: This might not be the best way to decide is COW is appropriate.  
                        //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                        //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            Cow(testWsNative, testWsRedirected, DllInstance, L"SetFileAttributes");
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                            return retfinal;
                        }
                        else
                        {
                            retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                            return retfinal;
                        }
                    }
                    else
                    {
                        retfinal = impl::SetFileAttributesW(testWsRedirected.c_str(), fileAttributes);
#if _DEBUG
                        Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, testWsRedirected.c_str(), retfinal);
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_redirection_area_other", DllInstance);
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
    LOGGED_CATCHHANDLER(DllInstance, L"SetFileAttributes")
#else
    catch (...)
    {
        Log(L"[%d] SetFileAttributes Exception=0x%x", DllInstance, GetLastError());
    }
#endif

    retfinal = impl::SetFileAttributes(fileName, fileAttributes);
#if _DEBUG
    Log(L"[%d] SetFileAttributes: returns retfinal=%d", DllInstance, retfinal);
    if (retfinal == 0)
    {
        Log(L"[%d] SetFileAttributes: returns GetLastError=0x%x", DllInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);

