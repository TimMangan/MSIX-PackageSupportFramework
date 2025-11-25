//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cwctype>
#include <filesystem>

#include <windows.h>
#include <combaseapi.h>
#include <Shlobj.h>
#include <KnownFolders.h>

#include "dos_paths.h"
#include "psf_logging.h"
// Helper function to convert GUID to wstring
#include <sstream>
#include <iomanip>

namespace psf
{
    struct cotaskmemfree_deleter
    {
        void operator()(wchar_t* ptr)
        {
            if (ptr)
            {
                ::CoTaskMemFree(ptr);
            }
        }
    };

    inline std::wstring GuidToString(const GUID& guid)
    {
        std::wstringstream ss;
        try
        {
            ss << std::hex << std::setfill(L'0')
                << L'{'
                << std::setw(8) << guid.Data1 << L'-'
                << std::setw(4) << guid.Data2 << L'-'
                << std::setw(4) << guid.Data3 << L'-'
                << std::setw(2) << static_cast<int>(guid.Data4[0])
                << std::setw(2) << static_cast<int>(guid.Data4[1]) << L'-'
                << std::setw(2) << static_cast<int>(guid.Data4[2])
                << std::setw(2) << static_cast<int>(guid.Data4[3])
                << std::setw(2) << static_cast<int>(guid.Data4[4])
                << std::setw(2) << static_cast<int>(guid.Data4[5])
                << std::setw(2) << static_cast<int>(guid.Data4[6])
                << std::setw(2) << static_cast<int>(guid.Data4[7])
                << L'}';
            return ss.str();
        }
        catch (...)
        {
            return L"{BAD}";
        }
    }

    using unique_cotaskmem_string = std::unique_ptr<wchar_t, cotaskmemfree_deleter>;

    inline std::filesystem::path remove_trailing_path_separators(std::filesystem::path path)
    {
        // To make string comparisons easier, don't terminate directory paths with trailing separators
        return path.has_filename() ? path : path.parent_path();
    }

    inline std::filesystem::path known_folder(const GUID& id, DWORD flags = KF_FLAG_DEFAULT)
    {
        PWSTR path;
        if (FAILED(::SHGetKnownFolderPath(id, flags, nullptr, &path)))
        {
            throw std::runtime_error("Failed to get known folder path");
        }
        unique_cotaskmem_string uniquePath(path);
        path[0] = std::towupper(path[0]);

        // For consistency, and therefore simplicity, ensure that all paths are drive-absolute
        if (auto pathType = path_type(path);
            (pathType == dos_path_type::root_local_device) || (pathType == dos_path_type::local_device))
        {
            path += 4;
        }
        // We have seen a case at a customer where one of the KnownFolders isn't drive_absolute.
        // For now, we will allow it here but log it.
        // Possibly we can convert some of them, like \\?\\C:\... to C:\... in the code here to resolve also,
        // once we know what type of paths we are getting.
        //assert(path_type(path) == dos_path_type::drive_absolute);
        auto updatedPathType = path_type(path);
        if (updatedPathType != dos_path_type::drive_absolute)
        {
            Log(Json_Debug_Levels::LogLevel_Exception, L"[M%d] ERROR IN KNOWN FOLDER maps to type %d  path%s\n", 0, (int)updatedPathType,path);
            Log(Json_Debug_Levels::LogLevel_Exception, L"[M%d] Guid to map is %s\n", 0, GuidToString(id).c_str());
        }

        return remove_trailing_path_separators(path);
    }
}
