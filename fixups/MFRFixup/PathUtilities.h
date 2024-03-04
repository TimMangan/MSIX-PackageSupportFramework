#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>
#include <utilities.h>
#include "win32_error.h"
#include "ManagedPathTypes.h"
#include "ManagedFileMappings.h"
#include "MfrConfiguration.h"


extern std::filesystem::path g_packageRootPath;
extern std::filesystem::path g_packageVfsRootPath;
extern std::filesystem::path g_redirectRootPath;
extern std::filesystem::path g_writablePackageRootPath;
extern std::filesystem::path g_finalPackageRootPath;

extern std::filesystem::path g_short_packageRootPath;
extern std::filesystem::path g_short_packageVfsRootPath;
extern std::filesystem::path g_short_redirectRootPath;
extern std::filesystem::path g_short_writablePackageRootPath;
extern std::filesystem::path g_short_finalPackageRootPath;

extern bool path_isSubsetOf_String(std::filesystem::path& basePath, const wchar_t* pathstring);
extern bool path_isSubsetOf_String(std::filesystem::path& basePath, const char* pathstring);

extern bool pathString_isSubsetOf_Path(const wchar_t* pathstring, std::filesystem::path& basePath);
extern bool pathString_isSubsetOf_Path(const char* pathstring, std::filesystem::path& basePath);

extern std::wstring ReplacePathPart(std::wstring inputWstring, std::filesystem::path from, std::filesystem::path to);

extern std::filesystem::path cwd_relative_to_normal(std::filesystem::path nativeRelativePath); 
extern std::filesystem::path drive_relative_to_normal(std::filesystem::path nativeRelativePath);
extern std::filesystem::path rooted_relative_to_normal(std::filesystem::path nativeRelativePath);
extern std::filesystem::path root_local_relative_to_normal(std::filesystem::path nativeRelativePath);

extern std::filesystem::path drive_absolute_to_normal(std::filesystem::path nativeAbsolutePath);

extern std::wstring AdjustSlashes(std::wstring path);
extern std::wstring AdjustBadUNC(std::wstring path, DWORD dllInstance, std::wstring CallerName);

extern std::wstring MakeLongPath(std::wstring path);
extern std::wstring MakeNotLongPath(std::wstring path);

extern bool PathExists(const wchar_t* path);
extern bool PathParentExists(const wchar_t* path);

extern void PreCreateFolders(std::wstring filepath, DWORD dllInstance, std::wstring DebugMessage);

extern BOOL Cow(std::wstring from, std::wstring to, int dllInstance, std::wstring DebugString);

extern std::filesystem::path ConvertPathToShortPath(std::filesystem::path inputPath);

extern bool IsCreateForChange(DWORD desiredAccess, DWORD creationDisposition, DWORD flagsAndAttributes);
extern bool IsSpecialCaseforChange(std::wstring filepath);
extern bool IsCreateForDirectory(DWORD desiredAccess, DWORD creationDisposition, DWORD flagsAndAttributes);

extern std::wstring Log_DesiredAccess(DWORD desiredAccess);
extern std::wstring Log_ShareMode(DWORD shareMode);
extern std::wstring Log_CreationDisposition(DWORD creationDisposition);
extern std::wstring Log_FlagsAndAttributes(DWORD flagsAndAttributes);


extern bool comparei(const std::wstring wstrA, const std::wstring wstrB);
extern bool comparei(const std::string strA, const std::string strB);

#if NEEDED
inline std::wstring widen(std::string_view str, UINT codePage = CP_UTF8)
{
    std::wstring result;
    if (str.empty())
    {
        // MultiByteToWideChar fails when given a length of zero
        return result;
    }

    // UTF-16 should occupy at most as many characters as UTF-8
    result.resize(str.length());

    // NOTE: Since we call MultiByteToWideChar with a non-negative input string size, the resulting string is not null
    //       terminated, so we don't need to '+1' the size on input and '-1' the size on resize
    if (auto size = ::MultiByteToWideChar(
        codePage,
        MB_ERR_INVALID_CHARS,
        str.data(), static_cast<int>(str.length()),
        result.data(), static_cast<int>(result.length())))
    {
        assert(static_cast<std::size_t>(size) <= result.length());
        result.resize(size);
    }
    else
    {
        throw_last_error();
    }

    return result;
};

inline std::wstring widen(std::wstring str, UINT = CP_UTF8)
{
    return str;
}

inline std::string narrow(std::wstring_view str, UINT codePage = CP_UTF8)
{
    std::string result;
    if (str.empty())
    {
        // WideCharToMultiByte fails when given a length of zero
        return result;
    }

    // UTF-8 can occupy more characters than an equivalent UTF-16 string. WideCharToMultiByte gives us the required
    // size, so leverage it before we resize the buffer on the first pass of the loop
    for (int size = 0; ; )
    {
        // NOTE: Since we call WideCharToMultiByte with a non-negative input string size, the resulting string is not
        //       null terminated, so we don't need to '+1' the size on input and '-1' the size on resize
        size = ::WideCharToMultiByte(
            codePage,
            (codePage == CP_UTF8) ? WC_ERR_INVALID_CHARS : 0,
            str.data(), static_cast<int>(str.length()),
            result.data(), static_cast<int>(result.length()),
            nullptr, nullptr);

        if (size > 0)
        {
            auto finished = (static_cast<std::size_t>(size) <= result.length());
            assert(finished ^ (result.length() == 0));
            result.resize(size);

            if (finished)
            {
                break;
            }
        }
        else
        {
            throw_last_error();
        }
    }

    return result;
}

inline std::string narrow(std::string str, UINT = CP_UTF8)
{
    return str;
}
#endif
