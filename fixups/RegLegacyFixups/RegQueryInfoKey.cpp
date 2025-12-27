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
        //CharT* Class;
        DWORD cchClass = 0;
        DWORD reserved = 0;
        DWORD cSubKeys = 0;
        DWORD cbMaxSubKeyLen = 0;
        DWORD cbMaxClassLen = 0;
        DWORD cValues = 0;
        DWORD cbMaxValueNameLen = 0;
        DWORD cbMaxValueLen = 0;
        DWORD cbSecurityDescriptor = 0;
        FILETIME ftLastWriteTime;
        ftLastWriteTime.dwLowDateTime = 0;
        ftLastWriteTime.dwHighDateTime = 0;


        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        RegCohorts regCohorts;
        KeyChildEnumerationsW keyChildEnumerations[3];
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

        Log(LogLevel_DebugBasic, L"[%s%d] RegQueryInfoKey:  key=0x%x keyname=%s", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str());

        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            bool bReq_Class = lpClass != NULL;
            bool bReq_cchClass = lpcchClass != NULL;
            // bool bReserved
            bool bReq_SubKeys = lpcSubKeys != NULL;
            bool bReq_MaxSubKeyLen = lpcbMaxSubKeyLen != NULL;
            bool bReq_MaxClassLen = lpcbMaxClassLen != NULL;
            bool bReq_Values = lpcValues != NULL;
            bool bReq_MaxValueNameLen = lpcbMaxValueNameLen != NULL;
            bool bReq_MaxValueLen = lpcbMaxValueLen != NULL;
            bool bReq_SecurityDescriptor = lpcbSecurityDescriptor != NULL;
            bool bReq_LastWriteTime = lpftLastWriteTime != NULL;
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: Optional params requested = %d %d reserved %d %d %d %d %d %d %d %d",
                g_RegModuleName, RegLocalInstance,
                bReq_Class, bReq_cchClass,
                bReq_SubKeys, bReq_MaxSubKeyLen, bReq_MaxClassLen, bReq_Values,
                bReq_MaxValueNameLen, bReq_MaxValueLen, bReq_SecurityDescriptor, bReq_LastWriteTime);
        }
        //DWORD* alt_lpClass = NULL;
        DWORD* alt_lpcchClass = NULL;
        DWORD* alt_lpreserved = NULL;
        DWORD* alt_lpcSubKeys = NULL;
        DWORD* alt_lpcbMaxSubKeyLen = NULL;
        DWORD* alt_lpcbMaxClassLen = NULL;
        DWORD* alt_lpcValues = NULL;
        DWORD* alt_lpcbMaxValueNameLen = NULL;
        DWORD* alt_lpcbMaxValueLen = NULL;
        DWORD* alt_lpcbSecurityDescriptor = NULL;
        FILETIME* alt_lpftLastWriteTime = NULL;

        //if (lpClass != NULL) {
        //    Class = *lpClass; 
        //    alt_lpClass = &Class;
        //}
        if (lpcchClass != NULL)
        {
            cchClass = *lpcchClass;
            alt_lpcchClass = &cchClass;
        }
        if (lpcSubKeys != NULL)
        {
            *lpcSubKeys = 0;
            cSubKeys = *lpcSubKeys;
            alt_lpcSubKeys = &cSubKeys;
        }
        if (lpcbMaxSubKeyLen != NULL)
        {
            *lpcbMaxSubKeyLen = 0;
            cbMaxSubKeyLen = *lpcbMaxSubKeyLen;
            alt_lpcbMaxSubKeyLen = &cbMaxSubKeyLen;
        }
        if (lpcbMaxClassLen != NULL)
        {
            *lpcbMaxClassLen = 0;
            cbMaxClassLen = *lpcbMaxClassLen;
            alt_lpcbMaxClassLen = &cbMaxClassLen;
        }
        if (lpcValues != NULL)
        {
            *lpcValues = 0;
            cValues = *lpcValues;
            alt_lpcValues = &cValues;
        }
        if (lpcbMaxValueNameLen != NULL)
        {
            *lpcbMaxValueNameLen = 0;
            cbMaxValueNameLen = *lpcbMaxValueNameLen;
            alt_lpcbMaxValueNameLen = &cbMaxValueNameLen;
        }
        if (lpcbMaxValueLen != NULL)
        {
            *lpcbMaxValueLen = 0;
            cbMaxValueLen = *lpcbMaxValueLen;
            alt_lpcbMaxValueLen = &cbMaxValueLen;
        }
        if (lpcbSecurityDescriptor != NULL)
        {
            cbSecurityDescriptor = *lpcbSecurityDescriptor;
            alt_lpcbSecurityDescriptor = &cbSecurityDescriptor;
        }
        if (lpftLastWriteTime != NULL)
        {
            ftLastWriteTime.dwLowDateTime = lpftLastWriteTime->dwLowDateTime;
            ftLastWriteTime.dwHighDateTime = lpftLastWriteTime->dwHighDateTime;
            alt_lpftLastWriteTime = &ftLastWriteTime;
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
                        keyChildEnumerations[0].SubKeyCount = cSubKeys;
                        keyChildEnumerations[0].ValueCount = cValues;
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
        rStatus = RegQueryInfoKeyImpl(keyChildEnumerations[1].Key, lpClass, alt_lpcchClass, alt_lpreserved,
            alt_lpcSubKeys, alt_lpcbMaxSubKeyLen, alt_lpcbMaxClassLen,
            alt_lpcValues, alt_lpcbMaxValueNameLen, alt_lpcbMaxValueLen,
            alt_lpcbSecurityDescriptor, alt_lpftLastWriteTime);
        if (rStatus == ERROR_SUCCESS)
        {
            keyChildEnumerations[1].SubKeyCount = cSubKeys;
            keyChildEnumerations[1].ValueCount = cValues;
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: requested success with SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
            if (lpcchClass != NULL) *lpcchClass = cchClass;
            if (lpcSubKeys != NULL) *lpcSubKeys += cSubKeys;
            if (lpcbMaxSubKeyLen != NULL && *lpcbMaxSubKeyLen < cbMaxSubKeyLen) *lpcbMaxSubKeyLen = cbMaxSubKeyLen;
            if (lpcbMaxClassLen != NULL && *lpcbMaxClassLen < cbMaxClassLen) *lpcbMaxClassLen += cbMaxClassLen;
            ///Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: mid saving", g_RegModuleName, RegLocalInstance);
            if (lpcValues != NULL) *lpcValues += cValues;
            ///Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: post-cValues saving", g_RegModuleName, RegLocalInstance);
            if (lpcbMaxValueNameLen != NULL && *lpcbMaxValueNameLen < cbMaxValueNameLen) *lpcbMaxValueNameLen = cbMaxValueNameLen;
            ///Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: post-cbMaxValueNameLen saving", g_RegModuleName, RegLocalInstance);
            if (lpcbMaxValueLen != NULL && *lpcbMaxValueLen < cbMaxValueLen) *lpcbMaxValueLen = cbMaxValueLen;
            ///Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: post-cbMaxValueLen saving", g_RegModuleName, RegLocalInstance);
            if (lpcbSecurityDescriptor != NULL ) *lpcbSecurityDescriptor = cbSecurityDescriptor;
            ///Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: post-cbSecurityDescriptor saving", g_RegModuleName, RegLocalInstance);
            if (lpftLastWriteTime != NULL && CompareFileTime(lpftLastWriteTime, &ftLastWriteTime) > 0)
            {
                lpftLastWriteTime->dwLowDateTime = ftLastWriteTime.dwLowDateTime;
                lpftLastWriteTime->dwHighDateTime = ftLastWriteTime.dwLowDateTime;
            }
            ///Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: post-ftLastWriteTime saving", g_RegModuleName, RegLocalInstance);
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  requested error=%s",
                g_RegModuleName, RegLocalInstance,
                LStatusToWstring(rStatus).c_str());

            // try again with root key and path
            if ( wKeyOnlyPath.length() > HKCU_RedirNameW.length()+1 &&
                 wKeyOnlyPath._Starts_with(HKCU_RedirNameW.c_str())  )
            {
                std::wstring wRemainder = wKeyOnlyPath.substr(HKCU_RedirNameW.length() + 1);
                HKEY altKey;
                LSTATUS altStatus = ::RegOpenKey(HKEY_LOCAL_MACHINE,wRemainder.c_str(),&altKey);
                if (altStatus == ERROR_SUCCESS)
                {
                    altStatus = RegQueryInfoKeyImpl(altKey, lpClass, alt_lpcchClass, alt_lpreserved,
                        alt_lpcSubKeys, alt_lpcbMaxSubKeyLen, alt_lpcbMaxClassLen,
                        alt_lpcValues, alt_lpcbMaxValueNameLen, alt_lpcbMaxValueLen,
                        alt_lpcbSecurityDescriptor, alt_lpftLastWriteTime);
                    if (altStatus == ERROR_SUCCESS)
                    {
                        keyChildEnumerations[1].SubKeyCount = cSubKeys;
                        keyChildEnumerations[1].ValueCount = cValues;
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey:  alt requested SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
                        if (lpcchClass != NULL) *lpcchClass = cchClass;
                        if (lpcSubKeys != NULL) *lpcSubKeys += cSubKeys;
                        if (lpcbMaxSubKeyLen != NULL && *lpcbMaxSubKeyLen > cbMaxSubKeyLen) *lpcbMaxSubKeyLen = cbMaxSubKeyLen;
                        if (lpcbMaxClassLen != NULL && *lpcbMaxClassLen > cbMaxClassLen) *lpcbMaxClassLen += cbMaxClassLen;
                        if (lpcValues != NULL) *lpcValues += cValues;
                        if (lpcbMaxValueNameLen != NULL && *lpcbMaxValueNameLen > cbMaxValueNameLen) *lpcbMaxValueNameLen = cbMaxValueNameLen;
                        if (lpcbMaxValueLen != NULL && CompareFileTime(lpftLastWriteTime, &ftLastWriteTime) > 0)
                        {
                            lpftLastWriteTime->dwLowDateTime = ftLastWriteTime.dwLowDateTime;
                            lpftLastWriteTime->dwHighDateTime = ftLastWriteTime.dwHighDateTime;
                        }
                        rStatus = altStatus;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: alt requested error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(altStatus).c_str());
                    }
                    RegCloseKey(altKey);
                }
            }
        }

        if (HasHKLM2HKCUSpecified())
        {
            if (regCohorts.ReverseRedirectionNotPossible == false)
            {
                LSTATUS resOpen = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(RegMachineW.length()+1).c_str(), &keyChildEnumerations[2].Key);
                if (resOpen == ERROR_SUCCESS)
                {
                    keyChildEnumerations[2].ValidKey = true;
                    rStatus = RegQueryInfoKeyImpl(keyChildEnumerations[2].Key, lpClass, alt_lpcchClass, alt_lpreserved,
                        alt_lpcSubKeys, alt_lpcbMaxSubKeyLen, alt_lpcbMaxClassLen,
                        alt_lpcValues, alt_lpcbMaxValueNameLen, alt_lpcbMaxValueLen,
                        alt_lpcbSecurityDescriptor, alt_lpftLastWriteTime);
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
                        if (lpcbMaxValueLen != NULL && *lpcbMaxValueLen > cbMaxValueLen) *lpcbMaxValueLen = cbMaxValueLen;
                        if (lpftLastWriteTime != NULL && CompareFileTime(lpftLastWriteTime, &ftLastWriteTime) > 0)
                        {
                            lpftLastWriteTime->dwLowDateTime = ftLastWriteTime.dwLowDateTime;
                            lpftLastWriteTime->dwHighDateTime = ftLastWriteTime.dwHighDateTime;
                        }
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
        Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: done", g_RegModuleName, RegLocalInstance);
        if (lpcSubKeys != NULL)
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: returning Count of subkeys=%d", g_RegModuleName, RegLocalInstance, *lpcSubKeys);
        }
        if (lpcValues != NULL)
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryInfoKey: returning Count of values=%d", g_RegModuleName, RegLocalInstance, *lpcValues);
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