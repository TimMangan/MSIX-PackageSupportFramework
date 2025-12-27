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


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:RegSetValueFixupAnsi_Fixup=_RegSetValueImplA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegSetValueFixupWide_Fixup=_RegSetValueImplW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegSetValueFixupAnsi_Fixup=RegSetValueImplA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegSetValueFixupWide_Fixup=RegSetValueImplW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


auto RegSetValueImpl = psf::detoured_string_function(&::RegSetValueA, &::RegSetValueW);
template <typename CharT>
LSTATUS __stdcall RegSetValueFixup(
    _In_       HKEY key,
    _In_opt_   const CharT* lpValueName,
    _In_       DWORD lpDwType,
    _In_       const CharT* lpData,
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
            Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueA: key=0x%x Key=%s valuename=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyOnlyPath.c_str(), lpValueName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueW: key=0x%x Key=%s valuename=%s", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str(), lpValueName);
        }

        // Additional note: Unlike RegSetValueEx, this API actually creates a subkey
        // using the name requested and sets the value of the default (unnamed) item with
        // the provided string.
        RegCohorts regCohorts;

#if TRYHKLM2HKCU
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegSetValue:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                regCohorts = GenerateRegCohorts(key, L"", RegLocalInstance);

                if (regCohorts.RedirectionNotPossible == false)
                {
                    // If redirection is possible, this is what we must do when creating the key.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegSetValue is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefixCU = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameOnlyW;
                    std::wstring newSubKey = prefixCU + regCohorts.RequestedPath.substr(std::wstring(L"HKEY_LOCAL_MACHINE\\").length());
                    LSTATUS altResult;
                    if (regCohorts.RedirectedPath.length() == prefixCU.length())
                    {
                        altResult = ::RegOpenKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), &altKey);
                        Log(LogLevel_DebugIntermediate, L"[%s%d] ::RegOpenKey Creation of redirection HKCU base path %s key=0x%x result=%s", g_RegModuleName, RegLocalInstance, HKLM2HKCU_RedirNameOnlyW.c_str(), altKey, LStatusToWstring(altResult).c_str());
                        if (altResult == ERROR_SUCCESS)
                        {
                            result = RegSetValueImpl(altKey, lpValueName, lpDwType, lpData, cbData);
                            Log(LogLevel_DebugBasic, L"[%s%d] RegSetValue: setting value result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                            RegCloseKey(altKey);
                            return result;
                        }
                    }
                    else
                    {
                        altResult = ::RegCreateKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), &altKey);
                        Log(LogLevel_DebugIntermediate, L"[%s%d] ::RegCreateKey Creation of redirection HKCU base path %s key=0x%x result=%s", g_RegModuleName, RegLocalInstance, HKLM2HKCU_RedirNameOnlyW.c_str(), altKey, LStatusToWstring(altResult).c_str());
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            HKEY altSubKey;
                            altResult = ::RegOpenKeyExW(altKey, newSubKey.c_str(), 0, KEY_SET_VALUE, &altSubKey);
                            if (altResult == ERROR_SUCCESS)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] ::RegOpenKeyExW: Redirecting to HKCU subkey=%s", g_RegModuleName, RegLocalInstance, newSubKey.c_str());
                                result = RegSetValueImpl(altKey, lpValueName, lpDwType, lpData, cbData);
                                Log(LogLevel_DebugBasic, L"[%s%d] RegSetValue (HKCU redirected): setting value result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                RegCloseKey(altSubKey);
                            }
                            RegCloseKey(altKey);
                        }
                        return result;
                    }
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegSetValueExW:  HKLM2HKCU processing exception.", g_RegModuleName, RegLocalInstance);
            }
        }
#endif

        result = RegSetValueImpl(key, lpValueName, lpDwType, lpData, cbData);
        Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW:  Returning failure %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueW: result=%s", g_RegModuleName, 0, LStatusToWstring(result).c_str());
    }
    else
    {
        result = RegSetValueImpl(key, lpValueName, lpDwType, lpData, cbData);
        Log(LogLevel_DebugBasic, L"[%s%d] RegSetValueW: (unguarded) result=%s", g_RegModuleName, 0, LStatusToWstring(result).c_str());
    }
    return result;
}
DECLARE_STRING_FIXUP(RegSetValueImpl, RegSetValueFixup);
