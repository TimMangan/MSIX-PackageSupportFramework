//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#if _DEBUG
///#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"


std::wstring DetermineIlvPathForReadOperations(Cohorts cohorts, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug)
{
    // Given the cohorts information for a file path, determine the correct path to use when attempting what would be a read operation under ILV.
    // - For anything with a valid mapping for traditional redirection, this means we want the package path.
    // - For anything with a valid mapping for local redirection, this means the local path (if it exists), or as requested if not.

    std::wstring UseFile;
    switch (cohorts.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
        if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
        {
            UseFile = cohorts.WsRequested;
            break;
        }
        else if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
        {
            UseFile = cohorts.WsPackage;
            break;
        }
        else
        {
            UseFile = cohorts.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        UseFile = cohorts.WsRequested;
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
        {
            if (cohorts.UsingNative && PathExists(cohorts.WsNative.c_str()))
            {
                UseFile = cohorts.WsNative;
                break;
            }
            else
            {
                UseFile = cohorts.WsRequested;
                break;
            }
        }
        else if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
        {
            UseFile = cohorts.WsPackage;
            break;
        }
        else
        {
            UseFile = cohorts.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect)
        {
            UseFile = cohorts.WsPackage;
        }
        else
        {
            UseFile = cohorts.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        UseFile = cohorts.WsRequested;
        break;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        UseFile = cohorts.WsRequested;
        break;
    }

    return UseFile;
}  // DetermineIlvPathForReadOperations()


std::wstring DetermineIlvPathForWriteOperations(Cohorts cohorts, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug)
{
    // Given the cohorts information for a file path, determine the correct path to use when attempting what would be a write/create operation under ILV.
    // - For anything with a valid mapping for traditional redirection, this means we want the package path.
    // - For anything with a valid mapping for local redirection, this means the local path. Note that in this case, the caller is responsible for creating 
    // - the local parent path if that parent path does not exist AND the parent path exists in the package.

    std::wstring UseFile;
    switch (cohorts.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
        if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
        {
            UseFile = cohorts.WsRequested;
            break;
        }
        else if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
        {
            UseFile = cohorts.WsPackage;
            break;
        }
        else
        {
            UseFile = cohorts.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        UseFile = cohorts.WsRequested;
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
        {
            if (cohorts.UsingNative)
            {
                UseFile = cohorts.WsNative;
                break;
            }
            else
            {
                UseFile = cohorts.WsRequested;
                break;
            }
        }
        else if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
        {
            UseFile = cohorts.WsPackage;
            break;
        }
        else
        {
            UseFile = cohorts.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect)
        {
            UseFile = cohorts.WsPackage;
        }
        else
        {
            UseFile = cohorts.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        UseFile = cohorts.WsRequested;
        break;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        UseFile = cohorts.WsRequested;
        break;
    }

    return UseFile;
} // DetermineIlvPathForWriteOperations()

bool IsThisALocalPathNow(std::wstring path)
{
    mfr::mfr_path mfr = mfr::create_mfr_path(path);
    if (mfr.Request_MfrPathType == mfr::mfr_path_types::in_native_area)
    {
        return true;
    }
    return false;
} // IsThisLocalPathNow

std::wstring SelectLocalOrPackageForRead(std::wstring localPath, std::wstring packagePath)
{
    if (IsThisALocalPathNow(localPath))
    {
        // In a redirect to local scenario, we are responsible for determing if source is local or in package
        if (!PathExists(localPath.c_str()) && PathExists(packagePath.c_str()))
        {
            return packagePath;
        }
    }
    return localPath;
}  // SelectLocalOrPackageForRead()

void PreCreateLocalFoldersIfNeededForWrite(std::wstring localPath, std::wstring packagePath, DWORD dllInstance,  bool debug, std::wstring debugString)
{
    if (IsThisALocalPathNow(localPath))
    {
        // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
        // if-and-only-if they are present in the package.
        std::filesystem::path usePathAsPath = std::filesystem::path(localPath);
        if (!PathExists(usePathAsPath.parent_path().c_str()))
        {
            std::filesystem::path packagePathAsPath = std::filesystem::path(packagePath);
            if (PathExists(packagePathAsPath.parent_path().c_str()))
            {
                if (debug)
                {
                    Log(L"[%d] %s: Pre-create local parent path to match the package first %s", dllInstance, debugString.c_str(), packagePathAsPath.parent_path().c_str());
                }
                PreCreateFolders(localPath, dllInstance, debugString.c_str());

            }
        }
    }
} // PreCreateLocalFoldersIfNeededForWrite()

void CowLocalFoldersIfNeededForWrite(std::wstring localPath, std::wstring packagePath, DWORD dllInstance, [[maybe_unused]] bool debug, std::wstring debugString)
{
    if (IsThisALocalPathNow(localPath))
    {
        // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
        if (!PathExists(localPath.c_str()) && PathExists(packagePath.c_str()))
        {
            Cow(packagePath, localPath, dllInstance, debugString);
        }
    }
} // CowLocalFoldersIfNeededForWrite()

bool IsThisUnsupportedForInterceptsNow(std::wstring path)
{
    mfr::mfr_path mfr = mfr::create_mfr_path(path);
    switch (mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
    case mfr::mfr_path_types::in_package_pvad_area:
    case mfr::mfr_path_types::in_package_vfs_area:
    case mfr::mfr_path_types::in_redirection_area_other:
        return false;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        return true;
    }
} // IsThisUnsupportedForInterceptsNow()