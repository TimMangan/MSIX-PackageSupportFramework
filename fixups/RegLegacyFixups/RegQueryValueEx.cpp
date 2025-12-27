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
#pragma comment(linker, "/EXPORT:RegQueryValueExAFixupAnsi_Fixup=impl::_KernelBaseRegQueryValueExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegQueryValueExWFixupWide_Fixup=impl::_KernelBaseRegQueryValueExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegQueryValueExAFixupAnsi_Fixup=impl::KernelBaseRegQueryValueExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegQueryValueExWFixupWide_Fixup=impl::KernelBaseRegQueryValueExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


#if INTERCEPT_KERNELBASE
LSTATUS __stdcall RegQueryValueExAFixup(
    _In_ HKEY key,
    _In_opt_ LPCSTR lpValueName,
    LPDWORD lpReservered,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{

    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        try
        {
            std::string keyOnlyPath = InterpretKeyPath(key);

            DWORD inputDataSize = 0;
            if (lpData != NULL && lpcbData != NULL)
            {
                inputDataSize = *lpcbData;
            }

            std::string sValueName = "NULL";
            if (lpValueName != NULL)
                sValueName = lpValueName;
            Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA:  key=0x%x keyname=%S ValueName=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyOnlyPath.c_str(), sValueName.c_str());
            if (lpData != NULL && lpcbData != NULL)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:     Buffer size for Data=0x%x", g_RegModuleName, RegLocalInstance, inputDataSize);
            }


            bool testDeletionMaker = HasDeletionMarkerSpecified();
            if (testDeletionMaker)
            {
                result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, sValueName.c_str(), RegLocalInstance);
                if (result != ERROR_SUCCESS)
                {

                    Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA blocked by deletion marker: key=%S subkey=%S", g_RegModuleName, RegLocalInstance, keyOnlyPath.c_str(), sValueName.c_str());
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
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                    regCohorts = GenerateRegCohorts(key, L"", RegLocalInstance);

                    if (regCohorts.RedirectionNotPossible == false)
                    {
                        // If redirection is possible, this is what we must do when creating the key.
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                        HKEY  altKey;
                        std::wstring prefix = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameOnlyW;
                        if (regCohorts.RedirectedPath.length() != prefix.length())
                        {
                            LSTATUS altResult = ::RegOpenKey(HKEY_CURRENT_USER, regCohorts.RedirectedPath.substr(18).c_str(), &altKey);
                            if (altResult == ERROR_ALREADY_EXISTS ||
                                altResult == ERROR_SUCCESS)
                            {
                                result = impl::KernelBaseRegQueryValueExA(altKey, lpValueName, lpReservered, lpDwType, lpData, lpcbData);
                                RegCloseKey(altKey);
                                if (result == ERROR_SUCCESS)
                                {
                                    try
                                    {
                                        LogRegistryValueA(LogLevel_DebugIntermediate, lpDwType, lpData, lpcbData, L"RegQueryValueExA", RegLocalInstance);
                                    }
                                    catch (...)
                                    {
                                        Log(LogLevel_Exception, L"[%s%d] RegGetQueryValueExA exception logging captured value.  May be ignored", g_RegModuleName, RegLocalInstance);
                                    }
                                    Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA redirected result=0x%x, path=%s", g_RegModuleName, RegLocalInstance, result, regCohorts.RedirectedPath.c_str());
                                    return result;
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA redirected path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA redirected parent key path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(altResult).c_str());
                            }
                        }
                    }
                }
                catch (...)
                {
                    // If anything goes wrong, just do the normal call
                    Log(LogLevel_Exception, L"[%s%d] RegQueryValueExA redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
                }
            }
#endif


            if (lpData != NULL && lpcbData != NULL)
            {
                *lpcbData = inputDataSize;
            }
            result = impl::KernelBaseRegQueryValueExA(key, lpValueName, lpReservered, lpDwType, lpData, lpcbData);
            Log(LogLevel_DebugMaximum, L"[%s%d] RegQueryValueExA requested result=0x%x", g_RegModuleName, RegLocalInstance, result);
            if (result == ERROR_SUCCESS)
            {
                try
                {
                    LogRegistryValueA(LogLevel_DebugIntermediate, lpDwType, lpData, lpcbData, L"RegQueryValueExA", RegLocalInstance);
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegGetQueryValueExA exception logging captured value.  May be ignored", g_RegModuleName, RegLocalInstance);
                }
                Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA requested path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
                return result;
            }
            else
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA requested path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
            }

#if TRYHKLM2HKCU
            // reverse redirection attempt if not found
            if (HasHKLM2HKCUSpecified())
            {
                try
                {
                    if (regCohorts.ReverseRedirectionNotPossible == false)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                        HKEY  altKey;
                        std::wstring prefix = L"HKEY_LOCAL_MACHINE\\" + HKLM2HKCU_RedirNameOnlyW;
                        if (regCohorts.RedirectedPath.length() != prefix.length())
                        {
                            LSTATUS altResult = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &altKey);
                            if (altResult == ERROR_ALREADY_EXISTS ||
                                altResult == ERROR_SUCCESS)
                            {
                                if (lpData != NULL && lpcbData != NULL)
                                {
                                    *lpcbData = inputDataSize;
                                }
                                result = impl::KernelBaseRegQueryValueExA(altKey, lpValueName, lpReservered, lpDwType, lpData, lpcbData);
                                RegCloseKey(altKey);
                                if (result == ERROR_SUCCESS)
                                {
                                    try
                                    {
                                        LogRegistryValueA(LogLevel_DebugIntermediate, lpDwType, lpData, lpcbData, L"RegQueryValueExA", RegLocalInstance);
                                    }
                                    catch (...)
                                    {
                                        Log(LogLevel_Exception, L"[%s%d] RegQueryValueExA exception logging captured value.  May be ignored", g_RegModuleName, RegLocalInstance);
                                    }
                                    Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA reverse redirected path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
                                    return result;
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA reverse redirected path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA reverse redirected parent key path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(altResult).c_str());
                            }
                        }
                    }
                }
                catch (...)
                { 
                    Log(LogLevel_Exception, L"[%s%d] RegQueryValueExA:  Exception thrown.", g_RegModuleName, RegLocalInstance);
                }
            }
#endif

            Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA:  Returning failure %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
        }
        catch (...)
        {
            Log(LogLevel_Exception, L"[%s%d] RegQueryValueExA:  Exception thrown.", g_RegModuleName, RegLocalInstance);
        }
    }
    else
    {
        result = impl::KernelBaseRegQueryValueExA(key, lpValueName, lpReservered, lpDwType, lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegQueryValueExA, RegQueryValueExAFixup);


LSTATUS __stdcall RegQueryValueExWFixup(
    _In_      HKEY key,
    _In_opt_  LPCWSTR lpValueName,
              LPDWORD lpReservered,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        try
        {
            std::string keyOnlyPath = InterpretKeyPath(key);

            DWORD inputDataSize = 0;
            if (lpData != NULL && lpcbData != NULL)
            {
                inputDataSize = *lpcbData;
            }

            std::string sValueName = "NULL";
            if (lpValueName != NULL)
                sValueName = narrow(lpValueName);
            Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW:  key=0x%x keyname=%S ValueName=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyOnlyPath.c_str(), sValueName.c_str());
            if (lpData != NULL && lpcbData != NULL)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:     Buffer size for Data=0x%x", g_RegModuleName, RegLocalInstance, inputDataSize);
            }


            bool testDeletionMaker = HasDeletionMarkerSpecified();
            if (testDeletionMaker)
            {
                result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, sValueName.c_str(), RegLocalInstance);
                if (result != ERROR_SUCCESS)
                {

                    Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW blocked by deletion marker: key=%S subkey=%S", g_RegModuleName, RegLocalInstance, keyOnlyPath.c_str(), sValueName.c_str());
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
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                    regCohorts = GenerateRegCohorts(key, L"", RegLocalInstance);

                    if (regCohorts.RedirectionNotPossible == false)
                    {
                        // If redirection is possible, this is what we must do when creating the key.
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                        HKEY  altKey;
                        std::wstring prefix = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameOnlyW;
                        if (regCohorts.RedirectedPath.length() != prefix.length())
                        {
                            LSTATUS altResult = ::RegOpenKey(HKEY_CURRENT_USER, regCohorts.RedirectedPath.substr(18).c_str(), &altKey);
                            if (altResult == ERROR_ALREADY_EXISTS ||
                                altResult == ERROR_SUCCESS)
                            {
                                result = impl::KernelBaseRegQueryValueExW(altKey, lpValueName, lpReservered, lpDwType, lpData, lpcbData);
                                RegCloseKey(altKey);
                                if (result == ERROR_SUCCESS)
                                {                                    
                                    LogRegistryValueW(LogLevel_DebugIntermediate, lpDwType, lpData, lpcbData, L"RegQueryValueExW", RegLocalInstance);
                                    Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW redirected path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
                                    return result;
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW redirected path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW redirected parent key path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(altResult).c_str());
                            }
                        }
                    }
                }
                catch (...)
                {
                    // If anything goes wrong, just do the normal call
                    Log(LogLevel_Exception, L"[%s%d] RegQueryValueExW redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
                }
            }
#endif

            if (lpData != NULL && lpcbData != NULL)
            {
                *lpcbData = inputDataSize;
            }
            result = impl::KernelBaseRegQueryValueExW(key, lpValueName, lpReservered, lpDwType, lpData, lpcbData);
            if (result == ERROR_SUCCESS)
            {
                LogRegistryValueW(LogLevel_DebugIntermediate, lpDwType, lpData, lpcbData, L"RegQueryValueExW", RegLocalInstance);
                Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW requested path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RequestedPath.c_str(), LStatusToWstring(result).c_str());
                return result;
            }
            else
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW requested path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.c_str(), LStatusToWstring(result).c_str());
            }


#if TRYHKLM2HKCU
            // reverse redirection attempt if not found
            if (HasHKLM2HKCUSpecified())
            {
                try
                {
                    if (regCohorts.ReverseRedirectionNotPossible == false)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                        HKEY  altKey;
                        std::wstring prefix = L"HKEY_LOCAL_MACHINE\\" + HKLM2HKCU_RedirNameOnlyW;
                        if (regCohorts.RedirectedPath.length() != prefix.length())
                        {
                            LSTATUS altResult = ::RegOpenKey(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), &altKey);
                            if (altResult == ERROR_ALREADY_EXISTS ||
                                altResult == ERROR_SUCCESS)
                            {
                                if (lpData != NULL && lpcbData != NULL)
                                {
                                    *lpcbData = inputDataSize;
                                }
                                result = impl::KernelBaseRegQueryValueExW(altKey, lpValueName, lpReservered, lpDwType, lpData, lpcbData);
                                RegCloseKey(altKey);
                                if (result == ERROR_SUCCESS)
                                {
                                    LogRegistryValueW(LogLevel_DebugIntermediate, lpDwType, lpData, lpcbData, L"RegQueryValueExW", RegLocalInstance);
                                    Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW reverse redirected result=0x%x, path=%s", g_RegModuleName, RegLocalInstance, result, regCohorts.RedirectedPath.c_str());
                                    return result;
                                }
                                else
                                {
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW reverse redirected result=0x%x, path=%s", g_RegModuleName, RegLocalInstance, result, regCohorts.RedirectedPath.c_str());
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW reverse redirected parent key result=0x%x, path=%s", g_RegModuleName, RegLocalInstance, altResult, regCohorts.RedirectedPath.c_str());
                            }
                        }
                    }
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegQueryValueEx:  Exception thrown.", g_RegModuleName, RegLocalInstance);
                }
            }
#endif

            Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW:  Returning failure %s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            
        }
        catch (...)
        {
            Log(LogLevel_Exception, L"[%s%d] RegQueryValueEx:  Exception thrown.", g_RegModuleName, RegLocalInstance);
        }
    }
    else
    {
        result = impl::KernelBaseRegQueryValueExW(key, lpValueName, lpReservered, lpDwType,  lpData, lpcbData);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegQueryValueExW, RegQueryValueExWFixup);

#endif