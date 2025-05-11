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

auto RegDeleteKeyTransactedImpl = psf::detoured_string_function(&::RegDeleteKeyTransactedA, &::RegDeleteKeyTransactedW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyTransactedFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved,
    HANDLE hTransaction,
    PVOID  pExtendedParameter)
{

    DWORD RegLocalInstance = ++g_RegInterceptInstance;


    auto result = RegDeleteKeyTransactedImpl(key, subKey, viewDesired, Reserved, hTransaction, pExtendedParameter);
    auto functionResult = from_win32(result);
 
    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%s%d] RegDeleteKeyTransacted:\n", g_RegModuleName, RegLocalInstance);
#endif
                std::string keyOnlyPath = InterpretStringA(subKey);
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + keyOnlyPath);
#if _DEBUG
                Log(L"[%s%d] RegDeleteKeyTransacted: Path=%s", g_RegModuleName, RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#endif
                {
#if _DEBUG
                    LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
                    Log(L"[%s%d] RegDeleteKeyTransacted:Fake Success\n", g_RegModuleName, RegLocalInstance);
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%s%d] RegDeleteKeyTransacted logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%s%d] RegDeleteKeyTransacted:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyTransactedImpl, RegDeleteKeyTransactedFixup);


