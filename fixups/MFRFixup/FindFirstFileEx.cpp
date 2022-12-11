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
#include "FunctionImplementations.h"
#include <memory>
#include "FindData3.h"
#include "FindFirstHelpers.h"
#include "DetermineCohorts.h"




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
    bool moreDebug = false;
#if _DEBUG
    debug = true;
#endif
#if MOREDEBUG
    moreDebug = true;
#endif

    if (guard)
    {
        std::wstring wfileName = widen(fileName);
        wfileName = AdjustSlashes(wfileName);

        auto result = std::make_unique<FindData3>();
        result->RememberedInstance = dllInstance;
        result->requested_path = wfileName;

#if _DEBUG
        if (psf::is_ansi<CharT>)
        {
            LogString(dllInstance, L"\tFindFirstFileExAFixup: for fileName", fileName);
        }
        else
        {
            LogString(dllInstance, L"\tFindFirstFileExWFixup: for fileName", fileName);
        }

        switch (infoLevelId)
        {
        case FindExInfoStandard:
            Log(L"[%d]\t\tLevel FindExInfoStandard", dllInstance);
            break;
        case FindExInfoBasic:
            Log(L"[%d]\t\tLevel FindExInfoBasic", dllInstance);
            break;
        case FindExInfoMaxInfoLevel:
            Log(L"[%d]\t\tLevel FindExInfoMaxInfoLevel", dllInstance);
            break;
        default:
            Log(L"[%d]\t\tLevel unknown", dllInstance);
            break;
        }
        switch (searchOp)
        {
        case FindExSearchNameMatch:
            Log(L"[%d]\t\tSearchOp FindExSearchNameMatch", dllInstance);
            break;
        case FindExSearchLimitToDirectories:
            Log(L"[%d]\t\tSearchOp FindExSearchLimitToDirectories", dllInstance);
            break;
        case FindExSearchLimitToDevices:
            Log(L"[%d]\t\tSearchOp FindExSearchLimitToDevices", dllInstance);
            break;
        case FindExSearchMaxSearchOp:
            Log(L"[%d]\t\tSearchOp FindExSearchMaxSearchOp", dllInstance);
            break;
        default:
            Log(L"[%d]\t\tSearchOp Unknown=0x%x", dllInstance, searchOp);
            break;
        }
#endif

        // Determine possible paths involved
        Cohorts cohorts;
        DetermineCohorts(wfileName, &cohorts, moreDebug, dllInstance, L"FindFirstFileExFixup");

#if MOREDEBUG
        Log(L"[%d] FindFirstFileExFixup:      RedirPath=%s", dllInstance, cohorts.WsRedirected.c_str());
        Log(L"[%d] FindFirstFileExFixup:    PackagePath=%s", dllInstance, cohorts.WsPackage.c_str());
        if (cohorts.UsingNative)
        {
            Log(L"[%d] FindFirstFileExFixup:     NativePath=%s", dllInstance, cohorts.WsNative.c_str());
        }
        else
        {
            Log(L"[%d] FindFirstFileExFixup:  NO NativePath", dllInstance);
        }
#endif

        //

        [[maybe_unused]] auto ansiData = reinterpret_cast<WIN32_FIND_DATAA*>(findFileData);
        [[maybe_unused]] auto wideData = reinterpret_cast<WIN32_FIND_DATAW*>(findFileData);
        WIN32_FIND_DATAW* findData = psf::is_ansi<CharT> ? &result->cached_data : wideData;
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
                copy_find_data(*wideData, result->cached_data);
            }
#if _DEBUG
            Log(L"[%d] FindFirstFileExFixup[%d] (from redirected): had results %ls", dllInstance, Result_Redirected, findData->cFileName);
#endif
            //AnyValidPath = true;
            //AnyValidResult = true;
        }
        else
        {
            //if (initialFindError == ERROR_FILE_NOT_FOUND)
            //    AnyValidPath = true;

            // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
            //result->redirect_path.clear();
#if _DEBUG
            Log(L"[%d] FindFirstFileExFixup[%d] (from redirected): no results.", dllInstance, Result_Redirected);
#endif
        }
        // save for next level
        findData = (result->find_handles[Result_Redirected] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;

        rldUseFile = MakeLongPath(cohorts.WsPackage);
        result->find_handles[Result_Package].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
        ///result->package_vfs_path.resize(vfspathSize);
        if (result->find_handles[Result_Package])
        {
#if _DEBUG
            Log(L"[%d] FindFirstFileExFixup[%d] (from package):   had results %ls", dllInstance, Result_Package, findData->cFileName);
#endif
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                initialFindError = ERROR_FILE_NOT_FOUND;
            ///result->package_vfs_path.clear();
#if _DEBUG
            Log(L"[%d] FindFirstFileExFixup[%d] (from package):   no results.", dllInstance, Result_Package);
#endif
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
#if _DEBUG
                        Log(L"[%d] FindFirstFileEx error set by caller", dllInstance);
#endif
                        // NOTE: Last error set by caller
                        return INVALID_HANDLE_VALUE;
                    }
                }
                else
                {
                    // No need to copy since we wrote directly into the output buffer
                    assert(findData == wideData);
                    copy_find_data(*wideData, result->cached_data);
                }
            }
        }

        // save for next level
        findData = (result->find_handles[Result_Redirected] || result->find_handles[Result_Package] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;

        if (cohorts.UsingNative)
        {
            rldUseFile = MakeLongPath(cohorts.WsNative);
            result->find_handles[Result_Native].reset(impl::FindFirstFileEx(rldUseFile.c_str(), infoLevelId, findData, searchOp, searchFilter, additionalFlags));
            if (result->find_handles[Result_Native])
            {
#if _DEBUG
                Log(L"[%d] FindFirstFileExFixup[%d] (from native)    had results=%ls", dllInstance, Result_Native, findData->cFileName);
#endif
                initialFindError = ERROR_SUCCESS;
            }
            else
            {
                if (GetLastError() == ERROR_FILE_NOT_FOUND)
                    initialFindError = ERROR_FILE_NOT_FOUND;
#if _DEBUG
                Log(L"[%d] FindFirstFileExFixupV2[%d] (from native):   no results.", dllInstance, Result_Native);
#endif
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
#if _DEBUG
                            Log(L"[%d] FindFirstFileExFixup error set by caller", dllInstance);
#endif
                            // NOTE: Last error set by caller
                            return INVALID_HANDLE_VALUE;
                        }
                    }
                    else
                    {
                        // No need to copy since we wrote directly into the output buffer
                        assert(findData == wideData);
                        copy_find_data(*wideData, result->cached_data);
                    }
                }
            }
        }
        else
        {
#if _DEBUG
            Log(L"[%d] FindFirstFileExFixup[%d] (from native):    no results possible.", dllInstance, Result_Native);
#endif
        }

        if (result->find_handles[Result_Redirected] ||
            result->find_handles[Result_Package] ||
            result->find_handles[Result_Native])
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                // return first result found
                copy_find_data(*ansiData, result->cached_data);
                //result->cached_data = ansiData;  
            }
#if _DEBUG
            Log(L"[%d] FindFirstFileExFixup returns %ls", dllInstance, result->cached_data.cFileName);
#endif
            result->wsAlready_returned_list.push_back(result->cached_data.cFileName);
            ::SetLastError(ERROR_SUCCESS);

        }
        else
        {
#if _DEBUG
            Log(L"[%d] FindFirstFileExFixup returns 0x%x", dllInstance, initialFindError);
#endif
            ::SetLastError(initialFindError);
            return INVALID_HANDLE_VALUE;
        }
        return reinterpret_cast<HANDLE>(result.release());
    }
#ifdef MOREDEBUG
    else
    {

    switch (infoLevelId)
    {
    case FindExInfoStandard:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) Level FindExInfoStandard", dllInstance);
        break;
    case FindExInfoBasic:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) Level FindExInfoBasic", dllInstance);
        break;
    case FindExInfoMaxInfoLevel:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) Level FindExInfoMaxInfoLevel", dllInstance);
        break;
    default:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) Level unknown", dllInstance);
        break;
    }
    switch (searchOp)
    {
    case FindExSearchNameMatch:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchNameMatch", dllInstance);
        break;
    case FindExSearchLimitToDirectories:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchLimitToDirectories", dllInstance);
        break;
    case FindExSearchLimitToDevices:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchLimitToDevices", dllInstance);
        break;
    case FindExSearchMaxSearchOp:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) SearchOp FindExSearchMaxSearchOp", dllInstance);
        break;
    default:
        Log(L"[%d]\tFindFirstFileExFixup: (unguarded) SearchOp Unknown=0x%x", dllInstance, searchOp);
        break;
    }

    }
#endif

    // If still here, call original.
#if _DEBUG
    LogString(dllInstance, L"\tFindFirstFileExFixup: (unguarded) for fileName", fileName);
#endif
    retfinal = impl::FindFirstFileEx(fileName, infoLevelId, findFileData, searchOp, searchFilter, additionalFlags);
#if _DEBUG
    Log(L"[%d] FindFirstFileFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
catch (...)
{
    // NOTE: Since we allocate our own "find handle" memory, we can't just forward on to the implementation
    ::SetLastError(win32_from_caught_exception());
    Log(L"***FindFirstFileExFixup Exception***");
    return INVALID_HANDLE_VALUE;
}
DECLARE_STRING_FIXUP(impl::FindFirstFileEx, FindFirstFileExFixup);

