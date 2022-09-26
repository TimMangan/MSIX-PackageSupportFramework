#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>


namespace mfr
{

    enum class mfr_redirect_flags
    {
        disabled = 0x0000,   // Pre-initialized value           
        prefer_redirection_none = 0x0001,   // The intent is that the path should be used as requested only.
        prefer_redirection_containerized = 0x0002,   // The intent is that any new/modified files are redirected to the containerized redirection area.
        prefer_redirection_if_package_vfs = 0x0003,   // The intent depends on whether the VFS folder is present in the package:
                                                        //      Yes: intent is that any new/modified files are redirected to the containerized redirection area.
                                                        //      No:  intent is that any new/modified files are directed to the native path area.
        prefer_redirection_local = 0x0004,   // The intent is that package files should be found and used, but new/modified files are directed to the native path area.
    };
    DEFINE_ENUM_FLAG_OPERATORS(mfr_redirect_flags);

#if _DEBUG
    inline const wchar_t* RedirectFlagsName(mfr_redirect_flags flag)
    {
        switch (flag)
        {
        case mfr_redirect_flags::prefer_redirection_none:
            return L"prefer_redirection_none";
        case mfr_redirect_flags::prefer_redirection_containerized:
            return L"prefer_redirection_containerized";
        case mfr_redirect_flags::prefer_redirection_if_package_vfs:
            return L"prefer_redirection_if_package_vfs";
        case mfr_redirect_flags::prefer_redirection_local:
            return L"prefer_redirection_local";
        case mfr_redirect_flags::disabled:
        default:
            return L"disabled";
        }
    }
#endif

    // Defines a mapping between Native, Package, and Redirected locations
    struct mfr_folder_mapping
    {
        bool                    Valid_mapping = false;  // used in place of a null mapping.
        bool                    IsAnExclusionToRedirect = false;
        std::filesystem::path   NativePathBase;
        std::wstring            FolderId;
        std::wstring            VFSFolderName;
        std::filesystem::path   PackagePathBase;
        bool                    DoesRuntimeMapNativeToVFS = false;   // Indicates that this is a path that is handled by MSIX runtime for redirection to the package.
        std::filesystem::path   RedirectedPathBase;

        mfr_redirect_flags      RedirectionFlags = mfr_redirect_flags::disabled;

    };

    extern std::vector<mfr_folder_mapping> g_MfrFolderMappings;

    extern void Initialize_MFR_Mappings();

    extern mfr_folder_mapping  MakeInvalidMapping();
    extern mfr_folder_mapping  Find_LocalRedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath);
    extern mfr_folder_mapping  Find_LocalRedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath);

    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath);
    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath);
    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(std::wstring WsPath);

#if DEAD2ME
    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirPath_BackwardSearch(std::wstring WsPath);
#endif
}