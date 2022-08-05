//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <regex>
#include <filesystem>
#include <dos_paths.h>

enum class redirect_flags
{
    none = 0x0000,
    ensure_directory_structure = 0x0001,
    copy_file = 0x0002,
    check_file_presence = 0x0004,
    ok_if_parent_in_pkg = 0x0008,

    copy_on_read = ensure_directory_structure | copy_file,
};
DEFINE_ENUM_FLAG_OPERATORS(redirect_flags);

enum FilePathArea
{
    FilePathArea_Package,
    FilePathArea_Redirection,
    FilePathArea_Native
};

template <typename T>
inline constexpr bool flag_set(T value, T flag)
{
    return (value & flag) == flag;
}

struct path_redirect_info
{

    bool should_redirect = false;
    std::filesystem::path redirect_path;
    std::filesystem::path vfs_path;
    bool shouldReadonly = false;

    FilePathArea Requested_FilePathArea = FilePathArea_Native;

    // These values are set only if the Should code sees that the requested/redirected file exists.
    // This might only happen if check_presnece option is included in the call.
    bool doesRequestedExist = false;        // File as requested exists'
    bool doesVFSExist = false;              // The request was a native path and file exists in the package.
    bool doesRedirectedExist = false;       // The redirected already exists
    bool doesDevirtualizedExist = false;    // The request was a package path and the native file/location exists.
    //bool doesReveseRedirectionExist = false; //The request was in the redirection area and the package path exists.
    bool doesPackageParentFolderExist = false; // The request may not be in the package, but the parent folder is.
};


struct vfs_folder_mapping
{
    std::filesystem::path path;
    std::filesystem::path package_vfs_relative_path; // E.g. "Windows"
    bool is_native_to_vfs_in_platform;  // Indicates that this is a path that is handled by MSIX runtime for redirection to the package.
};


struct path_redirection_spec
{
    std::filesystem::path base_path;
    std::wregex pattern;
    std::wstring patternWstring;
    std::filesystem::path redirect_targetbase;
    bool isExclusion;
    bool isReadOnly;
};


struct normalized_path
{
    // The full_path could either be:
    //      1.  A drive-absolute path. E.g. "C:\foo\bar.txt"
    //      2.  A local device path. E.g. "\\.\C:\foo\bar.txt" or "\\.\COM1"
    //      3.  A root-local device path. E.g. "\\?\C:\foo\bar.txt" or "\\?\HarddiskVolume1\foo\bar.txt"
    //      4.  A UNC-absolute path. E.g. "\\server\share\foo\bar.txt"
    // or empty if there was a failure
    std::wstring full_path;

    psf::dos_path_type path_type;

    // A pointer inside of full_path if the path explicitly uses a drive symbolic link at the root, otherwise nullptr.
    // Note that this isn't perfect; e.g. we don't handle scenarios such as "\\localhost\C$\foo\bar.txt"
    wchar_t* drive_absolute_path = nullptr;
};


struct normalized_pathV2
{
    // The full_path could either be:
    //      1.  A drive-absolute path. E.g. "C:\foo\bar.txt"
    //      2.  A local device path. E.g. "\\.\C:\foo\bar.txt" or "\\.\COM1"
    //      3.  A root-local device path. E.g. "\\?\C:\foo\bar.txt" or "\\?\HarddiskVolume1\foo\bar.txt"
    //      4.  A UNC-absolute path. E.g. "\\server\share\foo\bar.txt"
    // or empty if there was a failure
    // All are stored wide
    std::wstring original_path;
    std::wstring full_path;

    psf::dos_path_type path_type;

    // A shortened full_path if the path explicitly uses a drive symbolic link at the root, otherwise the full_path or a path with working directory added; always wide.
    std::wstring drive_absolute_path;
};

inline normalized_pathV2 ClonePathV2(normalized_pathV2 input)
{
    normalized_pathV2 output;
    output.original_path = input.original_path;
    output.drive_absolute_path = input.drive_absolute_path;
    output.full_path = input.full_path;
    output.path_type = input.path_type;
    return output;
}

path_redirect_info ShouldRedirect(const char* path, redirect_flags flags, DWORD inst = 0);
path_redirect_info ShouldRedirect(const wchar_t* path, redirect_flags flags, DWORD inst = 0);


path_redirect_info ShouldRedirectV2(const char* path, redirect_flags flags, DWORD inst = 0);
path_redirect_info ShouldRedirectV2(const wchar_t* path, redirect_flags flags, DWORD inst = 0);

normalized_path NormalizePath(const char* path, DWORD inst);
normalized_path NormalizePath(const wchar_t* path, DWORD inst);


normalized_pathV2 NormalizePathV2(const char* path, DWORD inst);
normalized_pathV2 NormalizePathV2(const wchar_t* path, DWORD inst);
void LogNormalizedPathV2(normalized_pathV2 np2, std::wstring desc, DWORD instance);

std::string RemoveAnyFinalDoubleSlash(std::string input);
std::wstring RemoveAnyFinalDoubleSlash(std::wstring input);

// If the input path is relative to the VFS folder under the package path (e.g. "${PackageRoot}\VFS\SystemX64\foo.txt"),
// then modifies that path to its virtualized equivalent (e.g. "C:\Windows\System32\foo.txt")
normalized_path DeVirtualizePath(normalized_path path);

std::wstring DeVirtualizePathV2(normalized_pathV2 path);

// If the input path is a physical path outside of the package (e.g. "C:\Windows\System32\foo.txt"),
// this returns what the package VFS equivalent would be (e.g "C:\Program Files\WindowsApps\Packagename\VFS\SystemX64\foo.txt");
// NOTE: Does not check if package has this virtualized path.
normalized_path VirtualizePath(normalized_path path, DWORD impl = 0);

std::wstring VirtualizePathV2(normalized_pathV2 path, DWORD impl = 0);

std::wstring GenerateRedirectedPath(std::wstring_view relativePath, bool ensureDirectoryStructure, std::wstring result, DWORD inst);

// Short-circuit to determine what the redirected path would be. No check to see if the path should be redirected is
// performed.
std::wstring RedirectedPath(const normalized_path& deVirtualizedPath, bool ensureDirectoryStructure, std::filesystem::path destinationTargetBase, DWORD inst);
std::wstring RedirectedPathV2(const normalized_pathV2& deVirtualizedPath, bool ensureDirectoryStructure, std::filesystem::path destinationTargetBase, DWORD inst);



std::wstring ReverseRedirectedToPackage(const std::wstring input);

// Return path to existing package VFS file (or NULL if not present) but only for AppData local and remote
std::filesystem::path GetPackageVFSPath(const wchar_t* fileName);
std::filesystem::path GetPackageVFSPath(const char* fileName);

// return the package path of a native path that uses this mapping
std::filesystem::path PackagePathFromNativePath(vfs_folder_mapping mapping, const char * fileName);
std::filesystem::path PackagePathFromNativePath(vfs_folder_mapping mapping, const wchar_t * fileName);


/// <summary>
///  Given a file path, return it in root local device form, if possible, or in original form.
/// </summary>
std::string TurnPathIntoRootLocalDevice(const char* path);
std::wstring TurnPathIntoRootLocalDevice(const wchar_t* path);

// Given a string used for the VFS variable folder name in a package, return the native path.
std::filesystem::path path_from_package_vfs_relative_path(std::wstring package_vfs_relative_path);

// Given a path within the package, return the name of the VFS var  (e.g. "Windows")
std::wstring GetVfsVarFromPackagePath(std::filesystem::path packagePath);

extern DWORD g_FileIntceptInstance;


///////////////////////////////////////
// Functions in PathTests
// does path start with basePath
bool path_relative_to(const wchar_t* path, const std::filesystem::path& basePath);
bool path_relative_to(const char* path, const std::filesystem::path& basePath);

// does path match basepath
bool path_same_as(const wchar_t* path, const std::filesystem::path& basePath);
bool path_same_as(const char* path, const std::filesystem::path& basePath);

bool _stdcall IsUnderFolder(_In_ const char * fileName, _In_ const std::filesystem::path folder);
bool _stdcall IsUnderFolder(_In_ const wchar_t * fileName, _In_ const std::filesystem::path folder);


// Determines if the path of the filename falls under the user's appdata local or roaming folders.
bool IsUnderUserAppDataLocal(_In_ const wchar_t* fileName);
bool IsUnderUserAppDataLocal(_In_ const char* fileName);

bool IsUnderUserAppDataLocalPackages(_In_ const wchar_t* fileName);
bool IsUnderUserAppDataLocalPackages(_In_ const char* fileName);

bool IsUnderUserAppDataRoaming(_In_ const wchar_t* fileName);
bool IsUnderUserAppDataRoaming(_In_ const char* fileName);

bool IsUnderUserPackageWritablePackageRoot(_In_ const char* fileName);
bool IsUnderUserPackageWritablePackageRoot(_In_ const wchar_t* fileName);

bool IsUnderPackageRoot(_In_ const wchar_t* fileName);
bool IsUnderPackageRoot(_In_ const char* fileName);

bool IsPackageRoot(_In_ const wchar_t* fileName);
bool IsPackageRoot(_In_ const char* fileName);

bool IsSpecialFile( const char* path);
bool IsSpecialFile( const wchar_t* path);

bool IsColonColonGuid(const char* path);
bool IsColonColonGuid(const wchar_t* path);

bool IsBlobColon(const std::string path);
bool IsBlobColon(const std::wstring path);

std::string UrlDecode(std::string str);
std::wstring UrlDecode(std::wstring str);

std::string ReplaceSlashBackwardOnly(std::string old_string);
std::wstring ReplaceSlashBackwardOnly(std::wstring old_string);

std::string StripFileColonSlash(std::string old_string);
std::wstring StripFileColonSlash(std::wstring old_string);

std::filesystem::path trim_absvfs2varfolder(std::filesystem::path abs);