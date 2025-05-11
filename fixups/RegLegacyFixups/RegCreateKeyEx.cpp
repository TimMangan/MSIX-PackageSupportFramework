//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if _DEBUG
//#define _ManualDebug 1
#define MOREDEBUG 1
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

template <typename CharT>
LSTATUS __stdcall RegCreateKeyExGeneric(
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

#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log(L"[%s%d] RegCreateKeyEx: key=0x%x subkey=%S Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
    }
    else
    {
        Log(L"[%s%d] RegCreateKeyEx: key=0x%x subKey=%ls Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
    }
#endif

    std::string keyonlypath = InterpretKeyPath(key);
    std::string keypath = keyonlypath + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired, RegLocalInstance);

    bool hasRedirection = false;
#if TRYHKLM2HKCU
    if (HasHKLM2HKCUSpecified())
    {
        std::string altkeyonlypath = NULL;
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

        if (altkeyonlypath != NULL)
        {
            HKEY  altkey;
            LSTATUS altresult = ::RegOpenKeyA(HKEY_CURRENT_USER, altkeyonlypath, &altkey);
            if (altresult == ERROR_FILE_NOT_FOUND)
            {
                alrresult = ::RegCreateKeyA(HKEY_CURRENT_USER, altkeyonlypath, &altkey);
            }
            if (altresult == ERROR_SUCCESS)
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    result = impl::KernelBaseRegCreateKeyExA(altkey, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                }
                else
                { 
                    result = impl::KernelBaseRegCreateKeyExW(altkey, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                }
                RegCloseKey(altkey);
                hasRedirection = true;
#if _DEBUG
                LogString(RegLocalInstance, L"\tRegCreateKeyEx Redirecting to HKCU", subKey);
                Log(L"[%s%d] RegCreateKeyEx result=%d", g_RegModuleName, RegLocalInstance, result);
#endif
            }
        }
    }
#endif

    if (!hasRedirection)
    {
        std::string sskey = narrow(subKey);
        result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {
            std::string fullpath = keypath;
            if (subKey != NULL)
            {
                fullpath += "\\" + sskey;
            }
            //if (!RegFixupJavaBlocker(fullpath, RegLocalInstance))
            //{
            if constexpr (psf::is_ansi<CharT>)
            {
                result = impl::KernelBaseRegCreateKeyExA(key, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
            }
            else
            {
                result = impl::KernelBaseRegCreateKeyExW(key, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
            }

            //}
            //else
            //{
            //    result = ERROR_PATH_NOT_FOUND;
            //    resultKey = NULL;
            //    isBlocked = true;
            //}
            if (result == ERROR_ACCESS_DENIED)
            {
                // Help awareness of the issue.
                bool savedLogging = g_psf_NoLogging;
                g_psf_NoLogging = true;
                std::string subkeystring = InterpretStringA(subKey);
                Log("[%S%d] RegCreateKeyEx result=0x%x, key=%s, name=%s; may need to precreate key in package", g_RegModuleName, RegLocalInstance, result, keyonlypath.c_str(), subkeystring.c_str());
                g_psf_NoLogging = savedLogging;

                if (keypath._Starts_with("HKEY_CURRENT_USER"))
                {
                    ;
                    // Known issue: Cannot create subkey of a package virtual key created by the app at runtime and not in the original package.
                    // Workaround: Try creating from the root of the hive.
                }
                if (keypath._Starts_with("\\REGISTRY\\WC\\Silo"))
                {
                    ;
                    // Known issue: Cannot create subkey of a package virtual key created by the app at runtime and not in the original package.
                    // Workaround: Try creating from the root of the hive.
                }
            }
        }
        else
        {
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
            isBlocked = true;
        }

        if (result != ERROR_SUCCESS)
        {
#if _DEBUG
            Log(L"[%s%d] RegCreateKeyEx result=0x%x", g_RegModuleName, RegLocalInstance, result);
#endif   
        }
        else
        {
#if _DEBUG
            Log(L"[%s%d] RegCreateKeyEx result=SUCCESS key=0x%x", g_RegModuleName, RegLocalInstance, *resultKey);
#endif
        }

    }



    if (result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
#if _DEBUG
#if MOREDEBUG
            try
            {
                LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
                LogKeyPath(RegLocalInstance, key);
                LogString(RegLocalInstance, L"Sub Key", subKey);
                Log(L"[%s%d] Reserved=%d\n", g_RegModuleName, RegLocalInstance, reserved);
                if (classType) LogString(g_RegModuleName, RegLocalInstance, L"\tClass", classType);
                LogRegKeyFlags(RegLocalInstance, options);
                Log(L"[%s%d] samDesired=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samDesired)).c_str());
                if (samDesired != samModified)
                {
                    Log(L"[%s%d] ModifiedSam=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samModified)).c_str());
                }
                LogSecurityAttributes(securityAttributes, RegLocalInstance);

                LogFunctionResultInstance(RegLocalInstance, functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32ErrorInstance(RegLocalInstance, result);
                }
                else if (disposition)
                {
                    LogRegKeyDisposition(RegLocalInstance,*disposition);
                }
                Log(L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
            }
            catch (...)
            {
                Log(L"[%s%d] RegCreateKeyEx logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
#endif
#endif
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
                    if (keyname._Starts_with(L"\\REGISTRY\\USER\\"))
                    {
                        size_t offset = keyname.find_first_of(L"\\", 15) + 1;
                        newsubkeyname = keyname.substr(offset).append(L"\\").append(widen(subKey));
#if _DEBUG
                        LogString(RegLocalInstance, L"\tModified HKCU Sub Key", newsubkeyname.c_str());
#endif
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
#if _DEBUG
                        Log(L"[%s%d]\tRegCreateKeyEx modified result=%d\n", g_RegModuleName, RegLocalInstance, result);
#endif
                    }
                    else if (keyname._Starts_with(L"\\REGISTRY\\MACHINE\\"))
                    {
                        size_t offset = keyname.find_first_of(L"\\", 18) + 1;
                        newsubkeyname = keyname.substr(offset).append(L"\\").append(widen(subKey));
#if _DEBUG
                        LogString(RegLocalInstance, L"\tModified HKLM Sub Key", newsubkeyname.c_str());
#endif
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
#if _DEBUG
                        Log(L"[%s%d]\tRegCreateKeyEx modified result=%d\n", g_RegModuleName, RegLocalInstance, result);
#endif
                    }
                }
            }
            catch (...)
            {
                Log(L"[%s%d]\tUnable to fix up Key Path.\n", g_RegModuleName, RegLocalInstance);
                SetLastError(ERROR_ACCESS_DENIED);
            }
        }
#endif

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
    return RegCreateKeyExGeneric(key, subKey, reserved, classType, options, samDesired, securityAttributes, resultKey, disposition);
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
    return RegCreateKeyExGeneric(key, subKey, reserved, classType, options, samDesired, securityAttributes, resultKey, disposition);
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

#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log(L"[%s%d] RegCreateKeyEx: key=0x%x subkey=%S Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
    }
    else
    {
        Log(L"[%s%d] RegCreateKeyEx: key=0x%x subKey=%ls Options=0x%x SamDesired=0x%x", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
    }
#endif

    std::string keyonlypath = InterpretKeyPath(key);
    std::string keypath = keyonlypath + "\\" + InterpretStringA(subKey);
    REGSAM samModified = RegFixupSam(keypath, samDesired, RegLocalInstance);

    bool hasRedirection = false;
#if TRYHKLM2HKCU
    if (HasHKLM2HKCUSpecified())
    {
        std::string altkeyonlypath = NULL;
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

        if (altkeyonlypath != NULL)
        {
            HKEY  altkey;
            LSTATUS altresult = ::RegOpenKeyA(HKEY_CURRENT_USER, altkeyonlypath, &altkey);
            if (altresult == ERROR_FILE_NOT_FOUND)
            {
                alrresult = ::RegCreateKeyA(HKEY_CURRENT_USER, altkeyonlypath, &altkey);
            }
            if (altresult == ERROR_SUCCESS)
            {
                result = RegCreateKeyExImpl(altkey, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
                RegCloseKey(altkey);
                hasRedirection = true;
#if _DEBUG
                LogString(RegLocalInstance, L"\tRegCreateKeyEx Redirecting to HKCU", subKey);
                Log(L"[%s%d] RegCreateKeyEx result=%d", g_RegModuleName, RegLocalInstance, result);
#endif
            }
        }
    }
#endif

    if (!hasRedirection)
    {
        std::string sskey = narrow(subKey);
        result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {
            std::string fullpath = keypath;
            if (subKey != NULL)
            {
                fullpath += "\\" + sskey;
            }
            //if (!RegFixupJavaBlocker(fullpath, RegLocalInstance))
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
#if _DEBUG
            Log(L"[%s%d] RegCreateKeyEx result=0x%x", g_RegModuleName, RegLocalInstance, result);
#endif   
        }
        else
        {
#if _DEBUG
            Log(L"[%s%d] RegCreateKeyEx result=SUCCESS key=0x%x", g_RegModuleName, RegLocalInstance, *resultKey);
#endif
        }

    }



    if (result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
#if _DEBUG
#if MOREDEBUG
            try
            {
                LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
                LogKeyPath(RegLocalInstance, key);
                LogString(RegLocalInstance, L"Sub Key", subKey);
                Log(L"[%s%d] Reserved=%d\n", g_RegModuleName, RegLocalInstance, reserved);
                if (classType) LogString(g_RegModuleName, RegLocalInstance, L"\tClass", classType);
                LogRegKeyFlags(RegLocalInstance, options);
                Log(L"[%s%d] samDesired=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samDesired)).c_str());
                if (samDesired != samModified)
                {
                    Log(L"[%s%d] ModifiedSam=%s\n", g_RegModuleName, RegLocalInstance, widen(InterpretRegKeyAccess(samModified)).c_str());
                }
                LogSecurityAttributes(securityAttributes, RegLocalInstance);

                LogFunctionResultInstance(RegLocalInstance, functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32Error(RegLocalInstance, result);
                }
                else if (disposition)
                {
                    LogRegKeyDisposition(RegLocalInstance, *disposition);
                }
                Log(L"[%s%d] This error often indicates that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
            }
            catch (...)
            {
                Log(L"[%s%d] RegCreateKeyEx logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
#endif
#endif
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
                    if (keyname._Starts_with(L"\\REGISTRY\\USER\\"))
                    {
                        size_t offset = keyname.find_first_of(L"\\", 15) + 1;
                        newsubkeyname = keyname.substr(offset).append(L"\\").append(widen(subKey));
#if _DEBUG
                        LogString(RegLocalInstance, L"\tModified HKCU Sub Key", newsubkeyname.c_str());
#endif
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
#if _DEBUG
                        Log(L"[%s%d]\tRegCreateKeyEx modified result=%d\n", g_RegModuleName, RegLocalInstance, result);
#endif
                    }
                    else if (keyname._Starts_with(L"\\REGISTRY\\MACHINE\\"))
                    {
                        size_t offset = keyname.find_first_of(L"\\", 18) + 1;
                        newsubkeyname = keyname.substr(offset).append(L"\\").append(widen(subKey));
#if _DEBUG
                        LogString(RegLocalInstance, L"\tModified HKLM Sub Key", newsubkeyname.c_str());
#endif
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
#if _DEBUG
                        Log(L"[%s%d]\tRegCreateKeyEx modified result=%d\n", g_RegModuleName, RegLocalInstance, result);
#endif
                    }
                }
            }
            catch (...)
            {
                Log(L"[%s%d]\tUnable to fix up Key Path.\n", g_RegModuleName, RegLocalInstance);
                SetLastError(ERROR_ACCESS_DENIED);
            }
        }
#endif

    }

    return result;
}
DECLARE_STRING_FIXUP(RegCreateKeyExImpl, RegCreateKeyExFixup);


#endif