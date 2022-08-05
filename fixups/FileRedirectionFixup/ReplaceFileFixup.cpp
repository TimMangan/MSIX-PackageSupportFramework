//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall ReplaceFileFixup(
    _In_ const CharT* replacedFileName,
    _In_ const CharT* replacementFileName,
    _In_opt_ const CharT* backupFileName,
    _In_ DWORD replaceFlags,
    _Reserved_ LPVOID exclude,
    _Reserved_ LPVOID reserved) noexcept
{
    DWORD ReplaceFileInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(ReplaceFileInstance,L"ReplaceFileFixup From", replacedFileName);
            LogString(ReplaceFileInstance,L"ReplaceFileFixup To",   replacementFileName);
            if (backupFileName != nullptr)
            {
                LogString(ReplaceFileInstance, L"ReplaceFileFixup with backup", backupFileName);
            }
            Log(L"[%d] ReplaceFileFixup replaceFlags 0x%x", ReplaceFileInstance, replaceFlags);
#endif

            // NOTE: ReplaceFile will delete the "replacement file" (the file we're copying from), so therefore we need
            //       delete access to it, thus we copy-on-read it here. I.e. we're copying the file only for it to
            //       immediately get deleted. We could improve this in the future if we wanted, but that would
            //       effectively require that we re-write ReplaceFile, which we opt not to do right now. Also note that
            //       this implies that we have the same file deletion limitation that we have for DeleteFile, etc.
            path_redirect_info  priTarget = ShouldRedirectV2(replacedFileName, redirect_flags::check_file_presence | redirect_flags::copy_on_read | redirect_flags::ok_if_parent_in_pkg, ReplaceFileInstance);
            //////path_redirect_info  priSource = ShouldRedirectV2(replacementFileName, redirect_flags::ensure_directory_structure, ReplaceFileInstance);
            path_redirect_info  priSource = ShouldRedirectV2(replacementFileName, redirect_flags::check_file_presence | redirect_flags::copy_on_read | redirect_flags::ensure_directory_structure | redirect_flags::ok_if_parent_in_pkg, ReplaceFileInstance);
            path_redirect_info  priBackup = ShouldRedirectV2(backupFileName, redirect_flags::ensure_directory_structure | redirect_flags::ok_if_parent_in_pkg, ReplaceFileInstance);
#if MOREDEBUG
            if (priTarget.should_redirect)
                LogString(ReplaceFileInstance, L"ReplaceFileFixup RedirTarget ", priTarget.redirect_path.c_str());
            if (priSource.should_redirect)
                LogString(ReplaceFileInstance, L"ReplaceFileFixup RedirSource ", priSource.redirect_path.c_str());
            if (priBackup.should_redirect)
                LogString(ReplaceFileInstance, L"ReplaceFileFixup RedirBackup ", priBackup.redirect_path.c_str());
            Log(L"[%d] Exists: %d %d %d", ReplaceFileInstance, priTarget.doesRedirectedExist, priSource.doesRedirectedExist, priBackup.doesRedirectedExist);
#endif
            if ( priTarget.should_redirect || priSource.should_redirect || priBackup.should_redirect)
            {
                std::wstring rldReplacedFileName = TurnPathIntoRootLocalDevice(priTarget.should_redirect ? priTarget.redirect_path.c_str() : widen_argument(replacedFileName).c_str());
                std::wstring rldReplacementFileName = TurnPathIntoRootLocalDevice(priSource.should_redirect ? priSource.redirect_path.c_str() : widen_argument(replacementFileName).c_str());
                if (backupFileName != nullptr)
                {
                    std::wstring rldBackupFileName = TurnPathIntoRootLocalDevice(priBackup.should_redirect ? priBackup.redirect_path.c_str() : widen_argument(backupFileName).c_str());
                    BOOL b = impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), rldBackupFileName.c_str(), replaceFlags, exclude, reserved);
                    if (b == 0)
                    {
                        Log(L"[%d] ReplaceFileFixup GetLastError 0x%x", ReplaceFileInstance, GetLastError());
                    }
                    return b;
                }
                else
                {
                    BOOL b = impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), nullptr, replaceFlags, exclude, reserved);
                    if (b == 0)
                    {
                        Log(L"[%d] ReplaceFileFixup GetLastError 0x%x", ReplaceFileInstance, GetLastError());
                    }
                    return b;
                }
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(ReplaceFileInstance, L"ReplaceFile")
#else
    catch (...)
    {
        Log(L"[%d] ReplaceFile Exception=0x%x", ReplaceFileInstance, GetLastError());
    }
#endif


    if constexpr (psf::is_ansi<CharT>)
    {
        std::string rldReplacedFileName = TurnPathIntoRootLocalDevice(replacedFileName);
        std::string rldReplacementFileName = TurnPathIntoRootLocalDevice(replacementFileName);
        if (backupFileName != nullptr)
        {
            std::string rldBackupFileName = TurnPathIntoRootLocalDevice(backupFileName);
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), rldBackupFileName.c_str(), replaceFlags, exclude, reserved);
        }
        else
        {
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), nullptr, replaceFlags, exclude, reserved);
        }
    }
    else
    {
        std::wstring rldReplacedFileName = TurnPathIntoRootLocalDevice(replacedFileName);
        std::wstring rldReplacementFileName = TurnPathIntoRootLocalDevice(replacementFileName);
        if (backupFileName != nullptr)
        {
            std::wstring rldBackupFileName = TurnPathIntoRootLocalDevice(backupFileName);
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), rldBackupFileName.c_str(), replaceFlags, exclude, reserved);
        }
        else
        {
            return impl::ReplaceFile(rldReplacedFileName.c_str(), rldReplacementFileName.c_str(), nullptr, replaceFlags, exclude, reserved);
        }
    }
    ///return impl::ReplaceFile(replacedFileName, replacementFileName, backupFileName, replaceFlags, exclude, reserved);
}
DECLARE_STRING_FIXUP(impl::ReplaceFile, ReplaceFileFixup);
