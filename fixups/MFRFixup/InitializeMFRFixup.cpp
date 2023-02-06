//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <vector>
#include <known_folders.h>
#include <objbase.h>
#include <psf_framework.h>

#include <utilities.h>
#include <psf_logging.h>
#include "FID.h"
#include "PathUtilities.h"

#include "ManagedFileMappings.h"

using namespace std::literals;


std::filesystem::path g_packageRootPath;
std::filesystem::path g_packageVfsRootPath;
std::filesystem::path g_redirectRootPath;
std::filesystem::path g_writablePackageRootPath;
std::filesystem::path g_finalPackageRootPath;

std::filesystem::path g_short_packageRootPath;
std::filesystem::path g_short_packageVfsRootPath;
std::filesystem::path g_short_redirectRootPath;
std::filesystem::path g_short_writablePackageRootPath;
std::filesystem::path g_short_finalPackageRootPath;


#if _DEBUG
//#define MOREDEBUG 1
#endif

DWORD g_InterceptInstance = 60000;


void InitializeMFRFixup()
{
#if MOREDEBUG
    Log("\t\tMFRFixup InitializeMFRFixup: start");
#endif  

    // For path comparison's sake - and the fact that std::filesystem::path doesn't handle (root-)local device paths all
    // that well - ensure that these paths are drive-absolute
    auto packageRootPath = std::wstring(::PSFQueryPackageRootPath());
    auto pathType = psf::path_type(packageRootPath.c_str());
    if (pathType == psf::dos_path_type::root_local_device || (pathType == psf::dos_path_type::local_device))
    {
        packageRootPath += 4;
    }
    assert(psf::path_type(packageRootPath.c_str()) == psf::dos_path_type::drive_absolute);
    //WHY???   transform(packageRootPath.begin(), packageRootPath.end(), packageRootPath.begin(), towlower);
    g_packageRootPath = psf::remove_trailing_path_separators(packageRootPath);

    g_packageVfsRootPath = g_packageRootPath / L"VFS";

    auto finalPackageRootPath = std::wstring(::PSFQueryFinalPackageRootPath());
    g_finalPackageRootPath = psf::remove_trailing_path_separators(finalPackageRootPath);  // has \\?\ prepended to PackageRootPath

    // Ensure that the redirected root path exists
    // We see some issues with multiple processes starting up and making the create_directories call simultaniously causing the second one to hit an exception.
    // We can ignore those issues.
    std::error_code ec;
    g_redirectRootPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\VFS)";
    try
    {
        std::filesystem::create_directories(g_redirectRootPath);
    }
    catch (...)
    {
#ifdef _DEBUG
        Log("\t\tMfrFixup ignorable exception creating directories.");
#endif
    }

    g_writablePackageRootPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";
    try
    {
        std::filesystem::create_directories(g_writablePackageRootPath);
    }
    catch (...)
    {
#ifdef _DEBUG
        Log("\t\tMfrFixup ignorable exception creating directories.");
#endif
    }

    g_short_packageRootPath = ConvertPathToShortPath(g_packageRootPath);
    g_short_packageVfsRootPath = ConvertPathToShortPath(g_packageVfsRootPath);
    g_short_redirectRootPath = ConvertPathToShortPath(g_redirectRootPath);
    g_short_writablePackageRootPath = ConvertPathToShortPath(g_writablePackageRootPath);
    g_short_finalPackageRootPath = ConvertPathToShortPath(g_finalPackageRootPath);

#if MOREDEBUG
    Log("\t\tMFRFixup InitializeMFRFixup: mid");
#endif

    mfr::Initialize_MFR_Mappings();

#if MOREDEBUG
    Log("\t\tMFRFixup InitializeMFRFixup: end");
#endif  
}  //InitializeMFRFixup()
