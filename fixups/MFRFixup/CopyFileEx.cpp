// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Microsoft documentation of this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-copyfileexw

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


#define  WRAPPER_COPYFILEEX(existingFileWs, newFileWs, dwCopyFlags, debug, moredebug) \
    { \
        std::wstring LongExistingFileWs = MakeLongPath(existingFileWs); \
        std::wstring LongNewFileWs = MakeLongPath(newFileWs); \
        retfinal = impl::CopyFileEx(LongExistingFileWs.c_str(), LongNewFileWs.c_str(), progressRoutine, data, cancel, dwCopyFlags); \
        if (moredebug) \
        { \
            LogString(DllInstance, L"CopyFileExFixup: Actual From", LongExistingFileWs.c_str()); \
            LogString(DllInstance, L"CopyFileExFixup: Actual To", LongNewFileWs.c_str()); \
        } \
        if (debug) \
        { \
            if (retfinal) \
            { \
                Log(L"[%d] CopyFileExFixup: return SUCCESS", DllInstance); \
            } \
            else \
            { \
                Log(L"[%d] CopyFileExFixup: return FAIL err=0x%x", DllInstance, GetLastError()); \
            } \
        } \
        return retfinal; \
    }



template <typename CharT>
BOOL __stdcall CopyFileExFixup(
    _In_ const CharT* existingFileName,
    _In_ const CharT* newFileName,
    _In_opt_ LPPROGRESS_ROUTINE progressRoutine,
    _In_opt_ LPVOID data,
    _When_(cancel != NULL, _Pre_satisfies_(*cancel == FALSE)) _Inout_opt_ LPBOOL cancel,
    _In_ DWORD copyFlags) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] BOOL retfinal;

    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(DllInstance, L"CopyFileExFixup from", existingFileName);
            LogString(DllInstance, L"CopyFileExFixup to", newFileName);
            Log(L"[%d] CopyFileExFixup FailIfExists 0x%x", DllInstance, copyFlags);
#endif
            std::wstring wExistingFileName = widen(existingFileName);
            std::wstring wNewFileName = widen(newFileName);

            // This get is inheirently a write operation in all cases.
            // We will always want the redirected location for the new file name.
            mfr::mfr_path existingfile_mfr = mfr::create_mfr_path(wExistingFileName);
            mfr::mfr_path newfile_mfr = mfr::create_mfr_path(wNewFileName);
            mfr::mfr_folder_mapping existingFileMap;
            mfr::mfr_folder_mapping newFileMap;
            std::wstring existingFileWsRequested = existingfile_mfr.Request_NormalizedPath.c_str();
            std::wstring existingFileWsNative;
            std::wstring existingFileWsPackage;
            std::wstring existingFileWsRedirected;
            std::wstring newFileWsRequested = newfile_mfr.Request_NormalizedPath.c_str();
            std::wstring newFileWsRedirected;


            switch (newfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                newFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (!newFileMap.Valid_mapping)
                {
                    newFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                }
                newFileWsRedirected = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.NativePathBase, newFileMap.RedirectedPathBase);
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                newFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                newFileWsRedirected = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                newFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (!newFileMap.Valid_mapping)
                {
                    newFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                }
                newFileWsRedirected = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                newFileMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                newFileWsRedirected = newFileWsRequested;
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                newFileMap = mfr::MakeInvalidMapping();
                newFileWsRedirected = newFileWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                newFileMap = mfr::MakeInvalidMapping();
                newFileWsRedirected = newFileWsRequested;
                break;
            }
#if MOREDEBUG
            Log(L"[%d] CopyFileExFixup: redirected destination=%s", DllInstance, newFileWsRedirected.c_str());
#endif

            switch (existingfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                existingFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (!existingFileMap.Valid_mapping)
                {
                    existingFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                existingFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (!existingFileMap.Valid_mapping)
                {
                    existingFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                existingFileMap = mfr::MakeInvalidMapping();
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                existingFileMap = mfr::MakeInvalidMapping();
                break;
            }


            switch (existingfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                Log(L"[%d] CopyFileExFixup    source is in_native_area", DllInstance);
#endif
                if (existingFileMap.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingFileMap.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CopyFileExFixup    match on LocalRedirMapping", DllInstance);
#endif
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent, or make original call to fail.
                    existingFileWsRedirected = existingFileWsRequested;
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.RedirectedPathBase, existingFileMap.PackagePathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsRedirected, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsPackage, newFileWsRedirected, copyFlags, debug, moredebug);

                    }
                    else
                    {
                        // There isn't such a file anywhere.  So the call will fail.
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                }

                if (existingFileMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingFileMap.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CopyFileExFixup    match on TraditionalRedirMapping", DllInstance);
#endif
                    // try the redirected path, then package, then native, or let fail using original.
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.RedirectedPathBase);
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.PackagePathBase);
                    existingFileWsNative = existingFileWsRequested.c_str();
#if MOREDEBUG
                    Log(L"[%d] CopyFileExFixup      RedirPath=%s", DllInstance, existingFileWsRedirected.c_str());
                    Log(L"[%d] CopyFileExFixup    PackagePath=%s", DllInstance, existingFileWsPackage.c_str());
                    Log(L"[%d] CopyFileExFixup     NativePath=%s", DllInstance, existingFileWsNative.c_str());
#endif
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsPackage, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsNative, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                Log(L"[%d] CopyFileExFixup    source is in_package_pvad_area", DllInstance);
#endif
                if (existingFileMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingFileMap.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyExFileFixup");
                        WRAPPER_COPYFILEEX(existingFileWsPackage, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                Log(L"[%d] CopyFileExFixup    source is in_package_vfs_area", DllInstance);
#endif
                if (existingFileMap.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingFileMap.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsPackage, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                }
                if (existingFileMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingFileMap.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);
                    existingFileWsNative = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.NativePathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsPackage, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsNative, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] CopyFileExFixup    source is in_redirection_area_writablepackageroot", DllInstance);
#endif
                if (existingFileMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingFileMap.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    existingFileWsRedirected = existingFileWsRequested;
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.RedirectedPathBase, existingFileMap.PackagePathBase);
                    existingFileWsNative = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.RedirectedPathBase, existingFileMap.NativePathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyExFileFixup");
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsPackage, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else if (PathExists(existingFileWsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), DllInstance, L"CopyFileExFixup");
                        WRAPPER_COPYFILEEX(existingFileWsNative, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILEEX(existingFileWsRequested, newFileWsRedirected, copyFlags, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] CopyFileExFixup    source is in_redirection_area_other", DllInstance);
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
    LOGGED_CATCHHANDLER(DllInstance, L"CopyFileExFixup")
#else
    catch (...)
    {
        Log(L"[%d] CopyFileExFixup Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    return impl::CopyFileEx(existingFileName, newFileName, progressRoutine, data, cancel, copyFlags);
}
DECLARE_STRING_FIXUP(impl::CopyFileEx, CopyFileExFixup);
