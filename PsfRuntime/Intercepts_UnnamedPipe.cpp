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
// // The PsfRuntime intercepts all NamedPipe calls so that paths may be fixed up. 

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
    inline auto CreatePipe = &::CreatePipe;
}

DWORD g_CreatePipeInterceptInstance = 23000;


BOOL WINAPI CreatePipeFixup(
    _Out_    PHANDLE                hReadPipe,
    _Out_    PHANDLE                hWritePipe,
    _In_opt_ LPSECURITY_ATTRIBUTES  lpPipeAttributes,
    _In_     DWORD                  nSize)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD CreatePipeInstance = ++g_CreatePipeInterceptInstance;
        Log(LogLevel_DebugBasic, L" [%s%d] CreatePipeFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, CreatePipeInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] CreatePipeFixup:     Size=0x%x", g_PsfRunTimeName, CreatePipeInstance, nSize);
        if (lpPipeAttributes != NULL)
        {
            Log(LogLevel_DebugBasic, L" [%s%d] CreatePipeFixup:     Attributes bInheritHandle=0x%x", g_PsfRunTimeName, CreatePipeInstance, lpPipeAttributes->bInheritHandle);
        }


        BOOL bRet = impl::CreatePipe(hReadPipe, hWritePipe, lpPipeAttributes, nSize);
        if (hReadPipe != NULL && hWritePipe != NULL)
        {
            Log(LogLevel_DebugBasic, L" [%s%d] CreatePipeFixup:     return value=0x%x handles read=0x%x write=0x%x LastError=0x%x", g_PsfRunTimeName, CreatePipeInstance, bRet, *hReadPipe, *hWritePipe, GetLastError());
        }
        else
        {
            if (hReadPipe != NULL && hWritePipe == NULL)
            {
                Log(LogLevel_DebugBasic, L" [%s%d] CreatePipeFixup:     return value=0x%x LastError=0x%x", g_PsfRunTimeName, CreatePipeInstance, bRet, GetLastError());
            }
            else if (hReadPipe == NULL)
            {
                Log(LogLevel_DebugBasic, L" [%s%d] CreatePipeFixup:     return value=0x%x handle write=0x%x LastError=0x%x", g_PsfRunTimeName, CreatePipeInstance, bRet, *hWritePipe, GetLastError());
            }
            else
            {
                Log(LogLevel_DebugBasic, L" [%s%d] CreatePipeFixup:     return value=0x%x handle read=0x%x LastError=0x%x", g_PsfRunTimeName, CreatePipeInstance, bRet, *hReadPipe, GetLastError());
            }
        }

        return bRet;
    }
    return impl::CreatePipe(hReadPipe, hWritePipe, lpPipeAttributes, nSize);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_FIXUP(impl::CreatePipe, CreatePipeFixup);
