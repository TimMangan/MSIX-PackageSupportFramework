//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// // The PsfRuntime intercepts all Mutex and Semaphore calls so that paths may be fixed up. 

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
#include <windows.h>

using namespace std::literals;


#include <reentrancy_guard.h>
#include <psf_framework.h>

extern const wchar_t* g_PsfRunTimeName;
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    auto CreateMailslotImpl = psf::detoured_string_function(&::CreateMailslotA, &::CreateMailslotW);
}

DWORD g_MailslotInterceptInstance = 25000;


template <typename CharT>
HANDLE WINAPI CreateMailslotFixup(
    _In_        const CharT*          lpName,
    _In_        DWORD                 nMaxMessageSize,
    _In_        DWORD                 lReadTimeout,
    _In_opt_    LPSECURITY_ATTRIBUTES lpSecurityAttributes)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD MailslotInstance = ++g_MailslotInterceptInstance;

        Log(LogLevel_DebugBasic, L" [%s%d] CreateMailslotFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, MailslotInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] CreateMailslotFixup:     nMaxMessageSize=0x%x lReadTimeout=0x%x", g_PsfRunTimeName, MailslotInstance, nMaxMessageSize, lReadTimeout);
        if (lpSecurityAttributes != NULL)
        {
            Log(LogLevel_DebugBasic, L" [%s%d] CreateMailslotFixup:     Attributes bInheritHandle=0x%x", g_PsfRunTimeName, MailslotInstance, lpSecurityAttributes->bInheritHandle);
        }
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, MailslotInstance, L"CreateMailslotFixup:     Name", lpName);
        }

        HANDLE hRet = impl::CreateMailslotImpl(lpName, nMaxMessageSize, lReadTimeout, lpSecurityAttributes);
        Log(LogLevel_DebugBasic, L" [%s%d] CreateMailslotFixup:     return Handle=0x%x LastError=0x%x", g_PsfRunTimeName, MailslotInstance, hRet, GetLastError());

        return hRet;
    }
    return impl::CreateMailslotImpl(lpName, nMaxMessageSize, lReadTimeout, lpSecurityAttributes);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::CreateMailslotImpl, CreateMailslotFixup);
