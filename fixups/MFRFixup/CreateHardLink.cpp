//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createhardlinka

/// NOTES:
///     This function creates an extra directory entry to an existing file, such that the file may be accessed in either way.
///     So for this intercept, determine the redirected path location (if any) for the existing file and the link to be created.
///     If there is not a copy of the existing file in the redirection area, perform a copy of it.
///     Make sure the folder in the redirected area for the link parent is present.
///     Then create the link in it's redirected area.

#if _DEBUG
#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"


#include "FunctionImplementations.h"
#include <psf_logging.h>



template <typename CharT>
BOOL __stdcall CreateHardLinkFixup(
    _In_ const CharT* fileName,
    _In_ const CharT* existingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(dllInstance, L"CopyHardLinkFixup for", fileName);
            LogString(dllInstance, L"CopyHardLinkFixup pointing to target", existingFileName);
#endif
            std::wstring wNewFileName = widen(fileName);
            std::wstring wExistingFileName = widen(existingFileName);

            mfr::mfr_path existingfile_mfr = mfr::create_mfr_path(wExistingFileName);
            mfr::mfr_folder_mapping existingFileMap;
            std::wstring existingFileWsRequested = existingfile_mfr.Request_NormalizedPath.c_str();
            std::wstring existingFileWsNative;
            std::wstring existingFileWsPackage;
            std::wstring existingFileWsRedirected;

            mfr::mfr_path newfile_mfr = mfr::create_mfr_path(wNewFileName);
            mfr::mfr_folder_mapping newFileMap;
            std::wstring newFileWsRequested = newfile_mfr.Request_NormalizedPath.c_str();
            std::wstring newFileWsNative;
            std::wstring newFileWsPackage;
            std::wstring newFileWsRedirected;

            // Determine if path of existing file.
            switch (existingfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                existingFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = existingFileWsRequested;
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.PackagePathBase);
                    existingFileWsRedirected = existingFileWsRequested;
                    break;
                }
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = existingFileWsRequested;
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.PackagePathBase);
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = existingFileWsRequested;
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                existingFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.NativePathBase);;
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);;
                    break;
                }
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative =ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.NativePathBase);
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.RedirectedPathBase, existingFileMap.NativePathBase);
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.RedirectedPathBase, existingFileMap.PackagePathBase);
                    existingFileWsRedirected = existingFileWsRequested;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                existingFileMap = mfr::MakeInvalidMapping();
                existingFileWsNative = existingFileWsRequested;
                existingFileWsPackage = existingFileWsRequested;
                existingFileWsRedirected = existingFileWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                existingFileMap = mfr::MakeInvalidMapping();
                existingFileWsNative = existingFileWsRequested;
                existingFileWsPackage = existingFileWsRequested;
                existingFileWsRedirected = existingFileWsRequested;
                break;
            }

            // Make a copy if needed so that all changes happen there
            if (!PathExists(existingFileWsRedirected.c_str()))
            {
                if (PathExists(existingFileWsPackage.c_str()))
                {
#if _DEBUG
                    Log(L"[%d] CreateHardLinkFixup:  Copy existing package file to redirection area.", dllInstance);
#endif
                    if (!Cow(existingFileWsPackage, existingFileWsRedirected, dllInstance, L"CreateHardLinkFixup"))
                    {
#if _DEBUG
                        Log(L"[%d] CreateHardLinkFixup:  Cow failure?", dllInstance);
#endif
                    }
                }
                else
                {
#if _DEBUG
                    Log(L"[%d] CreateHardLinkFixup:  Copy existing native file to redirection area.", dllInstance);
#endif
                    if (!Cow(existingFileWsNative, existingFileWsRedirected, dllInstance, L"CreateHardLinkFixup"))
                    {
#if _DEBUG
                        Log(L"[%d] CreateHardLinkFixup:  Cow failure?", dllInstance);
#endif
                    }
                }
            }

            // Determine if path of new file.
            switch (newfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                newFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    newFileWsNative = newFileWsRequested;
                    newFileWsPackage = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.NativePathBase, newFileMap.PackagePathBase);
                    newFileWsRedirected = newFileWsRequested;
                    break;
                }
                newFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    newFileWsNative = newFileWsRequested;
                    newFileWsPackage = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.NativePathBase, newFileMap.PackagePathBase);
                    newFileWsRedirected = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.NativePathBase, newFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                newFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    newFileWsNative = newFileWsRequested;
                    newFileWsPackage = newFileWsRequested;
                    newFileWsRedirected = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                newFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    newFileWsNative = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.NativePathBase);;
                    newFileWsPackage = newFileWsRequested;
                    newFileWsRedirected = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);;
                    break;
                }
                newFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    newFileWsNative = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.NativePathBase);
                    newFileWsPackage = newFileWsRequested;
                    newFileWsRedirected = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                newFileMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    newFileWsNative = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.RedirectedPathBase, newFileMap.NativePathBase);
                    newFileWsPackage = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.RedirectedPathBase, newFileMap.PackagePathBase);
                    newFileWsRedirected = newFileWsRequested;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                newFileMap = mfr::MakeInvalidMapping();
                newFileWsNative = newFileWsRequested;
                newFileWsPackage = newFileWsRequested;
                newFileWsRedirected = newFileWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                newFileMap = mfr::MakeInvalidMapping();
                newFileWsNative = newFileWsRequested;
                newFileWsPackage = newFileWsRequested;
                newFileWsRedirected = newFileWsRequested;
                break;
            }

            if (existingFileMap.Valid_mapping && newFileMap.Valid_mapping)
            {
                std::wstring rldNewFileNameRedirected = MakeLongPath(newFileWsRedirected);
                std::wstring rldExistingFileNameRedirected = MakeLongPath(existingFileWsRedirected);
                PreCreateFolders(rldNewFileNameRedirected, dllInstance, L"CreateHardLinkFixup");
#if MOREDEBUG
                Log(L"[%d] CreateHardLinkFixup: link is to   %s", dllInstance, rldNewFileNameRedirected.c_str());
                Log(L"[%d] CreateHardLinkFixup: link is from %s", dllInstance, rldExistingFileNameRedirected.c_str());
#endif
                retfinal = impl::CreateHardLink(rldNewFileNameRedirected.c_str(), rldExistingFileNameRedirected.c_str(), securityAttributes);
#if _DEBUG
                Log(L"[%d] CreateHardLinkFixup returns %d", dllInstance, retfinal);
#endif
                return retfinal;
            }

        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"CreateHardlinkFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateHardLinkFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif


    // Improve app compat by allowing long paths always
    std::wstring rldFileName = MakeLongPath(widen(fileName));
    std::wstring rldExistingFileName = MakeLongPath(widen(existingFileName));
    retfinal = impl::CreateHardLink(rldFileName.c_str(), rldExistingFileName.c_str(), securityAttributes);
#if _DEBUG
    Log(L"[%d] CreateHardLinkFixup (default) returns %d", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateHardLink, CreateHardLinkFixup);
