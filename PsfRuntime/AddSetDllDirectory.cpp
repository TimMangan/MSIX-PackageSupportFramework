//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// The PsfRuntime intercepts all AddDirectory and SetDirectory[AW] calls so that paths may be fixed up. 

#define ENABLE_DIRECTORY_INTERCEPTS 1

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

#include <filesystem>
#include <dos_paths.h>
#include <known_folders.h>
#include "Config.h"


using namespace std::literals;


#include <reentrancy_guard.h>
#include <psf_framework.h>

#ifdef ENABLE_DIRECTORY_INTERCEPTS
extern const wchar_t* g_PsfRunTimeName;

inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    inline auto AddDllDirectory = &::AddDllDirectory;
    inline auto RemoveDllDirectory = &::RemoveDllDirectory;
    inline auto SetDefaultDllDirectories = &::SetDefaultDllDirectories;
    inline auto SetDllDirectory = psf::detoured_string_function(&::SetDllDirectoryA, &::SetDllDirectoryW);
    //inline auto GetDllDirectory = psf::detoured_string_function(&::GetDllDirectoryA, &::GetDllDirectoryW);
    inline auto GetDllDirectoryA = &::GetDllDirectoryA;
    inline auto GetDllDirectoryW = &::GetDllDirectoryW;

}

DWORD g_AddSetDllDirectoryInterceptInstance = 20000;


// Helper function to match what ILV and or MfrFixup would have done for package paths.
std::wstring FixupPathToRedirectionPathIfNeeded(Json_Debug_Levels debugLevel, const wchar_t* moduleName, DWORD instance, std::wstring wPath)
{
    size_t offset;
    std::filesystem::path PackageRootPath = psf::current_package_path();
    std::wstring PackageFullName = psf::current_package_full_name();

    std::wstring fixedPath = wPath;

    psf::dos_path_type pathType = psf::path_type(wPath.c_str());
    switch (pathType)
    {
    case psf::dos_path_type::unknown:
        break;
    case psf::dos_path_type::unc_absolute:
        break;
    case psf::dos_path_type::drive_absolute:
        offset = wPath.find(PackageRootPath.c_str());
        if (offset != std::wstring::npos)
        {
            // This is a package path.  We need to see if it needs to be redirected.

            // input path may be C:\... or \\?\C:\... so let's not forget about offset.
            std::wstring remainder = wPath.substr(wcslen(offset + PackageRootPath.c_str())+1);

            std::filesystem::path newPath = psf::known_folder(FOLDERID_LocalAppData) / L"\\Packages\\" / PackageFullName / L"\\Local Cache\\Local\\Microsoft\\WritablePackageRoot\\" / remainder;
            
            fixedPath = newPath.wstring();
            Log(debugLevel, L" [%s%d] Add/SetDllDirectory:      Fixed up path=%s", moduleName, instance,fixedPath);
        }
        break;
    case psf::dos_path_type::drive_relative:
        break;
    case psf::dos_path_type::rooted:
        break;
    case psf::dos_path_type::relative:
        break;
    case psf::dos_path_type::local_device:
        break;
    case psf::dos_path_type::storage_namespace:
        break;
    case psf::dos_path_type::root_local_device:
        break;
    case psf::dos_path_type::DosSpecial:
        break;
    case psf::dos_path_type::protocol:
        break;
    case psf::dos_path_type::shell:
        break;
    default:
        break;
    }
    return fixedPath;

}



BOOL WINAPI SetDefaultDllDirectoriesFixup(
    _In_ DWORD DirectoryFlags)  noexcept try
{
    BOOL Bret;

    DWORD AddSetDllDirectoryInstance = g_AddSetDllDirectoryInterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryInterceptInstance;
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDefaultDllDirectoriesFixup: (Informational) DirectoryFlags=0x%x", g_PsfRunTimeName, AddSetDllDirectoryInstance, DirectoryFlags);
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDefaultDllDirectoriesFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, AddSetDllDirectoryInstance);

        Bret = impl::SetDefaultDllDirectories(DirectoryFlags);
    }
    else
    {
        // Unguarded
        Bret = impl::SetDefaultDllDirectories(DirectoryFlags);
    }

    if (Bret)
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDefaultDllDirectoriesFixup: Succeeded", g_PsfRunTimeName, AddSetDllDirectoryInstance);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDefaultDllDirectoriesFixup: Failed with error=0x%x", g_PsfRunTimeName, AddSetDllDirectoryInstance, ::GetLastError());
    }
    return Bret;
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
    DLL_DIRECTORY_COOKIE Ddcret;
    DWORD AddSetDllDirectoryInstance = g_AddSetDllDirectoryInterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryInterceptInstance;

        // We may need to alter the path in some cases.  But first we need to trap an app
        // needing this.
        bool FallThrough = false;
        if (path == NULL)
        {
            Log(LogLevel_DebugBasic, L"\t[%s%d]\tAddDllDirectoryFixup: Input path is null (restores search order)", g_PsfRunTimeName, AddSetDllDirectoryInstance);
            FallThrough = true;
        }
        else
        {
            if (wcslen(path) == 0)
            {
                // SetDllDirectory with empty string removes the current directory, but is an invalid parameter for this function.
                // The caller should have called RemoveDllDirectory with the cookie from the add, or maybe used SetDllDirectory
                // We should not try to fix the bad app here.
                Log(LogLevel_DebugBasic, L"\t[%s%d]\tAddDllDirectoryFixup: Input path is empty (incorrect way to call this) %s", g_PsfRunTimeName, AddSetDllDirectoryInstance,path);
                FallThrough = true;
            }
            else
            {
                LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tAddDllDirectoryFixup: Input path", path);
            }
        }

        if (FallThrough)
        {
             Ddcret = impl::AddDllDirectory(path);
        }
        else
        {
            std::wstring wPath = path;
            DWORD att = GetFileAttributes(wPath.c_str());
            if (att == INVALID_FILE_ATTRIBUTES)
            {
                // Path does not exist, see if we can fix it up
                std::wstring fixedPath = FixupPathToRedirectionPathIfNeeded(LogLevel_DebugMaximum, g_PsfRunTimeName, AddSetDllDirectoryInstance, wPath);
                if (fixedPath != wPath)
                {
                    LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tAddDllDirectoryFixup: Adjusted path to", fixedPath.c_str());
                    Ddcret = impl::AddDllDirectory(fixedPath.c_str());
                }
                else
                {
                    LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tAddDllDirectoryFixup: No adjustment made for path", wPath.c_str());
                    Ddcret = impl::AddDllDirectory(path);
                }
            }
            else
            {
                LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tAddDllDirectoryFixup: (Informational) Path exists, no adjustment needed for path", wPath.c_str());
                Ddcret = impl::AddDllDirectory(path);
            }
        }     
    }
    else
    {
        // Unguarded
        Ddcret = impl::AddDllDirectory(path);
    }
    if (Ddcret != NULL)
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tAddDllDirectoryFixup: Succeeded returning cookie=%p", g_PsfRunTimeName, AddSetDllDirectoryInstance, Ddcret);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tAddDllDirectoryFixup: Failed with error=0x%x", g_PsfRunTimeName, AddSetDllDirectoryInstance, ::GetLastError());
    }
    return Ddcret;
}
catch (...)
{
    Log(LogLevel_DebugBasic, L"\t[%s%d]\tAddDllDirectoryFixup: ***Exception***", g_PsfRunTimeName, g_AddSetDllDirectoryInterceptInstance);
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_FIXUP(impl::AddDllDirectory, AddDllDirectoryFixup);

BOOL WINAPI RemoveDllDirectoryFixup(
    _In_ DLL_DIRECTORY_COOKIE cookie) noexcept try
{
    BOOL Bret;
    DWORD AddSetDllDirectoryInstance = g_AddSetDllDirectoryInterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryInterceptInstance;
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tRemoveDllDirectoryFixup: Called with cookie from AddDllDirectory %p", g_PsfRunTimeName, AddSetDllDirectoryInstance,cookie);
        Bret = impl::RemoveDllDirectory(cookie);
    }
    else
    {
        // Unguarded
        Bret = impl::RemoveDllDirectory(cookie);
    }
    if (Bret)
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tRemoveDllDirectoryFixup: Succeeded", g_PsfRunTimeName, AddSetDllDirectoryInstance);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tRemoveDllDirectoryFixup: Failed with error=0x%x", g_PsfRunTimeName, AddSetDllDirectoryInstance, ::GetLastError());
    }
    return Bret;
}
catch (...)
{
    Log(LogLevel_DebugBasic, L"\t[%s%d]\tRemoveDllDirectoryFixup: ***Exception***", g_PsfRunTimeName, g_AddSetDllDirectoryInterceptInstance);
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_FIXUP(impl::RemoveDllDirectory, RemoveDllDirectoryFixup);


DWORD WINAPI GetDllDirectoryAFixup(
    _In_ DWORD nBufferLength,
    _Out_ LPSTR lpBuffer)
{
    DWORD dRet;
    auto guard = g_reentrancyGuard.enter();
    DWORD AddSetDllDirectoryInstance = g_AddSetDllDirectoryInterceptInstance;
    if (guard)
    {
        AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryInterceptInstance;
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tGetDllDirectoryAFixup: (Informational) length=0x%x", g_PsfRunTimeName, g_AddSetDllDirectoryInterceptInstance, nBufferLength);
        dRet = ::impl::GetDllDirectoryA(nBufferLength, lpBuffer);
    }
    else
    {
        // Unguarded
        dRet = ::impl::GetDllDirectoryA(nBufferLength, lpBuffer);
    }
    Log(LogLevel_DebugBasic, L"\t[%s%d]\tGetDllDirectoryAFixup: return length=0x%x", g_PsfRunTimeName, g_AddSetDllDirectoryInterceptInstance, dRet);
    return dRet;

}
DECLARE_FIXUP(impl::GetDllDirectoryA, GetDllDirectoryAFixup);


DWORD WINAPI GetDllDirectoryWFixup(
    _In_ DWORD nBufferLength,
    _Out_ LPWSTR lpBuffer)
{
    DWORD dRet;
    auto guard = g_reentrancyGuard.enter();
    DWORD AddSetDllDirectoryInstance = g_AddSetDllDirectoryInterceptInstance;
    if (guard)
    {
        AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryInterceptInstance;
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tGetDllDirectoryWFixup: (Informational) length=0x%x", g_PsfRunTimeName, g_AddSetDllDirectoryInterceptInstance, nBufferLength);
        dRet = ::impl::GetDllDirectoryW(nBufferLength, lpBuffer);
    }
    else
    {
        // Unguarded
        dRet = ::impl::GetDllDirectoryW(nBufferLength, lpBuffer);
    }
    Log(LogLevel_DebugBasic, L"\t[%s%d]\tGetDllDirectoryWFixup: return length=0x%x", g_PsfRunTimeName, g_AddSetDllDirectoryInterceptInstance, dRet);
    return dRet;

}
DECLARE_FIXUP(impl::GetDllDirectoryW, GetDllDirectoryWFixup);


// SetDllDirectoryFixup adds the specified directory to the dll search paths used by the app, or clears out the previous dynamic entries by passing in a null or empty string.
// When adding a directory, we must consider if the path is a package path and does not exist in the real file system, in which case we would want the redirected path.

template <typename CharT>
BOOL WINAPI SetDllDirectoryFixup(
    _In_opt_ const CharT* path) noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    DWORD AddSetDllDirectoryInstance = g_AddSetDllDirectoryInterceptInstance;
    BOOL Bret;
    if (guard)
    {
        AddSetDllDirectoryInstance = ++g_AddSetDllDirectoryInterceptInstance;
        
        // We may need to alter the path in some cases.  But first we need to trap an app
        // needing this.
        bool FallThrough = false;
        if (path == NULL)
        {
            Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDllDirectoryFixup: Input path is null (restores search order)", g_PsfRunTimeName, AddSetDllDirectoryInstance);
            FallThrough = true;
        }
        else
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                if (strlen(path) == 0)
                {
                    Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDllDirectoryFixup: Input path is empty (remove current directory from list)", g_PsfRunTimeName, AddSetDllDirectoryInstance);
                    FallThrough = true;
                }
                else
                {
                    LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, "SetDllDirectoryFixupA: Input path", path);
                }
            }
            else
            {
                if (wcslen(path) == 0)
                {
                    Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDllDirectoryFixup: Input path is empty (remove current directory from list)", g_PsfRunTimeName, AddSetDllDirectoryInstance);
                    FallThrough = true;
                }
                else
                {
                    LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tSetDllDirectoryFixupW: Input path", path);
                }
            }
        }

        if (FallThrough)
        {
            Bret = impl::SetDllDirectory(path);
        }
        else
        {
            std::wstring wPath = widen(path);
            DWORD att = GetFileAttributes(wPath.c_str());
            if (att == INVALID_FILE_ATTRIBUTES)
            {
                // Path does not exist, see if we can fix it up
                std::wstring fixedPath = FixupPathToRedirectionPathIfNeeded(LogLevel_DebugMaximum, g_PsfRunTimeName, AddSetDllDirectoryInstance, wPath);
                if (fixedPath != wPath)
                {
                    LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tSetDllDirectoryFixup: Adjusted path to", fixedPath.c_str());
                    Bret = impl::SetDllDirectory(fixedPath.c_str());
                }
                else
                {
                    LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tSetDllDirectoryFixup: No adjustment made for path", wPath.c_str());
                    Bret = impl::SetDllDirectory(path);
                }
            }
            else
            {
                LogString(LogLevel_DebugBasic, g_PsfRunTimeName, AddSetDllDirectoryInstance, L"\tSetDllDirectoryFixup: (Informational) Path exists, no adjustment needed for path", wPath.c_str());
                Bret = impl::SetDllDirectory(path);
            }
        }
    }
    else
    {
        // Unguarded
        Bret = impl::SetDllDirectory(path);
    }

    if (Bret)
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDllDirectoryFixup: Succeeded", g_PsfRunTimeName, AddSetDllDirectoryInstance);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDllDirectoryFixup: Failed with error=0x%x", g_PsfRunTimeName, AddSetDllDirectoryInstance, ::GetLastError());
    }
    return Bret;
}
catch (...)
{
    Log(LogLevel_DebugBasic, L"\t[%s%d]\tSetDllDirectoryFixup: ***Exception***", g_PsfRunTimeName, g_AddSetDllDirectoryInterceptInstance);
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::SetDllDirectory, SetDllDirectoryFixup);

#endif