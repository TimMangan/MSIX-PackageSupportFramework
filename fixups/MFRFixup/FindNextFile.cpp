//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextfilew

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include <psf_logging.h>
#include <memory>
#include "FindData3.h"

template <typename CharT>
using win32_find_data_t = std::conditional_t<psf::is_ansi<CharT>, WIN32_FIND_DATAA, WIN32_FIND_DATAW>;

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:FindNextFile_Ansi_Fixup=impl::_FindNextFileW.ansi@8")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:FindNextFile_Wide_Fixup=impl::_FindNextFileW.wide@8")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:FindNextFileFixupAnsi_Fixup=impl::FindNextFileW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:FindNextFileFixupWide_Fixup=impl::FindNextFileW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

template <typename CharT>
BOOL __stdcall FindNextFileFixup(_In_ HANDLE findFile, _Out_ win32_find_data_t<CharT>* findFileData) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    [[maybe_unused]] DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    ///HANDLE retfinal = INVALID_HANDLE_VALUE;
    if (guard)
    {

        if (findFile == INVALID_HANDLE_VALUE)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindNextFileFixup invalid handle.", g_MfrModuleName, dllInstance);

            ::SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }


        auto data3A = reinterpret_cast<FindData3A*>(findFile);
        auto data3W = reinterpret_cast<FindData3W*>(findFile);

        if (data3A && data3A->IsAnsi)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against original request=%ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, data3A->requested_path.c_str());
            //Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against redir    =%ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, data3A->redirect_path.c_str());
            //Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against pkgVfs   =%ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, data3A->package_vfs_path.c_str());
            //Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against deVfs    =%ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, data3A->package_devfs_path.c_str());

            auto wasFileAlreadyProvided = [&](std::string findrequest, auto filename)
                {
                    LogString(LogLevel_DebugBasic, g_MfrModuleName, data3A->RememberedInstance, dllInstance, L"\tFindNextFileFixup wasFileAlreadyProvided versus ", filename);

                    if (data3A->sAlready_returned_list.empty())
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d][%s%d]\tFindNextFileFixup wasFileAlreadyProvided returns false.", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance);

                        return false;
                    }

                    std::wstring wFilename = widen(filename);

                    // always return false on directories as these are always considered merged.
                    std::filesystem::path fullpath = widen(findrequest.c_str());
                    fullpath = fullpath.parent_path() / wFilename.c_str();

                    _locale_t locale = _wcreate_locale(LC_ALL, L"");
                    for (std::string check : data3A->sAlready_returned_list)
                    {
                        if (_wcsicmp_l(widen(check).c_str(), wFilename.c_str(), locale) == 0)
                        {
                            Log(LogLevel_DebugBasic, L"[%s%d][%s%d]\tFindNextFileFixup A wasFileAlreadyProvided returns true %ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, wFilename.c_str());
                            _free_locale(locale);
                            return true;
                        }
                    }
                    _free_locale(locale);

                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d]\tFindNextFileFixup wasFileAlreadyProvided returns false", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance);

                    return false;
                };


            while (data3A->find_handles[Result_Redirected])
            {
                if (impl::FindNextFile(data3A->find_handles[Result_Redirected].get(), findFileData))
                {
                    // Skip the file if the name was previously used, unless it is a directory
                    if (!wasFileAlreadyProvided(data3A->requested_path, findFileData->cFileName))
                    {
                        Log(LogLevel_DebugBasic, "[%s%d][%s%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected, widen(findFileData->cFileName).c_str());

                        data3A->sAlready_returned_list.push_back(narrow(findFileData->cFileName));
                        ::SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        // Otherwise, skip this file and check the next one
                        Log(LogLevel_DebugMaximum, L"[%s%d][%s%d] FindNextFileFixup[%d] skips file %ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected, widen(findFileData->cFileName).c_str());
                    }
                }
                else if (::GetLastError() == ERROR_NO_MORE_FILES)
                {
                    ///Log(L"[%s%d][%s%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected);
                    data3A->find_handles[Result_Redirected].reset();
                    ::SetLastError(ERROR_NO_MORE_FILES);
                    // now check next
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns FALSE 0x%x", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected, ::GetLastError());
                    // Error due to something other than reaching the end
                    return FALSE;
                }
            }


            while (data3A->find_handles[Result_Package])
            {
                if (impl::FindNextFile(data3A->find_handles[Result_Package].get(), findFileData))
                {
                    // Skip the file if the name was previously used, unless it is a directory
                    if (!wasFileAlreadyProvided(data3A->requested_path, findFileData->cFileName))
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package, widen(findFileData->cFileName).c_str());
                        data3A->sAlready_returned_list.push_back(narrow(findFileData->cFileName));
                        ::SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        // Otherwise, skip this file and check the next one
                        Log(LogLevel_DebugMaximum, L"[%s%d][%s%d] FindNextFileFixup[%d] skips file %ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package, widen(findFileData->cFileName).c_str());
                    }
                }
                else if (::GetLastError() == ERROR_NO_MORE_FILES)
                {
                    ///Log(L"[%s%d][%s%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package);
                    data3A->find_handles[Result_Package].reset();
                    ::SetLastError(ERROR_NO_MORE_FILES);
                    // now check next
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns FALSE 0x%x", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package, ::GetLastError());
                    // Error due to something other than reaching the end
                    return FALSE;
                }
            }


            while (data3A->find_handles[Result_Native])
            {
                if (impl::FindNextFile(data3A->find_handles[Result_Native].get(), findFileData))
                {
                    // Skip the file if the name was previously used, unless it is a directory
                    if (!wasFileAlreadyProvided(data3A->requested_path, findFileData->cFileName))
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native, widen(findFileData->cFileName).c_str());
                        data3A->sAlready_returned_list.push_back(narrow(findFileData->cFileName));
                        ::SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        // Otherwise, skip this file and check the next one
                        Log(LogLevel_DebugMaximum, L"[%s%d][%s%d] FindNextFileFixup[%d] skips file %ls", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native, widen(findFileData->cFileName).c_str());
                    }
                }
                else if (::GetLastError() == ERROR_NO_MORE_FILES)
                {
                    ///Log(L"[%s%d][%s%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native);
                    data3A->find_handles[Result_Native].reset();
                    ::SetLastError(ERROR_NO_MORE_FILES);
                    // now check next
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns FALSE 0x%x", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native, ::GetLastError());
                    // Error due to something other than reaching the end
                    return FALSE;
                }
            }
        }
        else 
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against original request=%s", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, data3W->requested_path.c_str());
            //Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against redir    =%ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, data3W->redirect_path.c_str());
            //Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against pkgVfs   =%ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, data3W->package_vfs_path.c_str());
            //Log(LogLevel_DebugIntermediate, L"[%s%d][%s%d] FindNextFileFixup is against deVfs    =%ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, data3W->package_devfs_path.c_str());


            auto wasFileAlreadyProvided = [&](std::wstring findrequest, auto filename)
                {
                    LogString(LogLevel_DebugBasic, g_MfrModuleName, data3W->RememberedInstance, dllInstance, L"\tFindNextFileFixup wasFileAlreadyProvided versus ", filename);

                    if (data3W->wsAlready_returned_list.empty())
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d][%s%d]\tFindNextFileFixup wasFileAlreadyProvided returns false.", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance);
                        return false;
                    }

                    std::wstring wFilename = widen(filename);
                   

                    // always return false on directories as these are always considered merged.
                    std::filesystem::path fullpath = findrequest.c_str();
                    fullpath = fullpath.parent_path() / wFilename.c_str();

                    _locale_t locale = _wcreate_locale(LC_ALL, L"");
                    for (std::wstring check : data3W->wsAlready_returned_list)
                    {
                        //            if (check.compare(wFilename.c_str()) == 0)
                        if (_wcsicmp_l(check.c_str(), wFilename.c_str(), locale) == 0)
                        {
                            Log(LogLevel_DebugBasic, L"[%s%d][%s%d]\tFindNextFileFixup A wasFileAlreadyProvided returns true %ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, wFilename.c_str());
                            _free_locale(locale);
                            return true;
                        }
                    }
                    _free_locale(locale);

                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d]\tFindNextFileFixup wasFileAlreadyProvided returns false", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance);
                    return false;
                };


            while (data3W->find_handles[Result_Redirected])
            {
                if (impl::FindNextFile(data3W->find_handles[Result_Redirected].get(), findFileData))
                {
                    // Skip the file if the name was previously used, unless it is a directory
                    if (!wasFileAlreadyProvided(data3W->requested_path, findFileData->cFileName))
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected, widen(findFileData->cFileName).c_str());

                        data3W->wsAlready_returned_list.push_back(widen(findFileData->cFileName));
                        ::SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        // Otherwise, skip this file and check the next one
                        Log(LogLevel_DebugMaximum, L"[%s%d][%s%d] FindNextFileFixup[%d] skips file %ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected, widen(findFileData->cFileName).c_str());
                    }
                }
                else if (::GetLastError() == ERROR_NO_MORE_FILES)
                {
                    ///Log(L"[%s%d][%s%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected);
                    data3W->find_handles[Result_Redirected].reset();
                    ::SetLastError(ERROR_NO_MORE_FILES);
                    // now check next
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns FALSE 0x%x", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Redirected, ::GetLastError());

                    // Error due to something other than reaching the end
                    return FALSE;
                }
            }


            while (data3W->find_handles[Result_Package])
            {
                if (impl::FindNextFile(data3W->find_handles[Result_Package].get(), findFileData))
                {
                    // Skip the file if the name was previously used, unless it is a directory
                    if (!wasFileAlreadyProvided(data3W->requested_path, findFileData->cFileName))
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package, widen(findFileData->cFileName).c_str());
                        data3W->wsAlready_returned_list.push_back(widen(findFileData->cFileName));
                        ::SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        // Otherwise, skip this file and check the next one
                        Log(LogLevel_DebugMaximum, L"[%s%d][%s%d] FindNextFileFixup[%d] skips file %ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package, widen(findFileData->cFileName).c_str());
                    }
                }
                else if (::GetLastError() == ERROR_NO_MORE_FILES)
                {
                    ///Log(L"[%s%d][%s%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package);
                    data3W->find_handles[Result_Package].reset();
                    ::SetLastError(ERROR_NO_MORE_FILES);
                    // now check next
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns FALSE 0x%x", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Package, ::GetLastError());
                    // Error due to something other than reaching the end
                    return FALSE;
                }
            }


            while (data3W->find_handles[Result_Native])
            {
                if (impl::FindNextFile(data3W->find_handles[Result_Native].get(), findFileData))
                {
                    // Skip the file if the name was previously used, unless it is a directory
                    if (!wasFileAlreadyProvided(data3W->requested_path, findFileData->cFileName))
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns TRUE with ERROR_SUCCESS and file %ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native, widen(findFileData->cFileName).c_str());

                        data3W->wsAlready_returned_list.push_back(widen(findFileData->cFileName));
                        ::SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        // Otherwise, skip this file and check the next one
                        Log(LogLevel_DebugMaximum, L"[%s%d][%s%d] FindNextFileFixup[%d] skips file %ls", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native, widen(findFileData->cFileName).c_str());
                    }
                }
                else if (::GetLastError() == ERROR_NO_MORE_FILES)
                {
                    ///Log(L"[%s%d][%s%d] FindNextFileFixup[%d] had FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native);
                    data3W->find_handles[Result_Native].reset();
                    ::SetLastError(ERROR_NO_MORE_FILES);
                    // now check next
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[%d] returns FALSE 0x%x", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance, Result_Native, ::GetLastError());
                    // Error due to something other than reaching the end
                    return FALSE;
                }
            }
        }

        // We ran out of data either on a previous call, or by ignoring files that have been redirected
        if (data3A && data3A->IsAnsi)
        {
            Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[ returns FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3A->RememberedInstance, g_MfrModuleName, dllInstance);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d][%s%d] FindNextFileFixup[ returns FALSE with ERROR_NO_MORE_FILES.", g_MfrModuleName, data3W->RememberedInstance, g_MfrModuleName, dllInstance);
        }
        ::SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;

    }

    Log(LogLevel_DebugBasic, L"[%s%d] FindNextFileFixup (unguarded) for file.", g_MfrModuleName, dllInstance );
    return impl::FindNextFile(findFile, findFileData);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    Log(LogLevel_Exception, L"***FindNextFile Exception***");
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::FindNextFile, FindNextFileFixup);
