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

// Here is the concept for handling enumeration with redirection involved.
// Unlike MFRFixup for FindNextFile, we do not have a place to store state between calls.
// Therefore, to keep from enumerating everything on all calls, we will read the index requested, from the appropriate first key.
// We can make a call to RegQueryInfoKey (without intercept) to get the count of values in that key.
// If the index falls within that range, we get the index value using the pass-through call to the function,  and return it.
// If it is outside that range, we create a list of the valid indexed on that list, and then continue enumerating the into the next list,
// ignoring any return that was also in the first list.
// 
// TODO: The new methods are ignoring the deletion list for now. That should be added.
// 


/*****
    Discussion:
        The intercepts for RegEnumValue must consider the possible effects of HKLM2HKCU layering, DeletionMarkers, and JavaBlockers
        For HKLM2HKCU that means we have 1 or 2 layers to look at so we may be making multiple calls.  In fact, we need to enumerate previous entries
        because as we hit the next layer we need to skip duplicates.  This means that we need full names and can't count on limitations imposed by the
        optional values and lengths provided by the caller.
        With multiple calls we need each call to be properly initialized.
*****/

#if INTERCEPT_KERNELBASE
#if TRYHKLM2HKCU


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:RegEnumValueAFixupAnsi_Fixup=impl::_KernelBaseRegEnumValueA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegEnumValueWFixupWide_Fixup=impl::_KernelBaseRegEnumValueW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegEnumValueAFixupAnsi_Fixup=impl::KernelBaseRegEnumValueA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegEnumValueWFixupWide_Fixup=impl::KernelBaseRegEnumValueW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


LSTATUS __stdcall RegEnumValueAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpValueName,
    _In_ _Out_ LPDWORD lpcchValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    DWORD origlpcchValueName = *lpcchValueName;
    DWORD origLpcdData = 0;
    if (lpcbData != NULL)
    {
        origLpcdData = *lpcbData;
    }
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        RegCohorts regCohorts;
        KeyChildEnumerationsA keyChildEnumerations[3];
        std::string keyOnlyPath = InterpretKeyPath(key);
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  key=0x%x keyname=%s dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str(), dwIndex);
        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:     Buffer size for ValueName=0x%x", g_RegModuleName, RegLocalInstance, origlpcchValueName);
        if (lpType != NULL)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:     Type pointer provided", g_RegModuleName, RegLocalInstance);
        }
        if (lpData != NULL && lpcbData != NULL)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:     Buffer size for ValueData=0x%x", g_RegModuleName, RegLocalInstance, origLpcdData);
        }

        //Determine counts and Maximums
        DWORD MaximalValueNameLen = 0;
        DWORD MaximalValueDataLen = 0;
        if (HasHKLM2HKCUSpecified())
        {
            regCohorts = GenerateRegCohorts(key, L"", RegLocalInstance);
            if (regCohorts.RedirectionNotPossible == false)
            {
                LSTATUS resOpen = ::RegOpenKey(HKEY_CURRENT_USER, regCohorts.RedirectedPath.substr(18).c_str(), &keyChildEnumerations[0].Key);
                if (resOpen == ERROR_SUCCESS)
                {
                    keyChildEnumerations[0].ValidKey = true;
                    resOpen = ::RegQueryInfoKeyA(keyChildEnumerations[0].Key, NULL, NULL, NULL,
                        &keyChildEnumerations[0].SubKeyCount, &keyChildEnumerations[0].MaxSubKeyNameLen, NULL,
                        &keyChildEnumerations[0].ValueCount, &keyChildEnumerations[0].MaxValueNameLen, &keyChildEnumerations[0].MaxValueLen,
                        NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        keyChildEnumerations[0].ValidCounts = true;
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  redirected MaxValueNameLen=0x%x MaxValueLen=0x%x",
                            g_RegModuleName, RegLocalInstance,
                           keyChildEnumerations[0].MaxValueNameLen, keyChildEnumerations[0].MaxValueLen);
                        MaximalValueNameLen = keyChildEnumerations[0].MaxValueNameLen;
                        MaximalValueDataLen = keyChildEnumerations[0].MaxValueLen;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                        keyChildEnumerations[0].SubKeyCount = 0;
                        keyChildEnumerations[0].MaxSubKeyNameLen = 0;
                        keyChildEnumerations[0].ValueCount = 0;
                        keyChildEnumerations[0].MaxValueNameLen = 0;
                        keyChildEnumerations[0].MaxValueLen = 0;
                    }
                }
                else
                {
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected open error=%s",
                        g_RegModuleName, RegLocalInstance,
                        LStatusToWstring(resOpen).c_str());
                    keyChildEnumerations[0].SubKeyCount = 0;
                    keyChildEnumerations[0].MaxSubKeyNameLen = 0;
                    keyChildEnumerations[0].ValueCount = 0;
                    keyChildEnumerations[0].MaxValueNameLen = 0;
                    keyChildEnumerations[0].MaxValueLen = 0;
                }
            }
        }

        keyChildEnumerations[1].Key = key;
        keyChildEnumerations[1].ValidKey = true;
        result = ::RegQueryInfoKeyA(keyChildEnumerations[1].Key, NULL, NULL, NULL,
                                             &keyChildEnumerations[1].SubKeyCount, &keyChildEnumerations[1].MaxSubKeyNameLen, NULL,
                                             &keyChildEnumerations[1].ValueCount, &keyChildEnumerations[1].MaxValueNameLen, &keyChildEnumerations[1].MaxValueLen,
                                             NULL, NULL);
        if (result == ERROR_SUCCESS)
        {
            keyChildEnumerations[1].ValidCounts = true;
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  requested SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  requested MaxValueNameLen=%0x%x MaxValueLen=0x%x",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].MaxValueNameLen, keyChildEnumerations[1].MaxValueLen);
            if (keyChildEnumerations[1].MaxValueNameLen > MaximalValueNameLen)
            {
                MaximalValueNameLen = keyChildEnumerations[1].MaxValueNameLen;
            }
            if (keyChildEnumerations[1].MaxValueLen > MaximalValueDataLen)
            {
                if (keyChildEnumerations[1].MaxValueLen < 4096)
                {
                    MaximalValueDataLen = keyChildEnumerations[1].MaxValueLen;
                }
                else
                {
                    // Have seen suspiciously large values here            
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  Limiting maxvaluedatalen to 4096",
                        g_RegModuleName, RegLocalInstance);
                    if (MaximalValueDataLen < 4096)
                    {
                        MaximalValueDataLen = 4096;
                    }
                }
            }
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  requested error=%s",
                g_RegModuleName, RegLocalInstance,
                LStatusToWstring(result).c_str());
            keyChildEnumerations[1].SubKeyCount = 0;
            keyChildEnumerations[1].MaxSubKeyNameLen = 0;
            keyChildEnumerations[1].ValueCount = 0;
            keyChildEnumerations[1].MaxValueNameLen = 0;
            keyChildEnumerations[1].MaxValueLen = 0;
        }
 
        bool testDeletionMaker = HasDeletionMarkerSpecified();
        bool testJavaBlocker = HasJavaBlockerSpecified();

        if (HasHKLM2HKCUSpecified())
        {
            if (regCohorts.ReverseRedirectionNotPossible == false)
            {
                LSTATUS resOpen = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &keyChildEnumerations[2].Key);
                if (resOpen == ERROR_SUCCESS)
                {
                    keyChildEnumerations[2].ValidKey = true;
                    resOpen = ::RegQueryInfoKeyA(keyChildEnumerations[2].Key, NULL, NULL, NULL,
                                                 &keyChildEnumerations[2].SubKeyCount, &keyChildEnumerations[2].MaxSubKeyNameLen, NULL,
                                                 &keyChildEnumerations[2].ValueCount, &keyChildEnumerations[2].MaxValueNameLen, &keyChildEnumerations[2].MaxValueLen,
                                                 NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        keyChildEnumerations[2].ValidCounts = true;
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  reverse redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].SubKeyCount, keyChildEnumerations[2].ValueCount);
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  reverse redirected MaxValueNameLen=0x%x MaxValueLen=0x%x",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].MaxValueNameLen, keyChildEnumerations[2].MaxValueLen);
                        if (keyChildEnumerations[2].MaxValueNameLen > MaximalValueNameLen)
                        {
                            MaximalValueNameLen = keyChildEnumerations[2].MaxValueNameLen;
                        }
                        if (keyChildEnumerations[2].MaxValueLen > MaximalValueDataLen)
                        {
                            if (keyChildEnumerations[2].MaxValueLen < 4096)
                            {
                                MaximalValueDataLen = keyChildEnumerations[1].MaxValueLen;
                            }
                            else
                            {
                                // Have seen suspiciously large values here 
                                Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  Limiting maxvaluedatalen to 4096",
                                    g_RegModuleName, RegLocalInstance);
                                if (MaximalValueDataLen < 4096)
                                {
                                    MaximalValueDataLen = 4096;
                                }
                            }
                        }
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  reverse redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                        keyChildEnumerations[2].SubKeyCount = 0;
                        keyChildEnumerations[2].MaxSubKeyNameLen = 0;
                        keyChildEnumerations[2].ValueCount = 0;
                        keyChildEnumerations[2].MaxValueNameLen = 0;
                        keyChildEnumerations[2].MaxValueLen = 0;
                    }
                }
                else
                {
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected open error=%s",
                        g_RegModuleName, RegLocalInstance,
                        LStatusToWstring(resOpen).c_str());
                    keyChildEnumerations[2].SubKeyCount = 0;
                    keyChildEnumerations[2].MaxSubKeyNameLen = 0;
                    keyChildEnumerations[2].ValueCount = 0;
                    keyChildEnumerations[2].MaxValueNameLen = 0;
                    keyChildEnumerations[2].MaxValueLen = 0;
                }
            }
        }


        // Adjust maximums based on caller provided lengths
        if (lpValueName != NULL && lpcchValueName != NULL && *lpcchValueName > MaximalValueNameLen)
        {
            MaximalValueNameLen = *lpcchValueName;
        }
        if (lpData != NULL && lpcbData != NULL && *lpcbData > MaximalValueDataLen)
        {
            MaximalValueDataLen = *lpcbData;
        }
        char* pTempValueName = new char[(MaximalValueNameLen)+1];
        BYTE* pTempValueData = new byte[MaximalValueDataLen];
        DWORD savedMaximalValueNameLen = MaximalValueNameLen;
        DWORD savedMaximalValueDataLen = MaximalValueDataLen;
        ///Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  Calculated maximum sizes: ValueName=0x%x ValueData=0x%x",
        ///    g_RegModuleName, RegLocalInstance, savedMaximalValueNameLen, savedMaximalValueDataLen);

        
        // Start enumerating the keys
        DWORD indexesOfPreviousVisibleEnumerations = 0;
        DWORD currentListIndex = 0;
        bool  Done = false;
        bool AnyNewResult = false;

        if (keyChildEnumerations[0].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[0].ValueCount)
            {
                AnyNewResult = true;
                try
                {
                    //*lpcchValueName = origlpcchValueName;
                    //if (lpcbData != NULL)
                    //{
                    //    *lpcbData = origLpcdData;
                    //}
                    //result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[0].Key, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
                    MaximalValueNameLen = savedMaximalValueNameLen;
                    MaximalValueDataLen = savedMaximalValueDataLen;
                    result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[0].Key, currentListIndex, pTempValueName, &MaximalValueNameLen, lpReserved, lpType, pTempValueData, &MaximalValueDataLen);
                }
                catch (...)
                {
                    // We sometimes see this happen but not sure why.  Possibly someone manipulated the underlying values?  But in any case we should not crash.
                    result = GetLastError();
                }
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, pTempValueName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + pTempValueName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    // No previous list to compare to
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            if (lpValueName != NULL && lpcchValueName != NULL)
                            {
                                ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(char));
                                if (*lpcchValueName >= MaximalValueNameLen)
                                {
                                    *lpcchValueName = MaximalValueNameLen;
                                    memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(char));
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  redirected override from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(char));
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            if (lpData != NULL && lpcbData != NULL)
                            {
                                ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                                if (*lpcbData >= MaximalValueDataLen)
                                {
                                    *lpcbData = MaximalValueDataLen;
                                    memcpy(lpData, pTempValueData, MaximalValueDataLen);
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  redirected override data from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpData, pTempValueData, *lpcbData);
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  redirected index=%d using result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  redirected index=%d save previous name=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                            keyChildEnumerations[0].ValueNames.push_back(pTempValueName);
                        }
                    }
                    else
                    {
                        // skipping, previously reported
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insufficient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {
                        if (lpValueName != NULL && lpcchValueName != NULL)
                        {
                            ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(char));
                            if (*lpcchValueName >= MaximalValueNameLen)
                            {
                                *lpcchValueName = MaximalValueNameLen;
                                memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(char));
                            }
                            else
                            {
                                memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(char));
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        if (lpData != NULL && lpcbData != NULL)
                        {
                            ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                            if (*lpcbData >= MaximalValueDataLen)
                            {
                                *lpcbData = MaximalValueDataLen;
                                memcpy(lpData, pTempValueData, MaximalValueDataLen);
                            }
                            else
                            {
                                memcpy(lpData, pTempValueData, *lpcbData);
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  redirected index=%d with name=%s using even though ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        Done = true;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  redirected index=%d save name=%s previous although ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        keyChildEnumerations[0].ValueNames.push_back(pTempValueName);
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  redirected index=%d ignoring unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
            if (keyChildEnumerations[0].ValidCounts &&
                keyChildEnumerations[0].ValueCount == 0)
            {
                result = ERROR_NO_MORE_ITEMS;
            }
        }


        if (!Done && keyChildEnumerations[1].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[1].ValueCount)
            {
                AnyNewResult = true;
                try
                {
                    //*lpcchValueName = origlpcchValueName;
                    //if (lpcbData != NULL)
                    //{
                    //    *lpcbData = origLpcdData;
                    //}
                  //result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[1].Key, currentListIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
                    MaximalValueNameLen = savedMaximalValueNameLen;
                    MaximalValueDataLen = savedMaximalValueDataLen;
                    result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[1].Key, currentListIndex, pTempValueName, &MaximalValueNameLen, lpReserved, lpType, pTempValueData, &MaximalValueDataLen);
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:     RAWRETURN requested index=%d result=%s", g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:     RAWRETURN ValueNameLen=0x%x ValueName='%S'", g_RegModuleName, RegLocalInstance, MaximalValueNameLen, pTempValueName);
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:     RAWRETURN ValueDataLen=0x%x", g_RegModuleName, RegLocalInstance, MaximalValueDataLen);
                }
                catch (...)
                {
                    // We sometimes see this happen but not sure why.  Possibly someone manipulated the underlying values?  But in any case we should not crash.
                    result = GetLastError();
                }
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, pTempValueName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + pTempValueName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::string lpValueNameStr = pTempValueName;
                    if (IsValueNameInEnumerationA(lpValueNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d skipped due to duplication",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            if (lpValueName != NULL && lpcchValueName != NULL)
                            {
                                ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(char));
                                if (*lpcchValueName >= MaximalValueNameLen)
                                {
                                    *lpcchValueName = MaximalValueNameLen;
                                    memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(char));
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested override from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(char));
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            if (lpData != NULL && lpcbData != NULL)
                            {
                                ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                                if (*lpcbData >= MaximalValueDataLen)
                                {
                                    *lpcbData = MaximalValueDataLen;
                                    memcpy(lpData, pTempValueData, MaximalValueDataLen);
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested override data from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpData, pTempValueData, *lpcbData);
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d using result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  requested index=%d save previous name=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                            keyChildEnumerations[1].ValueNames.push_back(pTempValueName);
                        }
                    }
                    else
                    {
                        // skipping, already logged
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insufficient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {

                        if (lpValueName != NULL && lpcchValueName != NULL)
                        {
                            ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(char));
                            if (*lpcchValueName >= MaximalValueNameLen)
                            {
                                *lpcchValueName = MaximalValueNameLen;
                                memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(char));
                            }
                            else
                            {
                                memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(char));
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        if (lpData != NULL && lpcbData != NULL)
                        {
                            ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                            if (*lpcbData >= MaximalValueDataLen)
                            {
                                *lpcbData = MaximalValueDataLen;
                                memcpy(lpData, pTempValueData, MaximalValueDataLen);
                            }
                            else
                            {
                                memcpy(lpData, pTempValueData, *lpcbData);
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d with name=%s using even though ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        Done = true;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  requested index=%d save name=%s previous although ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        keyChildEnumerations[1].ValueNames.push_back(pTempValueName);
                    }
                }
                else if (result == ERROR_NO_MORE_ITEMS)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d returned result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d ignoring unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
            if (keyChildEnumerations[1].ValidCounts &&
                keyChildEnumerations[1].ValueCount == 0)
            {
                result = ERROR_NO_MORE_ITEMS;
            }
        }


        if (!Done && keyChildEnumerations[2].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[2].ValueCount)
            {
                AnyNewResult = true;
                try
                {
                    //*lpcchValueName = origlpcchValueName;
                    //if (lpcbData != NULL)
                    //{
                    //    *lpcbData = origLpcdData;
                    //}
                  //result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[2].Key, currentListIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
                    MaximalValueNameLen = savedMaximalValueNameLen;
                    MaximalValueDataLen = savedMaximalValueDataLen;
                    result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[2].Key, currentListIndex, pTempValueName, &MaximalValueNameLen, lpReserved, lpType, pTempValueData, &MaximalValueDataLen);
                }
                catch (...)
                {
                    // We sometimes see this happen but not sure why.  Possibly someone manipulated the underlying values?  But in any case we should not crash.
                    result = GetLastError();
                }
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, pTempValueName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + pTempValueName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::string lpValueNameStr = pTempValueName;
                    if (IsValueNameInEnumerationA(lpValueNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected index=%d skipped due to duplication[0]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (IsValueNameInEnumerationA(lpValueNameStr, keyChildEnumerations[1]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected index=%d skipped due to duplication[1]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            if (lpValueName != NULL && lpcchValueName != NULL)
                            {
                                ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(char));
                                if (*lpcchValueName >= MaximalValueNameLen)
                                {
                                    *lpcchValueName = MaximalValueNameLen;
                                    memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(char));
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected override from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(char));
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            if (lpData != NULL && lpcbData != NULL)
                            {
                                ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                                if (*lpcbData >= MaximalValueDataLen)
                                {
                                    *lpcbData = MaximalValueDataLen;
                                    memcpy(lpData, pTempValueData, MaximalValueDataLen);
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected override data from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpData, pTempValueData, *lpcbData);
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected index=%d using result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  reverse redirected index=%d save previous name=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                            keyChildEnumerations[2].ValueNames.push_back(pTempValueName);
                        }
                    }
                    else
                    {
                        // skipping, already logged
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insufficient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {

                        if (lpValueName != NULL && lpcchValueName != NULL)
                        {
                            ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(char));
                            if (*lpcchValueName >= MaximalValueNameLen)
                            {
                                *lpcchValueName = MaximalValueNameLen;
                                memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(char));
                            }
                            else
                            {
                                memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(char));
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }    
                        if (lpData != NULL && lpcbData != NULL)
                        {
                            ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                            if (*lpcbData >= MaximalValueDataLen)
                            {
                                *lpcbData = MaximalValueDataLen;
                                memcpy(lpData, pTempValueData, MaximalValueDataLen);
                            }
                            else
                            {
                                memcpy(lpData, pTempValueData, *lpcbData);
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected index=%d with name=%s using even though ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        Done = true;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  reverse redirected index=%d  name=%s previous although ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        keyChildEnumerations[2].ValueNames.push_back(pTempValueName);
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected index=%d ignoring unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
            if (keyChildEnumerations[2].ValidCounts &&
                keyChildEnumerations[2].ValueCount == 0)
            {
                result = ERROR_NO_MORE_ITEMS;
            }
        }

        if (AnyNewResult)
        {
            if (!Done)
            {
                result = ERROR_NO_MORE_ITEMS;
                if (lpValueName != NULL && lpcchValueName != NULL && *lpcchValueName > 0)
                {
                    lpValueName[0] = '\0';
                    *lpcchValueName = 0;
                }
                if (lpData != NULL && lpcbData != NULL && *lpcbData > 0)
                {
                    *lpData = 0;
                    *lpcbData = 0;
                }
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  Returning ERROR_NO_MORE_ITEMS (normal failure) %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  Returning %S success %s", g_RegModuleName, RegLocalInstance, lpValueName, LStatusToWstring(result).c_str());
            }
        }
        else
        {
            // No keys found at all, just return the original result
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  No redirected or reverse redirected keys found, returning original result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
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
        result = impl::KernelBaseRegEnumValueA(key, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueA, RegEnumValueAFixup);


LSTATUS __stdcall RegEnumValueWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpValueName,
    _In_ _Out_ LPDWORD lpcchValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    DWORD origlpcchValueName = *lpcchValueName;
    DWORD origLpcdData = 0;
    if (lpcbData != NULL)
    {
        origLpcdData = *lpcbData;
    }
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        RegCohorts regCohorts;
        KeyChildEnumerationsW keyChildEnumerations[3];
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  key=0x%x keyname=%s dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str(), dwIndex);
        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:     Buffer size for ValueName=0x%x", g_RegModuleName, RegLocalInstance, origlpcchValueName);
        if (lpType)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:     Type pointer provided", g_RegModuleName, RegLocalInstance);
        }
        if (lpData != NULL && lpcbData != NULL)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:     Buffer size for ValueData=0x%x", g_RegModuleName, RegLocalInstance, origLpcdData);
        }

        // Determine maximal counts and sizes
        DWORD MaximalValueNameLen = 0;
        DWORD MaximalValueDataLen = 0;
        if (HasHKLM2HKCUSpecified())
        {
            regCohorts = GenerateRegCohorts(key, L"", RegLocalInstance);
            if (regCohorts.RedirectionNotPossible == false)
            {
                LSTATUS resOpen = ::RegOpenKey(HKEY_CURRENT_USER, regCohorts.RedirectedPath.substr(18).c_str(), &keyChildEnumerations[0].Key);
                if (resOpen == ERROR_SUCCESS)
                {
                    keyChildEnumerations[0].ValidKey = true;
                    resOpen = ::RegQueryInfoKeyW(keyChildEnumerations[0].Key, NULL, NULL, NULL,
                        &keyChildEnumerations[0].SubKeyCount, &keyChildEnumerations[0].MaxSubKeyNameLen, NULL,
                        &keyChildEnumerations[0].ValueCount, &keyChildEnumerations[0].MaxValueNameLen, &keyChildEnumerations[0].MaxValueLen,
                        NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        keyChildEnumerations[0].ValidCounts = true;
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected MaxValueNameLen=0x%x MaxValueLen=0x%x",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].MaxValueNameLen, keyChildEnumerations[0].MaxValueLen);
                        MaximalValueNameLen = keyChildEnumerations[0].MaxValueNameLen;
                        MaximalValueDataLen = keyChildEnumerations[0].MaxValueLen;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                        keyChildEnumerations[0].SubKeyCount = 0;
                        keyChildEnumerations[0].MaxSubKeyNameLen = 0;
                        keyChildEnumerations[0].ValueCount = 0;
                        keyChildEnumerations[0].MaxValueNameLen = 0;
                        keyChildEnumerations[0].MaxValueLen = 0;
                    }
                }
                else
                {
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected open error=%s",
                        g_RegModuleName, RegLocalInstance,
                        LStatusToWstring(resOpen).c_str());
                    keyChildEnumerations[0].SubKeyCount = 0;
                    keyChildEnumerations[0].MaxSubKeyNameLen = 0;
                    keyChildEnumerations[0].ValueCount = 0;
                    keyChildEnumerations[0].MaxValueNameLen = 0;
                    keyChildEnumerations[0].MaxValueLen = 0;
                }
            }
        }

        keyChildEnumerations[1].Key = key;
        keyChildEnumerations[1].ValidKey = true;
        result = ::RegQueryInfoKeyW(keyChildEnumerations[1].Key, NULL, NULL, NULL,
            &keyChildEnumerations[1].SubKeyCount, &keyChildEnumerations[1].MaxSubKeyNameLen, NULL,
            &keyChildEnumerations[1].ValueCount, &keyChildEnumerations[1].MaxValueNameLen, &keyChildEnumerations[1].MaxValueLen,
            NULL, NULL);
        if (result == ERROR_SUCCESS)
        {
            keyChildEnumerations[1].ValidCounts = true;
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested MaxValueNameLen=0x%x MaxValueLen=0x%x",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].MaxValueNameLen, keyChildEnumerations[1].MaxValueLen);
            if (keyChildEnumerations[1].MaxValueNameLen > MaximalValueNameLen)
            {
                MaximalValueNameLen = keyChildEnumerations[1].MaxValueNameLen;
            }
            if (keyChildEnumerations[1].MaxValueLen > MaximalValueDataLen)
            {
                if (keyChildEnumerations[1].MaxValueLen < 4096)
                {
                    MaximalValueDataLen = keyChildEnumerations[1].MaxValueLen;
                }
                else
                {
                    // Have seen suspiciously large values here 
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW  Limiting maxvaluedatalen to 4096",
                        g_RegModuleName, RegLocalInstance);
                    if (MaximalValueDataLen < 4096)
                    {
                        MaximalValueDataLen = 4096;
                    }
                }
            }
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested error=%s",
                g_RegModuleName, RegLocalInstance,
                LStatusToWstring(result).c_str());
            keyChildEnumerations[1].SubKeyCount = 0;
            keyChildEnumerations[1].MaxSubKeyNameLen = 0;
            keyChildEnumerations[1].ValueCount = 0;
            keyChildEnumerations[1].MaxValueNameLen = 0;
            keyChildEnumerations[1].MaxValueLen = 0;
        }

        bool testDeletionMaker = HasDeletionMarkerSpecified();
        bool testJavaBlocker = HasJavaBlockerSpecified();

        if (HasHKLM2HKCUSpecified())
        {
            if (regCohorts.ReverseRedirectionNotPossible == false)
            {
                LSTATUS resOpen = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &keyChildEnumerations[2].Key);
                if (resOpen == ERROR_SUCCESS)
                {
                    keyChildEnumerations[2].ValidKey = true;
                    resOpen = ::RegQueryInfoKeyW(keyChildEnumerations[2].Key, NULL, NULL, NULL,
                        &keyChildEnumerations[2].SubKeyCount, &keyChildEnumerations[2].MaxSubKeyNameLen, NULL,
                        &keyChildEnumerations[2].ValueCount, &keyChildEnumerations[2].MaxValueNameLen, &keyChildEnumerations[2].MaxValueLen,
                        NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        keyChildEnumerations[1].ValidCounts = true;
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].SubKeyCount, keyChildEnumerations[2].ValueCount);
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected MaxValueNameLen=0x%x MaxValueLen=0x%x",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].MaxValueNameLen, keyChildEnumerations[2].MaxValueLen);
                        if (keyChildEnumerations[2].MaxValueNameLen > MaximalValueNameLen)
                        {
                            MaximalValueNameLen = keyChildEnumerations[2].MaxValueNameLen;
                        }
                        if (keyChildEnumerations[2].MaxValueLen > MaximalValueDataLen)
                        {
                            if (keyChildEnumerations[2].MaxValueLen < 4096)
                            {
                                MaximalValueDataLen = keyChildEnumerations[1].MaxValueLen;
                            }
                            else
                            {
                                // Have seen suspiciously large values here 
                                Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  Limiting maxvaluedatalen to 4096",
                                    g_RegModuleName, RegLocalInstance);
                                if (MaximalValueDataLen < 4096)
                                {
                                    MaximalValueDataLen = 4096;
                                }
                            }
                        }
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                        keyChildEnumerations[2].SubKeyCount = 0;
                        keyChildEnumerations[2].MaxSubKeyNameLen = 0;
                        keyChildEnumerations[2].ValueCount = 0;
                        keyChildEnumerations[2].MaxValueNameLen = 0;
                        keyChildEnumerations[2].MaxValueLen = 0;
                    }
                }
                else
                {
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected open error=%s",
                        g_RegModuleName, RegLocalInstance,
                        LStatusToWstring(resOpen).c_str());
                }
            }
        }


        // Determine buffer sizes for local calls
        ///Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  queried maximum sizes: ValueName=0x%x ValueData=0x%x",
        ///    g_RegModuleName, RegLocalInstance, MaximalValueNameLen, MaximalValueDataLen);
        if (lpValueName != NULL && lpcchValueName != NULL && *lpcchValueName > MaximalValueNameLen)
        {
            MaximalValueNameLen = *lpcchValueName;
        }
        if (lpData != NULL && lpcbData != NULL && *lpcbData > MaximalValueDataLen)
        {
            MaximalValueDataLen = *lpcbData;
        }
        wchar_t* pTempValueName = new wchar_t[(MaximalValueNameLen)+1];
        BYTE* pTempValueData = new byte[MaximalValueDataLen];
        DWORD savedMaximalValueNameLen = MaximalValueNameLen;
        DWORD savedMaximalValueDataLen = MaximalValueDataLen;
        ///Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  Calculated maximum sizes: ValueName=0x%x ValueData=0x%x",
        ///    g_RegModuleName, RegLocalInstance, savedMaximalValueNameLen, savedMaximalValueDataLen);


        // Start enumerating through the possible keys
        DWORD indexesOfPreviousVisibleEnumerations = 0;
        DWORD currentListIndex = 0;
        bool  Done = false;
        bool  AnyNewResult = false;

        if (keyChildEnumerations[0].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[0].ValueCount)
            {
                AnyNewResult = true;
                try
                {
                    //*lpcchValueName = origlpcchValueName;
                    //if (lpcbData != NULL)
                    //{
                    //    *lpcbData = origLpcdData;
                    //}
                  //result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[0].Key, currentListIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
                    MaximalValueNameLen = savedMaximalValueNameLen;
                    MaximalValueDataLen = savedMaximalValueDataLen;
                    result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[0].Key, currentListIndex, pTempValueName, &MaximalValueNameLen, lpReserved, lpType, pTempValueData, &MaximalValueDataLen);
                }
                catch (...)
                {
                    // We sometimes see this happen but not sure why.  Possibly someone manipulated the underlying values?  But in any case we should not crash.
                    result = GetLastError();
                }
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, pTempValueName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + pTempValueName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    // No previous list to compare to
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            if (lpValueName != NULL && lpcchValueName != NULL)
                            {
                                ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(wchar_t));
                                if (*lpcchValueName >= MaximalValueNameLen)
                                {
                                    *lpcchValueName = MaximalValueNameLen;
                                    memcpy(lpValueName, pTempValueName, MaximalValueNameLen*sizeof(wchar_t));
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  redirected override from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(wchar_t));
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            if (lpData != NULL && lpcbData != NULL)
                            {
                                ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                                if (*lpcbData >= MaximalValueDataLen)
                                {
                                    *lpcbData = MaximalValueDataLen;
                                    memcpy(lpData, pTempValueData, MaximalValueDataLen);
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  redirected override data from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpData, pTempValueData, *lpcbData);
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  redirected index=%d using result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected index=%d save previous name='%s'",
                                g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                            keyChildEnumerations[0].ValueNames.push_back(pTempValueName);
                        }
                    }
                    else
                    {
                        // skipping, already said why
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insufficient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  redirected index=%d with name=%s using even though ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        if (lpValueName != NULL && lpcchValueName != NULL)
                        {
                            ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(wchar_t));
                            if (*lpcchValueName >= MaximalValueNameLen)
                            {
                                *lpcchValueName = MaximalValueNameLen;
                                memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(wchar_t));
                            }
                            else
                            {
                                memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(wchar_t));
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        if (lpData != NULL && lpcbData != NULL)
                        {
                            ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                            if (*lpcbData >= MaximalValueDataLen)
                            {
                                *lpcbData = MaximalValueDataLen;
                                memcpy(lpData, pTempValueData, MaximalValueDataLen);
                            }
                            else
                            {
                                memcpy(lpData, pTempValueData, *lpcbData);
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        Done = true;
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected  on index=%d save name=%s although result=ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        keyChildEnumerations[0].ValueNames.push_back(pTempValueName);
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  redirected index=%d ignoring due to unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
            if (keyChildEnumerations[0].ValidCounts &&
                keyChildEnumerations[0].ValueCount == 0)
            {
                result = ERROR_NO_MORE_ITEMS;
            }
        }


        if (!Done && keyChildEnumerations[1].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[1].ValueCount)
            {
                AnyNewResult = true;
                try
                {
                    //*lpcchValueName = origlpcchValueName;
                    //if (lpcbData != NULL)
                    //{
                    //    *lpcbData = origLpcdData;
                    //}
                  //result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[1].Key, currentListIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
                    MaximalValueNameLen = savedMaximalValueNameLen;
                    MaximalValueDataLen = savedMaximalValueDataLen;
                    result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[1].Key, currentListIndex, pTempValueName, &MaximalValueNameLen, lpReserved, lpType, pTempValueData, &MaximalValueDataLen);
                    ///Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:     RAWRETURN requested index=%d result=%s", g_RegModuleName, RegLocalInstance,currentListIndex, LStatusToWstring(result).c_str());
                    ///Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:     RAWRETURN ValueNameLen=0x%x ValueName='%s'", g_RegModuleName, RegLocalInstance, MaximalValueNameLen,pTempValueName);
                    ///Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:     RAWRETURN ValueDataLen=0x%x", g_RegModuleName, RegLocalInstance, MaximalValueDataLen);
                }
                catch (...)
                {
                    // We sometimes see this happen but not sure why.  Possibly someone manipulated the underlying values?  But in any case we should not crash.
                    result = GetLastError();
                }
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, pTempValueName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + pTempValueName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::wstring wlpValueNameStr = pTempValueName;
                    if (IsValueNameInEnumerationW(wlpValueNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested index=%d skipped due to duplication",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            if (lpValueName != NULL && lpcchValueName != NULL)
                            {
                                ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(wchar_t));
                                if (*lpcchValueName >= MaximalValueNameLen)
                                {
                                    *lpcchValueName = MaximalValueNameLen;
                                    memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(wchar_t));
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested override from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(wchar_t));
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            if (lpData != NULL && lpcbData != NULL)
                            {
                                ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                                if (*lpcbData >= MaximalValueDataLen)
                                {
                                    *lpcbData = MaximalValueDataLen;
                                    memcpy(lpData, pTempValueData, MaximalValueDataLen);
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested override data from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpData, pTempValueData, *lpcbData);
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested index=%d using result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested on index=%d save previous name=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                            keyChildEnumerations[1].ValueNames.push_back(pTempValueName);
                        }
                    }
                    else
                    {
                        // skipping
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested skipping hidden or deleted on index=%d",
                            g_RegModuleName, RegLocalInstance, currentListIndex);
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insufficient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested index=%d name=%s using even though ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        if (lpValueName != NULL && lpcchValueName != NULL)
                        {
                            ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(wchar_t));
                            if (*lpcchValueName >= MaximalValueNameLen)
                            {
                                *lpcchValueName = MaximalValueNameLen;
                                memcpy(lpValueName, pTempValueName, MaximalValueNameLen*sizeof(wchar_t));
                            }
                            else
                            {
                                memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(wchar_t));
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        if (lpData != NULL && lpcbData != NULL)
                        {
                            ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                            if (*lpcbData >= MaximalValueDataLen)
                            {
                                *lpcbData = MaximalValueDataLen;
                                memcpy(lpData, pTempValueData, MaximalValueDataLen);
                            }
                            else
                            {
                                memcpy(lpData, pTempValueData, *lpcbData);
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        Done = true;
                    }
                    else
                    {
                        // Add what we have to the list
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested on index=%d save name=%s although result=ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        keyChildEnumerations[1].ValueNames.push_back(pTempValueName);
                    }
                }
                else if (result == ERROR_NO_MORE_ITEMS)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested index=%d returned result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested index=%d returned unexpected result so ignoring result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
            if (keyChildEnumerations[1].ValidCounts &&
                keyChildEnumerations[1].ValueCount == 0)
            {
                result = ERROR_NO_MORE_ITEMS;
            }
        }


        if (!Done && keyChildEnumerations[2].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[2].ValueCount)
            {
                AnyNewResult = true;
                try
                {
                    //*lpcchValueName = origlpcchValueName;
                    //if (lpcbData != NULL)
                    //{
                    //    *lpcbData = origLpcdData;
                    //}
                  //result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[2].Key, currentListIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
                    MaximalValueNameLen = savedMaximalValueNameLen;
                    MaximalValueDataLen = savedMaximalValueDataLen;
                    result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[2].Key, currentListIndex, pTempValueName, &MaximalValueNameLen, lpReserved, lpType, pTempValueData, &MaximalValueDataLen);
                }
                catch (...)
                {
                    // We sometimes see this happen but not sure why.  Possibly someone manipulated the underlying values?  But in any case we should not crash.
                    result = GetLastError();
                }
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, pTempValueName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + pTempValueName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), pTempValueName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::wstring wlpValueNameStr = pTempValueName;
                    if (IsValueNameInEnumerationW(wlpValueNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d skipped due to duplication[0]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (IsValueNameInEnumerationW(wlpValueNameStr, keyChildEnumerations[1]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d skipped due to duplication[1]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            if (lpValueName != NULL && lpcchValueName != NULL)
                            {
                                ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(wchar_t));
                                if (*lpcchValueName >= MaximalValueNameLen)
                                {
                                    *lpcchValueName = MaximalValueNameLen;
                                    memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(wchar_t));
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected override from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(wchar_t));
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            if (lpData != NULL && lpcbData != NULL)
                            {
                                ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                                if (*lpcbData >= MaximalValueDataLen)
                                {
                                    *lpcbData = MaximalValueDataLen;
                                    memcpy(lpData, pTempValueData, MaximalValueDataLen);
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected override data from result=%s",
                                        g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
                                    memcpy(lpData, pTempValueData, *lpcbData);
                                    result = ERROR_MORE_DATA;
                                }
                            }
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d using result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected on index=%d save previous name=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                            keyChildEnumerations[2].ValueNames.push_back(pTempValueName);
                        }
                    }
                    else
                    {
                        // skipping
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected skipping hidden or deleted on index=%d",
                            g_RegModuleName, RegLocalInstance, currentListIndex);
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insufficient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d  using even though ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        if (lpValueName != NULL && lpcchValueName != NULL)
                        {
                            ZeroMemory(lpValueName, (*lpcchValueName) * sizeof(wchar_t));
                            if (*lpcchValueName >= MaximalValueNameLen)
                            {
                                *lpcchValueName = MaximalValueNameLen;
                                memcpy(lpValueName, pTempValueName, MaximalValueNameLen * sizeof(wchar_t));
                            }
                            else
                            {
                                memcpy(lpValueName, pTempValueName, (*lpcchValueName) * sizeof(wchar_t));
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        if (lpData != NULL && lpcbData != NULL)
                        {
                            ZeroMemory(lpData, (*lpcbData) * sizeof(char));
                            if (*lpcbData >= MaximalValueDataLen)
                            {
                                *lpcbData = MaximalValueDataLen;
                                memcpy(lpData, pTempValueData, MaximalValueDataLen);
                            }
                            else
                            {
                                memcpy(lpData, pTempValueData, *lpcbData);
                                //already set: result = ERROR_MORE_DATA;
                            }
                        }
                        Done = true;
                    }
                    else
                    {
                        // Add what we have to the list
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected on index=%d save name=%s although result=ERROR_MORE_DATA",
                            g_RegModuleName, RegLocalInstance, currentListIndex, pTempValueName);
                        keyChildEnumerations[2].ValueNames.push_back(pTempValueName);
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d returned unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
            if (keyChildEnumerations[2].ValidCounts &&
                keyChildEnumerations[2].ValueCount == 0)
            {
                result = ERROR_NO_MORE_ITEMS;
            }
        }
       
        if (AnyNewResult)
        {
            if (!Done)
            {
                result = ERROR_NO_MORE_ITEMS;
                if (lpValueName != NULL && lpcchValueName != NULL && *lpcchValueName > 0)
                {
                    lpValueName[0] = '\0';
                    *lpcchValueName = 0;
                }
                if (lpData != NULL && lpcbData != NULL && *lpcbData > 0)
                {
                    *lpData = 0;
                    *lpcbData = 0;
                }
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  Returning ERROR_NO_MORE_ITEMS (normal failure) %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  Returning %s with success %s", g_RegModuleName, RegLocalInstance, lpValueName, LStatusToWstring(result).c_str());
            }
        }
        else
        {
            // No keys found at all, just return the original result
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  No redirected or reverse redirected keys found, returning original result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
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
        result = impl::KernelBaseRegEnumValueW(key, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueW, RegEnumValueWFixup);

#else
LSTATUS __stdcall RegEnumValueAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpValueName,
    _In_ _Out_ LPDWORD lpcchValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;


        std::string keyonlypath = InterpretKeyPath(key);


        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

        bool stillWorking = true;
        DWORD onIndex = dwIndex;
        while (stillWorking)
        {
            result = impl::KernelBaseRegEnumValueA(key, onIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
            if (result == ERROR_SUCCESS)
            {
                std::string sskey = narrow(lpValueName);
                result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
                if (result == ERROR_SUCCESS)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  Returning lpValueName=%S", g_RegModuleName, RegLocalInstance, lpValueName);
                    stillWorking = false;
                }
                else
                {
                    // We have a deletion marker on this particular item, so we need to skip it.
                    // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                    // because we do not have a way to remember this, like done in FindFirstFile.
                    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  DeletionMarker Blocking lpValueName=%S, try again.", g_RegModuleName, RegLocalInstance, lpValueName);
                    onIndex++;
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
                stillWorking = false;;
            }
        }



        if (result == ERROR_ACCESS_DENIED)
        {
            auto functionResult = from_win32(result);
            if (auto lock = acquire_output_lock(function_type::registry, functionResult))
            {
                try
                {
                    LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
                    LogFunctionResultInstance(LogLevel_DebugIntermediate, RegLocalInstance, functionResult);
                    if (function_failed(functionResult))
                    {
                        LogWin32ErrorInstance(LogLevel_DebugIntermediate, RegLocalInstance, (DWORD)result);
                    }
                    LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance);
                    Log(LogLevel_DebugIntermediate, L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegEnumValueA logging failure.\n", g_RegModuleName, RegLocalInstance);
                }
            }
        }
    }
    else
    {
        result = impl::KernelBaseRegEnumValueA(key, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueA, RegEnumValueAFixup);

LSTATUS __stdcall RegEnumValueWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpValueName,
    _In_ _Out_ LPDWORD lpcchValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;


        std::string keyonlypath = InterpretKeyPath(key);


        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

        bool stillWorking = true;
        DWORD onIndex = dwIndex;
        while (stillWorking)
        {
            result = impl::KernelBaseRegEnumValueW(key, onIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
            if (result == ERROR_SUCCESS)
            {
                std::string sskey = narrow(lpValueName);
                result = RegFixupDeletionMarker(LogLevel_DebugIntermediate, keyonlypath, sskey, RegLocalInstance);
                if (result == ERROR_SUCCESS)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  Returning lpValueName=%s", g_RegModuleName, RegLocalInstance, lpValueName);
                    stillWorking = false;
                }
                else
                {
                    // We have a deletion marker on this particular item, so we need to skip it.
                    // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                    // because we do not have a way to remember this, like done in FindFirstFile.
                    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValue:  DeletionMarker Blocking lpValueName=%s, try again.", g_RegModuleName, RegLocalInstance, lpValueName);
                    onIndex++;
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValue:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
                stillWorking = false;;
            }
        }



        if (result == ERROR_ACCESS_DENIED)
        {
            auto functionResult = from_win32(result);
            if (auto lock = acquire_output_lock(function_type::registry, functionResult))
            {
                try
                {
                    LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
                    LogFunctionResultInstance(LogLevel_DebugIntermediate, RegLocalInstance, functionResult);
                    if (function_failed(functionResult))
                    {
                        LogWin32ErrorInstance(LogLevel_DebugIntermediate, RegLocalInstance, (DWORD)result);
                    }
                    LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance);
                    Log(LogLevel_DebugIntermediate, L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegEnumValueW logging failure.\n", g_RegModuleName, RegLocalInstance);
                }
            }
        }
    }
    else
    {
        result = impl::KernelBaseRegEnumValueW(key, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueW, RegEnumValueWFixup);
#endif
#else


#endif