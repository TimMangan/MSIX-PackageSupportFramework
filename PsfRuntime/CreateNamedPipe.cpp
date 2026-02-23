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

extern const wchar_t* g_PsfRunTimeName;
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    inline auto CreateNamedPipeA = &::CreateNamedPipeA;
}

DWORD g_CreateNamedPipeInterceptInstance = 21000;


HANDLE WINAPI CreateNamedPipeAFixup(
    _In_     LPCSTR                lpName,
    _In_     DWORD                 dwOpenMode,
    _In_     DWORD                 dwPipeMode,
    _In_     DWORD                 nMaxInstances,
    _In_     DWORD                 nOutBufferSize,
    _In_     DWORD                 nInBufferSize,
    _In_     DWORD                 nDefaultTimeOut,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD CreateNamedPipeInstance = ++g_CreateNamedPipeInterceptInstance;
        Log(LogLevel_DebugBasic, L" [%s%d] CreateNamedPipeFixupW: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, CreateNamedPipeInstance);
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, CreateNamedPipeInstance, L"CreateNamedPipeFixupW: Input lpName", lpName);
        }


        HANDLE HPipe = impl::CreateNamedPipeA(lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
        Log(LogLevel_DebugBasic, L" [%s%d] CreateNamedPipeFixupW: (Informational) return handle 0x%x LastError=0x%x", g_PsfRunTimeName, CreateNamedPipeInstance,HPipe, GetLastError());

        return HPipe;
    }
    return impl::CreateNamedPipeA(lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_FIXUP(impl::CreateNamedPipeA, CreateNamedPipeAFixup);
