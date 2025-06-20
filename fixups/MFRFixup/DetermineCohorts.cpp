//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include "DetermineCohorts.h"
#include <psf_logging.h>
#include "PathUtilities.h"
#include "FunctionImplementations.h"




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

    // Temporary debug code to help with a specific issue.
    //if (requestedPath._Equal(L"C:\\Users") ||
    //    requestedPath._Equal(L"C:\\Users\\"))
    //{
    //    UseMoreDebug = true;
    //}

    switch (cohorts->file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
        // This means we were given a path that was not a relative path.
        if (UseMoreDebug)
        {
            Log(L"[%s%d] %s: DetermineCohorts: Request is in_native_area.", g_MfrModuleName, dllInstance, FixupName);
        }
        cohorts->map = mfr::Find_RedirMapping_FromNativePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (UseMoreDebug)
            {
                if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against local  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
                else
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping match against local  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
            }
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping is for redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                //cohorts->WsRedirected = cohorts->WsRequested;
                //cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.PackagePathBase);
                ////cohorts->WsNative = cohorts->WsRequested;
                //cohorts->UsingNative = false;
                //cohorts->WsRedirected = cohorts->WsRequested;
                //cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.PackagePathBase);
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.RedirectedPathBase);
                cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.PackagePathBase);
                cohorts->WsNative = cohorts->WsRequested;
                break;
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Maping is for EXCLUSION redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                //cohorts->WsRedirected = cohorts->WsRequested;
                //cohorts->WsPackage = cohorts->WsRequested;
                ////cohorts->WsNative = cohorts->WsRequested;
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
                Log(L"[%s%d] %s: DetermineCohorts: No redirect to local mapping matched.", g_MfrModuleName, dllInstance, FixupName);
            }
        }

        // If the path entered is mappable from a native path to a package path, or redirection area to a package path, we catch these here
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (UseMoreDebug)
            {
                if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
                else
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
            }
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping is for redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                // Exception processing
                // We shouln't redirect to traditional area if the call was only to the WindowsApps folder.
                // This can cause an issue in an app like R (language) that deals with Short Names and we can't force shortnames to be the same
                // in the redirection area that they are natively. (Well, we could if MFR did everything but not with ILV in use as it creates these things behind our back.
                if (comparei(cohorts->WsRequested, L"C:\\Program Files\\WindowsApps"))
                {
#if _DEBUG
                    Log(L"[%s%d] %s: DetermineCohorts: Windows Apps special exclusion.", g_MfrModuleName, dllInstance, cohorts->WsRequested.c_str());
#endif
                    cohorts->WsPackage = cohorts->WsRequested; // ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.PackagePathBase);
                    cohorts->WsRedirected = cohorts->WsPackage;
                    cohorts->WsNative = cohorts->WsRequested;
                    cohorts->map.IsAnExclusionToRedirect = mfr::mfr_exclusion_types::excluded;
                }
                else
                {
                    // Change 2025/2/15: I think this was wrong.
                    //cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.RedirectedPathBase);
                    //cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.PackagePathBase);
                    //cohorts->WsNative = cohorts->WsRequested;

                    // We have a native path that has a mapping (it was OK)
                    cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.RedirectedPathBase);
                    cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.PackagePathBase);
                    cohorts->WsNative = cohorts->WsRequested;
                }
            }
            else
            {
                // We a mapping, but it is marked for exclusion.  Just use as requested.
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping is for EXCLUSION redirection type %s.", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
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
                Log(L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        if (UseMoreDebug)
        {
            Log(L"[%s%d] %s: DetermineCohorts: Request is in package_pvad_area.", g_MfrModuleName, dllInstance, FixupName);
        }
        cohorts->WsPackage = cohorts->WsRequested;
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (UseMoreDebug)
            {
                Log(L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Maps with known traditional redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.RedirectedPathBase);
                cohorts->UsingNative = false;
            }
            else
            {
                // We don't actually have any of these as it makes no sense.
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Maps to a known traditional redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
                }
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        if (UseMoreDebug)
        {
            Log(L"[%s%d] %s: DetermineCohorts: Request is in_package_vfs_area.", g_MfrModuleName, dllInstance, FixupName);
        }
        cohorts->map = mfr::Find_RedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (UseMoreDebug)
            {
                if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against local  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
                else
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping match against local  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
            }
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                if (cohorts->map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                {
                    if (UseMoreDebug)
                    {
                        Log(L"[%s%d] %s: DetermineCohorts: Maps with known local redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                    }
                    cohorts->WsPackage = cohorts->WsRequested;
                    cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                    cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                    //cohorts->UsingNative = false;
                    break;
                }
            }
            else
            {
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Maps to a known local redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
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
                Log(L"[%s%d] %s DetermineCohorts: No local mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
            }
        }

        cohorts->map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (UseMoreDebug)
            {
                if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
                else
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
            }
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Maps with known traditional redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.RedirectedPathBase);
                cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
            }
            else
            {
                // We have a tratidional redirection mapping marked for exclusion. This means that we don't want to look in the package
                // or redirection area but look at the native path only
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Maps to a known traditional redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
                }
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
            }
        }
        else
        {
            if (UseMoreDebug)
            {
                Log(L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        if (UseMoreDebug)
        {
            Log(L"[%s%d] %s: DetermineCohorts: Request is in_redirection_area_writablepackageroot.", g_MfrModuleName, dllInstance, FixupName);
        }
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(),dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (UseMoreDebug)
            {
                if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
                else
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
                }
            }
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                if (UseMoreDebug)
                {
                    Log(L"[%s%d] %s: DetermineCohorts: Maps with known traditional redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
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
                    Log(L"[%s%d] %s: DetermineCohorts: Maps to a known traditional redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
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
                Log(L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
            }
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        if (UseMoreDebug)
        {
            Log(L"[%s%d] %s: DetermineCohorts: Request is in_redirection_area_other.", g_MfrModuleName, dllInstance, FixupName);
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
            Log(L"[%s%d] %s: DetermineCohorts: Request is in_non_redirectable_areas.", g_MfrModuleName, dllInstance, FixupName);
        }
        cohorts->UsingNative = false;
        break;
    }

    
    // Remove trailing backslashes, except for C:\ 
    if (cohorts->WsRedirected.length() > 3)
    {
        if (cohorts->WsRedirected.back() == L'\\')
        {
            cohorts->WsRedirected.pop_back();
        }
    }

    if (UseMoreDebug)
    {
        Log(L"[%s%d] %s: DetermineCohorts:   Cohort->WsRequested  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->WsRequested.c_str());
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            Log(L"[%s%d] %s: DetermineCohorts:   Cohort->WsRedirected %s", g_MfrModuleName, dllInstance, FixupName, cohorts->WsRedirected.c_str());
            Log(L"[%s%d] %s: DetermineCohorts:   Cohort->WsPackage    %s", g_MfrModuleName, dllInstance, FixupName, cohorts->WsPackage.c_str());
            Log(L"[%s%d] %s: DetermineCohorts:   Cohort->WsNative UseNative=%d   %s", g_MfrModuleName, dllInstance, FixupName, cohorts->UsingNative, cohorts->WsNative.c_str());
        }
        else
        {
            Log(L"[%s%d] %s: DetermineCohorts:   Cohort->map.Valid_Mapping %d", g_MfrModuleName, dllInstance, FixupName, cohorts->map.Valid_mapping);
        }
    }

    
}




