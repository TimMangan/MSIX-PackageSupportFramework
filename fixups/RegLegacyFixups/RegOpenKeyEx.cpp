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

LSTATUS __stdcall RegOpenKeyExAFixup(
    _In_ HKEY key,
    _In_ const char* subKey,
    _In_ DWORD options,
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;
    bool isBlocked = false;


    if (subKey != NULL)
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA(KernelBase): key=0x%x subKey=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    else
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA(KernelBase): key=0x%x subKey=NULL", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key);

    std::string keyonlypath = InterpretKeyPath(key);
    std::string keypath;
    if (subKey != NULL)
        keypath = keyonlypath + "\\" + InterpretStringA(subKey);
    else
        keypath = keyonlypath;

    REGSAM samModified = RegFixupSam(LogLevel_DebugMaximum, keypath, samDesired, RegLocalInstance);

    bool hasRedirection = false;

    if (!hasRedirection)
    {
        std::string sskey;
        if (subKey != NULL)
            sskey = narrow(subKey);
        else
            sskey = "";
        result = RegFixupDeletionMarker(LogLevel_DebugMaximum,  keyonlypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {


            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA:  JavaBlocker checking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());

            if (!RegFixupJavaBlocker(LogLevel_DebugMaximum, keypath, RegLocalInstance))
            {
                result = impl::KernelBaseRegOpenKeyExA(key, subKey, options, samModified, resultKey);
            }
            else
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExA:  JavaBlocker Blocking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());
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
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA result=%d", g_RegModuleName, RegLocalInstance, result);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExA result=SUCCESS key=0x%x", g_RegModuleName, RegLocalInstance, *resultKey);
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
                if (subKey != NULL)
                    LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L" Sub Key", subKey);
                else
                    LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L" Sub Key", L"NULL");
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
                if (result == ERROR_ACCESS_DENIED)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] An ACCESS DENIED error may indicate that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegOpenKeyExA logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
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
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;
    bool isBlocked = false;


    if (subKey != NULL)
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW(KernelBase): key=0x%x subKey=%ls", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, subKey);
    else
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW(KernelBase): key=0x%x subKey=NULL", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key);

    std::string keyonlypath = InterpretKeyPath(key);
    std::string keypath;
    if (subKey != NULL)
        keypath = keyonlypath + "\\" + InterpretStringA(subKey);
    else
        keypath = keyonlypath;
    REGSAM samModified = RegFixupSam(LogLevel_DebugMaximum, keypath, samDesired, RegLocalInstance);

    bool hasRedirection = false;

    if (!hasRedirection)
    {
        std::string sskey;
        if (subKey != NULL)
            sskey = narrow(subKey);
        else
            sskey = "";
        result = RegFixupDeletionMarker(LogLevel_DebugIntermediate, keyonlypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {


            Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW:  JavaBlocker checking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());

            if (!RegFixupJavaBlocker(LogLevel_DebugMaximum, keypath, RegLocalInstance))
            {
                result = impl::KernelBaseRegOpenKeyExW(key, subKey, options, samModified, resultKey);
            }
            else
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegOpenKeyExW:  JavaBlocker Blocking path=%S", g_RegModuleName, RegLocalInstance, keypath.c_str());
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


    if (true) //result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogCallingModuleInstanceCommon(LogLevel_DebugIntermediate, g_RegModuleName,RegLocalInstance);
                LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
                if (subKey != NULL)
                    LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L" Sub Key", subKey);
                else
                    LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L" Sub Key", L"NULL");
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
                if (result == ERROR_ACCESS_DENIED)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] An ACCESS DENIED error may indicate that the key must be added to the original package.", g_RegModuleName, RegLocalInstance);
                }
            }
            catch (...)
            {
                Log(LogLevel_Exception, L"[%s%d] RegOpenKeyExW logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }



    if (result != ERROR_SUCCESS)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW result=%d", g_RegModuleName, RegLocalInstance, result);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] RegOpenKeyExW result=SUCCESS key=0x%x", g_RegModuleName, RegLocalInstance, *resultKey);
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