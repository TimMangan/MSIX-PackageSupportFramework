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

#if INTERCEPT_KERNELBASE
#if TRYHKLM2HKCU


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:RegEnumKeyExAFixupAnsi_Fixup=impl::_KernelBaseRegEnumKeyExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegEnumKeyExWFixupWide_Fixup=impl::_KernelBaseRegEnumKeyExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegEnumKeyExAFixupAnsi_Fixup=impl::KernelBaseRegEnumKeyExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegEnumKeyExWFixupWide_Fixup=impl::KernelBaseRegEnumKeyExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

struct KeyChildEnumerationA
{
    std::string Path;
    bool VisibleItem = true;
};
struct KeyChildEnumerationW
{
    std::wstring Path;
    bool VisibleItem = true;
};

LSTATUS __stdcall RegEnumKeyExAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _In_ _Out_ LPSTR lpClass,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _Out_opt_ PFILETIME lpftLastWriteTime)
{
    LSTATUS result = -1;
    DWORD origLpcchName = *lpcchName;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        std::string keyOnlyPath = InterpretKeyPath(key);
        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExA:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyOnlyPath.c_str(), dwIndex);

        RegCohorts regCohorts;
        KeyChildEnumerationsA keyChildEnumerations[3];
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);

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
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA:  redirected SubKeys=%d Values=%d", 
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA:  redirected error=0x%x",
                            g_RegModuleName, RegLocalInstance,
                            resOpen);
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
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA:  requested SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA:  requested error=0x%x",
                g_RegModuleName, RegLocalInstance,
                result);
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
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA:  reverse redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].SubKeyCount, keyChildEnumerations[2].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA:  reverse redirected error=0x%x",
                            g_RegModuleName, RegLocalInstance,
                            resOpen);
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
            while (!Done && currentListIndex < keyChildEnumerations[0].SubKeyCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumKeyExA(keyChildEnumerations[0].Key, currentListIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    // No previous list to compare to
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  redirected index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[0].SubKeys.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  redirected index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done && keyChildEnumerations[1].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[1].SubKeyCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumKeyExA(keyChildEnumerations[1].Key, currentListIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::string lpNameStr = lpName;
                    if (IsSubKeyNameInEnumerationA(lpNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  requested index=%d skipped due to duplication",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  requested index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[1].SubKeys.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  requested index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }

        if (!Done && keyChildEnumerations[2].ValidKey)
        {
            currentListIndex = 0;
            while (!Done && currentListIndex < keyChildEnumerations[2].SubKeyCount)
            {
                *lpcchName = origLpcchName;
                result = impl::KernelBaseRegEnumKeyExA(keyChildEnumerations[2].Key, currentListIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
                if (result == ERROR_SUCCESS)
                {
                    bool isHiddenOrDeletion = false;
                    if (testDeletionMaker)
                    {
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA blocked by deletion marker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::string fullPath = keyOnlyPath + "\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, fullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExA blocked by JavaBlocker: key=%s subkey=%S", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::string lpNameStr = lpName;
                    if (IsSubKeyNameInEnumerationA(lpNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  reverse redirected index=%d skipped due to duplication[0]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (IsSubKeyNameInEnumerationA(lpNameStr, keyChildEnumerations[1]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  reverse redirected index=%d skipped due to duplication[1]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  reverse redirected index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[2].SubKeys.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  reverse redirected index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done)
        {
            result = ERROR_NO_MORE_ITEMS;
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExA:  Returning No more items (normal failure) %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExA:  Returning %S  with success %s", g_RegModuleName, RegLocalInstance, lpName, LStatusToWstring(result).c_str());
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
        result = impl::KernelBaseRegEnumKeyExA(key, dwIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumKeyExA, RegEnumKeyExAFixup);

LSTATUS __stdcall RegEnumKeyExWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _In_ _Out_ LPWSTR lpClass,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _Out_opt_ PFILETIME lpftLastWriteTime)
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

        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExW:  key=0x%x keyname=%s dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, wKeyOnlyPath.c_str(), dwIndex);




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
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW:  redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[0].SubKeyCount, keyChildEnumerations[0].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW:  redirected error=%s",
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
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW:  requested SubKeys=%d Values=%d",
                g_RegModuleName, RegLocalInstance,
                keyChildEnumerations[1].SubKeyCount, keyChildEnumerations[1].ValueCount);
        }
        else
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW:  requested error=%s",
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
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW:  reverse redirected SubKeys=%d Values=%d",
                            g_RegModuleName, RegLocalInstance,
                            keyChildEnumerations[2].SubKeyCount, keyChildEnumerations[2].ValueCount);
                    }
                    else
                    {
                        Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW:  reverse redirected error=%s",
                            g_RegModuleName, RegLocalInstance,
                            LStatusToWstring(resOpen).c_str());
                    }
                }
            }
        }

        DWORD indexesOfPreviousVisibleEnumerations = 0;
        DWORD currentListIndex = 0;
        bool  Done = false;

        if (keyChildEnumerations[0].ValidKey && keyChildEnumerations[0].SubKeyCount > 0)
        {
            currentListIndex = 0;
            keyChildEnumerations[0].SubKeys.reserve(keyChildEnumerations[0].SubKeyCount);
            while (!Done && currentListIndex < keyChildEnumerations[0].SubKeyCount)
            {
                *lpcchName = origLpcchName;
                try
                {
                    result = impl::KernelBaseRegEnumKeyExW(keyChildEnumerations[0].Key, currentListIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
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
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    // No previous list to compare to
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  redirected index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[0].SubKeys.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insuccient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  redirected index=%d used result=%s",
                            g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                        Done = true;
                    }
                    else
                    {
                        // Ignore it as we don't care about this entry
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  redirected index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done && keyChildEnumerations[1].ValidKey && keyChildEnumerations[1].SubKeyCount>0)
        {
            currentListIndex = 0;
            keyChildEnumerations[1].SubKeys.reserve(keyChildEnumerations[1].SubKeyCount);
            while (!Done && currentListIndex < keyChildEnumerations[1].SubKeyCount)
            {
                *lpcchName = origLpcchName;
                try
                {
                    result = impl::KernelBaseRegEnumKeyExW(keyChildEnumerations[1].Key, currentListIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
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
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::wstring wLpNameStr = lpName;
                    if (IsSubKeyNameInEnumerationW(wLpNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  requested index=%d skipped due to duplication",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  requested index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[1].SubKeys.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insuccient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  requested index=%d used result=%s",
                            g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                        Done = true;
                    }
                    else
                    {
                        // Ignore it as we don't care about this entry
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  requested index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }


        if (!Done && keyChildEnumerations[2].ValidKey && keyChildEnumerations[2].SubKeyCount>0)
        {
            currentListIndex = 0;
            keyChildEnumerations[2].SubKeys.reserve(keyChildEnumerations[2].SubKeyCount);
            while (!Done && currentListIndex < keyChildEnumerations[2].SubKeyCount)
            {
                *lpcchName = origLpcchName;
                try
                {
                    result = impl::KernelBaseRegEnumKeyExW(keyChildEnumerations[2].Key, currentListIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
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
                        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, lpName, RegLocalInstance);
                        if (result != ERROR_SUCCESS)
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    if (testJavaBlocker)
                    {
                        std::wstring wFullPath = wKeyOnlyPath + L"\\" + lpName;
                        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
                        {
                            Log(LogLevel_DebugMaximum, L"[%s%d] RegEnumKeyExW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), lpName);
                            isHiddenOrDeletion = true;
                        }
                    }
                    std::wstring wLpNameStr = lpName;
                    if (IsSubKeyNameInEnumerationW(wLpNameStr, keyChildEnumerations[0]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  reverse redirected index=%d skipped due to duplication[0]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (IsSubKeyNameInEnumerationW(wLpNameStr, keyChildEnumerations[1]))
                    {
                        // This one is a duplicate, skip it
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  reverse redirected index=%d skipped due to duplication[1]",
                            g_RegModuleName, RegLocalInstance);
                        isHiddenOrDeletion = true;
                    }
                    if (!isHiddenOrDeletion)
                    {
                        if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  reverse redirected index=%d used result=%s",
                                g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                            Done = true;
                        }
                        else
                        {
                            keyChildEnumerations[2].SubKeys.push_back(lpName);
                        }
                    }
                    else
                    {
                        // skipping
                    }
                }
                else if (result == ERROR_MORE_DATA)
                {
                    // It turns out that we can't trust that max length field.  Some apps call with insuccient buffers.
                    if (currentListIndex + indexesOfPreviousVisibleEnumerations == dwIndex)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  reverse redirected index=%d used result=%s",
                            g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                        Done = true;
                    }
                    else
                    {
                        // Ignore it as we don't care about this entry
                    }
                }
                else
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  reverse redirected index=%d used unexpected result=%s",
                        g_RegModuleName, RegLocalInstance, currentListIndex, LStatusToWstring(result).c_str());
                }
                currentListIndex++;
            }
            indexesOfPreviousVisibleEnumerations += currentListIndex;
        }

        if (!Done)
        {
            result = ERROR_NO_MORE_ITEMS;
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExW:  Returning No more items (normal failure) %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExW:  Returning %s with success %s", g_RegModuleName, RegLocalInstance, lpName, LStatusToWstring(result).c_str());
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
        result = impl::KernelBaseRegEnumKeyExW(key, dwIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumKeyExW, RegEnumKeyExWFixup);

#else
LSTATUS __stdcall RegEnumKeyExAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _In_ _Out_ LPSTR lpClass,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _Out_opt_ PFILETIME lpftLastWriteTime)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;


        std::string keyonlypath = InterpretKeyPath(key);


        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExA:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

        bool stillWorking = true;
        DWORD onIndex = dwIndex;
        while (stillWorking)
        {
            result = impl::KernelBaseRegEnumKeyExA(key, onIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
            if (result == ERROR_SUCCESS)
            {
                std::string sskey = narrow(lpName);
                result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
                if (result == ERROR_SUCCESS)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExA:  Returning lpName=%S", g_RegModuleName, RegLocalInstance, lpName);
                    stillWorking = false;
                }
                else
                {
                    // We have a deletion marker on this particular item, so we need to skip it.
                    // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                    // because we do not have a way to remember this, like done in FindFirstFile.
                    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExA:  DeletionMarker Blocking lpName=%S, try again.", g_RegModuleName, RegLocalInstance, lpName);
                    onIndex++;
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExA:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
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
                    Log(LogLevel_Exception, L"[%s%d] RegEnumKeyExA logging failure.\n", g_RegModuleName, RegLocalInstance);
                }
            }
        }
    }
    else
    {
        result = impl::KernelBaseRegEnumKeyExA(key, dwIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumKeyExA, RegEnumKeyExAFixup);


LSTATUS __stdcall RegEnumKeyExWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _In_ _Out_ LPWSTR lpClass,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _Out_opt_ PFILETIME lpftLastWriteTime)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;


        std::string keyonlypath = InterpretKeyPath(key);


        Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExW:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

        bool stillWorking = true;
        DWORD onIndex = dwIndex;
        while (stillWorking)
        {
            result = impl::KernelBaseRegEnumKeyExW(key, onIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
            if (result == ERROR_SUCCESS)
            {
                std::string sskey = narrow(lpName);
                result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
                if (result == ERROR_SUCCESS)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  Returning lpName=%s", g_RegModuleName, RegLocalInstance, lpName);
                    stillWorking = false;
                }
                else
                {
                    // We have a deletion marker on this particular item, so we need to skip it.
                    // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                    // because we do not have a way to remember this, like done in FindFirstFile.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyExW:  DeletionMarker Blocking lpName=%s, try again.", g_RegModuleName, RegLocalInstance, lpName);
                    onIndex++;
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyExW:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
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
                    Log(LogLevel_Exception, L"[%s%d] RegEnumKeyExW logging failure.\n", g_RegModuleName, RegLocalInstance);
                }
            }
        }
    }
    else
    {
        result = impl::KernelBaseRegEnumKeyExW(key, dwIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumKeyExW, RegEnumKeyExWFixup);

#endif
#else
namespace impl
{
    inline auto RegEnumKeyExA = &::RegEnumKeyExA;
    inline auto RegEnumKeyExW = &::RegEnumKeyExW;
}

//auto RegEnumKeyExImpl = psf::detoured_string_function(&::RegEnumKeyExA, &::RegEnumKeyExW);
//template <typename CharT>
LSTATUS __stdcall RegEnumKeyExAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _In_ _Out_ LPSTR lpClass,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _Out_opt_ PFILETIME lpftLastWriteTime)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyEx:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

    bool stillWorking = true;
    DWORD onIndex = dwIndex;
    while(stillWorking)
    {
        result = impl::RegEnumKeyExA(key, onIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = narrow(lpName);
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyEx:  Returning lpName=%S", g_RegModuleName, RegLocalInstance, lpName);
                stillWorking = false;
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyEx:  DeletionMarker Blocking lpName=%S, try again.", g_RegModuleName, RegLocalInstance, lpName);
                onIndex++;
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyEx:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
            stillWorking = false;;
        }
    }



    if (true) //result == ERROR_ACCESS_DENIED)
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
                LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName,RegLocalInstance);
                Log(LogLevel_DebugIntermediate, L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegEnumKeyEx logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    return result;
}
DECLARE_FIXUP(impl::RegEnumKeyExA, RegEnumKeyExAFixup);

LSTATUS __stdcall RegEnumKeyExWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _In_ _Out_ LPWSTR lpClass,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _Out_opt_ PFILETIME lpftLastWriteTime)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyEx:  key=0x%x keyname=%s dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

    bool stillWorking = true;
    DWORD onIndex = dwIndex;
    while (stillWorking)
    {
        result = impl::RegEnumKeyExW(key, onIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = narrow(lpName);
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumKeyEx:  Returning lpName=%S", g_RegModuleName, RegLocalInstance, lpName);
                stillWorking = false;
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
                Log(LogLevel_DebugBasic, "[%s%d] RegEnumKeyEx:  DeletionMarker Blocking lpName=%S, try again.", g_RegModuleName, RegLocalInstance, lpName);
                onIndex++;
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumKeyEx:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
            stillWorking = false;;
        }
    }



    if (true) //result == ERROR_ACCESS_DENIED)
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
                LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName,RegLocalInstance);
                Log(LogLevel_DebugIntermediate, L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegEnumKeyEx logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    return result;
}
DECLARE_FIXUP(impl::RegEnumKeyExW, RegEnumKeyExWFixup);

#endif