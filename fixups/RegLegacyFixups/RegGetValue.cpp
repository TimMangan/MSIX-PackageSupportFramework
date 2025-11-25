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

#ifdef INTERCEPT_KERNELBASE_PlusRegGetValue
LSTATUS __stdcall RegGetValueAFixup(
    _In_ HKEY key,
    _In_opt_ LPCSTR lpSubKey,
    _In_opt_ LPCSTR lpValue,
    _In_opt_ DWORD dwFlags,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;
        DWORD dwType;


        std::string keyOnlyPath = InterpretKeyPath(key);


        std::string sSubKey = "NULL";
        std::string sValue = "NULL";
        if (lpSubKey != NULL)
            sSubKey = lpSubKey;
        if (lpValue != NULL)
            sValue = lpValue;
        Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueA:  key=0x%x keyname=%S SubKey=%S SubName=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyOnlyPath.c_str(), sSubKey.c_str(), sValue.c_str());



        bool testDeletionMaker = HasDeletionMarkerSpecified();
        if (testDeletionMaker)
        {
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, sSubKey.c_str(), RegLocalInstance);
            if (result != ERROR_SUCCESS)
            {

                Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueA blocked by deletion marker: key=%S subkey=%S", g_RegModuleName, RegLocalInstance, keyOnlyPath.c_str(), sSubKey.c_str());
                result = ERROR_FILE_NOT_FOUND;
                return result;
            }
        }

        // JavaBlocker not needed on this intercept.

        RegCohorts regCohorts;
#if TRYHKLM2HKCU
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegGetValueA:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                regCohorts = GenerateRegCohorts(key, widen(sSubKey), RegLocalInstance);

                if (regCohorts.RedirectionNotPossible == false)
                {
                    // If redirection is possible, this is what we must do when creating the key.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegGetValueA is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefix = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameW;
                    if (regCohorts.RedirectedPath.length() != prefix.length())
                    {
                        LSTATUS altResult = ::RegOpenKey(HKEY_CURRENT_USER, regCohorts.RedirectedPath.substr(18).c_str(), &altKey);
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = impl::KernelBaseRegGetValueA(altKey, "", lpValue, dwFlags, &dwType, lpData, lpcbData);
                            RegCloseKey(altKey);
                            if (result == ERROR_SUCCESS)
                            {    
                                if (lpDwType != NULL)
                                {
                                    *lpDwType = dwType;
                                }
                                try
                                {
                                    StoreAndLogRegistryValueA(LogLevel_DebugIntermediate, dwType, lpData, lpcbData, L"RegGetValueA", RegLocalInstance);
                                }
                                catch (...)
                                {
                                    Log(LogLevel_Exception, L"[%s%d] RegGetValueA exception logging captured value.  May be ignored", g_RegModuleName, RegLocalInstance);
                                }
                                Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueA redirected result=0x%x, path=%s", g_RegModuleName, RegLocalInstance, result, regCohorts.RedirectedPath.c_str());
                                return result;
                            }
                        }
                    }
                }
            }
            catch (...)
            {
                // If anything goes wrong, just do the normal call
                Log(LogLevel_Exception, L"[%s%d] RegGetValueA redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
            }
        }
#endif



        // Try as requested
        result = impl::KernelBaseRegGetValueA(key, lpSubKey, lpValue, dwFlags, &dwType, lpData, lpcbData);
        if (result == ERROR_SUCCESS)
        {
            if (lpDwType != NULL)
            {
                *lpDwType = dwType;
            }
            try
            {
                StoreAndLogRegistryValueA(LogLevel_DebugIntermediate, dwType, lpData, lpcbData, L"RegGetValueA", RegLocalInstance);
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegGetValueA exception logging captured value.  May be ignored", g_RegModuleName, RegLocalInstance);
            }
            Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueA requested result=%s, path=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), regCohorts.RequestedPath.c_str());
            return result;
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueA:  non-redirected failure %s.", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        }

#if TRYHKLM2HKCU
        // reverse redirection attempt if not found
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                if (regCohorts.ReverseRedirectionNotPossible == false)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegGetValueA is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefix = L"HKEY_LOCAL_MACHINE\\" + HKLM2HKCU_RedirNameW;
                    if (regCohorts.RedirectedPath.length() != prefix.length())
                    {
                        LSTATUS altResult = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &altKey);
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = impl::KernelBaseRegGetValueA(altKey, lpSubKey, lpValue, dwFlags, &dwType, lpData, lpcbData);
                            RegCloseKey(altKey);
                            if (result == ERROR_SUCCESS)
                            {
                                if (lpDwType != NULL)
                                {
                                    *lpDwType = dwType;
                                }
                                try
                                {
                                    StoreAndLogRegistryValueA(LogLevel_DebugIntermediate, dwType, lpData, lpcbData, L"RegGetValueA", RegLocalInstance);
                                }
                                catch (...)
                                {
                                    Log(LogLevel_Exception, L"[%s%d] RegGetValueA exception logging captured value.  May be ignored", g_RegModuleName, RegLocalInstance);
                                }
                                Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueA reverse redirected result=%s, path=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), regCohorts.RedirectedPath.c_str());
                                return result;
                            }
                        }
                    }
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegGetValueA:  Exception thrown.", g_RegModuleName, RegLocalInstance);
            }
        }
#endif

        Log(LogLevel_DebugBasic, L"[%s%d] tRegGetValueA: return=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
    }
    else
    { 
        result = impl::KernelBaseRegGetValueA(key, lpSubKey, lpValue, dwFlags, lpDwType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegGetValueA, RegGetValueAFixup);

LSTATUS __stdcall RegGetValueWFixup(
    _In_ HKEY key,
    _In_opt_ LPCWSTR lpSubKey,
    _In_opt_ LPCWSTR lpValue,
    _In_opt_ DWORD dwFlags,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    DWORD dwType;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;


        std::string keyOnlyPath = InterpretKeyPath(key);


        std::string sSubKey = "NULL";
        std::string sValue = "NULL";
        if (lpSubKey != NULL)
            sSubKey = narrow(lpSubKey);
        if (lpValue != NULL)
            sValue = narrow(lpValue);
        Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueW:  key=0x%x keyname=%S SubKey=%S SubName=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyOnlyPath.c_str(), sSubKey.c_str(), sValue.c_str());



        bool testDeletionMaker = HasDeletionMarkerSpecified();
        if (testDeletionMaker)
        {
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, sSubKey.c_str(), RegLocalInstance);
            if (result != ERROR_SUCCESS)
            {

                Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueW blocked by deletion marker: key=%S subkey=%S", g_RegModuleName, RegLocalInstance, keyOnlyPath.c_str(), sSubKey.c_str());
                result = ERROR_FILE_NOT_FOUND;
                return result;
            }
        }

        // JavaBlocker not needed on this intercept.

        RegCohorts regCohorts;
#if TRYHKLM2HKCU
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegGetValueW:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                regCohorts = GenerateRegCohorts(key, widen(sSubKey), RegLocalInstance);

                if (regCohorts.RedirectionNotPossible == false)
                {
                    // If redirection is possible, this is what we must do when creating the key.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegGetValueW is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefix = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameW;
                    if (regCohorts.RedirectedPath.length() != prefix.length())
                    {
                        LSTATUS altResult = ::RegOpenKey(HKEY_CURRENT_USER, regCohorts.RedirectedPath.substr(18).c_str(), &altKey);
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = impl::KernelBaseRegGetValueW(altKey, L"", lpValue, dwFlags, &dwType, lpData, lpcbData);
                            RegCloseKey(altKey);
                            if (result == ERROR_SUCCESS)
                            {
                                if (lpDwType != NULL)
                                {
                                    *lpDwType = dwType;
                                }
                                StoreAndLogRegistryValueW(LogLevel_DebugIntermediate, dwType, lpData, lpcbData, L"RegGetValueExW", RegLocalInstance);
                                Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueW redirected result=%s, path=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), regCohorts.RedirectedPath.c_str());
                                return result;
                            }
                        }
                    }
                }
            }
            catch (...)
            {
                // If anything goes wrong, just do the normal call
                Log(LogLevel_Exception, L"[%s%d] RegGetValueW redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
            }
        }
#endif


        result = impl::KernelBaseRegGetValueW(key, lpSubKey, lpValue, dwFlags, &dwType, lpData, lpcbData);
        if (result == ERROR_SUCCESS)
        {
            if (lpDwType != NULL)
            {
                *lpDwType = dwType;
            }
            try
            {
                StoreAndLogRegistryValueW(LogLevel_DebugIntermediate, dwType, lpData, lpcbData, L"RegGetValueW", RegLocalInstance);
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegGetValueW exception logging captured value.  May be ignored", g_RegModuleName, RegLocalInstance);
            }
            Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueW requested result=%s, path=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), regCohorts.RequestedPath.c_str());
            return result;
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueW:  non-redirected failure %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        }

#if TRYHKLM2HKCU
        // reverse redirection attempt if not found
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                if (regCohorts.ReverseRedirectionNotPossible == false)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegGetValueW is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefix = L"HKEY_LOCAL_MACHINE\\" + HKLM2HKCU_RedirNameW;
                    if (regCohorts.RedirectedPath.length() != prefix.length())
                    {
                        LSTATUS altResult = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &altKey);
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = impl::KernelBaseRegGetValueW(altKey, lpSubKey, lpValue, dwFlags, &dwType, lpData, lpcbData);
                            RegCloseKey(altKey);
                            if (result == ERROR_SUCCESS)
                            {
                                if (lpDwType != NULL)
                                {
                                    *lpDwType = dwType;
                                }
                                StoreAndLogRegistryValueW(LogLevel_DebugIntermediate, dwType, lpData, lpcbData, L"RegGetValueExW", RegLocalInstance);
                                Log(LogLevel_DebugBasic, L"[%s%d] RegGetValueW reverse redirected result=%s, path=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), regCohorts.RedirectedPath.c_str());
                                return result;
                            }
                        }
                    }
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegGetValueW:  Exception thrown.", g_RegModuleName, RegLocalInstance);
            }
        }
#endif


        Log(LogLevel_DebugBasic, L"[%s%d] tRegGetValueW return=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
    }
    else
    {
        result = impl::KernelBaseRegGetValueW(key, lpSubKey, lpValue, dwFlags, lpDwType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegGetValueW, RegGetValueWFixup);

#endif