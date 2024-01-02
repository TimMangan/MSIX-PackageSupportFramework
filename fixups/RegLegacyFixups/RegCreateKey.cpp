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




auto RegCreateKeyImpl = psf::detoured_string_function(&::RegCreateKeyA, &::RegCreateKeyW);
template <typename CharT>
LSTATUS __stdcall RegCreateKeyFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _Out_ PHKEY resultKey)
{
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;
    bool isBlocked = false;

#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log(L"[%d] RegCreateKey: key=0x%x subkey=%S", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
    else
    {
        Log(L"[%d] RegCreateKey: key=0x%x subKey=%ls", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
#endif

    std::string keyonlypath = InterpretKeyPath(key);
    std::string keypath = keyonlypath + "\\" + InterpretStringA(subKey);

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
                result = RegCreateKeyImpl(altkey, subKey, resultKey);
                RegCloseKey(altkey);
                hasRedirection = true;
#ifdef _DEBUG
                LogString(RegLocalInstance, L"\RegCreateKey Redirecting to HKCU", subKey);
                Log("[%d] RegCreateKey result=%d", RegLocalInstance, result);
#endif
            }
        }
    }
#endif

    if (!hasRedirection)
    {
        std::string sskey = narrow(subKey);
        result = RegFixupDeletionMarker(keypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {
            std::string fullpath = keypath;
            if (subKey != NULL)
            {
                fullpath += "\\" + sskey;
            }
            //if (!RegFixupJavaBlocker(fullpath, RegLocalInstance))
            //{
                result = RegCreateKeyImpl(key, subKey,  resultKey);
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
#ifdef _DEBUG
            Log("[%d] RegCreateKey result=0x%x", RegLocalInstance, result);
#endif   
        }
        else
        {
#ifdef _DEBUG
            Log("[%d] RegCreateKey result=SUCCESS key=0x%x", RegLocalInstance, key);
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
                
                LogFunctionResult(functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32Error(result);
                }
                LogCallingModule();
                Log("[%d] This error often indicates that the key must be added to the original package.", RegLocalInstance);
            }
            catch (...)
            {
                Log("[%d] RegCreateKey logging failure.\n", RegLocalInstance);
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

                            result = ::RegCreateKeyA(HKEY_CURRENT_USER, nsknarrow.c_str(), resultKey);
                        }
                        else
                        {
                            result = ::RegCreateKeyW(HKEY_CURRENT_USER, newsubkeyname.c_str(), resultKey);
                        }
#ifdef _DEBUG
                        Log("[%d]\tRegCreateKey modified result=%d\n", RegLocalInstance, result);
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

                            result = ::RegCreateKeyA(HKEY_LOCAL_MACHINE, nsknarrow.c_str(), resultKey);
                        }
                        else
                        {
                            result = ::RegCreateKeyW(HKEY_LOCAL_MACHINE, newsubkeyname.c_str(), resultKey);
                        }
#ifdef _DEBUG
                        Log("[%d]\RegCreateKey modified result=%d\n", RegLocalInstance, result);
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
DECLARE_STRING_FIXUP(RegCreateKeyImpl, RegCreateKeyFixup);
