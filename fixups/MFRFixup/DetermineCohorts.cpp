//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include "DetermineCohorts.h"
#include <psf_logging.h>
#include "PathUtilities.h"


using namespace std::literals;

/// DetermineCohorts
///
/// This function takes in a file path that was one of the inputs to an API call that we are intercepting,
/// and it determines what all of the possible filepaths we should consider.  This is done based on
/// the kind of path sent in, as well as the configured mappings that should be used.
/// 
/// All work in this function is being done in memory with string manipulation and no file API calls are made.
/// 
/// This information is returned in the cohorts structure, which has all three possible file paths, as well as mapping
/// information that will be useful to the caller in deciding which paths to use and what to do with them.
/// 
void DetermineCohorts(std::wstring requestedPath, Cohorts *cohorts, bool UseMoreDebug, DWORD dllInstance, const wchar_t * FixupName)
{

    cohorts->file_mfr = mfr::create_mfr_path(requestedPath);
    cohorts->WsRequested = cohorts->file_mfr.Request_NormalizedPath.c_str();
    cohorts->UsingNative = true;

    switch (cohorts->file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
        if (UseMoreDebug)
        {
            Log(L"[%d] %s:  Request is in_native_area.", dllInstance, FixupName);
        }
        cohorts->map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str());
        if (cohorts->map.Valid_mapping)
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s:  Mapping match against local  %s", dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            if (!cohorts->map.IsAnExclusionToRedirect)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps with known local redirection type %s", dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.PackagePathBase);
                //cohorts->WsNative = cohorts->WsRequested;
                cohorts->UsingNative = false;
                break;
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps to a known local redirection exclusion path.", dllInstance, FixupName);
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                //cohorts->WsNative = cohorts->WsRequested;
                break;
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s:  No local mapping is valid.", dllInstance, FixupName);
            }
        }

        cohorts->map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str());
        if (cohorts->map.Valid_mapping)
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s:  Mapping match against traditional  %s", dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            if (!cohorts->map.IsAnExclusionToRedirect)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps with known traditional redirection type %s", dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                // Exception processing
                // We shouln't redirect to traditional area if the call was only to the WindowsApps folder.
                // This can cause an issue in an app like R (language) that deals with Short Names and we can't force shortnames to be the same
                // in the redirection area that they are natively. (Well, we could if MFR did everything but not with ILV in use as it creates these things behind our back.
                if (comparei(cohorts->WsRequested, L"C:\\Program Files\\WindowsApps"))
                {
                    cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.PackagePathBase);
                    cohorts->WsRedirected = cohorts->WsPackage;
                }
                else
                {
                    cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.RedirectedPathBase);
                    cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.PackagePathBase);
                }
                cohorts->WsNative = cohorts->WsRequested;
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps to a known traditional redirection exclusion path.", dllInstance, FixupName);
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s  No traditional mapping is valid.", dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        if (UseMoreDebug)
        {
            Log(L"[%d] %s:  Request is in_package_pvad_area.", dllInstance, FixupName);
        }
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str());
        if (cohorts->map.Valid_mapping)
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s:  Mapping match against traditional  %s", dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            if (!cohorts->map.IsAnExclusionToRedirect)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps with known traditional redirection type %s", dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.RedirectedPathBase);
                cohorts->UsingNative = false;
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps to a known traditional redirection exclusion path.", dllInstance, FixupName);
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s  No traditional mapping is valid.", dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        if (UseMoreDebug)
        {
            Log(L"[%d] %s:  Request is in_package_vfs_area.", dllInstance, FixupName);
        }
        cohorts->map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str());
        if (cohorts->map.Valid_mapping)
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s:  Mapping match against local  %s", dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            if (!cohorts->map.IsAnExclusionToRedirect)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps with known local redirection type %s", dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.RedirectedPathBase);
                //cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                cohorts->UsingNative = false;
                break;
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps to a known local redirection exclusion path.", dllInstance, FixupName);
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
                break;
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s  No local mapping is valid.", dllInstance, FixupName);
            }
        }

        cohorts->map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str());
        if (cohorts->map.Valid_mapping)
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s:  Mapping match against traditional  %s", dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            if (!cohorts->map.IsAnExclusionToRedirect)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps with known traditional redirection type %s", dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.RedirectedPathBase);
                cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps to a known traditional redirection exclusion path.", dllInstance, FixupName);
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s  No traditional mapping is valid.", dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        if (UseMoreDebug)
        {
            Log(L"[%d] %s:  Request is in_redirection_area_writablepackageroot.", dllInstance, FixupName);
        }
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str());
        if (cohorts->map.Valid_mapping)
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s:  Mapping match against traditional  %s", dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            if (!cohorts->map.IsAnExclusionToRedirect)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps with known traditional redirection type %s", dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.PackagePathBase);
                if (cohorts->WsPackage.find(L"\\VFS\\") != std::wstring::npos)
                {
                    cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.NativePathBase);
                }
                else
                {
                    cohorts->UsingNative = false;  //request was redirected area for a PVAD path in package.  No native possible.
                }
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%d] %s:  Maps to a known traditional redirection exclusion path.", dllInstance, FixupName);
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%d] %s  No traditional mapping is valid.", dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        if (UseMoreDebug)
        {
            Log(L"[%d] %s:  Request is in_redirection_area_other.", dllInstance, FixupName);
        }
        cohorts->UsingNative = false;
        break;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        if (UseMoreDebug)
        {
            Log(L"[%d] %s:  Request is in_non_redirectable_areas.", dllInstance, FixupName);
        }
        cohorts->UsingNative = false;
        break;
    }

    if (UseMoreDebug)
    {
        if (cohorts->map.Valid_mapping)
        {
            Log(L"[%d] %s:    Cohort->WsRedirected %s", dllInstance, FixupName, cohorts->WsRedirected.c_str());
            Log(L"[%d] %s:    Cohort->WsPackage    %s", dllInstance, FixupName, cohorts->WsPackage.c_str());
            Log(L"[%d] %s:    Cohort->WsNative %d   %s", dllInstance, FixupName, cohorts->UsingNative, cohorts->WsNative.c_str());
        }
        else
        {
            Log(L"[%d] %s:    Cohort->map.Valid_Mapping %d", dllInstance, FixupName, cohorts->map.Valid_mapping);
        }
    }

    
}