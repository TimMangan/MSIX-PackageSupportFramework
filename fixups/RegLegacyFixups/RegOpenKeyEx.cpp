//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"



auto RegOpenKeyExImpl = psf::detoured_string_function(&::RegOpenKeyExA, &::RegOpenKeyExW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey)
{
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;


#if _DEBUG
    if constexpr (psf::is_ansi<CharT>)
    {
        Log(L"[%d] RegOpenKeyEx:  key=0x%x subkey=%S", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    }
    else
    {
        Log(L"[%d] RegOpenKeyEx: key=0x%x subKey=%ls", RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
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
                result = RegOpenKeyExImpl(altkey, subKey, options, samModified, resultKey);
                RegCloseKey(altkey);
                hasRedirection = true;
#ifdef _DEBUG
                LogString(RegLocalInstance, L"\tRegOpenKeyEx Redirecting to HKCU", subKey);
#endif
            }
        }
    }
#endif

    if (!hasRedirection)
    {
        result = RegOpenKeyExImpl(key, subKey, options, samModified, resultKey);
    }
#ifdef _DEBUG
    Log("[%d] RegOpenKeyEx result=%d", RegLocalInstance, result);
#endif

#ifdef _DEBUG
#ifdef MOREDEBUG
    if (true) //result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogKeyPath(key);
                LogString(L" Sub Key", subKey);
                LogRegKeyFlags(options);
                Log(L"[%d] samDesired=%s\n", RegLocalInstance, widen(InterpretRegKeyAccess(samDesired)).c_str());
                if (samDesired != samModified)
                {
                    Log(L"[%d] ModifiedSam=%s\n", RegLocalInstance, widen(InterpretRegKeyAccess(samModified)).c_str());
                }
                LogFunctionResult(functionResult);
                if (function_failed(functionResult))
                {
                    LogWin32Error(result);
                }
                LogCallingModule();
            }
            catch (...)
            {
                Log(L"[%d] RegOpenKeyEx logging failure.\n", RegLocalInstance);
            }
        }
    }
#endif
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyExImpl, RegOpenKeyExFixup);
