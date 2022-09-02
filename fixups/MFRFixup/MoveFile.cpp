//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefile

/// NOTES:
///     MoveFile requires the permission to delete the old file.  This is not possible when the old file is in the package.
///     We could let it fail, but instead we will use a copy technique when the source ends up to be a package file and leave
///     the old file in place.

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
BOOL __stdcall MoveFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(dllInstance, L"MoveFileFixup From", existingFileName);
            LogString(dllInstance, L"MoveFileFixup To", newFileName);
#endif

            std::wstring wNewFileName = widen(newFileName);
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

            // Determine if path of existing file and if in package.
            std::wstring UseExistingFile;
            bool         ExistingFileIsPackagePath = false;
            switch (existingfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                existingFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = existingFileWsRequested;
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.PackagePathBase);
                    existingFileWsRedirected = existingFileWsRequested;
                    if (PathExists(existingFileWsNative.c_str()))
                    {
                        UseExistingFile = existingFileWsNative;
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        UseExistingFile = existingFileWsPackage;
                        ExistingFileIsPackagePath = true;
                    }
                    else
                    {
                        UseExistingFile = existingFileWsNative;
                    }
                    break;
                }
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = existingFileWsRequested;
                    existingFileWsPackage = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.PackagePathBase);
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.NativePathBase, existingFileMap.RedirectedPathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        UseExistingFile = existingFileWsRedirected;
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        UseExistingFile = existingFileWsPackage;
                        ExistingFileIsPackagePath = true;
                    }
                    else //if (PathExists(existingFileWsNative) or not since this is what was requested
                    {
                        UseExistingFile = existingFileWsNative;
                    }
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
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        UseExistingFile = existingFileWsRedirected;
                    }
                    else
                    {
                        UseExistingFile = existingFileWsPackage;
                        ExistingFileIsPackagePath = true;
                    }
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                existingFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.NativePathBase);
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        UseExistingFile = existingFileWsRedirected;
                    }
                    else
                    {
                        UseExistingFile = existingFileWsPackage;
                        ExistingFileIsPackagePath = true;
                    }
                    break;
                }
                existingFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(existingfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    existingFileWsNative = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.NativePathBase);
                    existingFileWsPackage = existingFileWsRequested;
                    existingFileWsRedirected = ReplacePathPart(existingFileWsRequested.c_str(), existingFileMap.PackagePathBase, existingFileMap.RedirectedPathBase);
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        UseExistingFile = existingFileWsRedirected;
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        UseExistingFile = existingFileWsPackage;
                        ExistingFileIsPackagePath = true;
                    }
                    else if (PathExists(existingFileWsNative.c_str()))
                    {
                        UseExistingFile = existingFileWsNative;
                    }
                    else
                    {
                        UseExistingFile = existingFileWsPackage;
                        ExistingFileIsPackagePath = true;
                    }
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
                    if (PathExists(existingFileWsRedirected.c_str()))
                    {
                        UseExistingFile = existingFileWsRedirected;
                    }
                    else if (PathExists(existingFileWsPackage.c_str()))
                    {
                        UseExistingFile = existingFileWsPackage;
                        ExistingFileIsPackagePath = true;
                    }
                    else if (PathExists(existingFileWsNative.c_str()))
                    {
                        UseExistingFile = existingFileWsNative;
                    }
                    else
                    {
                        UseExistingFile = existingFileWsRedirected;
                    }
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                existingFileMap = mfr::MakeInvalidMapping();
                existingFileWsNative = existingFileWsRequested;
                existingFileWsPackage = existingFileWsRequested;
                existingFileWsRedirected = existingFileWsRequested;
                UseExistingFile = existingFileWsRequested;
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
                UseExistingFile = existingFileWsRequested;
                break;
            }

            std::wstring UseNewFile;
            // Determing the new destination
            switch (newfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                newFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    UseNewFile = newFileWsRequested;
                    break;
                }
                newFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    UseNewFile = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.NativePathBase, newFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                newFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    UseNewFile = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                newFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    UseNewFile = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);;
                    break;
                }
                newFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (existingFileMap.Valid_mapping)
                {
                    UseNewFile = ReplacePathPart(newFileWsRequested.c_str(), newFileMap.PackagePathBase, newFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                newFileMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(newfile_mfr.Request_NormalizedPath.c_str());
                if (newFileMap.Valid_mapping)
                {
                    UseNewFile = newFileWsRequested;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                newFileMap = mfr::MakeInvalidMapping();
                UseNewFile = newFileWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                newFileMap = mfr::MakeInvalidMapping();
                UseNewFile = newFileWsRequested;
                break;
            }

#if MOREDEBUG
            Log(L"[%d] MoveFileFixup: Source      to be is %s", dllInstance, UseExistingFile.c_str());
            Log(L"[%d] MoveFileFixup: Destination to be is %s", dllInstance, UseNewFile.c_str());
            if (ExistingFileIsPackagePath)
            {
                Log(L"[%d] MoveFileFixup: ExistingIsInPackagePath", dllInstance);
            }
#endif

            if (!ExistingFileIsPackagePath)
            {
                // Can try Move
                std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                PreCreateFolders(rldUseNewFile, dllInstance, L"MoveFileFixup");
#if MOREDEBUG
                Log(L"[%d] MoveFileFixup: from is %s", dllInstance, rldUseExistingFile.c_str());
                Log(L"[%d] MoveFileFixup:   to is %s", dllInstance, rldUseNewFile.c_str());
#endif
                retfinal = impl::MoveFile(rldUseExistingFile.c_str(), rldUseNewFile.c_str());
#if _DEBUG
                Log(L"[%d] MoveFileFixup returns %d", dllInstance, retfinal);
#endif
                return retfinal;
            }
            else
            {
                // Replace move with copy since can't move due to package protections (or file doesn't exist anyway)
                std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                PreCreateFolders(rldUseNewFile, dllInstance, L"MoveFileFixup");
                
                // MoveFile supports directory moves, but CopyFile does not
                auto atts = impl::GetFileAttributes(rldUseExistingFile.c_str());
                if (atts != INVALID_FILE_ATTRIBUTES &&
                    (atts & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
#if MOREDEBUG
                    Log(L"[%d] MoveFileFixup: Implemeting stdcopy from is %s", dllInstance, rldUseExistingFile.c_str());
                    Log(L"[%d] MoveFileFixup: Implemeting stdcopy   to is %s", dllInstance, rldUseNewFile.c_str());
#endif
                    // std::filesystem::copy has some edge cases that might throw us for a loop requiring detection of edge
                    // cases that need to be handled differently.  
                    // Limiting use of this as a substitution to the directory scenario *should* keep that from happening.
                    const std::filesystem::copy_options copyOptions =  std::filesystem::copy_options::recursive;
                    std::error_code eCode;
                    std::filesystem::copy(UseExistingFile.c_str(),   // Not sure if std supports long path syntax
                                          UseNewFile.c_str(),
                                          copyOptions, eCode); 
                    if (eCode)
                    {
                        retfinal = 0; // error
                    }
                    else
                    {
                        retfinal = 1; // success
                    }
#if _DEBUG
                        Log(L"[%d] MoveFileFixup returns %d", dllInstance, retfinal);
#endif
                    return retfinal;
                }
                else
                {
                    // Directory Copy
                    // std::filesystem::copy has some edge cases that might throw us for a loop requiring detection of edge
                    // cases that need to be handled differently.  
                    // Limiting use of this as a substitution to the directory scenario *should* keep that from happening.
                    const std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive;
                    std::error_code eCode;
                    std::filesystem::copy(UseExistingFile.c_str(),   // Not sure if std supports long path syntax
                        UseNewFile.c_str(),
                        copyOptions, eCode);
                    if (eCode)
                    {
                        retfinal = 0; // error
                    }
                    else
                    {
                        retfinal = 1; // success
                    }
#if _DEBUG
                    Log(L"[%d] MoveFileFixup returns %d", dllInstance, retfinal);
#endif
                    return retfinal;
                }
            }

        }
        else
        {
#if _DEBUG
            LogString(dllInstance, L"MoveFileFixup Unguarded From", existingFileName);
            LogString(dllInstance, L"MoveFileFixup Unguarded To", newFileName);
#endif
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"MoveFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] MoveFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif


    return impl::MoveFile(existingFileName, newFileName);
}
DECLARE_STRING_FIXUP(impl::MoveFile, MoveFileFixup);

