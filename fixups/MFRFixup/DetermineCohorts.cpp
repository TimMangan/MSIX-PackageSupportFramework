//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include "DetermineCohorts.h"
#include <psf_logging.h>
#include "PathUtilities.h"
#include "FunctionImplementations.h"




using namespace std::literals;

// 8.3 short paths: the mapping tables only know the long form of the package root and of the writable package root,
// but applications may use any mix of long and short folder names, e.g. C:\PROGRA~1\WindowsApps\<SHORTN~1>\... or
// C:\Program Files\WindowsApps\<SHORTN~1>\... (seen on a CimFS app attach volume). Such a request is not mapped and
// never redirected (CreateDirectory into the package -> ERROR_ACCESS_DENIED, or a nested redirection path).
// Compare the request folder by folder with the root, accepting the long or the short name of each root folder,
// and replace the matching part with the long root.
struct RootForms
{
    std::wstring LongRoot;
    std::vector<std::pair<std::wstring, std::wstring>> Parts;  // long and short name of each folder of the root
};

static std::vector<std::wstring> SplitPathParts(const std::wstring& p)
{
    std::vector<std::wstring> parts;
    std::wstring cur;
    for (wchar_t c : p)
    {
        if (c == L'\\' || c == L'/')
        {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        }
        else
        {
            cur += c;
        }
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

static RootForms MakeRootForms(const std::filesystem::path& longRoot)
{
    RootForms forms;
    forms.LongRoot = longRoot.wstring();
    while (!forms.LongRoot.empty() && (forms.LongRoot.back() == L'\\' || forms.LongRoot.back() == L'/')) forms.LongRoot.pop_back();
    auto longParts = SplitPathParts(forms.LongRoot);
    std::wstring prefix;
    for (size_t i = 0; i < longParts.size(); i++)
    {
        prefix += (i == 0) ? longParts[i] : (L"\\" + longParts[i]);
        std::wstring shortName = longParts[i];
        if (i > 0)
        {
            // called while the reentrancy guard is held: the underlying file APIs are not redirected
            DWORD len = ::GetShortPathNameW(prefix.c_str(), nullptr, 0);
            if (len != 0)
            {
                std::wstring buf(len, L'\0');
                len = ::GetShortPathNameW(prefix.c_str(), &buf[0], len);
                if (len != 0)
                {
                    buf.resize(len);
                    auto sp = SplitPathParts(buf);
                    if (!sp.empty()) shortName = sp.back();
                }
            }
        }
        forms.Parts.push_back({ longParts[i], shortName });
    }
    return forms;
}

static std::filesystem::path ExpandShortRoot(const std::filesystem::path& path, const RootForms& root)
{
    if (root.Parts.empty()) return path;
    std::wstring p = path.wstring();
    // keep a \\?\ prefix out of the comparison
    std::wstring lead;
    if (p.rfind(L"\\\\?\\", 0) == 0) { lead = L"\\\\?\\"; p = p.substr(4); }
    auto parts = SplitPathParts(p);
    if (parts.size() < root.Parts.size()) return path;
    bool anyShort = false;
    for (size_t i = 0; i < root.Parts.size(); i++)
    {
        if (_wcsicmp(parts[i].c_str(), root.Parts[i].first.c_str()) == 0) continue;
        if (_wcsicmp(parts[i].c_str(), root.Parts[i].second.c_str()) == 0) { anyShort = true; continue; }
        return path;
    }
    if (!anyShort) return path;   // already the long form
    std::wstring result = lead + root.LongRoot;
    for (size_t i = root.Parts.size(); i < parts.size(); i++) result += L"\\" + parts[i];
    if (p.size() > 0 && (p.back() == L'\\' || p.back() == L'/') && parts.size() > root.Parts.size()) result += L"\\";
    return std::filesystem::path(result);
}
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
void DetermineCohorts(Json_Debug_Levels debugRequestLevel, std::wstring requestedPath, Cohorts *cohorts, DWORD dllInstance, const wchar_t * FixupName)
{

    cohorts->file_mfr = mfr::create_mfr_path(requestedPath);
    static const RootForms packageRootForms = MakeRootForms(g_packageRootPath);
    static const RootForms writableRootForms = MakeRootForms(g_writablePackageRootPath);
    {
        // the path type was determined from the original form: determine it again for the long form
        auto expandedPath = ExpandShortRoot(ExpandShortRoot(cohorts->file_mfr.Request_NormalizedPath, packageRootForms), writableRootForms);
        if (expandedPath != cohorts->file_mfr.Request_NormalizedPath)
        {
            Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: 8.3 root expanded to %s", g_MfrModuleName, dllInstance, FixupName, expandedPath.c_str());
            cohorts->file_mfr = mfr::create_mfr_path(expandedPath);
        }
    }
    cohorts->WsRequested = cohorts->file_mfr.Request_NormalizedPath.c_str();
    cohorts->NativeIsValidOptionInScenario = true;

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
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Request is in_native_area.", g_MfrModuleName, dllInstance, FixupName);

        cohorts->map = mfr::Find_RedirMapping_FromNativePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against redir %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping match against redir %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping is for redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                
                //cohorts->WsRedirected = cohorts->WsRequested;
                //cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.PackagePathBase);
                ////cohorts->WsNative = cohorts->WsRequested;
                //cohorts->NativeIsValidOptionInScenario = false;
                //cohorts->WsRedirected = cohorts->WsRequested;
                //cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.PackagePathBase);
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.RedirectedPathBase);
                cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.NativePathBase, cohorts->map.PackagePathBase);
                cohorts->WsNative = cohorts->WsRequested;
                break;
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping is for EXCLUSION redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                
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
            Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: No redirect to local mapping matched.", g_MfrModuleName, dllInstance, FixupName);
        }

        // If the path entered is mappable from a native path to a package path, or redirection area to a package path, we catch these here
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping is for redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                
                // Exception processing
                // We shouln't redirect to traditional area if the call was only to the WindowsApps folder.
                // This can cause an issue in an app like R (language) that deals with Short Names and we can't force shortnames to be the same
                // in the redirection area that they are natively. (Well, we could if MFR did everything but not with ILV in use as it creates these things behind our back.
                if (comparei(cohorts->WsRequested, L"C:\\Program Files\\WindowsApps"))
                {
                    Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Windows Apps special exclusion.", g_MfrModuleName, dllInstance, cohorts->WsRequested.c_str());
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
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping is for EXCLUSION redirection type %s.", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Request is in package_pvad_area.", g_MfrModuleName, dllInstance, FixupName);
        
        cohorts->WsPackage = cohorts->WsRequested;
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps with known traditional redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.RedirectedPathBase);
                cohorts->NativeIsValidOptionInScenario = false;
            }
            else
            {
                // We don't actually have any of these as it makes no sense.
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps to a known traditional redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
                
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
            
        }
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Request is in_package_vfs_area.", g_MfrModuleName, dllInstance, FixupName);

        // A request for a package VFS path without a mapping (e.g. <PackageRoot>\VFS itself) must still find the
        // package location, as in the in_package_pvad_area case; otherwise all cohorts stay empty and the request
        // fails with ERROR_PATH_NOT_FOUND although the folder exists.
        cohorts->WsPackage = cohorts->WsRequested;
        cohorts->map = mfr::Find_RedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against local  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping match against local  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                if (cohorts->map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                {
                    Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps with known local redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                    
                    cohorts->WsPackage = cohorts->WsRequested;
                    cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                    cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                    //cohorts->NativeIsValidOptionInScenario = false;
                    break;
                }
                else if (cohorts->map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs ||
                         cohorts->map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized)
                {
                    Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Ignore because it maps with known traditional redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
                else
                {
                    Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: ignore because it maps with unknown local redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                }
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps to a known local redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
                
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
                break;
            }
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] %s DetermineCohorts: No local mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
        }

        cohorts->map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(), dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps with known traditional redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.RedirectedPathBase);
                cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
            }
            else
            {
                // We have a tratidional redirection mapping marked for exclusion. This means that we don't want to look in the package
                // or redirection area but look at the native path only
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps to a known traditional redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
                
                cohorts->WsRedirected = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
                cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.PackagePathBase, cohorts->map.NativePathBase);
            }
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Request is in_redirection_area_writablepackageroot.", g_MfrModuleName, dllInstance, FixupName);
        
        cohorts->map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(cohorts->file_mfr.Request_NormalizedPath.c_str(),dllInstance);
        if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
        {
            if (cohorts->map.IsExactMatchOnly == mfr::mfr_exactmatchonly_types::exactmatchonly)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: ExactOnly Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Mapping match against traditional  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->map.VFSFolderName.c_str());
            }
            
            if (cohorts->map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps with known traditional redirection type %s", g_MfrModuleName, dllInstance, FixupName, RedirectFlagsName(cohorts->map.RedirectionFlags));
                
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.PackagePathBase);
                if (cohorts->WsPackage.find(L"\\VFS\\") != std::wstring::npos)
                {
                    cohorts->WsNative = ReplacePathPart(cohorts->WsRequested.c_str(), cohorts->map.RedirectedPathBase, cohorts->map.NativePathBase);
                }
                else
                {
                    cohorts->NativeIsValidOptionInScenario = false;  //request was redirected area for a PVAD path in package.  No native possible.
                }
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Maps to a known traditional redirection exclusion path.", g_MfrModuleName, dllInstance, FixupName);
                
                cohorts->WsRedirected = cohorts->WsRequested;
                cohorts->WsPackage = cohorts->WsRequested;
                cohorts->WsNative = cohorts->WsRequested;
            }
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] %s DetermineCohorts: No traditional mapping is valid.", g_MfrModuleName, dllInstance, FixupName);
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Request is in_redirection_area_other.", g_MfrModuleName, dllInstance, FixupName);
        cohorts->NativeIsValidOptionInScenario = false;
        break;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts: Request is in_non_redirectable_areas.", g_MfrModuleName, dllInstance, FixupName);
        cohorts->NativeIsValidOptionInScenario = false;
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
    
    if (MFRConfiguration.Ilv_Aware)
    {
        if (cohorts->WsRedirected.find(L"WritablePackageRoot\\VFS\\ProgramFilesX") != std::wstring::npos ||
            cohorts->WsRedirected.find(L"WritablePackageRoot\\VFS\\Windows") != std::wstring::npos ||
            cohorts->WsRedirected.find(L"WritablePackageRoot\\VFS\\CommonFiles") != std::wstring::npos)
        {
            cohorts->IsPathIlvEligible = true;
        }
    }

    Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts:   Cohort->WsRequested  %s", g_MfrModuleName, dllInstance, FixupName, cohorts->WsRequested.c_str());
    if (cohorts->map.Valid_mapping == mfr::mfr_enabled_types::enabled)
    {
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts:   Cohort->WsRedirected %s", g_MfrModuleName, dllInstance, FixupName, cohorts->WsRedirected.c_str());
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts:   Cohort->WsPackage    %s", g_MfrModuleName, dllInstance, FixupName, cohorts->WsPackage.c_str());
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts:   Cohort->WsNative     %s", g_MfrModuleName, dllInstance, FixupName, cohorts->WsNative.c_str());
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts:   MayUseNative=%d   IsIlvEligable=%d", g_MfrModuleName, dllInstance, FixupName, cohorts->NativeIsValidOptionInScenario, cohorts->IsPathIlvEligible);
    }
    else
    {
        Log(debugRequestLevel, L"[%s%d] %s: DetermineCohorts:   Cohort->map.Valid_Mapping %d", g_MfrModuleName, dllInstance, FixupName, cohorts->map.Valid_mapping);
    }

    
}




