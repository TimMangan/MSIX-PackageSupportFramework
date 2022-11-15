//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ManagedPathTypes.h"
#include "FID.h"
#include "PathUtilities.h"
#include <psf_logging.h>

namespace mfr
{

#if _DEBUG
    const wchar_t* MfrPathTypeName(mfr_path_types mfr)
    {
        switch (mfr)
        {
        case mfr_path_types::in_native_area:
            return L"in_native_area";
        case mfr_path_types::in_other_drive_area:
            return L"in_other_drive_area";
        case mfr_path_types::in_package_pvad_area:
            return L"in_package_pvad_area";
        case mfr_path_types::in_package_vfs_area:
            return L"in_package_vfs_area";
        case mfr_path_types::in_redirection_area_writablepackageroot:
            return L"in_redirection_area_writablepackageroot";
        case mfr_path_types::in_redirection_area_other:
            return L"in_redirection_area_other";
        case mfr_path_types::is_Protocol:
            return L"is_Protocol";
        case mfr_path_types::is_Shell:
            return L"is_Shell";
        case mfr_path_types::is_DosSpecial:
            return L"is_DosSpecial";
        case mfr_path_types::is_UNC_path:
            return L"is_UNC_path";
        case mfr_path_types::unsupported_for_intercepts:
            return L"unsuppotred_for_intercepts";
        case mfr_path_types::unknown:
        default:
            return L"unknown";
        }
    } // MfrPathTypeName()
#endif

    mfr_path_types Get_ManagedPathTypeForDriveAbsolute(std::filesystem::path path)
    {
        if (pathString_isSubsetOf_Path(g_writablePackageRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_redirection_area_writablepackageroot;
        }
        if (pathString_isSubsetOf_Path(g_short_writablePackageRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_redirection_area_writablepackageroot;
        }

        if (pathString_isSubsetOf_Path(g_redirectRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_redirection_area_other;
        }
        if (pathString_isSubsetOf_Path(g_short_redirectRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_redirection_area_other;
        }

        if (pathString_isSubsetOf_Path(g_packageVfsRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_package_vfs_area;
        }
        if (pathString_isSubsetOf_Path(g_short_packageVfsRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_package_vfs_area;
        }

        if (pathString_isSubsetOf_Path(g_packageRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_package_pvad_area;
        }
        if (pathString_isSubsetOf_Path(g_short_packageRootPath.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_package_pvad_area;
        }

        //Log(L"FID_RootDrive  %s", FID_RootDrive.generic_wstring().c_str());
        if (!pathString_isSubsetOf_Path(FID_RootDrive.generic_wstring().c_str(), path))
        {
            return mfr_path_types::in_other_drive_area;
        }
        return mfr_path_types::in_native_area;
    } // Get_ManagedPathTypeForDriveAbsolute()


/// <summary>
///     Given an input path, determine the mfr_path
/// 
    mfr_path create_mfr_path(std::filesystem::path inputPath)
    {
        mfr_path outputPath;
        outputPath.Request_OriginalPath = inputPath;
        outputPath.Request_DosPathType = psf::path_type(inputPath.c_str());
        std::wstring wInputPath = inputPath.c_str();
        switch (outputPath.Request_DosPathType)
        {
        case psf::dos_path_type::drive_absolute:
            outputPath.Request_NormalizedPath = drive_absolute_to_normal(inputPath);
            outputPath.Request_MfrPathType = mfr::Get_ManagedPathTypeForDriveAbsolute(outputPath.Request_NormalizedPath);
            break;
        case psf::dos_path_type::drive_relative:   // E.g. "C:path\to\file"  or shell::{...}

            outputPath.Request_NormalizedPath = drive_relative_to_normal(inputPath);
            outputPath.Request_MfrPathType = mfr::Get_ManagedPathTypeForDriveAbsolute(outputPath.Request_NormalizedPath);
            break;
        case psf::dos_path_type::local_device:   // E.g. "\\.\named_pipe"
            outputPath.Request_NormalizedPath = inputPath;
            outputPath.Request_MfrPathType = mfr::mfr_path_types::unsupported_for_intercepts;
            break;
        case psf::dos_path_type::relative:  // E.g. like  "path\to\file"  
            outputPath.Request_NormalizedPath = cwd_relative_to_normal(inputPath);
            outputPath.Request_MfrPathType = mfr::Get_ManagedPathTypeForDriveAbsolute(outputPath.Request_NormalizedPath);
            break;
        case psf::dos_path_type::rooted: // E.g. "\path\to\file"
            outputPath.Request_NormalizedPath = rooted_relative_to_normal(inputPath);
            outputPath.Request_MfrPathType = mfr::Get_ManagedPathTypeForDriveAbsolute(outputPath.Request_NormalizedPath);
            break;
        case psf::dos_path_type::root_local_device: // E.g. "\\?\C:\path\to\file"
            outputPath.Request_NormalizedPath = root_local_relative_to_normal(inputPath);
            outputPath.Request_MfrPathType = mfr::Get_ManagedPathTypeForDriveAbsolute(outputPath.Request_NormalizedPath);
            break;
        case psf::dos_path_type::unc_absolute:   // E.g. "\\servername\share\path\to\file"
            outputPath.Request_NormalizedPath = inputPath;
            outputPath.Request_MfrPathType = mfr::mfr_path_types::is_UNC_path;
            break;
        case psf::dos_path_type::protocol:      // e.g: "ftp:\\..."
            outputPath.Request_NormalizedPath = inputPath;
            outputPath.Request_MfrPathType = mfr::mfr_path_types::is_Protocol;
            break;
        case psf::dos_path_type::shell:
            outputPath.Request_NormalizedPath = inputPath;
            outputPath.Request_MfrPathType = mfr::mfr_path_types::is_Shell;
            break;
        case psf::dos_path_type::DosSpecial:
            outputPath.Request_NormalizedPath = inputPath;
            outputPath.Request_MfrPathType = mfr::mfr_path_types::is_DosSpecial;
            break;
        case psf::dos_path_type::unknown:
        default:
            outputPath.Request_NormalizedPath = inputPath;
            outputPath.Request_MfrPathType = mfr::mfr_path_types::unsupported_for_intercepts;
            break;
        }
        return outputPath;
    } // create_mfr_path()



}