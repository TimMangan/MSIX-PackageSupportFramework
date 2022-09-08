//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextfilew

#if _DEBUG
#define MOREDEBUG 1
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

template <typename CharT>
using win32_find_data_t = std::conditional_t<psf::is_ansi<CharT>, WIN32_FIND_DATAA, WIN32_FIND_DATAW>;


template <typename CharT>
BOOL __stdcall FindNextFileFixup(_In_ HANDLE findFile, _Out_ win32_find_data_t<CharT>* findFileData) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    [[maybe_unused]] DWORD DllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    ///HANDLE retfinal = INVALID_HANDLE_VALUE;
    if (guard)
    {

        if (findFile == INVALID_HANDLE_VALUE)
        {
#if _DEBUG
            Log(L"[%d] FindNextFileFixup invaid handle.", DllInstance);
#endif
            ::SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }


        auto data = reinterpret_cast<FindData3*>(findFile);

#if _DEBUG
        Log(L"[%d][%d] FindNextFileFixup is against original request=%ls", data->RememberedInstance, DllInstance, data->requested_path.c_str());
#ifdef MOREDEBUG
        //Log(L"[%d][%d] FindNextFileFixup is against redir    =%ls", data->RememberedInstance, DllInstance, data->redirect_path.c_str());
        //Log(L"[%d][%d] FindNextFileFixup is against pkgVfs   =%ls", data->RememberedInstance, DllInstance, data->package_vfs_path.c_str());
        //Log(L[%d]"[%d] FindNextFileFixup is against deVfs    =%ls", data->RememberedInstance, DllInstance, data->package_devfs_path.c_str());
#endif
#endif

        auto wasFileAlreadyProvided = [&](std::wstring findrequest, auto filename)
        {
#if _DEBUG
            LogString(data->RememberedInstance, DllInstance, L"\tFindNextFileFixup wasFileAlreadyProvided versus ", filename);
#endif

            if (data->wsAlready_returned_list.empty())
            {
#if _DEBUG
                Log(L"[%d][%d]\tFindNextFileFixup wasFileAlreadyProvided returns false.", data->RememberedInstance, DllInstance);
#endif
                return false;
            }

            std::wstring wFilename;
            // NOTE: 'is_ansi' evaluation not inline due to the bug:
            //       https://developercommunity.visualstudio.com/content/problem/324366/illegal-indirection-error-when-the-evaluation-of-i.html
            constexpr bool is_ansi = psf::is_ansi<std::decay_t<decltype(*filename)>>;
            if constexpr (is_ansi)
            {
                wFilename = widen(filename);
            }
            else
            {
                wFilename = filename;
            }

            // always return false on directories as these are always considered merged.
            std::filesystem::path fullpath = findrequest.c_str();
            fullpath = fullpath.parent_path() / wFilename.c_str();

            _locale_t locale = _wcreate_locale(LC_ALL, L"");
            for (std::wstring check : data->wsAlready_returned_list)
            {
                //            if (check.compare(wFilename.c_str()) == 0)
                if (_wcsicmp_l(check.c_str(), wFilename.c_str(), locale) == 0)
                {
#if MOREDEBUG
                    Log(L"[%d][%d]\tFindNextFileFixup A wasFileAlreadyProvided returns true %ls", data->RememberedInstance, DllInstance, wFilename.c_str());
#endif
                    _free_locale(locale);
                    return true;
                }
            }
            _free_locale(locale);

#if MOREDEBUG
            Log(L"[%d][%d]\tFindNextFileFixup wasFileAlreadyProvided returns false", data->RememberedInstance, DllInstance);
#endif
            return false;
        };


        while (data->find_handles[Result_Redirected])
        {
            if (impl::FindNextFile(data->find_handles[Result_Redirected].get(), findFileData))
            {
                // Skip the file if the name was previously used, unless it is a directory
                if (!wasFileAlreadyProvided(data->requested_path, findFileData->cFileName))
                {
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", data->RememberedInstance, DllInstance, Result_Redirected, widen(findFileData->cFileName).c_str());
#endif
                    data->wsAlready_returned_list.push_back(widen(findFileData->cFileName));
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                else
                {
                    // Otherwise, skip this file and check the next one
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileFixup[%d] skips file %ls", data->RememberedInstance, DllInstance, Result_Redirected, widen(findFileData->cFileName).c_str());
#endif
                }
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                ///Log(L"[%d][%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, DllInstance, Result_Redirected);
                data->find_handles[Result_Redirected].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                // now check next
            }
            else
            {
#if _DEBUG
                Log(L"[%d][%d] FindNextFileFixup[%d] returns FALSE 0x%x", data->RememberedInstance, DllInstance, Result_Redirected, ::GetLastError());
#endif
                // Error due to something other than reaching the end
                return FALSE;
            }
        }


        while (data->find_handles[Result_Package])
        {
            if (impl::FindNextFile(data->find_handles[Result_Package].get(), findFileData))
            {
                // Skip the file if the name was previously used, unless it is a directory
                if (!wasFileAlreadyProvided(data->requested_path, findFileData->cFileName))
                {
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", data->RememberedInstance, DllInstance, Result_Package, widen(findFileData->cFileName).c_str());
#endif
                    data->wsAlready_returned_list.push_back(widen(findFileData->cFileName));
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                else
                {
                    // Otherwise, skip this file and check the next one
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileFixup[%d] skips file %ls", data->RememberedInstance, DllInstance, Result_Package, widen(findFileData->cFileName).c_str());
#endif
                }
        }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                ///Log(L"[%d][%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, DllInstance, Result_Package);
                data->find_handles[Result_Package].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                // now check next
            }
            else
            {
#if _DEBUG
                Log(L"[%d][%d] FindNextFileFixup[%d] returns FALSE 0x%x", data->RememberedInstance, DllInstance, Result_Package, ::GetLastError());
#endif
                // Error due to something other than reaching the end
                return FALSE;
            }
        }


        while (data->find_handles[Result_Native])
        {
            if (impl::FindNextFile(data->find_handles[Result_Native].get(), findFileData))
            {
                // Skip the file if the name was previously used, unless it is a directory
                if (!wasFileAlreadyProvided(data->requested_path, findFileData->cFileName))
                {
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", data->RememberedInstance, DllInstance, Result_Native, widen(findFileData->cFileName).c_str());
#endif
                    data->wsAlready_returned_list.push_back(widen(findFileData->cFileName));
                    ::SetLastError(ERROR_SUCCESS);
                    return TRUE;
                }
                else
                {
                    // Otherwise, skip this file and check the next one
#if _DEBUG
                    Log(L"[%d][%d] FindNextFileFixup[%d] skips file %ls", data->RememberedInstance, DllInstance, Result_Native, widen(findFileData->cFileName).c_str());
#endif
                }
            }
            else if (::GetLastError() == ERROR_NO_MORE_FILES)
            {
                ///Log(L"[%d][%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, DllInstance, Result_Native);
                data->find_handles[Result_Native].reset();
                ::SetLastError(ERROR_NO_MORE_FILES);
                // now check next
            }
            else
            {
#if _DEBUG
                Log(L"[%d][%d] FindNextFileFixup[%d] returns FALSE 0x%x", data->RememberedInstance, DllInstance, Result_Native, ::GetLastError());
#endif
                // Error due to something other than reaching the end
                return FALSE;
            }
        }


        // We ran out of data either on a previous call, or by ignoring files that have been redirected
#if _DEBUG
        Log(L"[%d][%d] FindNextFileFixu[ returns FALSE with ERROR_NO_MORE_FILES.", data->RememberedInstance, DllInstance);
#endif
        ::SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;

    }

#if _DEBUG
    Log(L"FindNextFileFixup (unguarded) for file.");
#endif
    return impl::FindNextFile(findFile, findFileData);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::FindNextFile, FindNextFileFixup);
