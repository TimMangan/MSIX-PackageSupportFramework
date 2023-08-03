//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// The PsfRuntime intercepts all AddDirectory and SetDirectory[AW] calls so that paths may be fixed up. 

#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <psf_constants.h>
#include <psf_framework.h>
#include <psf_logging.h>

#include "Config.h"
#include <StartInfo_helper.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <findStringIC.h>

using namespace std::literals;


#include <reentrancy_guard.h>
#include <psf_framework.h>

inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    inline auto AddDllDirectory = &::AddDllDirectory;
    inline auto SetDefaultDllDirectories = &::SetDefaultDllDirectories;
    inline auto SetDllDirectory = psf::detoured_string_function(&::SetDllDirectoryA, &::SetDllDirectoryW);
}

DWORD g_AddSetDllDirectoryIntceptInstance = 20000;

///auto AddDllDirectoryImpl = psf::detoured_string_function(nullptr, &::AddDllDirectory);


BOOL WINAPI SetDefaultDllDirectoriesFixup(
    _In_ DWORD DirectoryFlags)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
#ifdef _DEBUG
        DWORD AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryIntceptInstance;
        Log(L" [%d] SetDefaultDllDirectoriesFixup: 0x%x", AddSetDllDirectoryInstance, DirectoryFlags);
#endif
    }
    return impl::SetDefaultDllDirectories(DirectoryFlags);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_FIXUP(impl::SetDefaultDllDirectories, SetDefaultDllDirectoriesFixup);


DLL_DIRECTORY_COOKIE WINAPI AddDllDirectoryFixup(
    _In_opt_ PCWSTR path) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
#ifdef _DEBUG
        DWORD AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryIntceptInstance;
        if (path == NULL)
        {
            Log(L" [%d] AddDllDirectoryFixup: Input path is null", AddSetDllDirectoryInstance);
        }
        else
        {
            LogString(AddSetDllDirectoryInstance, L"AddDllDirectoryFixup: Input path", path);
        }
        // We may need to alter the path in some cases.  But first we need to trap an app
        // needing this.
#endif

        DLL_DIRECTORY_COOKIE Ddcret = impl::AddDllDirectory(path);
        return Ddcret;
    }
    return impl::AddDllDirectory(path);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}

DECLARE_FIXUP(impl::AddDllDirectory, AddDllDirectoryFixup);



//auto SetDllDirectoryImpl = psf::detoured_string_function(&::SetDllDirectoryA, &::SetDllDirectoryW);

template <typename CharT>
BOOL WINAPI SetDllDirectoryFixup(
    _In_opt_ const CharT* path) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
#ifdef _DEBUG
        DWORD AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryIntceptInstance;
        // We may need to alter the path in some cases.  But first we need to trap an app
        // needing this.
        if (path == NULL)
        {
            Log(L"\t[%d] SetDllDirectoryFixup: Input path is null (restores search order)", AddSetDllDirectoryInstance);
        }
        else
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                if (strlen(path) == 0)
                {
                    Log(L"\t[%d] SetDllDirectoryFixup: Input path is empty (remove current directory from list)", AddSetDllDirectoryInstance);
                }
                else
                {
                    LogString(AddSetDllDirectoryInstance, "SetDllDirectoryFixup: Input path", path);
                }
            }
            else
            {
                if (wcslen(path) == 0)
                {
                    Log(L"\t[%d] SetDllDirectoryFixup: Input path is empty (remove current directory from list)", AddSetDllDirectoryInstance);
                }
                else
                {
                    LogString(AddSetDllDirectoryInstance, L"SetDllDirectoryFixup: Input path", path);
                }
            }
        }
#endif

        BOOL Bret = impl::SetDllDirectory(path);
        return Bret;
    }
    return impl::SetDllDirectory(path);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}

DECLARE_STRING_FIXUP(impl::SetDllDirectory, SetDllDirectoryFixup);