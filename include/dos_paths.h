//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cassert>
#include <cwctype>
#include <string>

#include <windows.h>

namespace psf
{
    template <typename CharT>
    inline constexpr bool is_path_separator(CharT ch)
    {
        return (ch == '\\') || (ch == '/');
    }

    struct path_compare
    {
        bool operator()(wchar_t lhs, wchar_t rhs)
        {
            if (std::towlower(lhs) == std::towlower(rhs))
            {
                return true;
            }

            // Otherwise, both must be separators
            return is_path_separator(lhs) && is_path_separator(rhs);
        }
    };

    enum class dos_path_type
    {
        unknown,
        unc_absolute,       // E.g. "\\servername\share\path\to\file"
        drive_absolute,     // E.g. "C:\path\to\file"
        drive_relative,     // E.g. "C:path\to\file"
        rooted,             // E.g. "\path\to\file"
        relative,           // E.g. "path\to\file"
        local_device,       // E.g. "\\.\C:\path\to\file" (ex: namedpipes)
        root_local_device,  // E.g. "\\?\C:\path\to\file"
        DosSpecial,         // E.g. "CON4:"
        protocol,           // E.g. "ftp:\\..."
        shell,              // E.g. "shell::{...}" or sometimes shortened as "::{...}
    };

    template <typename CharT>
    inline bool Comp(wchar_t* that, int len, CharT* path)
    {
        return std::equal(that, that + len, path);
    }

    template <typename CharT>
    inline dos_path_type path_type(const CharT* path) noexcept
    {
        if (Comp((wchar_t*)L"CONOUT$", 7, path) ||
            Comp((wchar_t*)L"CONIN$", 6, path) ||
            Comp((wchar_t*)L"CON:",  4, path) || // see list in https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
            Comp((wchar_t*)L"PRN:",  4, path) ||
            Comp((wchar_t*)L"AUX:",  4, path) ||
            Comp((wchar_t*)L"NUL",   3, path) ||
            Comp((wchar_t*)L"COM1:", 5, path) ||
            Comp((wchar_t*)L"COM2:", 5, path) ||
            Comp((wchar_t*)L"COM3:", 5, path) ||
            Comp((wchar_t*)L"COM4:", 5, path) ||
            Comp((wchar_t*)L"COM5:", 5, path) ||
            Comp((wchar_t*)L"COM6:", 5, path) ||
            Comp((wchar_t*)L"COM7:", 5, path) ||
            Comp((wchar_t*)L"COM8:", 5, path) ||
            Comp((wchar_t*)L"COM9:", 5, path) ||
            Comp((wchar_t*)L"LPT1:", 5, path) ||
            Comp((wchar_t*)L"LPT2:", 5, path) ||
            Comp((wchar_t*)L"LPT3:", 5, path) ||
            Comp((wchar_t*)L"LPT4:", 5, path) ||
            Comp((wchar_t*)L"LPT5:", 5, path) ||
            Comp((wchar_t*)L"LPT6:", 5, path) ||
            Comp((wchar_t*)L"LPT7:", 5, path) ||
            Comp((wchar_t*)L"LPT8:", 5, path) ||
            Comp((wchar_t*)L"LPT9:", 5, path))
        {
            return dos_path_type::DosSpecial;
        }
        // NOTE: Root-local device paths don't get normalized and therefore do not allow forward slashes
        constexpr wchar_t root_local_device_prefix[] = LR"(\\?\)";
        if (std::equal(root_local_device_prefix, root_local_device_prefix + 4, path))
        {
            return dos_path_type::root_local_device;
        }

        constexpr wchar_t root_local_device_prefix_dot[] = LR"(\\.\)";
        if (std::equal(root_local_device_prefix_dot, root_local_device_prefix_dot + 4, path))
        {
            return dos_path_type::local_device;
        }

        constexpr wchar_t unc_prefix[] = LR"(\\)";
        if (std::equal(unc_prefix, unc_prefix + 2, path))
        {
            // Otherwise assume any other character is the start of a server name
            return dos_path_type::unc_absolute;
        }

        constexpr wchar_t rooted_prefix[] = LR"(\)";
        if (std::equal(rooted_prefix, rooted_prefix + 1, path))
        {
            return dos_path_type::rooted;
        }

        constexpr wchar_t shell1_prefix[] = LR"(shell::)";
        constexpr wchar_t shell2_prefix[] = LR"(::)";
        if (std::equal(shell1_prefix, shell1_prefix + 7, path) ||
            std::equal(shell2_prefix, shell2_prefix + 2, path))
        {
            return dos_path_type::shell;
        }

        if (std::iswalpha(path[0]) && (path[1] == ':'))
        {
            return is_path_separator(path[2]) ? dos_path_type::drive_absolute : dos_path_type::drive_relative;
        }

        if constexpr (std::is_same_v<CharT, char>)
        {
            if (std::string(path).find(":") != std::string::npos)
            {
                return dos_path_type::protocol;
            }
        }
        else
        {
            if (std::wstring(path).find(L":") != std::wstring::npos)
            {
                return dos_path_type::protocol;
            }
        }

        // Otherwise assume that it's a relative path
        return dos_path_type::relative;
    }


    inline const wchar_t* DosPathTypeName(psf::dos_path_type input)
    {
        switch (input)
        {
        case psf::dos_path_type::unc_absolute:
            return L"unc_absolute";
        case psf::dos_path_type::drive_absolute:
            return L"drive_absolute";
        case psf::dos_path_type::drive_relative:
            return L"drive_relative";
        case psf::dos_path_type::rooted:
            return L"rooted";
        case psf::dos_path_type::relative:
            return L"relative";
        case psf::dos_path_type::local_device:
            return L"local_device";
        case psf::dos_path_type::root_local_device:
            return L"root_local_device";
        case psf::dos_path_type::shell:
            return L"shell::";
        case psf::dos_path_type::protocol:
            return L"protocol:";
        case psf::dos_path_type::unknown:
        default:
            return L"unknown";
        }
    }
        inline DWORD get_full_path_name(const char* path, DWORD length, char* buffer, char** filePart = nullptr)
    {
        return ::GetFullPathNameA(path, length, buffer, filePart);
    }

    inline DWORD get_full_path_name(const wchar_t* path, DWORD length, wchar_t* buffer, wchar_t** filePart = nullptr)
    {
        return ::GetFullPathNameW(path, length, buffer, filePart);
    }

    // Wrapper around GetFullPathName
    // NOTE: We prefer this over std::filesystem::absolute since it saves one allocation/copy in the best case and is
    //       equivalent in the worst case since there's no generic null-terminated-string overload. It also gives us
    //       future flexibility to fixup GetFullPathName, which is something that we can't control in the implementation
    //       of the <filesystem> header
    template <typename CharT>
    inline std::basic_string<CharT> full_path(const CharT* path)
    {
        // Root-local device paths are forwarded to the object manager with minimal modification, so we shouldn't be
        // trying to expand them here
        /////assert(path_type(path) != dos_path_type::root_local_device);

        auto len = get_full_path_name(path, 0, nullptr);
        if (!len)
        {
            // Error occurred. We don't expect to ever see this, but in the event that we do, give back an empty path
            // and let the caller decide how to handle it
            assert(false);
            return {};
        }

        std::basic_string<CharT> buffer(len - 1, '\0');
        len = get_full_path_name(path, len, buffer.data());
        assert(len <= buffer.length());
        if (!len || (len > buffer.length()))
        {
            assert(false);
            return {};
        }

        buffer.resize(len);
        return buffer;
    }
}
