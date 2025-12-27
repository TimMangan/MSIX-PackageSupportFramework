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


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:RegOpenKeyTransactedAFixupAnsi_Fixup=_RegOpenKeyTransactedImplA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegOpenKeyTransactedWFixupWide_Fixup=_RegOpenKeyTransactedImplW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegOpenKeyTransactedAFixupAnsi_Fixup=RegOpenKeyTransactedImplA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegOpenKeyTransactedWFixupWide_Fixup=RegOpenKeyTransactedImplW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


auto RegOpenKeyTransactedImpl = psf::detoured_string_function(&::RegOpenKeyTransactedA, &::RegOpenKeyTransactedW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyTransactedFixup(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _In_opt_ DWORD options,         // reserved
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey,
    _In_ HANDLE hTransaction,
    _In_ PVOID  pExtendedParameter)  // reserved
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {

        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        std::string keyOnlyPath = InterpretKeyPath(key);
        std::string keyPath = keyOnlyPath + "\\" + InterpretStringA(subKey);
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);


        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyTransacted:\n", g_RegModuleName, RegLocalInstance);
        REGSAM samModified = RegFixupSam(LogLevel_DebugMaximum, keyPath, samDesired, RegLocalInstance);


        bool testDeletionMaker = HasDeletionMarkerSpecified();
        bool testJavaBlocker = HasJavaBlockerSpecified();
        if (testDeletionMaker)
        {
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, widen(subKey).c_str(), RegLocalInstance);
            if (result != ERROR_SUCCESS)
            {

                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyTransacted blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
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
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyTransacted blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
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
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyTransacted:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                regCohorts = GenerateRegCohorts(key, InterpretStringW(subKey), RegLocalInstance);

                if (regCohorts.RedirectionNotPossible == false)
                {
                    // If redirection is possible, this is what we must do when creating the key.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyTransacted is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefix = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameOnlyW;
                    if (regCohorts.RedirectedPath.length() == prefix.length())
                    {
                        result = ::RegOpenKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), resultKey);
                    }
                    else
                    {
                        LSTATUS altResult = ::RegCreateKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), &altKey);
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = RegOpenKeyTransactedImpl(altKey, regCohorts.RedirectedPath.substr(prefix.length() + 1).c_str(), options, samModified, resultKey, hTransaction, pExtendedParameter);
                            RegCloseKey(altKey);
                        }
                    }
                    Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyTransacted redirected result=%s, path=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), regCohorts.RedirectedPath.c_str());
                    return result;
                }
            }
            catch (...)
            {
                // If anything goes wrong, just do the normal call
                Log(LogLevel_Exception, L"[%s%d] RegOpenKeyTransacted redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
            }
        }
#endif
        result = RegOpenKeyTransactedImpl(key, subKey, options, samModified, resultKey, hTransaction, pExtendedParameter);


        if (result != ERROR_SUCCESS)
        {
#if TRYHKLM2HKCU
            if (HasHKLM2HKCUSpecified())
            {
                if (regCohorts.ReverseRedirectionNotPossible == false)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyTransacted is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    DWORD RememberLastError = GetLastError();
                    LSTATUS altResult = RegOpenKeyTransactedImpl(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), options, samModified, resultKey, hTransaction, pExtendedParameter);
                    if (altResult != ERROR_SUCCESS)
                    {
                        SetLastError(RememberLastError);
                    }
                    else
                    {
                        result = altResult;
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyTransacted using reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
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
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyTransacted Result=%d here indicates that PSF injections are complete and the process is ready to run.", g_RegModuleName, RegLocalInstance, result);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyTransacted result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyTransacted key=0x%x result=SUCCESS %s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());
        }
    }
    else
    {
        result = RegOpenKeyTransactedImpl(key, subKey, options, samDesired, resultKey, hTransaction, pExtendedParameter);
    }
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyTransactedImpl, RegOpenKeyTransactedFixup);
