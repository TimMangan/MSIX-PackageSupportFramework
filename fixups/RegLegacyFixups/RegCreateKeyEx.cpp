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
#pragma comment(linker, "/EXPORT:RegCreateKeyExAFixupAnsi_Fixup=impl::_KernelBaseRegCreateKeyExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegCreateKeyExWFixupWide_Fixup=impl::_KernelBaseRegCreateKeyExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegCreateKeyExHelperWide_Fixup=_RegCreateKeyExHelper.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:RegCreateKeyExAFixupAnsi_Fixup=impl::KernelBaseRegCreateKeyExA.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegCreateKeyExWFixupWide_Fixup=impl::KernelBaseRegCreateKeyExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:RegCreateKeyExHelperWide_Fixup=RegCreateKeyExHelper.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


LSTATUS __stdcall RegCreateKeyExHelper(
    _In_ HKEY key,
    _In_ const wchar_t* subKey,
    _Reserved_ DWORD reserved,
    _In_opt_ wchar_t* classType,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
    _Out_ PHKEY resultKey,
    _Out_opt_ LPDWORD disposition)
{
    DWORD RegLocalInstance = g_RegInterceptInstance;
    LSTATUS result = -1;


    std::wstring wKeyOnlyPath = InterpretKeyPathW(key);
    std::wstring wKeyPath = wKeyOnlyPath + L"\\" + InterpretStringW(subKey);
    REGSAM samModifiedRequested = RegFixupSam(LogLevel_DebugMaximum, wKeyPath, samDesired, RegLocalInstance);
    REGSAM samModifiedRedirected;

    bool testDeletionMaker = false; // Not needed on this intercept: HasDeletionMarkerSpecified();
    bool testJavaBlocker = false;   // Not needed on this intercept: HasJavaBlockerSpecified();

    if (testDeletionMaker)
    {
        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, wKeyOnlyPath, subKey, RegLocalInstance);
        if (result != ERROR_SUCCESS)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyExHelper blocked by deletion marker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
            return result;
        }
    }

    if (testJavaBlocker)
    {
        std::wstring wFullPath = wKeyPath;
        if (subKey != NULL)
        {
            wFullPath += L"\\";
            wFullPath += subKey;
        }
        if (RegFixupJavaBlocker(LogLevel_DebugMaximum, wFullPath, RegLocalInstance))
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyExHelper blocked by JavaBlocker: key=%s subkey=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), InterpretStringW(subKey).c_str());
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
            return result;
        }
    }


#if TRYHKLM2HKCU
    RegCohorts regCohorts;
    if (HasHKLM2HKCUSpecified())
    {
        try
        {
            regCohorts = GenerateRegCohorts(key, InterpretStringW(subKey), RegLocalInstance);
            if (regCohorts.RedirectionNotPossible == false)
            {
                // If redirection is possible, this is what we must do when creating the key.
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegCreateKeyExHelper is candidate for HKCU replacement.", g_RegModuleName, RegLocalInstance);
                samModifiedRedirected = RegFixupSam(LogLevel_DebugMaximum, regCohorts.RedirectedPath, samDesired, RegLocalInstance);
                HKEY  altKey;
                std::wstring prefix = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameOnlyW;
                if (regCohorts.RedirectedPath.length() == prefix.length())
                {
                    result = impl::KernelBaseRegCreateKeyExW(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), reserved, classType, options, samModifiedRedirected, securityAttributes, resultKey, disposition);
                }
                else
                {
                    LSTATUS altResult = ::RegOpenKeyExW(HKEY_CURRENT_USER, HKLM2HKCU_RedirNameOnlyW.c_str(), options, samModifiedRedirected, &altKey);
                    if (altResult == ERROR_ALREADY_EXISTS ||
                        altResult == ERROR_SUCCESS)
                    {
                        result = impl::KernelBaseRegCreateKeyExW(altKey, regCohorts.RedirectedPath.substr(prefix.length() + 1).c_str(), reserved, classType, options, samModifiedRedirected, securityAttributes, resultKey, disposition);
                        RegCloseKey(altKey);
                    }
                }
                Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyExHelper redirected result=%s,key=0x%x path=%s", g_RegModuleName, RegLocalInstance, LStatusToWstring(result).c_str(), *resultKey, regCohorts.RedirectedPath.c_str());
                return result;
            }
        }
        catch (...)
        {
            // If anything goes wrong, just do the normal call
            Log(LogLevel_Exception, L"[%s%d] RegCreateKeyExHelper redirection exception, try original request.\n", g_RegModuleName, RegLocalInstance);
        }
    }
#endif

    // This is the normal processing path.
    
    result = impl::KernelBaseRegCreateKeyExW(key, subKey, reserved, classType, options, samModifiedRequested, securityAttributes, resultKey, disposition);

    if (result == ERROR_ACCESS_DENIED)
    {
        // Help awareness of the issue.
        bool savedLogging = g_psf_NoLogging;
        g_psf_NoLogging = true;
        std::wstring wSubKeyString = InterpretStringW(subKey);
        Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyExHelper key=%s, name=%s; may need to pre-create key in package? result=%s", g_RegModuleName, RegLocalInstance, wKeyOnlyPath.c_str(), wSubKeyString.c_str(), LStatusToWstring(result).c_str());
        g_psf_NoLogging = savedLogging;

        if (wKeyPath._Starts_with(L"HKEY_CURRENT_USER"))
        {
            ;
            // Known issue: Cannot create subkey of a package virtual key created by the app at runtime and not in the original package.
            // Workaround: Try creating from the root of the hive.
        }
        if (wKeyPath._Starts_with(L"=\\REGISTRY\\USER\\"))
        {
            ;
            // Known issue: Cannot create subkey of a package virtual key created by the app at runtime and not in the original package.
            // Workaround: Try creating from the root of the hive.
            size_t offset = regCohorts.RequestedPath.find(HKLM2HKCU_RedirNameOnlyW.c_str());
            if (offset != std::wstring::npos)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d]  RegCreateKeyExHelper retry using HKCU and %s", g_RegModuleName, RegLocalInstance, regCohorts.RequestedPath.substr(offset).c_str());
                result = impl::KernelBaseRegCreateKeyExW(HKEY_CURRENT_USER, regCohorts.RequestedPath.substr(offset).c_str(), reserved, classType, options, samModifiedRequested, securityAttributes, resultKey, disposition);
                Log(LogLevel_DebugIntermediate, L"[%s%d]  RegCreateKeyExHelper retry  key=0x%x result=%s", g_RegModuleName, RegLocalInstance, regCohorts.RequestedPath.substr(offset).c_str(), LStatusToWstring(result).c_str());
            }
        }
    }

    if (result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance);
                LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
                LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"Sub Key", subKey);
                Log(LogLevel_DebugIntermediate, L"[%s%d] Reserved=%d\n", g_RegModuleName, RegLocalInstance, reserved);
                if (classType)
                {
                    LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"\tClass", classType);
                }
                LogRegKeyFlags(LogLevel_DebugIntermediate, RegLocalInstance, options);
                Log(LogLevel_DebugIntermediate, L"[%s%d] samDesired=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samDesired)).c_str());
                if (samDesired != samModifiedRequested)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] ModifiedSam=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samModifiedRequested)).c_str());
                }
                LogSecurityAttributes(LogLevel_DebugIntermediate, securityAttributes, RegLocalInstance);

                LogFunctionResultInstance(LogLevel_DebugIntermediate, RegLocalInstance, functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32ErrorInstance(LogLevel_DebugIntermediate, RegLocalInstance, (DWORD)result);
                }
                else if (disposition)
                {
                    LogRegKeyDisposition(LogLevel_DebugIntermediate, RegLocalInstance, *disposition);
                }
                Log(LogLevel_DebugIntermediate, L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegCreateKeyExHelper logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    return result;
}

LSTATUS __stdcall RegCreateKeyExAFixup(
    _In_ HKEY key,
    _In_ const char* subKey,
    _Reserved_ DWORD reserved,
    _In_opt_ char* classType,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
    _Out_ PHKEY resultKey,
    _Out_opt_ LPDWORD disposition)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;
        Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyExA: key=0x%x subkey=%S Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);

        std::wstring wSubKey = widen(subKey);
        std::wstring wClassType = classType ? widen(classType) : L"";

        LSTATUS result;
        if (classType != NULL)
        {
            result = RegCreateKeyExHelper(key, wSubKey.c_str(), reserved, wClassType.data(), options, samDesired, securityAttributes, resultKey, disposition);
        }
        else
        {
            result = RegCreateKeyExHelper(key, wSubKey.c_str(), reserved, NULL, options, samDesired, securityAttributes, resultKey, disposition);
        }

        Log(LogLevel_DebugBasic, L"[%s%d]\tRegCreateKeyExA modified key=0x%x result=%s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());
        return result;
    }
    else
    {
        return impl::KernelBaseRegCreateKeyExA(key, subKey, reserved, classType, options, samDesired, securityAttributes, resultKey, disposition);
    }
}
DECLARE_FIXUP(impl::KernelBaseRegCreateKeyExA, RegCreateKeyExAFixup);

LSTATUS __stdcall RegCreateKeyExWFixup(
    _In_ HKEY key,
    _In_ const wchar_t* subKey,
    _Reserved_ DWORD reserved,
    _In_opt_ wchar_t* classType,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
    _Out_ PHKEY resultKey,
    _Out_opt_ LPDWORD disposition)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD RegLocalInstance = ++g_RegInterceptInstance;
        Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyExW: key=0x%x subKey=%ls Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);

        LSTATUS result = RegCreateKeyExHelper(key, subKey, reserved, classType, options, samDesired, securityAttributes, resultKey, disposition);
     
        Log(LogLevel_DebugBasic, L"[%s%d]\tRegCreateKeyExW modified key=0x%x result=%s", g_RegModuleName, RegLocalInstance, *resultKey, LStatusToWstring(result).c_str());
        return result;
    }
    else
    {
        return impl::KernelBaseRegCreateKeyExW(key, subKey, reserved, classType, options, samDesired, securityAttributes, resultKey, disposition);
    }
}
DECLARE_FIXUP(impl::KernelBaseRegCreateKeyExW, RegCreateKeyExWFixup);



#else


auto RegCreateKeyExImpl = psf::detoured_string_function(&::RegCreateKeyExA, &::RegCreateKeyExW);
template <typename CharT>
LSTATUS __stdcall RegCreateKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _Reserved_ DWORD reserved,
    _In_opt_ CharT* classType,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
    _Out_ PHKEY resultKey,
    _Out_opt_ LPDWORD disposition)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;
    bool isBlocked = false;

    if constexpr (psf::is_ansi<CharT>)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyEx: key=0x%x subkey=%S Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
    }
    else
    {
        LogLogLevel_DebugBasic, (L"[%s%d] RegCreateKeyEx: key=0x%x subKey=%ls Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
    }

    std::string keyOnlyPath = InterpretKeyPath(key);
    std::string keyPath = keyOnlyPath + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(LogLevel_DebugMaximum, keyPath, samDesired, RegLocalInstance);

    bool hasRedirection = false;
#if TRYHKLM2HKCU
    if (HasHKLM2HKCUSpecified())
    {
        std::string altKeyOnlyPath;
        if (key == HKEY_LOCAL_MACHINE)
        {
            altKeyOnlyPath = HKLM2HKCU_Replacement("");
        }
        else if (keyOnlyPath._Starts_with("HKEY_LOCAL_MACHINE"))
        {
            if (keyOnlyPath.length == 18)
                altKeyOnlyPath = HKLM2HKCU_Replacement("");
            else
                altKeyOnlyPath = HKLM2HKCU_Replacement(keyOnlyPath.substr(19)));
        }

        if (altKeyOnlyPath.empty())
        {
            HKEY  altKey;
            LSTATUS altresult = ::RegOpenKeyA(HKEY_CURRENT_USER, altKeyOnlyPath, &altKey);
            if (altresult == ERROR_FILE_NOT_FOUND)
            {
                alrresult = ::RegCreateKeyA(HKEY_CURRENT_USER, altKeyOnlyPath, &altKey);
            }
            if (altresult == ERROR_SUCCESS)
            {
                result = RegCreateKeyExImpl(altKey, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                RegCloseKey(altKey);
                hasRedirection = true;
                LogString(LogLevel_DebugBasic, g_RegModuleName, RegLocalInstance, L"\tRegCreateKeyEx Redirecting to HKCU", subKey);
                Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyEx result=%d", g_RegModuleName, RegLocalInstance, result);
            }
        }
    }
#endif

    if (!hasRedirection)
    {
        std::string sskey = narrow(subKey);
        result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyOnlyPath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {
            std::string fullpath = keyPath;
            if (subKey != NULL)
            {
                fullpath += "\\" + sskey;
            }
            //if (!RegFixupJavaBlocker(LogLevel_DebugMaximum, fullpath, RegLocalInstance))
            //{
            result = RegCreateKeyExImpl(key, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
            //}
            //else
            //{
            //    result = ERROR_PATH_NOT_FOUND;
            //    resultKey = NULL;
            //    isBlocked = true;
            //}
        }
        else
        {
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
            isBlocked = true;
        }

        if (result != ERROR_SUCCESS)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyEx result=0x%x", g_RegModuleName, RegLocalInstance, result);
   
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegCreateKeyEx result=SUCCESS key=0x%x", g_RegModuleName, RegLocalInstance, *resultKey);
        }
    }



    if (result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName,RegLocalInstance);
                LogKeyPath(LogLevel_DebugIntermediate, g_ModuleName, RegLocalInstance, key);
                LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"Sub Key", subKey);
                Log(LogLevel_DebugIntermediate, L"[%s%d] Reserved=%d\n", g_RegModuleName, RegLocalInstance, reserved);
                if (classType)
                {
                    LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"\tClass", classType);
                }
                LogRegKeyFlags(RegLocalInstance, options);
                Log(LogLevel_DebugIntermediate, L"[%s%d] samDesired=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samDesired)).c_str());
                if (samDesired != samModified)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] ModifiedSam=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samModified)).c_str());
                }
                LogSecurityAttributes(securityAttributes, RegLocalInstance);

                LogFunctionResultInstance(RegLocalInstance, functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32ErrorInstance(LogLevel_DebugIntermediate, RegLocalInstance, (DWORD)result);
                }
                else if (disposition)
                {
                    LogRegKeyDisposition(LogLevel_DebugIntermediate, RegLocalInstance, *disposition);
                }
                Log(LogLevel_DebugIntermediate, L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegCreateKeyEx logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }

#if THISCOULDHELPBUTDOESNT
        // Creating a subkey of a created key in the virtual registry seems to faile with access denied.
        // try again to workaround bug in vreg
        ULONG size;
        if (auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);
            (status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))
        {
            try
            {
                auto buffer = std::make_unique<std::uint8_t[]>(size + 2);
                if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))
                {
                    buffer[size] = 0x0;
                    buffer[size + 1] = 0x0;  // Add string termination character
                    auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());
                    std::wstring keyname = info->Name;
                    std::wstring newsubkeyname;
                    if (keyname._Starts_with(L"=\\REGISTRY\\USER\\"))
                    {
                        size_t offset = keyname.find_first_of(L"\\", 15) + 1;
                        newsubkeyname = keyname.substr(offset).append(L"\\").append(widen(subKey));
                        LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"\tModified HKCU Sub Key", newsubkeyname.c_str());
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            std::string nsknarrow = narrow(newsubkeyname);

                            result = ::RegCreateKeyExA(HKEY_CURRENT_USER, nsknarrow.c_str(), reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                            //result = ::RegCreateKeyA(HKEY_CURRENT_USER, nsknarrow.c_str(), resultKey);
                        }
                        else
                        {
                            result = ::RegCreateKeyExW(HKEY_CURRENT_USER, newsubkeyname.c_str(), reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                            //result = ::RegCreateKeyW(HKEY_CURRENT_USER, newsubkeyname.c_str(), resultKey);
                        }
                        Log(LogLevel_DebugIntermediate, L"[%s%d]\tRegCreateKeyEx modified result=%d\n", g_RegModuleName, RegLocalInstance, result);
                    }
                    else if (keyname._Starts_with(L"=\\REGISTRY\\MACHINE\\"))
                    {
                        size_t offset = keyname.find_first_of(L"\\", 18) + 1;
                        newsubkeyname = keyname.substr(offset).append(L"\\").append(widen(subKey));
                        LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"\tModified HKLM Sub Key", newsubkeyname.c_str());
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            std::string nsknarrow = narrow(newsubkeyname);

                            result = ::RegCreateKeyExA(HKEY_LOCAL_MACHINE, nsknarrow.c_str(), reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                            //result = ::RegCreateKeyA(HKEY_LOCAL_MACHINE, nsknarrow.c_str(), resultKey);
                        }
                        else
                        {
                            result = ::RegCreateKeyExW(HKEY_LOCAL_MACHINE, newsubkeyname.c_str(), reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                            //result = ::RegCreateKeyW(HKEY_LOCAL_MACHINE, newsubkeyname.c_str(), resultKey);
                        }
                        Log(LogLevel_DebugIntermediate, L"[%s%d]\tRegCreateKeyEx modified result=%d\n", g_RegModuleName, RegLocalInstance, result);
                    }
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d]\tUnable to fix up Key Path.\n", g_RegModuleName, RegLocalInstance);
                SetLastError(ERROR_ACCESS_DENIED);
            }
        }
#endif

    }

    return result;
}
DECLARE_STRING_FIXUP(RegCreateKeyExImpl, RegCreateKeyExFixup);


#endif