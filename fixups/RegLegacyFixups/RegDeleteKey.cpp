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


auto RegDeleteKeyImpl = psf::detoured_string_function(&::RegDeleteKeyA, &::RegDeleteKeyW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey)
{

    DWORD RegLocalInstance = ++g_RegInterceptInstance;


    auto result = RegDeleteKeyImpl(key, subKey);
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%s%d] RegDeleteKey:\n", g_RegModuleName, RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));

#if _DEBUG
                Log(L"[%s%d] RegDeleteKey: Path=%s", g_RegModuleName, RegLocalInstance, keypath.c_str());
#endif
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
                {
#if _DEBUG
                    LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
                    Log(L"[%s%d] RegDeleteKey:Fake Success\n", g_RegModuleName,RegLocalInstance);
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%s%d] RegDeleteKey logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%s%d] RegDeleteKey:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyImpl, RegDeleteKeyFixup);

