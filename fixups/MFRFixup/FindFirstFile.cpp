//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilew

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
// NOTE2: Unlike many MFR intercepts, this function returns strings to the caller.

// =======================================================================================================  
// NOTE3: ****VERY IMPORTANT*** Changes to this file MUST be made in parallel with FindFirstFileEx.cpp
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
#include <psf_logging.h>
#include <memory>
#include "FindData3.h"
#include "FindFirstHelpers.h"
#include "DetermineCohorts.h"


template <typename CharT>
HANDLE __stdcall FindFirstFileFixup(_In_ const CharT* fileName, _Out_ win32_find_data_t<CharT>* findFileData) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    [[maybe_unused]] DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    bool moreDebug = false;
#if _DEBUG
    debug = true;
#endif
#if MOREDEBUG
    //moreDebug = true;
#endif

    if (guard)
    {
        std::wstring wfileName = AdjustSlashes(widen(fileName));

        auto result = std::make_unique<FindData3>();
        result->RememberedInstance = dllInstance;
        result->requested_path = wfileName;

#if _DEBUG
        LogString(dllInstance, L"FindFirstFileFixup: for fileName", fileName);
#endif

        // Determine possible paths involved
        Cohorts cohorts; 
        DetermineCohorts(wfileName, &cohorts, moreDebug, dllInstance, L"FindFirstFileFixup");


#if MOREDEBUG
        Log(L"[%d] FindFirstFileFixup:  Adjusted Path=%s", dllInstance, wfileName.c_str());
        Log(L"[%d] FindFirstFileFixup:      RedirPath=%s", dllInstance, cohorts.WsRedirected.c_str());
        Log(L"[%d] FindFirstFileFixup:    PackagePath=%s", dllInstance, cohorts.WsPackage.c_str());
        if (cohorts.UsingNative)
        {
            Log(L"[%d] FindFirstFileFixup:     NativePath=%s", dllInstance, cohorts.WsNative.c_str());
        }
        else
        {
            Log(L"[%d] FindFirstFileFixup:  NO NativePath", dllInstance);
        }
#endif

        //

        [[maybe_unused]] auto ansiData = reinterpret_cast<WIN32_FIND_DATAA*>(findFileData);
        [[maybe_unused]] auto wideData = reinterpret_cast<WIN32_FIND_DATAW*>(findFileData);
        WIN32_FIND_DATAW* findData = psf::is_ansi<CharT> ? &result->cached_data : wideData;
        DWORD initialFindError = ERROR_PATH_NOT_FOUND;

        // First find the redirected area results
        std::wstring rldUseFile = MakeLongPath(cohorts.WsRedirected);
        result->find_handles[Result_Redirected].reset(impl::FindFirstFile(rldUseFile.c_str(), findData));
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
            if (findData->cAlternateFileName != NULL)
            {
                Log(L"[%d] FindFirstFileFixup[%d] (from redirected): had results %ls %ls", dllInstance, Result_Redirected, findData->cFileName, findData->cAlternateFileName);
            }
            else
            {
                Log(L"[%d] FindFirstFileFixup[%d] (from redirected): had results %ls", dllInstance, Result_Redirected, findData->cFileName);
            }
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
            Log(L"[%d] FindFirstFileFixup[%d] (from redirected): no results.", dllInstance, Result_Redirected);
#endif
        }
        // save for next level
        findData = (result->find_handles[Result_Redirected] || psf::is_ansi<CharT>) ? &result->cached_data : wideData;

        rldUseFile = MakeLongPath(cohorts.WsPackage);
        result->find_handles[Result_Package].reset(impl::FindFirstFile(rldUseFile.c_str(), findData));
        ///result->package_vfs_path.resize(vfspathSize);
        if (result->find_handles[Result_Package])
        {
#if _DEBUG
            if (findData->cAlternateFileName != NULL)
            {
                Log(L"[%d] FindFirstFileFixup[%d] (from package):   had results %ls %ls", dllInstance, Result_Package, findData->cFileName, findData->cAlternateFileName);
            }
            else
            {
                Log(L"[%d] FindFirstFileFixup[%d] (from package):   had results %ls", dllInstance, Result_Package, findData->cFileName);
            }
#endif
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
                initialFindError = ERROR_FILE_NOT_FOUND;
            ///result->package_vfs_path.clear();
#if _DEBUG
            Log(L"[%d] FindFirstFileFixup[%d] (from package):   no results.", dllInstance, Result_Package);
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
                        Log(L"[%d] FindFirstFile error set by caller", dllInstance);
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
            result->find_handles[Result_Native].reset(impl::FindFirstFile(rldUseFile.c_str(), findData));
            if (result->find_handles[Result_Native])
            {
#if _DEBUG
                if (findData->cAlternateFileName != NULL)
                {
                    Log(L"[%d] FindFirstFileFixup[%d] (from native)    had results=%ls %ls", dllInstance, Result_Native, findData->cFileName, findData->cAlternateFileName);
                }
                else
                {
                    Log(L"[%d] FindFirstFileFixup[%d] (from native)    had results=%ls", dllInstance, Result_Native, findData->cFileName);
                }
#endif
                initialFindError = ERROR_SUCCESS;
            }
            else
            {
                if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
                    initialFindError = ERROR_FILE_NOT_FOUND;
#if _DEBUG
                Log(L"[%d] FindFirstFileFixup[%d] (from native):   no results.", dllInstance, Result_Native);
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
                            Log(L"[%d] FindFirstFileFixup error set by caller", dllInstance);
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
            Log(L"[%d] FindFirstFileFixup[%d] (from native):    no results possible.", dllInstance, Result_Native);
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
            if (result->cached_data.cAlternateFileName != NULL)
            {
                Log(L"[%d] FindFirstFileFixup returns %ls %ls", dllInstance, result->cached_data.cFileName, result->cached_data.cAlternateFileName);
            }
            else
            {
                Log(L"[%d] FindFirstFileFixup returns %ls", dllInstance, result->cached_data.cFileName);
            }
#endif
            result->wsAlready_returned_list.push_back(result->cached_data.cFileName);
            ::SetLastError(ERROR_SUCCESS);

        }
        else
        {
#if _DEBUG
            Log(L"[%d] FindFirstFileFixup returns 0x%x", dllInstance, initialFindError);
#endif
            ::SetLastError(initialFindError);
            return INVALID_HANDLE_VALUE;
        }
        return reinterpret_cast<HANDLE>(result.release());
    }

    // If still here, call original.
#if _DEBUG
    LogString(dllInstance, L"\tFindFirstFileFixup: (unguarded) for fileName", fileName);
#endif
    return impl::FindFirstFile(fileName, findFileData);
}
catch (...)
{
    // NOTE: Since we allocate our own "find handle" memory, we can't just forward on to the implementation
    ::SetLastError(win32_from_caught_exception());
    Log(L"***FindFirstFileFixup Exception***");
    return INVALID_HANDLE_VALUE;
}
DECLARE_STRING_FIXUP(impl::FindFirstFile, FindFirstFileFixup);

