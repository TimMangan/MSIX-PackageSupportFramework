//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if _DEBUG
//#define _ManualDebug 1
#define MOREDEBUG 1
#include <thread>
#include <windows.h>
#endif

#include <psf_framework.h>
#include <psf_logging.h>
#include "Logging.h"

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"

#if _DEBUG
#if DEBUG_NEW_FIXUPS 
#define DEBUG_NEW_FIXUPS_REGLEG 1
#endif
#endif

#if INTERCEPT_KERNELBASE


template <typename CharT>
LSTATUS __stdcall RegDeleteKeyExGeneric(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved)
{

    DWORD RegLocalInstance = ++g_RegInterceptInstance;

    LSTATUS result;
    if constexpr (psf::is_ansi<CharT>)
    {
        result = impl::KernelBaseRegDeleteKeyExA(key, subKey, viewDesired, Reserved);
    }
    else
    {
        result = impl::KernelBaseRegDeleteKeyExW(key, subKey, viewDesired, Reserved);
    }
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%s%d] RegDeleteKeyEx:\n", g_RegModuleName, RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));
#if _DEBUG
                Log(L"[%s%d] RegDeleteKeyEx: Path=%s", g_RegModuleName, RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#endif
                {
#if _DEBUG
                    LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
                    Log(L"[%s%d] RegDeleteKeyEx:Fake Success\n", g_RegModuleName, RegLocalInstance);
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%s%d] RegDeleteKeyEx logging failure.\n", g_RegModuleName,RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%s%d] RegDeleteKeyEx:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    return result;
}

LSTATUS __stdcall RegDeleteKeyExAFixup(
    _In_ HKEY key,
    _In_ const char* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved)
{
    return RegDeleteKeyExGeneric(key, subKey, viewDesired, Reserved);
}
DECLARE_FIXUP(impl::KernelBaseRegDeleteKeyExA, RegDeleteKeyExAFixup);


LSTATUS __stdcall RegDeleteKeyExWFixup(
    _In_ HKEY key,
    _In_ const wchar_t* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved)
{
    return RegDeleteKeyExGeneric(key, subKey, viewDesired, Reserved);
}
DECLARE_FIXUP(impl::KernelBaseRegDeleteKeyExW, RegDeleteKeyExWFixup);


#else
auto RegDeleteKeyExImpl = psf::detoured_string_function(&::RegDeleteKeyExA, &::RegDeleteKeyExW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved)
{

    DWORD RegLocalInstance = ++g_RegInterceptInstance;


    auto result = RegDeleteKeyExImpl(key, subKey, viewDesired, Reserved);
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%s%d] RegDeleteKeyEx:\n", g_RegModuleName, RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));
#if _DEBUG
                Log(L"[%s%d] RegDeleteKeyEx: Path=%s", g_RegModuleName, RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#endif
                {
#if _DEBUG
                    LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
                    Log(L"[%s%d] RegDeleteKeyEx:Fake Success\n", g_RegModuleName, RegLocalInstance);
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%s%d] RegDeleteKeyEx logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%s%d] RegDeleteKeyEx:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyExImpl, RegDeleteKeyExFixup);
#endif