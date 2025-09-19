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
LSTATUS __stdcall RegDeleteValueGeneric(
    _In_ HKEY key,
    _In_ const CharT* subValueName)
{

    DWORD RegLocalInstance = ++g_RegInterceptInstance;

    LSTATUS result;
    if constexpr (psf::is_ansi<CharT>)
    {
        result = impl::KernelBaseRegDeleteValueA(key, subValueName);
    }
    else
    {
        result = impl::KernelBaseRegDeleteValueW(key, subValueName);
    }
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue: key=0x%x\n", g_RegModuleName, RegLocalInstance,key);
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subValueName));
                if (keypath.find("InterpretKeyPath failure") != std::string::npos)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteValue (A): Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());
                    result = 0;
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteValue: Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());
                    if (RegFixupFakeDelete(LogLevel_DebugIntermediate, keypath, RegLocalInstance) == true)
                    {
                        LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance);
                        Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:Fake Success\n", g_RegModuleName, RegLocalInstance);
                        result = 0;
                    }
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegDeleteValue logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
        Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
    }
    else
    {
#if _DEBUG
        if constexpr (psf::is_ansi<CharT>)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteValue: Key=0x%x Name=%s", g_RegModuleName, RegLocalInstance, key, widen(subValueName).c_str());
        }
        else
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteValue: Key=0x%x Name=%s", g_RegModuleName, RegLocalInstance, key, subValueName);
        }
        Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:Real returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    }
    return result;
}

LSTATUS __stdcall RegDeleteValueAFixup(
    _In_ HKEY key,
    _In_ const char* subValueName)
{
    return RegDeleteValueGeneric(key, subValueName);
}
DECLARE_FIXUP(impl::KernelBaseRegDeleteValueA, RegDeleteValueAFixup);


LSTATUS __stdcall RegDeleteValueWFixup(
    _In_ HKEY key,
    _In_ const wchar_t* subValueName)
{
    return RegDeleteValueGeneric(key, subValueName);
}
DECLARE_FIXUP(impl::KernelBaseRegDeleteValueW, RegDeleteValueWFixup);




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
                Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:\n", g_RegModuleName, RegLocalInstance);
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subValueName));
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteValue: Path=%s", g_RegModuleName, RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(LogLevel_DebugIntermediate, keypath, RegLocalInstance) == true)
                {
                    LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName,RegLocalInstance);
                    Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:Fake Success\n", g_RegModuleName, RegLocalInstance);
                    result = 0;
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegDeleteValue logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteValueImpl, RegDeleteValueFixup);

#endif