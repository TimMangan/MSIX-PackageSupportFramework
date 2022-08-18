//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Collection of function pointers that will always point to (what we think are) the actual function implementations.
// That is, impl::CreateFile will (presumably) call kernelbase!CreateFileA/W, even though we detour that call. Useful to
// reduce the risk of us fixing ourselves, which can easily lead to infinite recursion. Note that this isn't perfect.
// For example, CreateFileFixup could call kernelbase!CopyFileW, which could in turn call (the fixed) CreateFile again
#pragma once

#include <reentrancy_guard.h>
#include <psf_framework.h>

extern DWORD g_InterceptInstance;

// A much bigger hammer to avoid reentrancy. Still, the impl::* functions are good to have around to prevent the
// unnecessary invocation of the fixup
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    inline auto CopyFile = psf::detoured_string_function(&::CopyFileA, &::CopyFileW);
    inline auto CopyFileEx = psf::detoured_string_function(&::CopyFileExA, &::CopyFileExW);
    inline auto CopyFile2 = &::CopyFile2;

    inline auto CreateDirectory = psf::detoured_string_function(&::CreateDirectoryA, &::CreateDirectoryW);
    inline auto CreateDirectoryEx = psf::detoured_string_function(&::CreateDirectoryExA, &::CreateDirectoryExW);

    inline auto CreateFile = psf::detoured_string_function(&::CreateFileA, &::CreateFileW);
    inline auto CreateFile2 = &::CreateFile2;

    inline auto CreateHardLink = psf::detoured_string_function(&::CreateHardLinkA, &::CreateHardLinkW);

    inline auto CreateSymbolicLink = psf::detoured_string_function(&::CreateSymbolicLinkA, &::CreateSymbolicLinkW);

    inline auto DeleteFile = psf::detoured_string_function(&::DeleteFileA, &::DeleteFileW);

    inline auto FindClose = &::FindClose;
    inline auto FindFirstFile = psf::detoured_string_function(&::FindFirstFileA, &::FindFirstFileW);
    inline auto FindFirstFileEx = psf::detoured_string_function(&::FindFirstFileExA, &::FindFirstFileExW);
    inline auto FindNextFile = psf::detoured_string_function(&::FindNextFileA, &::FindNextFileW);

    inline auto GetFileAttributes = psf::detoured_string_function(&::GetFileAttributesA, &::GetFileAttributesW);
    inline auto GetFileAttributesEx = psf::detoured_string_function(&::GetFileAttributesExA, &::GetFileAttributesExW);
    inline auto GetPrivateProfileInt = psf::detoured_string_function(&::GetPrivateProfileIntA, &::GetPrivateProfileIntW);
    inline auto GetPrivateProfileSection = psf::detoured_string_function(&::GetPrivateProfileSectionA, &::GetPrivateProfileSectionW);
    inline auto GetPrivateProfileSectionNames = psf::detoured_string_function(&::GetPrivateProfileSectionNamesA, &::GetPrivateProfileSectionNamesW);
    inline auto GetPrivateProfileString = psf::detoured_string_function(&::GetPrivateProfileStringA, &::GetPrivateProfileStringW);
    inline auto GetPrivateProfileStruct = psf::detoured_string_function(&::GetPrivateProfileStructA, &::GetPrivateProfileStructW);

    inline auto MoveFile = psf::detoured_string_function(&::MoveFileA, &::MoveFileW);
    inline auto MoveFileEx = psf::detoured_string_function(&::MoveFileExA, &::MoveFileExW);

    inline auto RemoveDirectory = psf::detoured_string_function(&::RemoveDirectoryA, &::RemoveDirectoryW);

    inline auto ReplaceFile = psf::detoured_string_function(&::ReplaceFileA, &::ReplaceFileW);

    inline auto SetFileAttributes = psf::detoured_string_function(&::SetFileAttributesA, &::SetFileAttributesW);

    inline auto GetCurrentDirectory = psf::detoured_string_function(&::GetCurrentDirectoryA, &::GetCurrentDirectoryW);
    inline auto SetCurrentDirectory = psf::detoured_string_function(&::SetCurrentDirectoryA, &::SetCurrentDirectoryW);

    inline auto WritePrivateProfileSection = psf::detoured_string_function(&::WritePrivateProfileSectionA, &::WritePrivateProfileSectionW);
    inline auto WritePrivateProfileString = psf::detoured_string_function(&::WritePrivateProfileStringA, &::WritePrivateProfileStringW);
    inline auto WritePrivateProfileStruct = psf::detoured_string_function(&::WritePrivateProfileStructA, &::WritePrivateProfileStructW);

    inline auto SearchPath = psf::detoured_string_function(&::SearchPathA, &::SearchPathW);

#if FIXUP_UCRT
    // ucrtbased.dll functions
    inline auto fopen = &::fopen;
    inline auto _wfopen = &::_wfopen;
    inline auto fopen_s = &::fopen_s;
    inline auto _wfopen_s = &::_wfopen_s;

#include <io.h>
#include <stdio.h>
    //inline auto _findfirst = &::_findfirst;    // is just a macro
    ////inline auto _findfirst32 = &::_findfirst32;         // can't find???
    ////inline auto _findfirst32i64 = &::_findfirst32i64;   // can't find???
    ////inline auto _findfirst64 = &::_findfirst64;         // can't find???
    ////inline auto _findfirst64i32 = &::_findfirst64i32;   // can't find???
    //inline auto _wfindfirst = &::_wfindfirst;    // is just a macro
    inline auto _wfindfirst32 = &::_wfindfirst32;
    inline auto _wfindfirst32i64 = &::_wfindfirst32i64;
    inline auto _wfindfirst64 = &::_wfindfirst64;
    inline auto _wfindfirst64i32 = &::_wfindfirst64i32;
    ////inline auto _mkdir = &::_mkdir;                      // can't find???
    inline auto _wmkdir = &::_wmkdir;
    ////inline auto _open = &::_open;                        // can't find???
    inline auto _wopen = &::_wopen;
    ////inline auto _rmdir = &::_rmdir;                      // can't find???
    inline auto _wrmdir = &::_wrmdir;
    ////inline auto _sopen = &::_sopen;                      // can't find???
    inline auto _wsopen = &::_wsopen;
    ////inline auto _sopen_s = &::_sopen_s;                  // can't find???
    inline auto _wsopen_s = &::_wsopen_s;
    //////////inline auto _unlink = &::_unlink;             // troublemaker
    inline auto _wunlink = &::_wunlink;
#endif

    inline auto ReadDirectoryChangesW = &::ReadDirectoryChangesW;
    inline auto ReadDirectoryChangesExW = &::ReadDirectoryChangesExW;

}
