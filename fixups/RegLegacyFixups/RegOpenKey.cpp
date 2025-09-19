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

auto RegOpenKeyImpl = psf::detoured_string_function(&::RegOpenKeyA, &::RegOpenKeyW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _Out_ PHKEY resultKey)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;
    bool isBlocked = false;


    if constexpr (psf::is_ansi<CharT>)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey:  key=0x%x subkey=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey: key=0x%x subKey=%ls", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }

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
                LogString(LogLevel_DebugBasic, g_RegModuleName, RegLocalInstance, L"\tRegOpenKey Redirecting to HKCU", subKey);
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
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey:  JavaBlocker checking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());

            if (!RegFixupJavaBlocker(LogLevel_DebugMaximum, keypath, RegLocalInstance))
            {
                result = RegOpenKeyImpl(key, subKey, resultKey);
            }
            else
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey:  JavaBlocker Blocking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());
                result = ERROR_PATH_NOT_FOUND;
                resultKey = NULL;
                isBlocked = true;
            }
        }
        else
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKey:  DeletionMarker Blocking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
            isBlocked = true;
        }
    }

    if (result != ERROR_SUCCESS)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey result=%d", g_RegModuleName, RegLocalInstance, result);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKey result=SUCCESS key=0x%x", g_RegModuleName, RegLocalInstance, *resultKey);
    }

    if (true) //resultKey == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
                LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L" Sub Key", subKey);
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
                Log(LogLevel_Exception, L"[%s%d] RegOpenKey logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyImpl, RegOpenKeyFixup);
