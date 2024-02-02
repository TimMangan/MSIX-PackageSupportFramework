//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//#define MOREDEBUG 1

#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"

auto RegOpenKeyImpl = psf::detoured_string_function(&::RegOpenKeyA, &::RegOpenKeyW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyFixup(
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
        Log(L"[%d] RegOpenKey:  key=0x%x subkey=%S", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
    else
    {
        Log(L"[%d] RegOpenKey: key=0x%x subKey=%ls", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
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
                result = RegOpenKeyImpl(altkey, subKey, resultKey);
                RegCloseKey(altkey);
                hasRedirection = true;
#ifdef _DEBUG
                LogString(RegLocalInstance, L"\tRegOpenKey Redirecting to HKCU", subKey);
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


#if MOREDEBUG
            Log(L"[%d] RegOpenKey:  JavaBlocker checking path=%S", RegLocalInstance, keypath.c_str());
#endif

            if (!RegFixupJavaBlocker(keypath, RegLocalInstance))
            {
                result = RegOpenKeyImpl(key, subKey, resultKey);
            }
            else
            {
#if _DEBUG
                Log(L"[%d] RegOpenKey:  JavaBlocker Blocking path=%S", RegLocalInstance, keypath.c_str());
#endif
                result = ERROR_PATH_NOT_FOUND;
                resultKey = NULL;
                isBlocked = true;
            }
        }
        else
        {
#if _DEBUG
            Log(L"[%d] RegOpenKey:  DeletionMarker Blocking path=%S", RegLocalInstance, keypath.c_str());
#endif            
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
            isBlocked = true;
        }
    }

#ifdef _DEBUG
    if (result != ERROR_SUCCESS)
    {
        Log(L"[%d] RegOpenKey result=%d", RegLocalInstance, result);
    }
    else
    {
        Log(L"[%d] RegOpenKey result=SUCCESS key=0x%x", RegLocalInstance, *resultKey);
    }
#endif

#ifdef _DEBUG
#ifdef MOREDEBUG2
    if (true) //resultKey == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogKeyPath(key);
                LogString(L" Sub Key", subKey);
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
                Log(L"[%d] RegOpenKey logging failure.\n", RegLocalInstance);
            }
        }
    }
#endif
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyImpl, RegOpenKeyFixup);
