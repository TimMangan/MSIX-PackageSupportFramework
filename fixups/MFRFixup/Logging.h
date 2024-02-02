#pragma once


#include <intrin.h>  // need for _ReturnAddress()


#include <psf_utils.h>
#include <psf_logging.h>

#define LogCallingModule() \
    { \
        if (!g_psf_NoLogging) \
        { \
            HMODULE moduleHandle; \
            if (::GetModuleHandleExW( \
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, \
                reinterpret_cast<const wchar_t*>(_ReturnAddress()), \
                &moduleHandle)) \
            { \
                Log(L"\tCalling Module=%ls\n", psf::get_module_path(moduleHandle).c_str()); \
            } \
        } \
    }