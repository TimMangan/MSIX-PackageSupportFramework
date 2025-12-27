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
#pragma comment(linker, "/EXPORT:RegOpenKeyAFixupAnsi_Fixup=_RegOpenKeyImplA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegOpenKeyWFixupWide_Fixup=_RegOpenKeyImplW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegOpenKeyAFixupAnsi_Fixup=RegOpenKeyImplA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegOpenKeyWFixupWide_Fixup=RegOpenKeyImplW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

auto RegOpenKeyImpl = psf::detoured_string_function(&::RegOpenKeyA, &::RegOpenKeyW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _Out_ PHKEY resultKey)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        std::string keyOnlyPath = InterpretKeyPath(key);
        std::string keypath = keyOnlyPath + "\\" + InterpretStringA(subKey);
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);


        if constexpr (psf::is_ansi<CharT>)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyA:  key=0x%x subkey=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyW: key=0x%x subKey=%s", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
        }

        bool testDeletionMaker = HasDeletionMarkerSpecified();
        bool testJavaBlocker = HasJavaBlockerSpecified();
        if (testDeletionMaker)
        {
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, widen(subKey).c_str(), RegLocalInstance);
            if (result != ERROR_SUCCESS)
            {

                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
                result = ERROR_PATH_NOT_FOUND;
                resultKey = NULL;
                return result;
            }
        }

        if (testJavaBlocker)
        {
            std::wstring wFullPath = wKeyOnlyPath;
            if (subKey != NULL)
            {
                wFullPath += L"\\";
                wFullPath += widen(subKey).c_str();
            }
            if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
                result = ERROR_PATH_NOT_FOUND;
                resultKey = NULL;
                return result;
            }
        }




        RegCohorts regCohorts;
#if TRYHKLM2HKCU
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                regCohorts = GenerateRegCohorts(key, InterpretStringW(subKey), RegLocalInstance);

                if (regCohorts.RedirectionNotPossible == false)
                {
                    // If redirection is possible, this is what we must do when creating the key.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefixCU = HKCU_RedirNameW;
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegOpenKey PrefixCU %s with length %d", g_RegModuleName, RegLocalInstance, prefixCU.c_str(), prefixCU.length());
                    if (regCohorts.RedirectedPath.length() == prefixCU.length())
                    {
                        result = ::RegOpenKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), resultKey);
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey Open of base key HKCU %s Key=0x%d result=%s", g_RegModuleName, RegLocalInstance, HKLM2HKCU_RedirNameOnlyW.c_str(), *resultKey, LStatusToWstring(result).c_str());
                    }
                    else
                    {
                        LSTATUS altResult = ::RegCreateKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), &altKey);
                        Log(LogLevel_DebugIntermediate, L"[%s%d] ::RegCreateKey Creation of the base key of HKCU %s Key0x%x result=%s", g_RegModuleName, RegLocalInstance, HKLM2HKCU_RedirNameOnlyW.c_str(), *resultKey, LStatusToWstring(altResult).c_str());
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = RegOpenKeyImpl(altKey, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), resultKey);  // +1 is for following '\\'
                            if (result == ERROR_SUCCESS)
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] ::RegOpenKey redirected subkey success key=0x%x path=%s result=%s", g_RegModuleName, RegLocalInstance, *resultKey, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), LStatusToWstring(result).c_str());
                                return result;
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] ::RegOpenKey redirected subkey does not yet exist path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), LStatusToWstring(result).c_str());
                                //result = ::RegCreateKey(altKey, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), resultKey);
                                //Log(LogLevel_DebugIntermediate, L"[%s%d] RegCreateKey Creation of the redirected subkey of %s Key0x%x result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), *resultKey, LStatusToWstring(result).c_str());
                            }
                            RegCloseKey(altKey);
                        }
                        else
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey Unable to create parent redirection base HKCU key?  err=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(altResult).c_str());
                        }
                    }
                }
            }
            catch (...)
            {
                // If anything goes wrong, just do the normal call
                Log(LogLevel_Exception, L"[%s%d] RegOpenKey redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
            }
        }
#endif
        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey try call as requested.", g_RegModuleName, RegLocalInstance);
        result = RegOpenKeyImpl(key, subKey, resultKey);
        
        if (result != ERROR_SUCCESS)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey requested result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
#if TRYHKLM2HKCU
            if (HasHKLM2HKCUSpecified())
            {
                if (regCohorts.ReverseRedirectionNotPossible== false)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    DWORD RememberLastError = GetLastError();
                    HKEY altKey;
                    LSTATUS altResult = RegOpenKeyImpl(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &altKey);
                    if (altResult != ERROR_SUCCESS)
                    {
                        SetLastError(RememberLastError);
                    }
                    else
                    {
                        if (*resultKey != NULL)
                        {
                            RegCloseKey(*resultKey);
                        }
                        *resultKey = altKey;
                        result = altResult;
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey success key=0x%x using reverse HKCU replacement.", g_RegModuleName, RegLocalInstance, *resultKey);
                    }
                }
            }
#endif
        }

        if (result != ERROR_SUCCESS)
        {
            std::wstring sskey = widen(subKey);
            if (sskey.find(L"PSF_READY_MARKER_") != std::wstring::npos)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey indicates that PSF injections are complete and the process is ready to run. Result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey returning result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
        } 
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey returning key=0x%x result=SUCCESS %s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());
        }
    }
    else
    {
        result = RegOpenKeyImpl(key, subKey, resultKey);
    }
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyImpl, RegOpenKeyFixup);
