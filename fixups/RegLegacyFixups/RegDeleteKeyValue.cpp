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

LSTATUS __stdcall RegDeleteKeyValueGenericFixup(
    _In_ HKEY key,
    _In_ const wchar_t* subKey,
    _In_ const wchar_t* subValueName)
{
    LSTATUS result;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {

        DWORD RegLocalInstance = ++g_RegInterceptInstance;
        Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteKeyValue: Key=0x%x subKey=%s subValueName=%s\n", g_RegModuleName, RegLocalInstance, key, subKey, subValueName);

        result = impl::KernelBaseRegDeleteKeyValueW(key, subKey, subValueName);
        auto functionResult = from_win32(result);

        if (functionResult != from_win32(0))
        {
            if (auto lock = acquire_output_lock(function_type::registry, functionResult))
            {
                try
                {
                    std::wstring keypath = ReplaceAppRegistrySyntaxW(InterpretKeyPathW(key) + L"\\" + InterpretStringW(subKey) + L"\\" + InterpretStringW(subValueName));
                    if (keypath.find(L"InterpretKeyPath failure") != std::string::npos)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteKeyValue: Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());
                        result = 0;
                    }
                    else
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteKeyValue: Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());
                        if (RegFixupFakeDelete(LogLevel_DebugIntermediate, keypath, RegLocalInstance) == true)
                        {
                            LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance);
                            Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteKeyValue:Fake Success\n", g_RegModuleName, RegLocalInstance);
                            result = 0;
                        }
                    }
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegDeleteKeyValue logging failure.\n", g_RegModuleName, RegLocalInstance);
                }
            }
            Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteKeyValue:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteKeyValue:Real returns %d\n", g_RegModuleName, RegLocalInstance, result);
        }
    }
    else
    {
        result = impl::KernelBaseRegDeleteKeyValueW(key, subKey, subValueName);
    }
    return result;
} // RegDeleteKeyValueGenericFixup()

LSTATUS __stdcall RegDeleteKeyValueAFixup(
    _In_ HKEY key,
    _In_ const char* subKey,
    _In_ const char* subValueName)
{

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        return RegDeleteKeyValueGenericFixup(key, widen(subKey).c_str(), widen(subValueName).c_str());
    }
    else

    {
        LSTATUS retVal = impl::KernelBaseRegDeleteKeyValueA(key, subKey, subValueName);
        return retVal;
    }
}
DECLARE_FIXUP(impl::KernelBaseRegDeleteKeyValueA, RegDeleteKeyValueAFixup);


LSTATUS __stdcall RegDeleteKeyValueWFixup(
    _In_ HKEY key,
    _In_ const wchar_t* subKey,
    _In_ const wchar_t* subValueName)
{

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        LSTATUS retVal = RegDeleteKeyValueGenericFixup(key, subKey, subValueName);
        return retVal;
    }
    else
    {
         return impl::KernelBaseRegDeleteKeyValueW(key, subKey, subValueName);
    }
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
                Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:\n", g_RegModuleName, RegLocalInstance);
                std::string keypath = ReplaceAppRegistrySyntaxA(InterpretKeyPath(key) + "\\" + InterpretStringA(subValueName));
                Log(LogLevel_DebugIntermediate, L"[$s%d] RegDeleteValue: Path=%s", g_RegModuleName, RegLocalInstance, keypath.c_str());
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