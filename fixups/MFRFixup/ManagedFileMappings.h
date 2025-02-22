#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>
#include <winternl.h>
#include <minwindef.h>


namespace mfr
{
    // Allows us to temporarily disable a mapping, meaning that the entry should be ignored.
    enum class mfr_enabled_types
    {
        disabled = false,
        enabled = true
    };
    DEFINE_ENUM_FLAG_OPERATORS(mfr_enabled_types);

    // An excluded mapping means that if a match happens, we should only consider the native path, and not the package or redirected paths.
    enum class mfr_exclusion_types
    {
        not_excluded = false,
        excluded = true
    };
    DEFINE_ENUM_FLAG_OPERATORS(mfr_exclusion_types);

    // Defines if the match is partial, or requires an exact match.  Exact matches are used to cover situations where a mapping is a subpath of another mapping.
    enum class mfr_exactmatchonly_types
    {
        not_exactmatchonly = false,
        exactmatchonly = true
    };
    DEFINE_ENUM_FLAG_OPERATORS(mfr_exactmatchonly_types);

    // Defines the redirection behavior for the mapping.
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


    // Defines a mapping between Native, Package, and Redirected locations
    struct mfr_folder_mapping
    {
        mfr_enabled_types        Valid_mapping; // = false;  // used in place of a null mapping.
        mfr_exactmatchonly_types IsExactMatchOnly; // = false;
        mfr_exclusion_types      IsAnExclusionToRedirect; // = false;
        mfr_redirect_flags       RedirectionFlags = mfr_redirect_flags::disabled;

        std::filesystem::path    NativePathBase;
        std::wstring             FolderId;
        std::wstring             VFSFolderName;
        std::filesystem::path    PackagePathBase;
        bool                     DoesRuntimeMapNativeToVFS; // = false;   // Indicates that this is a path that is handled by MSIX runtime for redirection to the package.
        std::filesystem::path    RedirectedPathBase;


    };

    struct mfr_vfs_remapping
    {
        // Used for when combining things that should be different
        std::wstring OrigPath;
        std::wstring SubPath;
        std::wstring  RetargetedPath;
        std::wstring RetargetedSubPath;
    };

    extern mfr_folder_mapping CloneFolderMapping(mfr_folder_mapping);

    extern std::vector<mfr_folder_mapping> g_MfrFolderMappings;
    extern std::vector<mfr_vfs_remapping> g_MfrVfsRemappings;

    extern void Initialize_MFR_Mappings();

    extern mfr_folder_mapping  MakeInvalidMapping();
    extern mfr_folder_mapping  Find_LocalRedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath, DWORD dllInstance);
    extern mfr_folder_mapping  Find_LocalRedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath, DWORD dllInstance);

    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath, DWORD dllInstance);
    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath, DWORD dllInstance);
    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(std::wstring WsPath, DWORD dllInstance);

#if DEAD2ME
    extern mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirPath_BackwardSearch(std::wstring WsPath DWORD dllInstance,);
#endif

    extern bool FindCohortVfsRemapping(IN std::wstring cohort, IN std::wstring nextLevel, OUT std::wstring returnCohort, OUT std::wstring returnNextLevel);

    extern void ToUnicodeString(std::wstring source, IN OUT UNICODE_STRING dest);

}