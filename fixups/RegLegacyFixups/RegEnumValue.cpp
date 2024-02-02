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

#ifdef INTERCEPT_KERNELBASE
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
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


#if _DEBUG
    Log(L"[%d] RegEnumValueA:  key=0x%x keyname=%S dwIndex=%d", RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);
#endif

    bool stillWorking = true;
    DWORD onIndex = dwIndex;
    while (stillWorking)
    {
        result = impl::KernelBaseRegEnumValueA(key, onIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = narrow(lpName);
            result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
#if MOREDEBUG
                Log(L"[%d] RegEnumValue:  Returning lpName=%S", RegLocalInstance, lpName);
#endif                
                stillWorking = false;
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
#if _DEBUG
                Log(L"[%d] RegEnumValue:  DeletionMarker Blocking lpName=%S, try again.", RegLocalInstance, lpName);
#endif                
                onIndex++;
            }
        }
        else
        {
#if _DEBUG
            Log(L"[%d] RegEnumValue:  Returning normal failure 0x%x.", RegLocalInstance, result);
#endif                
            stillWorking = false;;
        }
    }



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
                Log(L"[%d] RegEnumValue logging failure.\n", RegLocalInstance);
            }
        }
    }
#endif
#endif
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
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


#if _DEBUG
    Log(L"[%d] RegEnumValueW:  key=0x%x keyname=%S dwIndex=%d", RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), dwIndex);
#endif

    bool stillWorking = true;
    DWORD onIndex = dwIndex;
    while (stillWorking)
    {
        result = impl::KernelBaseRegEnumValueW(key, onIndex, lpName, lpcchName, lpReserved, lpType, lpData, lpcbData);
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = narrow(lpName);
            result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
#if MOREDEBUG
                Log(L"[%d] RegEnumValue:  Returning lpName=%S", RegLocalInstance, lpName);
#endif                
                stillWorking = false;
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
#if _DEBUG
                Log(L"[%d] RegEnumValue:  DeletionMarker Blocking lpName=%S, try again.", RegLocalInstance, lpName);
#endif                
                onIndex++;
            }
        }
        else
        {
#if _DEBUG
            Log(L"[%d] RegEnumValue:  Returning normal failure 0x%x.", RegLocalInstance, result);
#endif                
            stillWorking = false;;
        }
    }



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
                Log(L"[%d] RegEnumValue logging failure.\n", RegLocalInstance);
            }
        }
    }
#endif
#endif
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegEnumValueW, RegEnumValueWFixup);

#else


#endif