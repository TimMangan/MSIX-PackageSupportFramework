//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if _DEBUG
//#define _ManualDebug 1
//#define MOREDEBUG 1
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
LSTATUS __stdcall RegDeleteKeyValueGeneric(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _In_ const CharT* subValueName)
{

    DWORD RegLocalInstance = ++g_RegInterceptInstance;

    LSTATUS result;
    if constexpr (psf::is_ansi<CharT>)
    {
        result = impl::KernelBaseRegDeleteKeyValueA(key, subKey, subValueName);
    }
    else
    {
        result = impl::KernelBaseRegDeleteKeyValueW(key, subKey, subValueName);
    }
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%s%d] RegDeleteKeyValue: Key=0x%x\n", g_RegModuleName, RegLocalInstance, key);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey) + "\\" + InterpretStringA(subValueName));
                if (keypath.find("InterpretKeyPath failure") != std::string::npos)
                {
#if _DEBUG
                    Log(L"[%s%d] RegDeleteKeyValue (A): Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());                   
#endif
                    result = 0;
                }
                else
                {
#if _DEBUG
                    Log(L"[%s%d] RegDeleteKeyValue (A): Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());
#endif
                    if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
                    {
#if _DEBUG
                        LogCallingModuleInstanceCommon(g_RegModuleName, RegLocalInstance);
                        Log(L"[%s%d] RegDeleteKeyValue:Fake Success\n", g_RegModuleName, RegLocalInstance);
#endif
                        result = 0;
                    }
                }
            }
            catch (...)
            {
                Log(L"[%s%d] RegDeleteKeyValue logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
#if _DEBUG
        Log(L"[%s%d] RegDeleteKeyValue:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    }
    else
    {
#if _DEBUG
        if constexpr (psf::is_ansi<CharT>)
        {
            Log(L"[%s%d] RegDeleteKeyValue (A): Key=0x%x, subkey=%s Name=%s", g_RegModuleName, RegLocalInstance, key, widen(subKey).c_str(), widen(subValueName).c_str());
        }
        else
        {
            Log(L"[%s%d] RegDeleteKeyValue (W): Key=0x%x, subkey=%s Name=%s", g_RegModuleName, RegLocalInstance, key, subKey, subValueName);
        }
        Log(L"[%s%d] RegDeleteKeyValue:Real returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    }
    return result;
}

LSTATUS __stdcall RegDeleteKeyValueAFixup(
    _In_ HKEY key,
    _In_ const char* subKey,
    _In_ const char* subValueName)
{
    return RegDeleteKeyValueGeneric(key, subKey, subValueName);
}
DECLARE_FIXUP(impl::KernelBaseRegDeleteKeyValueA, RegDeleteKeyValueAFixup);


LSTATUS __stdcall RegDeleteKeyValueWFixup(
    _In_ HKEY key,
    _In_ const wchar_t* subKey,
    _In_ const wchar_t* subValueName)
{
    return RegDeleteKeyValueGeneric(key, subKey, subValueName);
}
DECLARE_FIXUP(impl::KernelBaseRegDeleteKeyValueW, RegDeleteKeyValueWFixup);



#else
auto RegDeleteValueImpl = psf::detoured_string_function(&::RegDeleteValueA, &::RegDeleteValueW);
template <typename CharT>
LSTATUS __stdcall RegDeleteValueFixup(
    _In_ HKEY key,
    _In_ const CharT* subValueName)
{

    DWORD RegLocalInstance = ++g_RegInterceptInstance;


    auto result = RegDeleteValueImpl(key, subValueName);
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%s%d] RegDeleteValue:\n", g_RegModuleName, RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subValueName));
#if _DEBUG
                Log(L"[$s%d] RegDeleteValue: Path=%s", g_RegModuleName, RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#endif
                {
#if _DEBUG
                    LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
                    Log(L"[%s%d] RegDeleteValue:Fake Success\n", g_RegModuleName, RegLocalInstance);
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%s%d] RegDeleteValue logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%s%d] RegDeleteValue:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteValueImpl, RegDeleteValueFixup);

#endif