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

#if DIDNTHELP
extern void CheckFileForIlvAnomoly(DWORD, std::wstring);
#endif

std::wstring DetermineIlvPathForReadOperations(Cohorts cohorts, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug)
{
    // Given the cohorts information for a file path, determine the correct path to use when attempting what would be a read operation under ILV.
    // - For anything with a valid mapping for traditional redirection, this means we want the package path.
    // - For anything with a valid mapping for local redirection, this means the local path (if it exists), or as requested if not.

    // In ILV mode, requesting the native path when it is in the package VFS or redirected area, will find it even if not present natively - but only for supported VFS paths.
    // However, if it reaally isn't present natively, we shouldn't use the native path but the VFS path.
    // This is because requests using the native path don't notice the ILV deletion marker.

    std::wstring UseFile;
    DWORD oldErr = GetLastError();
    DWORD RequestedAttributes = impl::GetFileAttributes(MakeLongPath(cohorts.WsRequested).c_str());
    [[maybe_unused]] DWORD RequestedError = GetLastError();
    DWORD PackageAttributes = impl::GetFileAttributes(MakeLongPath(cohorts.WsPackage).c_str());
    [[maybe_unused]] DWORD PackageError = GetLastError();
    DWORD RedirectedAttributes = impl::GetFileAttributes(MakeLongPath(cohorts.WsRedirected).c_str());
    [[maybe_unused]] DWORD RedirectedError = GetLastError();
    bool RedirectoinDeletionMarker = false;
    if (RedirectedAttributes != INVALID_FILE_ATTRIBUTES &&
        (RedirectedAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
    {
        // Might want to also make additional checks, but this does seem sufficient as no other redirected files should be marked system-hidden.
        RedirectoinDeletionMarker = true;
    }
    //std::wstring lcRedirected = (MakeLongPath(cohorts.WsRedirected).c_str());
    //std::transform(
    //    lcRedirected.begin(), lcRedirected.end(),
    //    lcRedirected.begin(),
    //        towlower);
#if DIDNTHELP
    WIN32_FILE_ATTRIBUTE_DATA finfoPackage;
    WIN32_FILE_ATTRIBUTE_DATA finfoRedirected;
    DWORD PackageAttributesEx = impl::GetFileAttributesEx(cohorts.WsPackage.c_str(), GetFileExMaxInfoLevel, &finfoRedirected);
    DWORD RedirectedAttributesEx = impl::GetFileAttributesEx(cohorts.WsRedirected.c_str(), GetFileExMaxInfoLevel, &finfoPackage);
#endif
    SetLastError(oldErr);
    if (moredebug)
    {
        Log(L"[0x%d]       DetermineILVPaths Atts Req=0x%x/0x%x Pkg=0x%x/0x%x Redir=0x%x/0x%x", dllInstance, RequestedAttributes, RequestedError, PackageAttributes, PackageError, RedirectedAttributes, RedirectedError);
#if DIDNTHELP
        Log(L"[0x%d]       DetermineILVPaths ExAtt Pkg ret=0x%x att=0x%x Szs=0x%x/0x%x ", dllInstance, PackageAttributesEx, finfoPackage.dwFileAttributes, finfoPackage.nFileSizeHigh, finfoPackage.nFileSizeLow);
        Log(L"[0x%d]       DetermineILVPaths ExAtt Rdr ret=0x%x att=0x%x Szs=0x%x/0x%x ", dllInstance, RedirectedAttributesEx, finfoRedirected.dwFileAttributes, finfoRedirected.nFileSizeHigh, finfoRedirected.nFileSizeLow);
        CheckFileForIlvAnomoly(dllInstance, cohorts.WsPackage);
        CheckFileForIlvAnomoly(dllInstance, cohorts.WsRedirected);
#endif
    }
    switch (cohorts.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area: 
        if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
        {
            // for REQUESTED, PACKAGE, REDIRECTED
            //K1 True, True, True    : means it might or mightnot be native, but we have a redirected copy and it was not deleted:                            USE=Package
            //K2 True, True, False   : means it might or mightnot be native, it is in package, but we not made a redirected copy  deleted it.:                USE=Package
            //K3 True, False, True   : means it might or moghtnot be native, it probably was in package, but we have a deletion marker, or redirected file    USE=Requested unless deleted, Package otherwise
            //K4 True, False, False  : means it is native, not in package, and no redirected copy:                                                            USE=Requested
            //K5 False, True, True   : means it is not native, is in package in unmapped VFS, and we have a redirected copy                                   USE=Package
            //K6 False, True, False  : means it is not native, is in package in unmapped VFS, and there is no redirected copy                                 USE=Package 
            //K7 False, False, True  : means is either didn't exist, or we have a redirected deletion marker, or unexpected copy                              USE=Package if deleted, Redirected otherwise
            //K8 False, False, False : means is doesn't exist                                                                                                 USE: Requested
            if (RequestedAttributes != INVALID_FILE_ATTRIBUTES)
            {
                if (PackageAttributes != INVALID_FILE_ATTRIBUTES)
                {
                    // K1 and K2
                    UseFile = cohorts.WsPackage;
                }
                else
                {
                    if (RedirectedAttributes != INVALID_FILE_ATTRIBUTES)
                    {
                        // K3
                        if (RedirectoinDeletionMarker)
                        {
                            UseFile = cohorts.WsPackage;
                        }
                        else
                        {
                            UseFile = cohorts.WsRequested;
                        }
                    }
                    else
                    {
                        // K4
                        UseFile = cohorts.WsRequested;
                    }
                }
            }
            else
            {
                if (PackageAttributes != INVALID_FILE_ATTRIBUTES)
                {
                    // K5 and K6
                    UseFile = cohorts.WsPackage;
                }
                else
                {
                    if (RedirectedAttributes != INVALID_FILE_ATTRIBUTES)
                    {
                        // K7
                        if (RedirectoinDeletionMarker)
                        {
                            UseFile = cohorts.WsPackage;
                        }
                        else
                        {
                            UseFile = cohorts.WsRedirected;
                        }
                    }
                    else
                    {
                        // K8
                        UseFile = cohorts.WsRequested;
                    }
                }
            }
            break;
        }
        else if (cohorts.map.Valid_mapping && !cohorts.map.IsAnExclusionToRedirect &&
            (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
        {
            // for REQUESTED, PACKAGE, REDIRECTED
            //K1 True, True, True    : means it might or mightnot be native, but we have a redirected copy and it was not deleted:                            USE=Package
            //K2 True, True, False   : means it might or mightnot be native, it is in package, but we not made a redirected copy or deleted it.:              USE=Package
            //K3 True, False, True   : means it might or moghtnot be native, it probably was in package, but we have a deletion marker, or redirected file    USE=Requested unless deleted, Package otherwise
            //K4 True, False, False  : means it is native, not in package, and no redirected copy:                                                            USE=Requested
            //K5 False, True, True   : means it is not native, is in package in unmapped VFS, and we have a redirected copy                                   USE=Package
            //K6 False, True, False  : means it is not native, is in package in unmapped VFS, and there is no redirected copy                                 USE=Package 
            //K7 False, False, True  : means is either didn't exist, or we have a redirected deletion marker, or unexpected copy                              USE=Package if deleted, Redirected otherwise
            //K8 False, False, False : means is doesn't exist                                                                                                 USE: Requested
            if (RequestedAttributes != INVALID_FILE_ATTRIBUTES)
            {
                if (PackageAttributes != INVALID_FILE_ATTRIBUTES)
                {
                    // K1 and K2
                    UseFile = cohorts.WsPackage;
                }
                else
                {
                    if (RedirectedAttributes != INVALID_FILE_ATTRIBUTES)
                    {
                        // K3
                        if (RedirectoinDeletionMarker)
                        {
                            UseFile = cohorts.WsPackage;
                        }
                        else
                        {
                            UseFile = cohorts.WsRequested;
                        }
                    }
                    else
                    {
                        // K4
                        UseFile = cohorts.WsRequested;
                    }
                }
            }
            else
            {
                if (PackageAttributes != INVALID_FILE_ATTRIBUTES)
                {
                    // K5 and K6
                    UseFile = cohorts.WsPackage;
                }
                else
                {
                    if (RedirectedAttributes != INVALID_FILE_ATTRIBUTES)
                    {
                        // K7
                        if (RedirectoinDeletionMarker)
                        {
                            UseFile = cohorts.WsPackage;
                        }
                        else
                        {
                            UseFile = cohorts.WsRedirected;
                        }
                    }
                    else
                    {
                        // K8
                        UseFile = cohorts.WsRequested;
                    }
                }
            }
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
            else if (PathExists(cohorts.WsRedirected.c_str()))
            {
                UseFile = cohorts.WsRedirected;
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
bool IsThisAPackagePathNow(std::wstring path)
{
    mfr::mfr_path mfr = mfr::create_mfr_path(path);
    if (mfr.Request_MfrPathType == mfr::mfr_path_types::in_package_pvad_area ||
        mfr.Request_MfrPathType == mfr::mfr_path_types::in_package_vfs_area)
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
} // PreCreateLocalFoldersIfNeededForWrite() // PreCreateLocalFoldersIfNeededForWrite()

void PreCreatePackageFoldersIfIlvNeededForWrite(std::wstring filePath, DWORD dllInstance, bool debug, std::wstring debugString)
{
    if (IsThisAPackagePathNow(filePath))
    {
        // In an ILV situation, we may need to create parent folders.
        if (!PathExists(filePath.c_str()))
        {
            std::filesystem::path packagePathAsPath = std::filesystem::path(filePath);
            if (!PathExists(packagePathAsPath.parent_path().c_str()))
            {
                if (debug)
                {
                    Log(L"[%d] %s: Pre-create package parent path to match the package first %s", dllInstance, debugString.c_str(), packagePathAsPath.parent_path().c_str());
                }
                PreCreateFolders(filePath, dllInstance, debugString.c_str());
            }
        }
    }
} // PreCreatePackageFoldersIfIlvNeededForWrite()

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

#if DIDNTHELP
void CheckFileForIlvAnomoly(DWORD dllInstance, std::wstring filePath)
{
    size_t pos = filePath.find_last_of('\\');
    if (pos != std::wstring::npos && pos>2)
    {
        std::wstring parentFolder = filePath.substr(0, pos);
        std::wstring basename = filePath.substr(pos + 1);
        std::wstring findlc = basename;
        std::transform(findlc.begin(), findlc.end(), findlc.begin(), towlower);

        WIN32_FIND_DATA FindData;
        std::wstring searchCriteria = parentFolder;
        searchCriteria.append(L"\\*");
        HANDLE h = impl::FindFirstFile(searchCriteria.c_str(), &FindData);
        if (h != NULL && h != INVALID_HANDLE_VALUE)
        {
            //std::wstring found = FindData.cFileName;
            //std::transform(found.begin(), found.end(), found.begin(), towlower);
            //if (findlc._Equal(found))
            {
                // got it
                Log(L"[%d] CheckFileForIlvAnomoly %s %s 0x%x 0x%x 0x%x", dllInstance, filePath.c_str(), FindData.cFileName, FindData.dwFileAttributes, FindData.dwReserved0, FindData.dwReserved1);
            }
            //else
            {
                while (FindNextFile(h, &FindData))
                { 
                    //found = FindData.cFileName;
                    //std::transform(found.begin(), found.end(), found.begin(), towlower);
                    //if (findlc._Equal(found))
                    {
                        // got it
                        Log(L"[%d] CheckFileForIlvAnomoly %s %s 0x%x 0x%x 0x%x", dllInstance, filePath.c_str(), FindData.cFileName, FindData.dwFileAttributes, FindData.dwReserved0, FindData.dwReserved1);
                        break;
                    } 
                }
            }
            FindClose(h);
        }
    }
}
#endif