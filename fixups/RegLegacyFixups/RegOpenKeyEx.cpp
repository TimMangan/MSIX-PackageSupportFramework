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


#if INTERCEPT_KERNELBASE

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:RegOpenKeyExAFixupAnsi_Fixup=impl::_KernelBaseRegOpenKeyExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegOpenKeyExWFixupWide_Fixup=impl::_KernelBaseRegOpenKeyExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegOpenKeyExAFixupAnsi_Fixup=impl::KernelBaseRegOpenKeyExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegOpenKeyExWFixupWide_Fixup=impl::KernelBaseRegOpenKeyExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

LSTATUS __stdcall RegOpenKeyExAFixup(
    _In_ HKEY key,
    _In_ const char* subKey,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        if (subKey != NULL)
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA(KernelBase): key=0x%x subKey=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
        else
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA(KernelBase): key=0x%x subKey=NULL", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key);


        std::string keyOnlyPath = InterpretKeyPath(key);
        std::string keyPath;
        if (subKey != NULL)
        {
            keyPath = keyOnlyPath + "\\" + InterpretStringA(subKey);
        }
        else
        {
            keyPath = keyOnlyPath;
        }
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);
        std::string aSubKey;
        if (subKey != NULL)
        {
            aSubKey = InterpretStringA(subKey);
        }

        bool testDeletionMaker = HasDeletionMarkerSpecified();
        bool testJavaBlocker = HasJavaBlockerSpecified();
        if (testDeletionMaker)
        {
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, subKey, RegLocalInstance);
            if (result != ERROR_SUCCESS)
            {
                if (subKey != NULL)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA blocked by deletion marker: key=%s subkey=NULL", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str());
                }
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
                wFullPath += widen(subKey);
            }
            if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
            {
                if (subKey != NULL)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyEx blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyEx blocked by JavaBlocker: key=%s subkey=NULL", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str());
                }
                result = ERROR_PATH_NOT_FOUND;
                resultKey = NULL;
                return result;
            }
        }



        REGSAM samModified = RegFixupSam(LogLevel_DebugMaximum, keyPath, samDesired, RegLocalInstance);
        RegCohorts regCohorts;

#if TRYHKLM2HKCU
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                std::wstring wSubKey;
                if (subKey != NULL)
                {
                    wSubKey = InterpretStringW(subKey);
                }
                regCohorts = GenerateRegCohorts(key, wSubKey, RegLocalInstance);

                if (regCohorts.RedirectionNotPossible == false)
                {
                    // If redirection is possible, this is what we must do when creating the key.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefixCU = HKCU_RedirNameW;
                    if (regCohorts.RedirectedPath.length() == prefixCU.length())
                    {
                        result = impl::KernelBaseRegOpenKeyExW(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), options, samModified, resultKey);
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA Creation of  base key %s result=%s", g_RegModuleName, RegLocalInstance, HKLM2HKCU_RedirNameOnlyW.c_str(), LStatusToWstring(result).c_str());
                    }
                    else
                    {
                        LSTATUS altResult = ::RegCreateKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), &altKey);
                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = impl::KernelBaseRegOpenKeyExW(altKey, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), options, samModified, resultKey);
                            RegCloseKey(altKey);
                            if (result == ERROR_SUCCESS)
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA redirected key=0x%x path=%s success result=%s", g_RegModuleName, RegLocalInstance, *resultKey, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), LStatusToWstring(result).c_str());
                                return result;
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA redirected fail, path=%s result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), LStatusToWstring(result).c_str());
                            }
                        }
                        else
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA Unable to create parent redirection base key?  result err=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(altResult).c_str());
                        }
                    }
                }
            }
            catch (...)
            {
                // If anything goes wrong, just do the normal call
                Log(LogLevel_Exception, L"[%s%d] RegOpenKeyExA redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
            }
        }
#endif

        result = impl::KernelBaseRegOpenKeyExA(key, subKey, options, samModified, resultKey);
        if (result != ERROR_SUCCESS)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA requested key=0x%x Result=%s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());

#if TRYHKLM2HKCU
            if (HasHKLM2HKCUSpecified())
            {
                if (regCohorts.ReverseRedirectionNotPossible == false)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    DWORD RememberLastError = GetLastError();
                    HKEY altKey;
                    LSTATUS altResult = impl::KernelBaseRegOpenKeyExW(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), options, samModified, &altKey);
                    if (altResult != ERROR_SUCCESS)
                    {
                        SetLastError(RememberLastError);
                    }
                    else
                    {
                        result = altResult;
                        if (*resultKey != NULL)
                        {
                            RegCloseKey(*resultKey);
                        }
                        *resultKey = altKey;
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA returns key=0x%x using reverse HKCU replacement. result success %s", g_RegModuleName, RegLocalInstance,  *resultKey, LStatusToWstring(result).c_str());
                    }
                }
            }
#endif
        }

        if (result != ERROR_SUCCESS)
        {
            if (aSubKey.find("PSF_READY_MARKER_") != std::string::npos)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA indicates that PSF injections are complete and the process is ready to run. Result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA key=0x%x result=SUCCESS %s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());
        }
    }
    else
    {
        result = impl::KernelBaseRegOpenKeyExA(key, subKey, options, samDesired, resultKey);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegOpenKeyExA, RegOpenKeyExAFixup);


LSTATUS __stdcall RegOpenKeyExWFixup(
    _In_ HKEY key,
    _In_ const wchar_t* subKey,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey)
{
    LSTATUS result = -1;
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;

        if (subKey != NULL)
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW(KernelBase): key=0x%x subKey=%ls", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
        else
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW(KernelBase): key=0x%x subKey=NULL", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key);

        std::string keyOnlyPath = InterpretKeyPath(key);
        std::string keyPath;
        if (subKey != NULL)
        {
            keyPath = keyOnlyPath + "\\" + InterpretStringA(subKey);
        }
        else
        {
            keyPath = keyOnlyPath;
        }
        std::wstring wKeyOnlyPath = InterpretKeyPathW(key);
        std::wstring wSubKey;
        if (subKey != NULL)
        {
            wSubKey = InterpretStringW(subKey);
        }

        bool testDeletionMaker = HasDeletionMarkerSpecified();
        bool testJavaBlocker = HasJavaBlockerSpecified();
        if (testDeletionMaker)
        {
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, subKey, RegLocalInstance);
            if (result != ERROR_SUCCESS)
            {
                if (subKey != NULL)
                {
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegOpenKeyExW blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), subKey);
                }
                else
                {
                    Log(LogLevel_DebugMaximum, L"[%s%d] RegOpenKeyExW blocked by deletion marker: key=%s subkey=NULL", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str());
                }
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
                wFullPath += widen(subKey);
            }
            if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
            {
                if (subKey != NULL)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), subKey);
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW blocked by JavaBlocker: key=%s subkey=NULL", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str());
                }
                result = ERROR_PATH_NOT_FOUND;
                resultKey = NULL;
                return result;
            }
        }

        REGSAM samModified = RegFixupSam(LogLevel_DebugMaximum, keyPath, samDesired, RegLocalInstance);
        RegCohorts regCohorts;


#if TRYHKLM2HKCU
        if (HasHKLM2HKCUSpecified())
        {
            try
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW:  HKLM2HKCU specified", g_RegModuleName, RegLocalInstance);
                regCohorts = GenerateRegCohorts(key, wSubKey, RegLocalInstance);

                if (regCohorts.RedirectionNotPossible == false)
                {
                    // If redirection is possible, this is what we must do when creating the key.
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    HKEY  altKey;
                    std::wstring prefixCU = HKCU_RedirNameW; // L"HKEY_CURRENT_USER\\vHKLM_Redirection"
                    if (regCohorts.RedirectedPath.length() == prefixCU.length())
                    {
                        result = impl::KernelBaseRegOpenKeyExW(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), options, samModified, resultKey);
                        Log(LogLevel_DebugIntermediate, L"[%s%d] ::KernelBaseRegOpenKeyExW Creation of  base key %s result=%s", g_RegModuleName, RegLocalInstance, HKLM2HKCU_RedirNameOnlyW.c_str(), LStatusToWstring(result).c_str());
                    }
                    else
                    {
                        LSTATUS altResult = ::RegCreateKey(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), &altKey);
                        Log(LogLevel_DebugBasic, L"[%s%d] ::RegCreateKey redirected base Key=0x%x HKCU path=%s success result=%s, ", g_RegModuleName, RegLocalInstance, altKey, HKLM2HKCU_RedirNameOnlyW.c_str(), LStatusToWstring(altResult).c_str());

                        if (altResult == ERROR_ALREADY_EXISTS ||
                            altResult == ERROR_SUCCESS)
                        {
                            result = impl::KernelBaseRegOpenKeyExW(altKey, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), options, samModified, resultKey);
                            RegCloseKey(altKey);
                            if (result == ERROR_SUCCESS)
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] ::KernelBaseRegOpenKeyExW redirected Key=0x%x path=%s success result=%s, ", g_RegModuleName, RegLocalInstance, *resultKey, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), LStatusToWstring(result).c_str());
                                return result;
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] ::KernelBaseRegOpenKeyExW redirected path=%s fail result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RedirectedPath.substr(prefixCU.length() + 1).c_str(), LStatusToWstring(result).c_str());
                            }
                        }
                        else
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] ::RegOpenKeyExW Unable to create parent redirection base key?  result err=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(altResult).c_str());
                        }
                    }
                }
            }
            catch (...)
            {
                // If anything goes wrong, just do the normal call
                Log(LogLevel_Exception, L"[%s%d] RegOpenKeyExW redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
            }
        }

#endif

        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW try as requested", g_RegModuleName, RegLocalInstance);
        result = impl::KernelBaseRegOpenKeyExW(key, subKey, options, samModified, resultKey);


        if (result != ERROR_SUCCESS)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW using requested path result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());

#if TRYHKLM2HKCU
            if (HasHKLM2HKCUSpecified())
            {
                if (regCohorts.ReverseRedirectionNotPossible == false)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW is candidate for reverse HKCU replacement.", g_RegModuleName, RegLocalInstance);
                    DWORD RememberLastError = GetLastError();
                    HKEY AltKey;
                    LSTATUS altResult = impl::KernelBaseRegOpenKeyExW(HKEY_LOCAL_MACHINE, regCohorts.StandardPath.substr(19).c_str(), options, samModified, &AltKey);
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
                        *resultKey = AltKey;
                        result = altResult;
                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW  key=0x%x using reverse HKCU replacement result success %s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());
                    }
                }
            }
#endif
        }

        if (result != ERROR_SUCCESS)
        {
            if (wSubKey.find(L"PSF_READY_MARKER_") != std::wstring::npos)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW  indicates that PSF injections are complete and the process is ready to run. Result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW result=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str());
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW key=0x%x result=SUCCESS %s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());
        }
    }
    else
    {
        result = impl::KernelBaseRegOpenKeyExW(key, subKey, options, samDesired, resultKey);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegOpenKeyExW, RegOpenKeyExWFixup);


#else


auto RegOpenKeyExImpl = psf::detoured_string_function(&::RegOpenKeyExA, &::RegOpenKeyExW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;
    bool isBlocked = false;
    

    if constexpr (psf::is_ansi<CharT>)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyEx:  key=0x%x subkey=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyEx: key=0x%x subKey=%ls", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }

    std::string keyonlypath = InterpretKeyPath(key);
    std::string keypath = keyonlypath + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(LogLevel_DebugMaximum, keypath, samDesired, RegLocalInstance);

    bool hasRedirection = false;
#if TRYHKLM2HKCU
    if (HasHKLM2HKCUSpecified())
    {
        std::string altkeyonlypath;
        if (key == HKEY_LOCAL_MACHINE)
        {
            altkeyonlypath = HKLM2HKCU_Replacement("");
        }
        else if (keyonlypath._Starts_with("HKEY_LOCAL_MACHINE"))
        {
            if (keyonlypath.length == 18)
                altkeyonlypath = HKLM2HKCU_Replacement("");
            else
                altkeyonlypath = HKLM2HKCU_Replacement(keyonlypath.substr(19)));
        }

        if (altkeyonlypath.empty())
        {
            HKEY  altkey;
            LSTATUS altresult = ::RegOpenKeyA(HKEY_CURRENT_USER, altkeyonlypath, &altkey);
            if (altresult == ERROR_FILE_NOT_FOUND)
            {
                alrresult = ::RegCreateKeyA(HKEY_CURRENT_USER, altkeyonlypath, &altkey);
            }
            if (altresult == ERROR_SUCCESS)
            {
                result = RegOpenKeyExImpl(altkey, subKey, options, samModified, resultKey);
                RegCloseKey(altkey);
                hasRedirection = true;
                LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"\tRegOpenKeyEx Redirecting to HKCU", subKey);
            }
        }
    }
#endif

    if (!hasRedirection)
    {
        std::string sskey = narrow(subKey);
        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {
            

            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyEx:  JavaBlocker checking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());

            if (!RegFixupJavaBlocker(LogLevel_DebugMaximum, keypath, RegLocalInstance))
            {
                result = RegOpenKeyExImpl(key, subKey, options, samModified, resultKey);
            }
            else
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyEx:  JavaBlocker Blocking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());
                result = ERROR_PATH_NOT_FOUND;
                resultKey = NULL;
                isBlocked = true;
            }
        }
        else
        {
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
            isBlocked = true;
        }
    }



    if (result != ERROR_SUCCESS)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyEx result=%d", g_RegModuleName, RegLocalInstance, result);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyEx result=SUCCESS key=0x%x", g_RegModuleName, RegLocalInstance,*resultKey);
    }

    if (true) //result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName,RegLocalInstance);
                LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
                LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L" Sub Key", subKey);
                LogRegKeyFlags(LogLevel_DebugIntermediate, RegLocalInstance, options);
                Log(LogLevel_DebugIntermediate, L"[%s%d] samDesired=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samDesired)).c_str());
                if (samDesired != samModified)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] ModifiedSam=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samModified)).c_str());
                }
                LogFunctionResultInstance(LogLevel_DebugIntermediate, RegLocalInstance, functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32ErrorInstance(LogLevel_DebugIntermediate, RegLocalInstance, (DWORD)result);
                }
                Log(LogLevel_DebugIntermediate, L"[%s%d] If a ACCESS DENIED error, this error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegOpenKeyEx logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyExImpl, RegOpenKeyExFixup);

#endif




#if INTERCEPT_NTLL

// NOTE: NtOpenKeyEx is only documented; it has no declaration
NTSTATUS WINAPI NtOpenKeyEx(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG OpenOptions);
auto NtOpenKeyExImpl = WINTERNL_FUNCTION(NtOpenKeyEx);
NTSTATUS __stdcall NtOpenKeyExFixup(
    _Out_ PHANDLE keyHandle,
    _In_ ACCESS_MASK desiredAccess,
    _In_ POBJECT_ATTRIBUTES objectAttributes,
    _In_ ULONG openOptions)
{
    Log(LogLevel_DebugBasic, L"[%s%d] NTOPENKEYEX - view only", g_RegModuleName, RegLocalInstance, g_RegInterceptInstance);
    auto result = NtOpenKeyExImpl(keyHandle, desiredAccess, objectAttributes, openOptions);

    return result;
}
DECLARE_FIXUP(NtOpenKeyExImpl, NtOpenKeyExFixup);

#endif