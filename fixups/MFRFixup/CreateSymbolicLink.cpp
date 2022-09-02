//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsymboliclinka

/// NOTES:
///     This function creates a file that resolves as pointing to an target file or directory.
///     This means that the app might use this link in part of paths in subsequent calls that we would
///     interpret as called.
///     So for this intercept, determine the redirected path location (if any) for the target target and the link to be created.
///     If the target target is a file, treat it the same as for CreateHardLink, in other words
///     if there is not a copy of the target file in the redirection area, perform a copy of it.
///     If the target target is a folder, we could copy the whole folder, but for now we'll just make
///     the link point to an empty folder there (which would get layered in subsequent calls anyway.
///     Make sure the folder in the redirected area for the link parent is present.
///     Then create the link in it's redirected area.
///
///     ERROR_PRIVILEGE_NOT_HELD gets returned inside MSIX, possibly due to a need for SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
///     to be part of the flag.  This is an adaption to the container to try to protect things.  Basically
///     symbolic links have been deemed bad (but not hard links) and this won't work. The extra flag only
///     works if running in developer mode according to documentation, but not developer mode not required.  
///     So we will automatically add the flag to all calls in the spirit of app-compat.

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
BOOLEAN __stdcall CreateSymbolicLinkFixup(
    _In_ const CharT* symlinkFileName,
    _In_ const CharT* targetFileName,
    _In_ DWORD flags) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    BOOLEAN retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(dllInstance, L"CreateSymbolicLinkFixup for", symlinkFileName);
            LogString(dllInstance, L"CreateSymbolicLinkFixup target", targetFileName);
            Log(L"[%d] CreateSymbolicLinkFixup flags=0x%x", dllInstance, flags);
#endif

            std::wstring wSymlinkFileName = widen(symlinkFileName);
            std::wstring wTargetFileName = widen(targetFileName);

            mfr::mfr_path targetfile_mfr = mfr::create_mfr_path(wTargetFileName);
            mfr::mfr_folder_mapping targetFileMap;
            std::wstring targetFileWsRequested = targetfile_mfr.Request_NormalizedPath.c_str();
            std::wstring targetFileWsNative;
            std::wstring targetFileWsPackage;
            std::wstring targetFileWsRedirected;

            mfr::mfr_path symlinkfile_mfr = mfr::create_mfr_path(wSymlinkFileName);
            mfr::mfr_folder_mapping symlinkFileMap;
            std::wstring symlinkFileWsRequested = symlinkfile_mfr.Request_NormalizedPath.c_str();
            std::wstring symlinkFileWsNative;
            std::wstring symlinkFileWsPackage;
            std::wstring symlinkFileWsRedirected;

            // Determine if path of target file.
            switch (targetfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                targetFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(targetfile_mfr.Request_NormalizedPath.c_str());
                if (targetFileMap.Valid_mapping)
                {
                    targetFileWsNative = targetFileWsRequested;
                    targetFileWsPackage = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.NativePathBase, targetFileMap.PackagePathBase);
                    targetFileWsRedirected = targetFileWsRequested;
                    break;
                }
                targetFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(targetfile_mfr.Request_NormalizedPath.c_str());
                if (targetFileMap.Valid_mapping)
                {
                    targetFileWsNative = targetFileWsRequested;
                    targetFileWsPackage = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.NativePathBase, targetFileMap.PackagePathBase);
                    targetFileWsRedirected = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.NativePathBase, targetFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                targetFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(targetfile_mfr.Request_NormalizedPath.c_str());
                if (targetFileMap.Valid_mapping)
                {
                    targetFileWsNative = targetFileWsRequested;
                    targetFileWsPackage = targetFileWsRequested;
                    targetFileWsRedirected = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.PackagePathBase, targetFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                targetFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(targetfile_mfr.Request_NormalizedPath.c_str());
                if (targetFileMap.Valid_mapping)
                {
                    targetFileWsNative = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.PackagePathBase, targetFileMap.NativePathBase);;
                    targetFileWsPackage = targetFileWsRequested;
                    targetFileWsRedirected = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.PackagePathBase, targetFileMap.RedirectedPathBase);;
                    break;
                }
                targetFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(targetfile_mfr.Request_NormalizedPath.c_str());
                if (targetFileMap.Valid_mapping)
                {
                    targetFileWsNative = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.PackagePathBase, targetFileMap.NativePathBase);
                    targetFileWsPackage = targetFileWsRequested;
                    targetFileWsRedirected = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.PackagePathBase, targetFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                targetFileMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(targetfile_mfr.Request_NormalizedPath.c_str());
                if (targetFileMap.Valid_mapping)
                {
                    targetFileWsNative = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.RedirectedPathBase, targetFileMap.NativePathBase);
                    targetFileWsPackage = ReplacePathPart(targetFileWsRequested.c_str(), targetFileMap.RedirectedPathBase, targetFileMap.PackagePathBase);
                    targetFileWsRedirected = targetFileWsRequested;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                targetFileMap = mfr::MakeInvalidMapping();
                targetFileWsNative = targetFileWsRequested;
                targetFileWsPackage = targetFileWsRequested;
                targetFileWsRedirected = targetFileWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                targetFileMap = mfr::MakeInvalidMapping();
                targetFileWsNative = targetFileWsRequested;
                targetFileWsPackage = targetFileWsRequested;
                targetFileWsRedirected = targetFileWsRequested;
                break;
            }

            // If file case, make a copy if needed so that all changes happen there
            if (flags == 0)
            {
                if (!PathExists(targetFileWsRedirected.c_str()))
                {
                    if (PathExists(targetFileWsPackage.c_str()))
                    {
#if _DEBUG
                        Log(L"[%d] CreateSymbolicLinkFixup:  Copy target package file to redirection area.", dllInstance);
#endif
                        if (!Cow(targetFileWsPackage, targetFileWsRedirected, dllInstance, L"CreateSymbolicLinkFixup"))
                        {
#if _DEBUG
                            Log(L"[%d] CreateSymbolicLinkFixup:  Cow failure?", dllInstance);
#endif
                        }
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] CreateSymbolicLinkFixup:  Copy target native file to redirection area.", dllInstance);
#endif
                        if (!Cow(targetFileWsNative, targetFileWsRedirected, dllInstance, L"CreateSymbolicLinkFixup"))
                        {
#if _DEBUG
                            Log(L"[%d] CreateSymbolicLinkFixup:  Cow failure?", dllInstance);
#endif
                        }
                    }
                }
            }
            else
            {
                // Until evidence says otherwise, skip any copy for the directory case.
                // Ignoring SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE as it isn't a call a production app would make.
            }

            // Determine if path of symlink file.
            switch (symlinkfile_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                symlinkFileMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(symlinkfile_mfr.Request_NormalizedPath.c_str());
                if (symlinkFileMap.Valid_mapping)
                {
                    symlinkFileWsNative = symlinkFileWsRequested;
                    symlinkFileWsPackage = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.NativePathBase, symlinkFileMap.PackagePathBase);
                    symlinkFileWsRedirected = symlinkFileWsRequested;
                    break;
                }
                symlinkFileMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(symlinkfile_mfr.Request_NormalizedPath.c_str());
                if (symlinkFileMap.Valid_mapping)
                {
                    symlinkFileWsNative = symlinkFileWsRequested;
                    symlinkFileWsPackage = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.NativePathBase, symlinkFileMap.PackagePathBase);
                    symlinkFileWsRedirected = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.NativePathBase, symlinkFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                symlinkFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(symlinkfile_mfr.Request_NormalizedPath.c_str());
                if (symlinkFileMap.Valid_mapping)
                {
                    symlinkFileWsNative = symlinkFileWsRequested;
                    symlinkFileWsPackage = symlinkFileWsRequested;
                    symlinkFileWsRedirected = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.PackagePathBase, symlinkFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                symlinkFileMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(symlinkfile_mfr.Request_NormalizedPath.c_str());
                if (symlinkFileMap.Valid_mapping)
                {
                    symlinkFileWsNative = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.PackagePathBase, symlinkFileMap.NativePathBase);;
                    symlinkFileWsPackage = symlinkFileWsRequested;
                    symlinkFileWsRedirected = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.PackagePathBase, symlinkFileMap.RedirectedPathBase);;
                    break;
                }
                symlinkFileMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(symlinkfile_mfr.Request_NormalizedPath.c_str());
                if (symlinkFileMap.Valid_mapping)
                {
                    symlinkFileWsNative = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.PackagePathBase, symlinkFileMap.NativePathBase);
                    symlinkFileWsPackage = symlinkFileWsRequested;
                    symlinkFileWsRedirected = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.PackagePathBase, symlinkFileMap.RedirectedPathBase);
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                symlinkFileMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(symlinkfile_mfr.Request_NormalizedPath.c_str());
                if (symlinkFileMap.Valid_mapping)
                {
                    symlinkFileWsNative = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.RedirectedPathBase, symlinkFileMap.NativePathBase);
                    symlinkFileWsPackage = ReplacePathPart(symlinkFileWsRequested.c_str(), symlinkFileMap.RedirectedPathBase, symlinkFileMap.PackagePathBase);
                    symlinkFileWsRedirected = symlinkFileWsRequested;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                symlinkFileMap = mfr::MakeInvalidMapping();
                symlinkFileWsNative = symlinkFileWsRequested;
                symlinkFileWsPackage = symlinkFileWsRequested;
                symlinkFileWsRedirected = symlinkFileWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                symlinkFileMap = mfr::MakeInvalidMapping();
                symlinkFileWsNative = symlinkFileWsRequested;
                symlinkFileWsPackage = symlinkFileWsRequested;
                symlinkFileWsRedirected = symlinkFileWsRequested;
                break;
            }

            if (targetFileMap.Valid_mapping && symlinkFileMap.Valid_mapping)
            {
                std::wstring rldSymlinkFileNameRedirected = MakeLongPath(symlinkFileWsRedirected);
                std::wstring rldTargetFileNameRedirected = MakeLongPath(targetFileWsRedirected);
                PreCreateFolders(rldSymlinkFileNameRedirected, dllInstance, L"CreateSymbolicLinkFixup");
#if MOREDEBUG
                Log(L"[%d] CreateSymbolicLinkFixup: link is to   %s", dllInstance, rldSymlinkFileNameRedirected.c_str());
                Log(L"[%d] CreateSymbolicLinkFixup: link is from %s", dllInstance, rldTargetFileNameRedirected.c_str());
#endif
                //retfinal = impl::CreateSymbolicLink(rldSymlinkFileNameRedirected.c_str(), rldTargetFileNameRedirected.c_str(), flags);
                retfinal = impl::CreateSymbolicLink(rldSymlinkFileNameRedirected.c_str(), rldTargetFileNameRedirected.c_str(), flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
#if _DEBUG
                Log(L"[%d] CreateSymbolicLinkFixup returns %d", dllInstance, retfinal);
#endif
                return retfinal;

            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"CreateSymbolicLinkFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateSymbolicLinkFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif


    std::wstring rldFileName = MakeLongPath(widen(symlinkFileName));
    std::wstring rldTargetFileName = MakeLongPath(widen(targetFileName));
    retfinal = impl::CreateSymbolicLink(rldFileName.c_str(), rldTargetFileName.c_str(), flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
#if _DEBUG
    Log(L"[%d] CreateSymbolicLinkFixup (default) returns %d", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateSymbolicLink, CreateSymbolicLinkFixup);
