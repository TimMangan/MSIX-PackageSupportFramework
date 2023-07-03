//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#ifdef _DEBUG
//#define MOREDEBUG
#endif

#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"




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
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;

#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log(L"[%d] RegCreateKeyEx: key=0x%x subkey=%S Options=0x%x SamDesired=0x%x", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
    }
    else
    {
        Log(L"[%d] RegCreateKeyEx: key=0x%x subKey=%ls Options=0x%x SamDesired=0x%x", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey, options, samDesired);
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
#ifdef _DEBUG
                LogString(RegLocalInstance, L"\tRegCreateKeyEx Redirecting to HKCU", subKey);
                Log("[%d] RegCreateKeyEx result=%d", RegLocalInstance, result);
#endif
            }
        }
    }
#endif

    if (!hasRedirection)
    {
        result = RegCreateKeyExImpl(key, subKey, reserved, classType, options, samModified, securityAttributes, resultKey, disposition);
        if (result != ERROR_SUCCESS)
        {
#ifdef _DEBUG
            Log("[%d] RegCreateKeyEx result=0x%x", RegLocalInstance,result);
#endif   
        }
        else
        {
#ifdef _DEBUG
            Log("[%d] RegCreateKeyEx result=SUCCESS key=0x%x", RegLocalInstance,key);
#endif
        }
    }



    if (result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
#ifdef _DEBUG
#ifdef MOREDEBUG
            try
            {
                LogKeyPath(key);
                LogString(RegLocalInstance, L"Sub Key", subKey);
                Log(L"[%d] Reserved=%d\n", RegLocalInstance, reserved);
                if (classType) LogString(L"\tClass", classType);
                LogRegKeyFlags(options);
                Log(L"[%d] samDesired=%s\n", RegLocalInstance, widen(InterpretRegKeyAccess(samDesired)).c_str());
                if (samDesired != samModified)
                {
                    Log(L"[%d] ModifiedSam=%s\n", RegLocalInstance, widen(InterpretRegKeyAccess(samModified)).c_str());
                }
                LogSecurityAttributes(securityAttributes, RegLocalInstance);

                LogFunctionResult(functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32Error(result);
                }
                else if (disposition)
                {
                    LogRegKeyDisposition(*disposition);
                }
                LogCallingModule();
                Log("[%d] This error often indicates that the key must be added to the original package.", RegLocalInstance);
            }
            catch (...)
            {
                Log("[%d] RegCreateKeyEx logging failure.\n", RegLocalInstance);
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
#ifdef _DEBUG
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
#ifdef _DEBUG
                        Log("[%d]\tRegCreateKeyEx modified result=%d\n", RegLocalInstance, result);
#endif
                    }
                    else if (keyname._Starts_with(L"\\REGISTRY\\MACHINE\\"))
                    {
                        size_t offset = keyname.find_first_of(L"\\", 18) + 1;
                        newsubkeyname = keyname.substr(offset).append(L"\\").append(widen(subKey));
#ifdef _DEBUG
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
#ifdef _DEBUG
                        Log("[%d]\tRegCreateKeyEx modified result=%d\n", RegLocalInstance, result);
#endif
                    }
                }
            }
            catch (...)
            {
                Log(L"[%d]\tUnable to fix up Key Path.\n", RegLocalInstance);
                SetLastError(ERROR_ACCESS_DENIED);
            }
        }
#endif

    }

    return result;
}
DECLARE_STRING_FIXUP(RegCreateKeyExImpl, RegCreateKeyExFixup);
