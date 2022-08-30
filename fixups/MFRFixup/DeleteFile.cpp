//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-deletefilea

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"


#include "FunctionImplementations.h"
#include <psf_logging.h>


BOOL  WRAPPER_DELETEFILE(std::wstring theDeletingFile, DWORD DllInstance, bool debug)
{
    std::wstring LongDeletingFile = MakeLongPath(theDeletingFile);
    BOOL retfinal = impl::DeleteFileW(LongDeletingFile.c_str());
    if (debug)
    {
        Log(L"[%d] DeleteFile returns on file '%s' and result 0x%x", DllInstance, LongDeletingFile.c_str(), retfinal);
    }
    return retfinal;
}


template <typename CharT>
BOOL __stdcall DeleteFileFixup(_In_ const CharT* pathName) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;
    try
    {
        if (guard)
        {
            std::wstring wPathName = widen(pathName);
#if _DEBUG
            LogString(DllInstance, L"DeleteFileFixup for pathName", wPathName.c_str());
#endif
            std::replace(wPathName.begin(), wPathName.end(), L'/', L'\\');

            // This get is inheirently a write operation in all cases.
            // There is no need to COW, just create the redirected folder, but may need to create parent folders first.
            mfr::mfr_path file_mfr = mfr::create_mfr_path(wPathName);
            mfr::mfr_folder_mapping map;
            std::wstring testWsRequested = file_mfr.Request_NormalizedPath.c_str();
            std::wstring testWsNative;
            std::wstring testWsPackage;
            std::wstring testWsRedirected;

            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                Log(L"[%d] DeleteFileFixup    in_native_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] DeleteFileFixup    match on LocalRedirMapping", DllInstance);
#endif
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent
                    testWsRedirected = testWsRequested;
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        // Still do this to set attributes
                        retfinal = WRAPPER_DELETEFILE(testWsRedirected, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(testWsPackage.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                            Log("[%d] DeleteFileFixup: Resetting return code to ERROR_ACCESS_DENIED.");
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.");
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere. 
                        retfinal = FALSE;
                        SetLastError(ERROR_FILE_NOT_FOUND);  // not important if PATH or FILE not found.
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] DeleteFileFixup    match on TraditionalRedirMapping", DllInstance);
#endif
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    testWsNative = testWsRequested.c_str();
#if MOREDEBUG
                    Log(L"[%d] DeleteFileFixup      RedirPath=%s", DllInstance, testWsRedirected.c_str());
                    Log(L"[%d] DeleteFileFixup    PackagePath=%s", DllInstance, testWsPackage.c_str());
                    Log(L"[%d] DeleteFileFixup     NativePath=%s", DllInstance, testWsNative.c_str());
#endif
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(testWsRedirected, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.");
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.
                        return WRAPPER_DELETEFILE(testWsRequested, DllInstance, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                Log(L"[%d] DeleteFileFixup    in_package_pvad_area", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(testWsRedirected, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Ssetting return code to ERROR_ACCESS_DENIED.");
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.
                        retfinal = false;
                        SetLastError(ERROR_FILE_NOT_FOUND); // doesn't matter if path or file not found.
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Ssetting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                Log(L"[%d] DeleteFileFixup    in_package_vfs_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(testWsRedirected, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        retfinal = false;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.");
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.  
                        retfinal = false;
                        SetLastError(ERROR_FILE_NOT_FOUND); // not important if file or path
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(testWsRedirected, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.");
#endif
                        return retfinal;
                    }
                    else
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_FILE_NOT_FOUND);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] DeleteFileFixup    in_redirection_area_writablepackageroot", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    testWsRedirected = testWsRequested;
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    testWsNative = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.NativePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(testWsRedirected, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.");
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(testWsNative, DllInstance, debug);
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.
                        retfinal = FALSE;
                        SetLastError(ERROR_FILE_NOT_FOUND);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] DeleteFileFixup    in_redirection_area_other", DllInstance);
#endif
                if (PathExists(testWsRedirected.c_str()))
                {
                    retfinal = WRAPPER_DELETEFILE(testWsRedirected, DllInstance, debug);
                    return retfinal;
                }
                else if (PathExists(testWsPackage.c_str()))
                {
                    retfinal = FALSE;
                    SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                    Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.");
#endif
                    return retfinal;
                }
                else if (PathExists(testWsNative.c_str()))
                {
                    retfinal = WRAPPER_DELETEFILE(testWsNative, DllInstance, debug);
                    return retfinal;
                }
                else
                {
                    // There isn't such a file anywhere.
                    retfinal = FALSE;
                    SetLastError(ERROR_FILE_NOT_FOUND);
#if _DEBUG
                    Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.");
#endif
                    return retfinal;
                }
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
    LOGGED_CATCHHANDLER(DllInstance, L"DeleteFile")
#else
    catch (...)
    {
        Log(L"[%d] DeleteFileFixup Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    std::wstring LongDeletingFile = MakeLongPath(widen(pathName));
    return impl::DeleteFile(LongDeletingFile.c_str());
}
DECLARE_STRING_FIXUP(impl::DeleteFile, DeleteFileFixup);
