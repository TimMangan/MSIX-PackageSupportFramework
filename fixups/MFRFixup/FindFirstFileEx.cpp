//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this api: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfileexw

// This implementation of FindFileFirst/Next supports finding files under three different possible locations, all layered together.
// 
// The general design is roughly as follows. 
// When a find is requested, it is possible that in addition to the location requested, there may be files that we should find in up to three other locations:
//   * The (equivalent) native location
//   * The (equivalent) package location
//   * The (equivalent) redirected location.
//
// First, we develope a "clean" version of the requested path that looks like C:\whatever to aid in creating the alternate paths:
// A) An area where package file changes may have been previously redirected to.
// B) A VFS path in the package (when the location requested is outside of the package boundary).
// C) The normalized path as requested.
// D) A devirtualized native path (when the location requested is inside a package VFS folder, this is the native location).
// E) A deredirected package path (when the requested location is in the writablepackageroot area).
// 
// So we calculate all five possible paths (or deterine the path is not possible/applicable),
// Then we build up results in a logical order, presenting in order of: Redirection, VFS, requested, and devirtualized native, deredirected package.
//
// NOTE: If we ever address the "delete package file" problem, we'll need to address that here, too.
// NOTE2: If successful, this replacement form might be a model needed in other file operations.
// NOTE2: Unlike many MFR intercepts, this function returns strings to the caller.  Most MFR intercepts just convert caller path strings into wide strings and just run with those.
//        There are two possible approaches for this call, to either implement keeping in narrow strings when called that way, or the approach chosen, which is to convert to wide string and 
//        then convert back the return data back to narrow when called that way.

// =======================================================================================================  
// NOTE3: ****VERY IMPORTANT*** Changes to this file MUST be made in parallel with FindFirstFile.cpp
// =======================================================================================================



#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include <memory>
#include "FindData3.h"
#include "FindFirstHelpers.h"
#include "DetermineCohorts.h"
#include "FID.h"

//#ifdef _M_IX86
//#pragma comment(linker, "/EXPORT:FindFirstFileEx_Ansi_Fixup=impl::FindFirstFileExW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#pragma comment(linker, "/EXPORT:FindFirstFileEx_Wide_Fixup=impl::FindFirstFileExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#else
//#pragma comment(linker, "/EXPORT:FindFirstFileExFixupAnsi_Fixup=impl::FindFirstFileExW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#pragma comment(linker, "/EXPORT:FindFirstFileExFixupWide_Fixup=impl::FindFirstFileExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#endif
#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:FindFirstFileExA_FixupHelper=_FindFirstFileExAFixupHelper@24")
#pragma comment(linker, "/EXPORT:FindFirstFileExW_FixupHelper=_FindFirstFileExWFixupHelper@24")
#pragma comment(linker, "/EXPORT:FindFirstFileEx_Ansi_Fixup=impl::_FindFirstFileExW.ansi@24")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:FindFirstFileEx_Wide_Fixup=impl::_FindFirstFileExW.wide@24")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:FindFirstFileExA_FixupHelper=FindFirstFileExAFixupHelper")
#pragma comment(linker, "/EXPORT:FindFirstFileExW_FixupHelper=FindFirstFileExWFixupHelper")
#pragma comment(linker, "/EXPORT:FindFirstFileExFixupAnsi_Fixup=impl::FindFirstFileExW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:FindFirstFileExFixupWide_Fixup=impl::FindFirstFileExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

#if BRINGBACK
bool FindFirstExHasSpecialCharacters(std::wstring wFileName)
{
    std::wstring wilds = L"*?";
    size_t wildOf = wFileName.find_first_of(wilds);
    if (wildOf == std::wstring::npos)
    {
        return false;
    }
    return true;
}
#endif


extern "C" HANDLE __stdcall FindFirstFileExAFixupHelper(_In_ const char* fileName,
    _In_ FINDEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(win32_find_data_t<char>)) LPVOID findFileData,
    _In_ FINDEX_SEARCH_OPS searchOp,
    _Reserved_ LPVOID searchFilter,
    _In_ DWORD additionalFlags)
{
    [[maybe_unused]] DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moreDebug = false;
#if _DEBUG
    debug = true;
#endif
#if MOREDEBUG
    moreDebug = true;
#endif

    std::string afileName = fileName;
    afileName = AdjustSlashes(afileName, dllInstance);

    auto result = std::make_unique<FindData3A>();
    result->RememberedInstance = dllInstance;
    result->requested_path = afileName;

    LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, "\tFindFirstFileExAFixup: for fileName", fileName);


    switch (infoLevelId)
    {
    case FindExInfoStandard:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoStandard", g_MfrModuleName, dllInstance);
        break;
    case FindExInfoBasic:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoBasic", g_MfrModuleName, dllInstance);
        break;
    case FindExInfoMaxInfoLevel:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoMaxInfoLevel", g_MfrModuleName, dllInstance);
        break;
    default:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel unknown", g_MfrModuleName, dllInstance);
        break;
    }
    switch (searchOp)
    {
    case FindExSearchNameMatch:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchNameMatch", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchLimitToDirectories:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchLimitToDirectories", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchLimitToDevices:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchLimitToDevices", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchMaxSearchOp:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchMaxSearchOp", g_MfrModuleName, dllInstance);
        break;
    default:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp Unknown=0x%x", g_MfrModuleName, dllInstance, searchOp);
        break;
    }

    afileName = AdjustBadUNC(afileName, dllInstance, "FindFirstFileExAFixup");

    // Adjust for the bad Winzip issue of asking for C:\ProgramFilesX64\WindowsApps...
    afileName = AdjustPFx64Path(afileName, dllInstance, L"FindFirstFileExAFixup");

    // Determine possible paths involved
    Cohorts cohorts;
    DetermineCohorts(LogLevel_DebugIntermediate, widen(afileName), &cohorts, dllInstance, L"FindFirstFileExAFixup");

    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup:      RedirPath=%s", g_MfrModuleName, dllInstance, cohorts.WsRedirected.c_str());
    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup:    PackagePath=%s", g_MfrModuleName, dllInstance, cohorts.WsPackage.c_str());
    if (cohorts.NativeIsValidOptionInScenario)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup:     NativePath=%s", g_MfrModuleName, dllInstance, cohorts.WsNative.c_str());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup:  NO NativePath", g_MfrModuleName, dllInstance);
    }

    //

    DWORD initialFindError = ERROR_PATH_NOT_FOUND;

    // First find the redirected area results
    std::string rldUseFile = narrow(MakeLongPath(cohorts.WsRedirected));
    result->find_handles[Result_Redirected].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, &result->cached_data[Result_Redirected], searchOp, searchFilter, additionalFlags));
    // Some applications really care about the failure reason. Try and make this the best that we can, preferring
    // something like "file not found" over "path does not exist"
    initialFindError = ::GetLastError();


    if (result->find_handles[Result_Redirected])
    {

        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from redirected): had results=%ls", g_MfrModuleName, dllInstance, Result_Redirected, result->cached_data[Result_Redirected].cFileName);
        //AnyValidPath = true;
        //AnyValidResult = true;
    }
    else
    {
        //if (initialFindError == ERROR_FILE_NOT_FOUND)
        //    AnyValidPath = true;

        // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
        //result->redirect_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from redirected): no results.", g_MfrModuleName, dllInstance, Result_Redirected);

    }

    rldUseFile = narrow(MakeLongPath(cohorts.WsPackage));
    result->find_handles[Result_Package].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, &result->cached_data[Result_Package], searchOp, searchFilter, additionalFlags));
    if (result->find_handles[Result_Package])
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from package):   had results=%ls", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName);

        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            initialFindError = ERROR_FILE_NOT_FOUND;
        ///result->package_vfs_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from package):   no results.", g_MfrModuleName, dllInstance, Result_Package);
    }

    if (!result->find_handles[Result_Redirected])
    {
        if (result->cached_data[Result_Package].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from package):   had results=%ls %ls", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName, result->cached_data[Result_Package].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from package):   had results=%ls", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName);
        }
        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
            initialFindError = ERROR_FILE_NOT_FOUND;
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from package):   no results.", g_MfrModuleName, dllInstance, Result_Package);

    }


    if (cohorts.NativeIsValidOptionInScenario)
    {
        rldUseFile = narrow(MakeLongPath(cohorts.WsNative));
        result->find_handles[Result_Native].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, &result->cached_data[Result_Native], searchOp, searchFilter, additionalFlags));
        if (result->find_handles[Result_Native])
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from native)    had results=%ls", g_MfrModuleName, dllInstance, Result_Native, result->cached_data[Result_Native].cFileName);
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                initialFindError = ERROR_FILE_NOT_FOUND;
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from native):   no results.", g_MfrModuleName, dllInstance, Result_Native);
        }
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup[%d] (from native):    no results possible.", g_MfrModuleName, dllInstance, Result_Native);
    }

    if (result->find_handles[Result_Redirected] ||
        result->find_handles[Result_Package] ||
        result->find_handles[Result_Native])
    {
        int UseIndex = ChooseIndexForFindResult(cohorts, result.get());

        // return first result found
        copy_find_data(result->cached_data[UseIndex], *reinterpret_cast<WIN32_FIND_DATAA*>(findFileData));


        // Consider the case where the name is a variablized MSIX name, such as ProgramFilesX64 and the app asked for "C:\Program Files".
        // In that case, we really want to return what they asked for, because although they already knew the name, they might use the value
        // we return instead, and bad things happen.
        if (UseIndex == Result_Redirected || UseIndex == Result_Package)
        {
            if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_native_area)
            {
                if (afileName.find_first_of("*?") == std::string::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::string origNameOnly = afileName;
                    if (origNameOnly.length() > 0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == "\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of("\\") != std::string::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of("\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup SWAP %s for %s", g_MfrModuleName, dllInstance, widen(origNameOnly).c_str(), widen(result->cached_data[UseIndex].cFileName).c_str());
                        strcpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        strcpy_s(reinterpret_cast<WIN32_FIND_DATAA*>(findFileData)->cFileName, MAX_PATH, origNameOnly.c_str());

                    }
                }
            }
            else if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_package_pvad_area)
            {
                // This means that the requested path is in a package area, but not in the VFS area.
                // Most likely this is the root path of the package, but possibly not.
                if (afileName.find_first_of("*?") == std::string::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::string origNameOnly = afileName;
                    if (origNameOnly.length() > 0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == "\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of("\\") != std::string::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of("\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup SWAP %s for %s", g_MfrModuleName, dllInstance, widen(origNameOnly).c_str(), widen(result->cached_data[UseIndex].cFileName).c_str());
                        strcpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        strcpy_s(reinterpret_cast<WIN32_FIND_DATAA*>(findFileData)->cFileName, MAX_PATH, origNameOnly.c_str());

                    }
                }
            }
        }

        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup returns using index=%d %ls", g_MfrModuleName, dllInstance, UseIndex, result->cached_data[UseIndex].cFileName);
        result->sAlready_returned_list.push_back(result->cached_data[UseIndex].cFileName);
        ::SetLastError(ERROR_SUCCESS);
        return reinterpret_cast<HANDLE>(result.release());

    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExAFixup returns 0x%x", g_MfrModuleName, dllInstance, initialFindError);

        ::SetLastError(initialFindError);
        return INVALID_HANDLE_VALUE;
    }
}

extern "C" HANDLE __stdcall FindFirstFileExWFixupHelper(_In_ const wchar_t* fileName,
    _In_ FINDEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(win32_find_data_t<wchar_t>)) LPVOID findFileData,
    _In_ FINDEX_SEARCH_OPS searchOp,
    _Reserved_ LPVOID searchFilter,
    _In_ DWORD additionalFlags)
{
    [[maybe_unused]] DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moreDebug = false;
#if _DEBUG
    debug = true;
#endif
#if MOREDEBUG
    moreDebug = true;
#endif

    LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"\tFindFirstFileExWFixup: for fileName", fileName);


    std::wstring wfileName = fileName;
    wfileName = AdjustSlashes(wfileName, dllInstance);

    auto result = std::make_unique<FindData3W>();
    result->RememberedInstance = dllInstance;
    result->requested_path = wfileName;


    switch (infoLevelId)
    {
    case FindExInfoStandard:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoStandard", g_MfrModuleName, dllInstance);
        break;
    case FindExInfoBasic:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoBasic", g_MfrModuleName, dllInstance);
        break;
    case FindExInfoMaxInfoLevel:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoMaxInfoLevel", g_MfrModuleName, dllInstance);
        break;
    default:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel unknown", g_MfrModuleName, dllInstance);
        break;
    }
    switch (searchOp)
    {
    case FindExSearchNameMatch:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchNameMatch", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchLimitToDirectories:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchLimitToDirectories", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchLimitToDevices:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchLimitToDevices", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchMaxSearchOp:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchMaxSearchOp", g_MfrModuleName, dllInstance);
        break;
    default:
        Log(LogLevel_DebugBasic, "[%s%d]\t\tSearchOp Unknown=0x%x", g_MfrModuleName, dllInstance, searchOp);
        break;
    }

    wfileName = AdjustBadUNC(wfileName, dllInstance, L"FindFirstFileExWFixup");

    // Adjust for the bad Winzip issue of asking for C:\ProgramFilesX64\WindowsApps...
    wfileName = AdjustPFx64Path(wfileName, dllInstance, L"FindFirstFileExWFixup");

    // Determine possible paths involved
    Cohorts cohorts;
    DetermineCohorts(LogLevel_DebugIntermediate, wfileName, &cohorts, dllInstance, L"FindFirstFileExWFixup");

    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup:      RedirPath=%s", g_MfrModuleName, dllInstance, cohorts.WsRedirected.c_str());
    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup:    PackagePath=%s", g_MfrModuleName, dllInstance, cohorts.WsPackage.c_str());
    if (cohorts.NativeIsValidOptionInScenario)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup:     NativePath=%s", g_MfrModuleName, dllInstance, cohorts.WsNative.c_str());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup:  NO NativePath", g_MfrModuleName, dllInstance);
    }


    //

    DWORD initialFindError = ERROR_PATH_NOT_FOUND;

    // First find the redirected area results
    std::wstring rldUseFile = MakeLongPath(cohorts.WsRedirected);
    result->find_handles[Result_Redirected].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, &result->cached_data[Result_Redirected], searchOp, searchFilter, additionalFlags));
    // Some applications really care about the failure reason. Try and make this the best that we can, preferring
    // something like "file not found" over "path does not exist"
    initialFindError = ::GetLastError();


    if (result->find_handles[Result_Redirected])
    {

        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from redirected): had results=%s", g_MfrModuleName, dllInstance, Result_Redirected, result->cached_data[Result_Redirected].cFileName);

        //AnyValidPath = true;
        //AnyValidResult = true;
    }
    else
    {
        //if (initialFindError == ERROR_FILE_NOT_FOUND)
        //    AnyValidPath = true;

        // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
        //result->redirect_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from redirected): no results.", g_MfrModuleName, dllInstance, Result_Redirected);
    }

    rldUseFile = MakeLongPath(cohorts.WsPackage);
    result->find_handles[Result_Package].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, &result->cached_data[Result_Package], searchOp, searchFilter, additionalFlags));
    if (result->find_handles[Result_Package])
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from package):   had results=%s", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName);
        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            initialFindError = ERROR_FILE_NOT_FOUND;
        ///result->package_vfs_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from package):   no results.", g_MfrModuleName, dllInstance, Result_Package);
    }

    if (!result->find_handles[Result_Redirected])
    {
        if (result->cached_data[Result_Package].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from package):   had results=%s %s", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName, result->cached_data[Result_Package].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from package):   had results=%s", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName);
        }
        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
            initialFindError = ERROR_FILE_NOT_FOUND;
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from package):   no results.", g_MfrModuleName, dllInstance, Result_Package);

    }


    if (cohorts.NativeIsValidOptionInScenario)
    {
        rldUseFile = MakeLongPath(cohorts.WsNative);
        result->find_handles[Result_Native].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, &result->cached_data[Result_Native], searchOp, searchFilter, additionalFlags));
        if (result->find_handles[Result_Native])
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from native)    had results=%s", g_MfrModuleName, dllInstance, Result_Native, result->cached_data[Result_Native].cFileName);
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                initialFindError = ERROR_FILE_NOT_FOUND;
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from native):   no results.", g_MfrModuleName, dllInstance, Result_Native);
        }
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup[%d] (from native):    no results possible.", g_MfrModuleName, dllInstance, Result_Native);
    }

    if (result->find_handles[Result_Redirected] ||
        result->find_handles[Result_Package] ||
        result->find_handles[Result_Native])
    {
        int UseIndex = ChooseIndexForFindResult(cohorts, result.get());

        // return first result found
        copy_find_data(result->cached_data[UseIndex], * reinterpret_cast<WIN32_FIND_DATAW *>(findFileData));


        // Consider the case where the name is a variablized MSIX name, such as ProgramFilesX64 and the app asked for "C:\Program Files".
        // In that case, we really want to return what they asked for, because although they already knew the name, they might use the value
        // we return instead, and bad things happen.
        if (UseIndex == Result_Redirected || UseIndex == Result_Package)
        {
            if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_native_area)
            {
                if (wfileName.find_first_of(L"*?") == std::wstring::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::wstring origNameOnly = wfileName;
                    if (origNameOnly.length() > 0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == L"\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of(L"\\") != std::wstring::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of(L"\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup SWAP %s for %s", g_MfrModuleName, dllInstance, origNameOnly.c_str(), result->cached_data[UseIndex].cFileName);
                        wcscpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        wcscpy_s(reinterpret_cast<WIN32_FIND_DATAW*>(findFileData)->cFileName, MAX_PATH, origNameOnly.c_str());

                    }
                }
            }
            else if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_package_pvad_area)
            {
                // This means that the requested path is in a package area, but not in the VFS area.
                // Most likely this is the root path of the package, but possibly not.
                if (wfileName.find_first_of(L"*?") == std::wstring::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::wstring origNameOnly = wfileName;
                    if (origNameOnly.length() > 0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == L"\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of(L"\\") != std::wstring::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of(L"\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup SWAP %s for %s", g_MfrModuleName, dllInstance, origNameOnly.c_str(), result->cached_data[UseIndex].cFileName);
                        wcscpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        wcscpy_s(reinterpret_cast<WIN32_FIND_DATAW*>(findFileData)->cFileName, MAX_PATH, origNameOnly.c_str());

                    }
                }
            }
        }

        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup returns using index=%d %ls", g_MfrModuleName, dllInstance, UseIndex, result->cached_data[UseIndex].cFileName);
        result->wsAlready_returned_list.push_back(result->cached_data[UseIndex].cFileName);
        ::SetLastError(ERROR_SUCCESS);
        return reinterpret_cast<HANDLE>(result.release());

    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExWFixup returns 0x%x", g_MfrModuleName, dllInstance, initialFindError);
        ::SetLastError(initialFindError);
        return INVALID_HANDLE_VALUE;
    }
}



template <typename CharT>
HANDLE __stdcall FindFirstFileExFixup(_In_ const CharT* fileName,
    _In_ FINDEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(win32_find_data_t<CharT>)) LPVOID findFileData,
    _In_ FINDEX_SEARCH_OPS searchOp,
    _Reserved_ LPVOID searchFilter,
    _In_ DWORD additionalFlags) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    [[maybe_unused]] DWORD dllInstance = ++g_InterceptInstance;
    HANDLE retfinal;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moreDebug = false;
#if _DEBUG
    debug = true;
#endif
#if MOREDEBUG
    moreDebug = true;
#endif

    if (guard)
    {
        if constexpr (psf::is_ansi<CharT>)
        {
            return FindFirstFileExAFixupHelper(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
        }
        else
        {
            return FindFirstFileExWFixupHelper(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
        }
    }

#if BINGBACK
    std::wstring wfileName = widen(fileName);
    wfileName = AdjustSlashes(wfileName, dllInstance);

    auto result = std::make_unique<FindData3>();
    result->RememberedInstance = dllInstance;
    result->requested_path = wfileName;

    if (psf::is_ansi<CharT>)
    {
        LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"\tFindFirstFileExAFixup: for fileName", fileName);
    }
    else
    {
        LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"\tFindFirstFileExWFixup: for fileName", fileName);
    }

    switch (infoLevelId)
    {
    case FindExInfoStandard:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoStandard", g_MfrModuleName, dllInstance);
        break;
    case FindExInfoBasic:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoBasic", g_MfrModuleName, dllInstance);
        break;
    case FindExInfoMaxInfoLevel:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel FindExInfoMaxInfoLevel", g_MfrModuleName, dllInstance);
        break;
    default:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tLevel unknown", g_MfrModuleName, dllInstance);
        break;
    }
    switch (searchOp)
    {
    case FindExSearchNameMatch:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchNameMatch", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchLimitToDirectories:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchLimitToDirectories", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchLimitToDevices:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchLimitToDevices", g_MfrModuleName, dllInstance);
        break;
    case FindExSearchMaxSearchOp:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp FindExSearchMaxSearchOp", g_MfrModuleName, dllInstance);
        break;
    default:
        Log(LogLevel_DebugBasic, L"[%s%d]\t\tSearchOp Unknown=0x%x", g_MfrModuleName, dllInstance, searchOp);
        break;
    }

    wfileName = AdjustBadUNC(wfileName, dllInstance, L"FindFirstFileExFixup");

    // Determine possible paths involved
    Cohorts cohorts;
    DetermineCohorts(LogLevel_DebugMaximum, wfileName, &cohorts, dllInstance, L"FindFirstFileExFixup");

    Log(LogLevel_DebugIntermediate, L"[%s%d] FindFirstFileExFixup:      RedirPath=%s", g_MfrModuleName, dllInstance, cohorts.WsRedirected.c_str());
    Log(LogLevel_DebugIntermediate, L"[%s%d] FindFirstFileExFixup:    PackagePath=%s", g_MfrModuleName, dllInstance, cohorts.WsPackage.c_str());
    if (cohorts.NativeIsValidOptionInScenario)
    {
        Log(LogLevel_DebugIntermediate, L"[%s%d] FindFirstFileExFixup:     NativePath=%s", g_MfrModuleName, dllInstance, cohorts.WsNative.c_str());
    }
    else
    {
        Log(LogLevel_DebugIntermediate, L"[%s%d] FindFirstFileExFixup:  NO NativePath", g_MfrModuleName, dllInstance);
    }

    //

    [[maybe_unused]] auto ansiData = reinterpret_cast<WIN32_FIND_DATAA*>(findFileData);
    [[maybe_unused]] auto wideData = reinterpret_cast<WIN32_FIND_DATAW*>(findFileData);
    WIN32_FIND_DATAW* findData = psf::is_ansi<CharT> ? &result->cached_data[Result_Redirected] : wideData;
    DWORD initialFindError = ERROR_PATH_NOT_FOUND;

    // First find the redirected area results
    std::wstring rldUseFile = MakeLongPath(cohorts.WsRedirected);
    result->find_handles[Result_Redirected].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
    // Some applications really care about the failure reason. Try and make this the best that we can, preferring
    // something like "file not found" over "path does not exist"
    initialFindError = ::GetLastError();


    if (result->find_handles[Result_Redirected])
    {
        if constexpr (psf::is_ansi<CharT>)
        {
            if (copy_find_data(*findData, *ansiData))
            {
                // NOTE: Last error set by caller
                return INVALID_HANDLE_VALUE;
            }
        }
        else
        {
            // No need to copy since we wrote directly into the output buffer
            assert(findData == wideData);
            copy_find_data(*wideData, result->cached_data[Result_Redirected]);
        }
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup[%d] (from redirected): had results %ls", g_MfrModuleName, dllInstance, Result_Redirected, findData->cFileName);
        //AnyValidPath = true;
        //AnyValidResult = true;
    }
    else
    {
        //if (initialFindError == ERROR_FILE_NOT_FOUND)
        //    AnyValidPath = true;

        // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
        //result->redirect_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup[%d] (from redirected): no results.", g_MfrModuleName, dllInstance, Result_Redirected);
    }
    // save for next level
    findData = (result->find_handles[Result_Redirected] || psf::is_ansi<CharT>) ? &result->cached_data[Result_Package] : wideData;

    rldUseFile = MakeLongPath(cohorts.WsPackage);
    result->find_handles[Result_Package].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
    ///result->package_vfs_path.resize(vfspathSize);
    if (result->find_handles[Result_Package])
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup[%d] (from package):   had results %ls", g_MfrModuleName, dllInstance, Result_Package, findData->cFileName);
        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            initialFindError = ERROR_FILE_NOT_FOUND;
        ///result->package_vfs_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup[%d] (from package):   no results.", g_MfrModuleName, dllInstance, Result_Package);
    }

    // Consolodate current results
    if (!result->find_handles[Result_Redirected])
    {
        if (result->find_handles[Result_Package])
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                if (copy_find_data(*findData, *ansiData))
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileEx error set by caller", g_MfrModuleName, dllInstance);
                    // NOTE: Last error set by caller
                    return INVALID_HANDLE_VALUE;
                }
            }
            else
            {
                // No need to copy since we wrote directly into the output buffer
                assert(findData == wideData);
                copy_find_data(*wideData, result->cached_data[Result_Package]);
            }
        }
    }

    // save for next level
    findData = (result->find_handles[Result_Redirected] || result->find_handles[Result_Package] || psf::is_ansi<CharT>) ? &result->cached_data[Result_Native] : wideData;

    if (cohorts.NativeIsValidOptionInScenario)
    {
        rldUseFile = MakeLongPath(cohorts.WsNative);
        result->find_handles[Result_Native].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
        if (result->find_handles[Result_Native])
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup[%d] (from native)    had results=%ls", g_MfrModuleName, dllInstance, Result_Native, findData->cFileName);
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                initialFindError = ERROR_FILE_NOT_FOUND;
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixupV2[%d] (from native):   no results.", g_MfrModuleName, dllInstance, Result_Native);
        }
        if (!result->find_handles[Result_Redirected] &&
            !result->find_handles[Result_Package])
        {
            if (result->find_handles[Result_Native])
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (copy_find_data(*findData, *ansiData))
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup error set by caller", g_MfrModuleName, dllInstance);
                        // NOTE: Last error set by caller
                        return INVALID_HANDLE_VALUE;
                    }
                }
                else
                {
                    // No need to copy since we wrote directly into the output buffer
                    assert(findData == wideData);
                    copy_find_data(*wideData, result->cached_data[Result_Native]);
                }
            }
        }
        else  if (!FindFirstExHasSpecialCharacters(wfileName))
        {
            // Feel like we need to do something in this case, but can't figure out what.
            // Draw.IO calls this with C:\Users\xxx\AppData\Roaming.  We are returning "Roaming", but see the app getting confused later on, as if it got the package/redirect "AppData" instead and
            // starts trying to work with ...\AppData\AppData
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup[%d] Mixed Results without Special Characters.", g_MfrModuleName, dllInstance, Result_Native);
        }
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup[%d] (from native):    no results possible.", g_MfrModuleName, dllInstance, Result_Native);
    }

    if (result->find_handles[Result_Redirected] ||
        result->find_handles[Result_Package] ||
        result->find_handles[Result_Native])
    {
        int UseIndex;
        if (result->find_handles[Result_Redirected])
        {
            UseIndex = Result_Redirected;
        }
        else if (result->find_handles[Result_Package])
        {
            UseIndex = Result_Package;
        }
        else
        {
            UseIndex = Result_Native;
        }

        if constexpr (psf::is_ansi<CharT>)
        {
            // return first result found
            copy_find_data(*ansiData, result->cached_data[UseIndex]);
            //result->cached_data[UseIndex] = ansiData;  
        }
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup returns %ls", g_MfrModuleName, dllInstance, result->cached_data[UseIndex].cFileName);
        result->wsAlready_returned_list.push_back(result->cached_data[UseIndex].cFileName);
        ::SetLastError(ERROR_SUCCESS);

        // Fill in the part of the structure that is like WIN32_FIND_DATA, in case the caller is using that structure.
        result->dwFileAttributes = result->cached_data[UseIndex].dwFileAttributes;
        result->ftCreationTime = result->cached_data[UseIndex].ftCreationTime;
        result->ftLastAccessTime = result->cached_data[UseIndex].ftLastAccessTime;
        result->ftLastWriteTime = result->cached_data[UseIndex].ftLastWriteTime;
        result->nFileSizeHigh = result->cached_data[UseIndex].nFileSizeHigh;
        result->nFileSizeLow = result->cached_data[UseIndex].nFileSizeLow;
        result->dwReserved0 = result->cached_data[UseIndex].dwReserved0;
        result->dwReserved1 = result->cached_data[UseIndex].dwReserved1;
        wcscpy_s(result->cFileName, MAX_PATH, result->cached_data[UseIndex].cFileName);
        wcscpy_s(result->cAlternateFileName, 14, result->cached_data[UseIndex].cAlternateFileName);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileExFixup returns 0x%x", g_MfrModuleName, dllInstance, initialFindError);
        ::SetLastError(initialFindError);
        return INVALID_HANDLE_VALUE;
    }
    return reinterpret_cast<HANDLE>(result.release());
#endif

    else
    {

        switch (infoLevelId)
        {
        case FindExInfoStandard:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) Level FindExInfoStandard", g_MfrModuleName, dllInstance);
            break;
        case FindExInfoBasic:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) Level FindExInfoBasic", g_MfrModuleName, dllInstance);
            break;
        case FindExInfoMaxInfoLevel:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) Level FindExInfoMaxInfoLevel", g_MfrModuleName, dllInstance);
            break;
        default:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) Level unknown", g_MfrModuleName, dllInstance);
            break;
        }
        switch (searchOp)
        {
        case FindExSearchNameMatch:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchNameMatch", g_MfrModuleName, dllInstance);
            break;
        case FindExSearchLimitToDirectories:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchLimitToDirectories", g_MfrModuleName, dllInstance);
            break;
        case FindExSearchLimitToDevices:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchLimitToDevices", g_MfrModuleName, dllInstance);
            break;
        case FindExSearchMaxSearchOp:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchMaxSearchOp", g_MfrModuleName, dllInstance);
            break;
        default:
            Log(LogLevel_DebugBasic, L"[%s%d]\tFindFirstFileExFixup: (unguarded) SearchOp Unknown=0x%x", g_MfrModuleName, dllInstance, searchOp);
            break;
        }

    }

    // If still here, call original.
    LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"\tFindFirstFileExFixup: (unguarded) for fileName", fileName);
    retfinal = impl::FindFirstFileEx(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileFixup returns 0x%x", g_MfrModuleName, dllInstance, retfinal);
    return retfinal;
}
catch (...)
{
    // NOTE: Since we allocate our own "find handle" memory, we can't just forward on to the implementation
    ::SetLastError(win32_from_caught_exception());
    Log(LogLevel_Exception, L"***FindFirstFileExFixup Exception***");
    return INVALID_HANDLE_VALUE;
}
DECLARE_STRING_FIXUP(impl::FindFirstFileEx, FindFirstFileExFixup);

