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

#if INTERCEPT_KERNELBASE
#if TRYHKLM2HKCU
LSTATUS __stdcall RegEnumValueAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    DWORD origLpcchName = *lpcchName;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        RegCohorts regCohorts;
        KeyChildEnumerationsA keyChildEnumerations[3];
        std::string keyOnlyPath = InterpretKeyPath(key);
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  key=0x%x keyname=%s dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str(), dwIndex);


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
                        &keyChildEnumerations[0].ValueCount, &keyChildEnumerations[0].MaxValueNameLen, NULL,
                        NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                    }
                }
            }
        }

        keyChildEnumerations[1].Key = key;
        keyChildEnumerations[1].ValidKey = true;
        result = ::RegQueryInfoKeyA(keyChildEnumerations[1].Key, NULL, NULL, NULL,
                                             &keyChildEnumerations[1].SubKeyCount, &keyChildEnumerations[1].MaxSubKeyNameLen, NULL,
                                             &keyChildEnumerations[1].ValueCount, &keyChildEnumerations[1].MaxValueNameLen, NULL,
                                             NULL, NULL);
        if (result == ERROR_SUCCESS)
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  requested SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  requested error=%s",
                g_RegModuleName, RegLocalInstance,
                LStatusToWstring(result).c_str());
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
                                                 &keyChildEnumerations[2].ValueCount, &keyChildEnumerations[2].MaxValueNameLen, NULL,
                                                 NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  reverse redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].SubKeyCount, keyChildEnumerations[2].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA:  reverse redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                    }
                }

            }
        }


        DWORD indexesOfPreviousVisibleEnumerations = 0;
        DWORD currentListIndex = 0;
        bool  Done = false;


        if (keyChildEnumerations[0].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[0].ValueCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[0].Key, dwIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    // No previous list to compare to
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  redirected index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[0].ValueNames.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  redirected index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done && keyChildEnumerations[1].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[1].ValueCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[1].Key, currentListIndex, lpName, lpcchName, lpReserved, NULL, NULL, NULL);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::string lpNameStr = lpName;
                    if (IsValueNameInEnumerationA(lpNameStr, keyChildEnumerations[0]))
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
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[1].ValueNames.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done && keyChildEnumerations[2].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[2].ValueCount)
            {
                *lpcchName = origLpcchName;
               result = impl::KernelBaseRegEnumValueA(keyChildEnumerations[2].Key, currentListIndex, lpName, lpcchName, lpReserved, NULL, NULL, NULL);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::string lpNameStr = lpName;
                    if (IsValueNameInEnumerationA(lpNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected index=%d skipped due to duplication[0]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (IsValueNameInEnumerationA(lpNameStr, keyChildEnumerations[1]))
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
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  reverse redirected index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[2].ValueNames.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  requested index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }

        if (!Done)
        {
            result = ERROR_NO_MORE_ITEMS;
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  Returning No more items (normal failure) %s.", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  Returning success %s with %S.", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), lpName);
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
        result = impl::KernelBaseRegEnumValueA(key, dwIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueA, RegEnumValueAFixup);


LSTATUS __stdcall RegEnumValueWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    DWORD origLpcchName = *lpcchName;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        RegCohorts regCohorts;
        KeyChildEnumerationsW keyChildEnumerations[3];
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  key=0x%x keyname=%s dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str(), dwIndex);

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
                        &keyChildEnumerations[0].ValueCount, &keyChildEnumerations[0].MaxValueNameLen, NULL,
                        NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                    }
                }
            }
        }

        keyChildEnumerations[1].Key = key;
        keyChildEnumerations[1].ValidKey = true;
        result = ::RegQueryInfoKeyW(keyChildEnumerations[1].Key, NULL, NULL, NULL,
            &keyChildEnumerations[1].SubKeyCount, &keyChildEnumerations[1].MaxSubKeyNameLen, NULL,
            &keyChildEnumerations[1].ValueCount, &keyChildEnumerations[1].MaxValueNameLen, NULL,
            NULL, NULL);
        if (result == ERROR_SUCCESS)
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  requested error=%s",
                g_RegModuleName, RegLocalInstance,
                LStatusToWstring(result).c_str());
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
                        &keyChildEnumerations[2].ValueCount, &keyChildEnumerations[2].MaxValueNameLen, NULL,
                        NULL, NULL);
                    if (resOpen == ERROR_SUCCESS)
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].SubKeyCount, keyChildEnumerations[2].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW:  reverse redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                    }
                }
            }
        }



        DWORD indexesOfPreviousVisibleEnumerations = 0;
        DWORD currentListIndex = 0;
        bool  Done = false;


        if (keyChildEnumerations[0].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[0].ValueCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[0].Key, currentListIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    // No previous list to compare to
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  redirected index=%d used result=%s",
                            g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[0].ValueNames.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done && keyChildEnumerations[1].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[1].ValueCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[1].Key, currentListIndex, lpName, lpcchName, lpReserved, NULL, NULL, NULL);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::wstring wLpNameStr = lpName;
                    if (IsValueNameInEnumerationW(wLpNameStr, keyChildEnumerations[0]))
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
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[1].ValueNames.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  requested index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done && keyChildEnumerations[2].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[2].ValueCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumValueW(keyChildEnumerations[2].Key, currentListIndex, lpName, lpcchName, lpReserved, NULL, NULL, NULL);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumValueW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::wstring wLpNameStr = lpName;
                    if (IsValueNameInEnumerationW(wLpNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d skipped due to duplication[0]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (IsValueNameInEnumerationW(wLpNameStr, keyChildEnumerations[1]))
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
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[2].ValueNames.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueW:  reverse redirected index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }
       
        if (!Done)
        {
            result = ERROR_NO_MORE_ITEMS;
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  Returning No more items (normal failure) %s.", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  Returning success %s with %s.", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), lpName);
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
        result = impl::KernelBaseRegEnumValueW(key, dwIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueW, RegEnumValueWFixup);

#else
LSTATUS __stdcall RegEnumValueAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
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
            result = impl::KernelBaseRegEnumValueA(key, onIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
            if (result == ERROR_SUCCESS)
            {
                std::string sskey = narrow(lpName);
                result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
                if (result == ERROR_SUCCESS)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  Returning lpName=%S", g_RegModuleName, RegLocalInstance, lpName);
                    stillWorking = false;
                }
                else
                {
                    // We have a deletion marker on this particular item, so we need to skip it.
                    // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                    // because we do not have a way to remember this, like done in FindFirstFile.
                    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  DeletionMarker Blocking lpName=%S, try again.", g_RegModuleName, RegLocalInstance, lpName);
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
        result = impl::KernelBaseRegEnumValueA(key, dwIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueA, RegEnumValueAFixup);

LSTATUS __stdcall RegEnumValueWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
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
            result = impl::KernelBaseRegEnumValueW(key, onIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
            if (result == ERROR_SUCCESS)
            {
                std::string sskey = narrow(lpName);
                result = RegFixupDeletionMarker(LogLevel_DebugIntermediate, keyonlypath, sskey, RegLocalInstance);
                if (result == ERROR_SUCCESS)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  Returning lpName=%s", g_RegModuleName, RegLocalInstance, lpName);
                    stillWorking = false;
                }
                else
                {
                    // We have a deletion marker on this particular item, so we need to skip it.
                    // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                    // because we do not have a way to remember this, like done in FindFirstFile.
                    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValue:  DeletionMarker Blocking lpName=%s, try again.", g_RegModuleName, RegLocalInstance, lpName);
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
        result = impl::KernelBaseRegEnumValueW(key, dwIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueW, RegEnumValueWFixup);
#endif
#else


#endif