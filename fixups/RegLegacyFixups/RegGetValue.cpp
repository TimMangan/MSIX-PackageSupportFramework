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

#ifdef INTERCEPT_KERNELBASE_PlusRegGetValue
LSTATUS __stdcall RegGetValueAFixup(
    _In_ HKEY key,
    _In_opt_ LPCSTR lpSubKey,
    _In_opt_ LPCSTR lpValue,
    _In_opt_ DWORD dwFlags,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


#if _DEBUG
    std::string sSubKey = "NULL";
    std::string sValue = "NULL";
    if (lpSubKey != NULL)
        sSubKey = lpSubKey;
    if (lpValue != NULL)
        sValue = lpValue;
    Log(L"[%d] RegGetValueA:  key=0x%x keyname=%S SubKey=%S SubName=%S", RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), sSubKey.c_str(), sValue.c_str());
#endif

    result = impl::KernelBaseRegGetValueA(key, lpSubKey, lpValue, dwFlags, lpDwType, lpData, lpcchClass, lpcbData);
    if (result == ERROR_SUCCESS)
    {
        std::string sskey = "";
        if (lpSubKey != NULL)
            sskey = lpSubKey;
        result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {
#if MOREDEBUG
            Log(L"[%d] RegGetValue:  Returning success", RegLocalInstance);
#endif                
        }
        else
        {
            // We have a deletion marker on this particular item, so we need to skip it.
            // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
            // because we do not have a way to remember this, like done in FindFirstFile.
#if _DEBUG
            Log(L"[%d] RegGetValue:  DeletionMarker Blocking this call.", RegLocalInstance);
#endif                

        }
    }
    else
    {
#if _DEBUG
        Log(L"[%d] RegGetValue:  Returning normal failure 0x%x.", RegLocalInstance, result);
#endif                
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegGetValueA, RegGetValueAFixup);

LSTATUS __stdcall RegGetValueWFixup(
    _In_ HKEY key,
    _In_opt_ LPCWSTR lpSubKey,
    _In_opt_ LPCWSTR lpValue,
    _In_opt_ DWORD dwFlags,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_  LPDWORD lpcchClass,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;


    std::string keyonlypath = InterpretKeyPath(key);


#if _DEBUG
    std::string sSubKey = "NULL";
    std::string sValue = "NULL";
    if (lpSubKey != NULL)
        sSubKey = narrow(lpSubKey);
    if (lpValue != NULL)
        sValue = narrow(lpValue);
    Log(L"[%d] RegGetValueW:  key=0x%x keyname=%S SubKey=%S SubName=%S", RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), sSubKey.c_str(), sValue.c_str());
#endif

    result = impl::KernelBaseRegGetValueW(key, lpSubKey, lpValue, dwFlags, lpDwType, lpData, lpcchClass, lpcbData);
    if (result == ERROR_SUCCESS)
    {
        std::string sskey = "";
        if (lpSubKey != NULL)
            sskey = narrow(lpSubKey);
        result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
        if (result == ERROR_SUCCESS)
        {
#if _DEBUG
            Log(L"[%d] RegGetValue:  Returning success", RegLocalInstance);
#endif                
        }
        else
        {
            // We have a deletion marker on this particular item, so we need to skip it.
            // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
            // because we do not have a way to remember this, like done in FindFirstFile.
#if _DEBUG
            Log(L"[%d] RegGetValue:  DeletionMarker Blocking this call.", RegLocalInstance);
#endif
        }
    }
    else
    {
#if _DEBUG
        Log(L"[%d] RegGetValue:  Returning normal failure 0x%x.", RegLocalInstance, result);
#endif                
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegGetValueW, RegGetValueWFixup);

#endif