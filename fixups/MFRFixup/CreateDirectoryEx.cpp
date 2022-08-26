//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createdirectoryexw


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



#define WRAPPER_CREATEDIRECTORYEX(theTemplateDirectory, theDestinationDirectory, securityAttributes, debug, moredebug) \
    { \
        std::wstring LongTemplateDirectory = MakeLongPath(theTemplateDirectory); \
        std::wstring LongDestinationDirectory = MakeLongPath(theDestinationDirectory); \
        retfinal = impl::CreateDirectoryExW(LongTemplateDirectory.c_str(), LongDestinationDirectory.c_str(), securityAttributes); \
        if (moredebug) \
        { \
            Log(L"[%d] CreateDirectoryEx uses template '%s'", DllInstance, LongTemplateDirectory.c_str()); \
        } \
        if (debug) \
        { \
            Log(L"[%d] CreateDirectoryEx returns directory '%s' and result 0x%x", DllInstance, LongDestinationDirectory.c_str(), retfinal); \
        } \
        return retfinal; \
    }



template <typename CharT>
BOOL __stdcall CreateDirectoryExFixup(
    _In_ const CharT* templateDirectory,
    _In_ const CharT* newDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // This function is very much like CopyFile, except that we have a folder instead.

#if _DEBUG
            LogString(DllInstance, L"CreateDirectoryExFixup using template", templateDirectory);
            LogString(DllInstance, L"CreateDirectoryExFixup to", newDirectory);
#endif
            std::wstring WtemplateDirectory = widen(templateDirectory);
            std::wstring WnewDirectory = widen(newDirectory);
            std::replace(WtemplateDirectory.begin(), WtemplateDirectory.end(), L'/', L'\\');
            std::replace(WnewDirectory.begin(), WnewDirectory.end(), L'/', L'\\');

            // This get is inheirently a write operation in all cases.
            // We will always want the redirected location for the new directory.
            mfr::mfr_path existingtemplate_mfr = mfr::create_mfr_path(WtemplateDirectory);
            mfr::mfr_path newdirectory_mfr = mfr::create_mfr_path(WnewDirectory);
            mfr::mfr_folder_mapping existingTemplateMap;
            mfr::mfr_folder_mapping newDirectoryMap;
            std::wstring existingTemplateWsRequested = existingtemplate_mfr.Request_NormalizedPath.c_str();
            std::wstring existingTemplateWsNative;
            std::wstring existingTemplateWsPackage;
            std::wstring existingTemplateWsRedirected;
            std::wstring newDirectoryWsRequested = newdirectory_mfr.Request_NormalizedPath.c_str();
            std::wstring newDirectoryWsRedirected;


            switch (newdirectory_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                newDirectoryMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(newdirectory_mfr.Request_NormalizedPath.c_str());
                if (!newDirectoryMap.Valid_mapping)
                {
                    newDirectoryMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(newdirectory_mfr.Request_NormalizedPath.c_str());
                }
                newDirectoryWsRedirected = ReplacePathPart(newDirectoryWsRequested.c_str(), newDirectoryMap.NativePathBase, newDirectoryMap.RedirectedPathBase);
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                newDirectoryMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newdirectory_mfr.Request_NormalizedPath.c_str());
                newDirectoryWsRedirected = ReplacePathPart(newDirectoryWsRequested.c_str(), newDirectoryMap.PackagePathBase, newDirectoryMap.RedirectedPathBase);
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                newDirectoryMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(newdirectory_mfr.Request_NormalizedPath.c_str());
                if (!newDirectoryMap.Valid_mapping)
                {
                    newDirectoryMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(newdirectory_mfr.Request_NormalizedPath.c_str());
                }
                newDirectoryWsRedirected = ReplacePathPart(newDirectoryWsRequested.c_str(), newDirectoryMap.PackagePathBase, newDirectoryMap.RedirectedPathBase);
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                newDirectoryMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(newdirectory_mfr.Request_NormalizedPath.c_str());
                newDirectoryWsRedirected = newDirectoryWsRequested;
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                newDirectoryMap = mfr::MakeInvalidMapping();
                newDirectoryWsRedirected = newDirectoryWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                newDirectoryMap = mfr::MakeInvalidMapping();
                newDirectoryWsRedirected = newDirectoryWsRequested;
                break;
            }
#if MOREDEBUG
            Log(L"[%d] CreateDirectoryExFixup: redirected destination=%s", DllInstance, newDirectoryWsRedirected.c_str());
#endif

            switch (existingtemplate_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                existingTemplateMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(existingtemplate_mfr.Request_NormalizedPath.c_str());
                if (!existingTemplateMap.Valid_mapping)
                {
                    existingTemplateMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(existingtemplate_mfr.Request_NormalizedPath.c_str());
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                existingTemplateMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(existingtemplate_mfr.Request_NormalizedPath.c_str());
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                existingTemplateMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(existingtemplate_mfr.Request_NormalizedPath.c_str());
                if (!existingTemplateMap.Valid_mapping)
                {
                    existingTemplateMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(existingtemplate_mfr.Request_NormalizedPath.c_str());
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                existingTemplateMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(existingtemplate_mfr.Request_NormalizedPath.c_str());
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                existingTemplateMap = mfr::MakeInvalidMapping();
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                existingTemplateMap = mfr::MakeInvalidMapping();
                break;
            }


            switch (existingtemplate_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryExFixup    source is in_native_area", DllInstance);
#endif
                if (existingTemplateMap.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingTemplateMap.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CreateDirectoryExFixup    match on LocalRedirMapping", DllInstance);
#endif
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent, or make original call to fail.
                    existingTemplateWsRedirected = existingTemplateWsRequested;
                    existingTemplateWsPackage = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.RedirectedPathBase, existingTemplateMap.PackagePathBase);
                    if (PathExists(existingTemplateWsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRedirected, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);

                    }
                    else
                    {
                        // There isn't such a file anywhere.  So the call will fail.
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }

                if (existingTemplateMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingTemplateMap.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CreateDirectoryExFixup    match on TraditionalRedirMapping", DllInstance);
#endif
                    // try the redirected path, then package, then native, or let fail using original.
                    existingTemplateWsRedirected = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.NativePathBase, existingTemplateMap.RedirectedPathBase);
                    existingTemplateWsPackage = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.NativePathBase, existingTemplateMap.PackagePathBase);
                    existingTemplateWsNative = existingTemplateWsRequested.c_str();
#if MOREDEBUG
                    Log(L"[%d] CreateDirectoryExFixup      RedirPath=%s", DllInstance, existingTemplateWsRedirected.c_str());
                    Log(L"[%d] CreateDirectoryExFixup    PackagePath=%s", DllInstance, existingTemplateWsPackage.c_str());
                    Log(L"[%d] CreateDirectoryExFixup     NativePath=%s", DllInstance, existingTemplateWsNative.c_str());
#endif
                    if (PathExists(existingTemplateWsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsNative.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsNative, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryExFixup    source is in_package_pvad_area", DllInstance);
#endif
                if (existingTemplateMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingTemplateMap.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    existingTemplateWsPackage = existingTemplateWsRequested;
                    existingTemplateWsRedirected = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.PackagePathBase, existingTemplateMap.RedirectedPathBase);
                    if (PathExists(existingTemplateWsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryExFixup    source is in_package_vfs_area", DllInstance);
#endif
                if (existingTemplateMap.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingTemplateMap.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    existingTemplateWsPackage = existingTemplateWsRequested;
                    existingTemplateWsRedirected = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.PackagePathBase, existingTemplateMap.RedirectedPathBase);
                    if (PathExists(existingTemplateWsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                if (existingTemplateMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingTemplateMap.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    existingTemplateWsPackage = existingTemplateWsRequested;
                    existingTemplateWsRedirected = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.PackagePathBase, existingTemplateMap.RedirectedPathBase);
                    existingTemplateWsNative = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.PackagePathBase, existingTemplateMap.NativePathBase);
                    if (PathExists(existingTemplateWsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsNative.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsNative, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryExFixup    source is in_redirection_area_writablepackageroot", DllInstance);
#endif
                if (existingTemplateMap.RedirectionFlags != mfr::mfr_redirect_flags::prefer_redirection_local &&
                    existingTemplateMap.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    existingTemplateWsRedirected = existingTemplateWsRequested;
                    existingTemplateWsPackage = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.RedirectedPathBase, existingTemplateMap.PackagePathBase);
                    existingTemplateWsNative = ReplacePathPart(existingTemplateWsRequested.c_str(), existingTemplateMap.RedirectedPathBase, existingTemplateMap.NativePathBase);
                    if (PathExists(existingTemplateWsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(existingTemplateWsNative.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), DllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsNative, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(existingTemplateWsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] CopyFile    source is in_redirection_area_other", DllInstance);
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
    LOGGED_CATCHHANDLER(DllInstance, L"CreateDirectoryExFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateDirectoryExFixup Exception=0x%x", DllInstance, GetLastError());
    }
#endif


    
    return impl::CreateDirectoryEx(templateDirectory, newDirectory, securityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateDirectoryEx, CreateDirectoryExFixup);
