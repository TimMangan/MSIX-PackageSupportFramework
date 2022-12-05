//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>
#include <psf_logging.h>


bool trace_function_entry = false;
bool m_inhibitOutput = false;
bool m_shouldLog = true;

void InitializeMFRFixup();
void InitializeConfiguration();


extern "C" {

    int __stdcall PSFInitialize() noexcept try
    {
#if _DEBUG
        int count = psf::attach_count_all();
        Log(L"[0] MFRFixup attaches %d fixups.", count);
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

    BOOL APIENTRY DllMain([[maybe_unused]] HMODULE hModule,
        DWORD  ul_reason_for_call,
        [[maybe_unused]] LPVOID lpReserved
    ) noexcept try
    {
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
            ::OutputDebugStringA("MFRFixup attached");
#endif
            InitializeMFRFixup();
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
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}

