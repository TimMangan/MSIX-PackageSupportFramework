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
#include <iostream>

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

#if TRYHKLM2HKCU


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:RegQueryInfoKeyFixupAnsi_Fixup=_RegQueryInfoKeyFixupA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegQueryInfoKeyFixupWide_Fixup=_RegQueryInfoKeyFixupW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegQueryInfoKeyFixupAnsi_Fixup=RegQueryInfoKeyFixupA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegQueryInfoKeyFixupWide_Fixup=RegQueryInfoKeyFixupW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

auto RegQueryInfoKeyImpl = psf::detoured_string_function(&::RegQueryInfoKeyA, &::RegQueryInfoKeyW);

template <typename CharT>
LSTATUS __stdcall RegQueryInfoKeyFixup(
    _In_                HKEY    key,
    _Out_opt_           CharT* lpClass,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _Reserved_          LPDWORD lpReserved,
    _Out_opt_           LPDWORD lpcSubKeys,
    _Out_opt_           LPDWORD lpcbMaxSubKeyLen,
    _Out_opt_           LPDWORD lpcbMaxClassLen,
    _Out_opt_           LPDWORD lpcValues,
    _Out_opt_           LPDWORD lpcbMaxValueNameLen,
    _Out_opt_           LPDWORD lpcbMaxValueLen,
    _Out_opt_           LPDWORD lpcbSecurityDescriptor,
    _Out_opt_           PFILETIME lpftLastWriteTime)
{
    LSTATUS rStatus = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD cchClass;
        DWORD reserved;
        DWORD cSubKeys;
        DWORD cbMaxSubKeyLen;
        DWORD cbMaxClassLen;
        DWORD cValues;
        DWORD cbMaxValueNameLen;
        DWORD cbMaxValueLen;
        DWORD cbSecurityDescriptor;
        FILETIME ftLastWriteTime;

        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        RegCohorts regCohorts;
        KeyChildEnumerationsW keyChildEnumerations[3];
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

        Log(LogLevel_DebugBasic, L"[%s%d] RegQueryInfoKey:  key=0x%x keyname=%s", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str());

        if (lpcSubKeys != NULL) *lpcSubKeys = 0;
        if (lpcbMaxSubKeyLen != NULL) *lpcbMaxSubKeyLen = 0;
        if (lpcbMaxClassLen != NULL) *lpcbMaxClassLen = 0;
        if (lpcValues != NULL) *lpcValues = 0;
        if (lpcbMaxValueNameLen != NULL) *lpcbMaxValueNameLen = 0;
        if (lpcbMaxValueLen != NULL) *lpcbMaxValueLen = 0;
        if (lpcbSecurityDescriptor != NULL) *lpcbSecurityDescriptor = 0;
        if (lpftLastWriteTime != NULL)
        {
            lpftLastWriteTime->dwLowDateTime = 0;
            lpftLastWriteTime->dwHighDateTime = 0;
        }

        // NOTE: We are not going to worry about duplicates or deletion markers as we are just giving counts, and it should be OK to give too many as the response.
        if (HasHKLM2HKCUSpecified())
        {
            regCohorts = GenerateRegCohorts(key, wKeyOnlyPath, RegLocalInstance);
            if (regCohorts.RedirectionNotPossible == false)
            {
                LSTATUS resOpen = ::RegOpenKey(HKEY_CURRENT_USER, regCohorts.RedirectedPath.substr(18).c_str(), &keyChildEnumerations[0].Key);
                if (resOpen == ERROR_SUCCESS)
                {
                    keyChildEnumerations[0].ValidKey = true;
                    resOpen = RegQueryInfoKeyImpl(keyChildEnumerations[0].Key, lpClass, &cchClass, &reserved,
                                                &cSubKeys, &cbMaxSubKeyLen, &cbMaxClassLen,
                                                &cValues, &cbMaxValueNameLen, &cbMaxValueLen,
                                                &cbSecurityDescriptor, &ftLastWriteTime);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                        if (lpcchClass != NULL) *lpcchClass = cchClass;
                        if (lpcSubKeys != NULL) *lpcSubKeys = cSubKeys;
                        if (lpcbMaxSubKeyLen != NULL) *lpcbMaxSubKeyLen = cbMaxSubKeyLen;
                        if (lpcbMaxClassLen != NULL) *lpcbMaxClassLen = cbMaxClassLen;
                        if (lpcValues != NULL) *lpcValues = cValues;
                        if (lpcbMaxValueNameLen != NULL) *lpcbMaxValueNameLen = cbMaxValueNameLen;
                        if (lpcbMaxValueLen != NULL) *lpcbMaxValueLen = cbMaxValueLen;
                        if (lpcbSecurityDescriptor != NULL) *lpcbSecurityDescriptor = cbSecurityDescriptor;
                        if (lpftLastWriteTime != NULL) *lpftLastWriteTime = ftLastWriteTime;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                    }
                }
            }
        }

        keyChildEnumerations[1].Key = key;
        keyChildEnumerations[1].ValidKey = true;
        rStatus = RegQueryInfoKeyImpl(keyChildEnumerations[1].Key, lpClass, &cchClass, &reserved,
                                    &cSubKeys, &cbMaxSubKeyLen, &cbMaxClassLen,
                                    &cValues, &cbMaxValueNameLen, &cbMaxValueLen,
                                    &cbSecurityDescriptor, &ftLastWriteTime);
        if (rStatus == ERROR_SUCCESS)
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  requested SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
            if (lpcchClass != NULL) *lpcchClass = cchClass;
            if (lpcSubKeys != NULL) *lpcSubKeys += cSubKeys;
            if (lpcbMaxSubKeyLen != NULL && *lpcbMaxSubKeyLen > cbMaxSubKeyLen) *lpcbMaxSubKeyLen = cbMaxSubKeyLen;
            if (lpcbMaxClassLen != NULL && *lpcbMaxClassLen > cbMaxClassLen) *lpcbMaxClassLen += cbMaxClassLen;
            if (lpcValues != NULL) *lpcValues += cValues;
            if (lpcbMaxValueNameLen != NULL && *lpcbMaxValueNameLen > cbMaxValueNameLen) *lpcbMaxValueNameLen = cbMaxValueNameLen;
            if (lpcbMaxValueLen != NULL && CompareFileTime(lpftLastWriteTime, &ftLastWriteTime) > 0) *lpftLastWriteTime = ftLastWriteTime;
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  requested error=%s",
                g_RegModuleName, RegLocalInstance,
                LStatusToWstring(rStatus).c_str());
        }

        if (HasHKLM2HKCUSpecified())
        {
            if (regCohorts.ReverseRedirectionNotPossible == false)
            {
                LSTATUS resOpen = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &keyChildEnumerations[2].Key);
                if (resOpen == ERROR_SUCCESS)
                {
                    keyChildEnumerations[2].ValidKey = true;
                    rStatus = RegQueryInfoKeyImpl(keyChildEnumerations[2].Key, lpClass, &cchClass, &reserved,
                                                &cSubKeys, &cbMaxSubKeyLen, &cbMaxClassLen,
                                                &cValues, &cbMaxValueNameLen, &cbMaxValueLen,
                                                &cbSecurityDescriptor, &ftLastWriteTime);
                    if (rStatus == ERROR_SUCCESS)
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  reverse redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                        if (lpcchClass != NULL) *lpcchClass = cchClass;
                        if (lpcSubKeys != NULL) *lpcSubKeys += cSubKeys;
                        if (lpcbMaxSubKeyLen != NULL && *lpcbMaxSubKeyLen > cbMaxSubKeyLen) *lpcbMaxSubKeyLen = cbMaxSubKeyLen;
                        if (lpcbMaxClassLen != NULL && *lpcbMaxClassLen > cbMaxClassLen) *lpcbMaxClassLen += cbMaxClassLen;
                        if (lpcValues != NULL) *lpcValues += cValues;
                        if (lpcbMaxValueNameLen != NULL && *lpcbMaxValueNameLen > cbMaxValueNameLen) *lpcbMaxValueNameLen = cbMaxValueNameLen;
                        if (lpcbMaxValueLen != NULL && CompareFileTime(lpftLastWriteTime, &ftLastWriteTime) > 0) *lpftLastWriteTime = ftLastWriteTime;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  reverse redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(rStatus).c_str());
                    }
                }
            }
        }


        // Cleanup
        if (keyChildEnumerations[0].ValidKey)
        {
            RegCloseKey(keyChildEnumerations[0].Key);
        }
        if (keyChildEnumerations[2].ValidKey)
        {
            RegCloseKey(keyChildEnumerations[2].Key);
        }
    }
    else
    {
        rStatus = RegQueryInfoKeyImpl(key, lpClass, lpcchClass, lpReserved, 
                                        lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, 
                                        lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, 
                                        lpcbSecurityDescriptor, lpftLastWriteTime);
    }
    return rStatus;

}
DECLARE_STRING_FIXUP(RegQueryInfoKeyImpl, RegQueryInfoKeyFixup);

#endif