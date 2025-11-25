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


auto RegDeleteKeyImpl = psf::detoured_string_function(&::RegDeleteKeyA, &::RegDeleteKeyW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey)
{
    LSTATUS result;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        if constexpr (psf::is_ansi<CharT>)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteKey (A): key=0x%x Name=%s", g_RegModuleName, RegLocalInstance, key, widen(subKey).c_str());
        }
        else
        {
            Log(LogLevel_DebugBasic, "[%s%d] RegDeleteKey (W): key=0x%x Name=%s", g_RegModuleName, RegLocalInstance, key, subKey);
        }
        result = RegDeleteKeyImpl(key, subKey);
        auto functionResult = from_win32(result);

        if (functionResult != from_win32(0))
        {
            if (auto lock = acquire_output_lock(function_type::registry, functionResult))
            {
                try
                {
                    std::string keypath = ReplaceAppRegistrySyntaxA(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));
                    if (keypath.find("InterpretKeyPath failure") != std::string::npos)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteKey (A): Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());
                        result = 0;
                    }
                    else
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteKey (A): Path=%s", g_RegModuleName, RegLocalInstance, widen(keypath).c_str());
                        if (RegFixupFakeDelete(LogLevel_DebugIntermediate, keypath, RegLocalInstance) == true)
                        {
                            LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance);
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegDeleteKey:Fake Success\n", g_RegModuleName, RegLocalInstance);
                            result = 0;
                        }
                    }
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegDeleteKey logging failure.\n", g_RegModuleName, RegLocalInstance);
                }
            }
            Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteValue:Fake returns %d\n", g_RegModuleName, RegLocalInstance, result);
        }
        else
        {
            
            Log(LogLevel_DebugBasic, L"[%s%d] RegDeleteKey:Real returns %d\n", g_RegModuleName, RegLocalInstance, result);
        }
    }
    else
    {
        // Reentrant call, just pass through
        result = RegDeleteKeyImpl(key, subKey);
    }
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyImpl, RegDeleteKeyFixup);

