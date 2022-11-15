#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>

namespace mfr
{

    // When an interect gets a file path from the caller, it is convenient to categorize the type of path that we are dealing with to aid in determining how to handle it.
    enum class mfr_path_types
    {
        unknown = 0x0000,   // Pre-initialized value           
        unsupported_for_intercepts = 0x0001,  // includes special file paths like CONOUT$ and NUL and \\.\pipe and others that we should just pass through and let the FS deal with as is.
        in_redirection_area_writablepackageroot = 0x0002,   // The path is in the container redirection area WritablePackageRoot
        in_redirection_area_other = 0x0003,   // The path is in the container redirection area that we don't manage 
        in_package_pvad_area = 0x0004,   // The path is in the package area (PVAD)
        in_package_vfs_area = 0x0005,   // The path is in the package area (VFS)
        in_native_area = 0x0006,   // In package drive but not in package.
        in_other_drive_area = 0x0007,   // On a different drive letter.
        is_UNC_path = 0x0008,   // The path is a a network share (like \\server\share\path)
        is_Shell = 0x0009,  // The path is a special shell::{guid} that should not be redirected.
        is_Protocol = 0x000A,   // the path is a protocol like "ftp:\\" 
        is_DosSpecial = 0x000B,  // like "COM1:"
    };
    DEFINE_ENUM_FLAG_OPERATORS(mfr_path_types);


    typedef struct mfr_path
    {
        std::filesystem::path Request_OriginalPath;
        std::filesystem::path Request_NormalizedPath;
        psf::dos_path_type    Request_DosPathType = psf::dos_path_type::unknown;
        mfr_path_types        Request_MfrPathType = mfr_path_types::unknown;
    } mfr_path;


#if _DEBUG
    extern const wchar_t* MfrPathTypeName(mfr_path_types mfr);
#endif


    extern mfr_path_types Get_ManagedPathTypeForDriveAbsolute(std::filesystem::path path);

    extern mfr_path create_mfr_path(std::filesystem::path inputPath);


}