//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if _DEBUG
//#define _ManualDebug 1
#include <thread>
#include <windows.h>
#endif

#include "pch.h"

///#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>
#include <psf_logging.h>

bool trace_function_entry = false;
bool m_inhibitOutput = false;
bool m_shouldLog = true;

void InitializeFixups();
void InitializeConfiguration();

extern "C" {

#if _ManualDebug
    void manual_LogWFD(const wchar_t* msg)
    {
        ::OutputDebugStringW(msg);
    }
    void manual_wait_for_debugger()
    {
        manual_LogWFD(L"Start WFD");
        // If a debugger is already attached, ignore as they have likely already set all breakpoints, etc. they need
        if (!::IsDebuggerPresent())
        {
            manual_LogWFD(L"WFD: not yet.");
            while (!::IsDebuggerPresent())
            {
                manual_LogWFD(L"WFD: still not yet.");
                ::Sleep(1000);
            }
            manual_LogWFD(L"WFD: Yes.");
            // NOTE: When a debugger attaches (invasively), it will inject a DebugBreak in a new thread. Unfortunately,
            //       that does not synchronize with, and may occur _after_ IsDebuggerPresent returns true, allowing
            //       execution to continue for a short period of time. In order to get around this, we'll insert our own
            //       DebugBreak call here. We also add a short(-ish) sleep so that this is likely to be the second break
            //       seen, so that the injected DebugBreak doesn't preempt us in the middle of debugging. This is of
            //       course best effort
            ::Sleep(5000);
            std::this_thread::yield();
            ::DebugBreak();
        }
        manual_LogWFD(L"WFD: Done.");
    }
#endif

    int __stdcall PSFInitialize() noexcept try
    {
#if _DEBUG
        //int count = psf::attach_count_all();
        psf::attach_count_all_debug();
        //Log(L"[0] RegLegacyFixup debug attaches %d fixups.", 0, count);
#if _ManualDebug
        manual_wait_for_debugger();
#endif
#else
        psf::attach_all();
#endif
        return ERROR_SUCCESS;
    }
    catch (...)
    {
        return win32_from_caught_exception();
    }


    int __stdcall PSFUninitialize() noexcept try
    {
        psf::detach_all();
        return ERROR_SUCCESS;
    }
    catch (...)
    {
        return win32_from_caught_exception();
    }

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:PSFInitialize=_PSFInitialize@0")
#pragma comment(linker, "/EXPORT:PSFUninitialize=_PSFUninitialize@0")
#else
#pragma comment(linker, "/EXPORT:PSFInitialize=PSFInitialize")
#pragma comment(linker, "/EXPORT:PSFUninitialize=PSFUninitialize")
#endif

    BOOL APIENTRY DllMain(HMODULE, // hModule,
        DWORD  ul_reason_for_call,
        LPVOID // lpReserved
    )  noexcept try
    {

        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
#if _DEBUG
            Log(L"Attaching RegLegacyFixups\n");
#endif
            InitializeFixups();
            InitializeConfiguration();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        }
        return TRUE;
    }
    catch (...)
    {
        Log(L"RegLegacyFixups attach ERROR\n");
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}
