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
#include "Logging.h"

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
LSTATUS __stdcall RegEnumValueAFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

    bool stillWorking = true;
    DWORD onIndex = dwIndex;
    while (stillWorking)
    {
        result = impl::KernelBaseRegEnumValueA(key, onIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = narrow(lpName);
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegEnumValueA:  Returning lpName=%S", g_RegModuleName, RegLocalInstance, lpName);
                stillWorking = false;
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  DeletionMarker Blocking lpName=%S, try again.", g_RegModuleName, RegLocalInstance, lpName);
                onIndex++;
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueA:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
            stillWorking = false;;
        }
    }



    if (result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
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
                Log(LogLevel_Exception, L"[%s%d] RegEnumValueA logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueA, RegEnumValueAFixup);

LSTATUS __stdcall RegEnumValueWFixup(
    _In_ HKEY key,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _In_ _Out_ LPDWORD lpcchName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_opt_ LPBYTE lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


    Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  key=0x%x keyname=%S dwIndex=%d", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);

    bool stillWorking = true;
    DWORD onIndex = dwIndex;
    while (stillWorking)
    {
        result = impl::KernelBaseRegEnumValueW(key, onIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = narrow(lpName);
            result = RegFixupDeletionMarker(LogLevel_DebugIntermediate, keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValueW:  Returning lpName=%s", g_RegModuleName, RegLocalInstance, lpName);
                stillWorking = false;
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
                Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValue:  DeletionMarker Blocking lpName=%s, try again.", g_RegModuleName, RegLocalInstance, lpName);
                onIndex++;
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegEnumValue:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
            stillWorking = false;;
        }
    }



    if (result == ERROR_ACCESS_DENIED)
    {
        auto functionResult = from_win32(result);
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
                LogKeyPath(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, key);
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
                Log(LogLevel_Exception, L"[%s%d] RegEnumValueW logging failure.\n", g_RegModuleName, RegLocalInstance);
            }
        }
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueW, RegEnumValueWFixup);

#else


#endif