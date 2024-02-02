//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"

#ifdef INTERCEPT_KERNELBASE

template <typename CharT>
LSTATUS __stdcall RegDeleteKeyValueGeneric(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _In_ const CharT* subValueName)
{

    DWORD RegLocalInstance = ++g_RegIntceptInstance;

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
                Log(L"[%d] RegDeleteKeyValue:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey) + "\\" + InterpretStringA(subValueName));
#ifdef _DEBUG
                Log(L"[%d] RegDeleteKeyValue: Path=%s", RegLocalInstance, keypath.c_str());
#endif
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
                {
#ifdef _DEBUG
                    Log(L"[%d] RegDeleteKeyValue:Fake Success\n", RegLocalInstance);
                    LogCallingModule();
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%d] RegDeleteKeyValue logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%d] RegDeleteKeyValue:Fake returns %d\n", RegLocalInstance, result);
#endif
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

    DWORD RegLocalInstance = ++g_RegIntceptInstance;


    auto result = RegDeleteValueImpl(key, subValueName);
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%d] RegDeleteValue:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subValueName));
#ifdef _DEBUG
                Log(L"[%d] RegDeleteValue: Path=%s", RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#endif
                {
#ifdef _DEBUG
                    Log(L"[%d] RegDeleteValue:Fake Success\n", RegLocalInstance);
                    LogCallingModule();
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%d] RegDeleteValue logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%d] RegDeleteValue:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteValueImpl, RegDeleteValueFixup);

#endif