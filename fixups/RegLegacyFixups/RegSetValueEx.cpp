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


//#ifdef _M_IX86
//#pragma comment(linker, "/EXPORT:RegSetValueExFixupAnsi_Fixup=_RegSetValueExFixupA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#pragma comment(linker, "/EXPORT:RegSetValueExFixupWide_Fixup=_RegSetValueExFixupW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#else
//#pragma comment(linker, "/EXPORT:RegSetValueExFixupAnsi_Fixup=RegSetValueExFixupA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#pragma comment(linker, "/EXPORT:RegSetValueExFixupWide_Fixup=RegSetValueExFixupW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#endif


auto RegSetValueExImpl = psf::detoured_string_function(&::RegSetValueExA, &::RegSetValueExW);
template <typename CharT>
LSTATUS __stdcall RegSetValueExFixup(
    _In_       HKEY key,
    _In_opt_   const CharT* lpValueName,
    _Reserved_ DWORD Reserved,
    _In_       DWORD dwType,
    _In_       const BYTE* lpData,
    _In_       DWORD cbData)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;


        std::string keyOnlyPath = InterpretKeyPath(key);
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

        if constexpr (psf::is_ansi<CharT>)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueExA: key=0x%x Key=%s valuename=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyOnlyPath.c_str(), lpValueName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueExW: key=0x%x Key=%s valuename=%s", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str(), lpValueName);
        }
        Log(LogLevel_DebugIntermediate, L"[%s%d] RegSetValueExA: dwType=0x%x cbData=0x%x", g_RegModuleName, RegLocalInstance, dwType, cbData);

        // This intercept exists simply to understand and log the call being made.
        // While it seems we might need to add support for HKLM2HKCU here, in reality
        // the key change ocurrs when opening the key.  But if we find entries here
        // that don't arrive as "=Registry\Users\S-1-5-..." then that code should be added
        // both here and in RegSetValue.

        result = RegSetValueExImpl(key, lpValueName, Reserved, dwType, lpData, cbData);
        Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueExW: result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
    }
    else
    {
        result = RegSetValueExImpl(key, lpValueName, Reserved, dwType, lpData, cbData);
        Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueExW: (unguarded) result=%s", g_RegModuleName, 0, LStatusToWstring(result).c_str());
    }
    return result;
}
DECLARE_STRING_FIXUP(RegSetValueExImpl, RegSetValueExFixup);
