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
LSTATUS __stdcall RegQueryValueExAFixup(
    _In_ HKEY key,
    _In_opt_ LPCSTR lpValueName,
    LPDWORD lpReservered,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;

    try
    {
        std::string keyonlypath = InterpretKeyPath(key);


#if _DEBUG
        std::string sValueName = "NULL";
        if (lpValueName != NULL)
            sValueName = lpValueName;
        Log(L"[%d] RegQueryValueExA:  key=0x%x keyname=%S ValueName=%S", RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), sValueName.c_str());
#endif
        DWORD dwType;
        result = impl::KernelBaseRegQueryValueExA(key, lpValueName, lpReservered, &dwType, lpData, lpcbData);
        if (lpDwType != NULL)
        {
            *lpDwType = dwType;
        }
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = "";
            result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
#if _DEBUG
                if (lpcbData != NULL)
                {
                    switch (dwType)
                    {
                    case REG_SZ:
                        if (lpData != NULL)
                        {
                            char* rstring = new char[(*lpcbData) + 1];
                            FillMemory(rstring, (*lpcbData) + 1, 0);
                            memcpy(rstring, lpData, *lpcbData);
                            LogString(RegLocalInstance, L"RegQueryValueExA: Returning success with value", rstring);
                        }
                        else
                        {
                            Log(L"[%d] RegQueryValueExA:  Returning success with no data", RegLocalInstance);
                        }
                        break;
                    case REG_DWORD:
                        if (lpData != NULL)
                        {
                            Log(L"[% d] RegQueryValueExA:  Returning success with DWORD x0x%x", RegLocalInstance, *((DWORD*)lpData));
                        }
                        else
                        {
                            Log(L"[% d] RegQueryValueExA:  Returning success with DWORD", RegLocalInstance);
                        }
                        break;
                    default:
                        Log(L"[%d] RegQueryValueExA:  Returning success of type 0x%x", RegLocalInstance, dwType);
                        break;
                    }
                }
                else
                {
                    Log(L"[%d] RegQueryValueExA:  Returning success", RegLocalInstance);
                }
#endif                
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
#if _DEBUG
                Log(L"[%d] RegQueryValueExA:  DeletionMarker Blocking this call.", RegLocalInstance);
#endif                
            }
        }
        else
        {
#if _DEBUG
            Log(L"[%d] RegQueryValueExA:  Returning normal failure 0x%x.", RegLocalInstance, result);
#endif                
        }
    }
    catch (...)
    {
        Log(L"[%d] RegQueryValueExA:  Exception thrown.", RegLocalInstance);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegQueryValueExA, RegQueryValueExAFixup);

LSTATUS __stdcall RegQueryValueExWFixup(
    _In_      HKEY key,
    _In_opt_  LPCWSTR lpValueName,
              LPDWORD lpReservered,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    DWORD RegLocalInstance = ++g_RegIntceptInstance;
    LSTATUS result = -1;

    try
    {
        std::string keyonlypath = InterpretKeyPath(key);


#if _DEBUG
        std::string sValueName = "NULL";
        if (lpValueName != NULL)
            sValueName = narrow(lpValueName);
        Log(L"[%d] RegQueryValueExW:  key=0x%x keyname=%S ValueName=%S", RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), sValueName.c_str());
#endif
        DWORD dwType;
        result = impl::KernelBaseRegQueryValueExW(key, lpValueName, lpReservered, &dwType, lpData, lpcbData);
        if (lpDwType != NULL)
        {
            *lpDwType = dwType;
        }
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = "";
            result = RegFixupDeletionMarker(keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
#if _DEBUG
                if (lpcbData != NULL)
                {
                    switch (dwType)
                    {
                    case REG_SZ:
                        if (lpData != NULL)
                        {
                            wchar_t* rstring = new wchar_t[(*lpcbData) + 2];
                            FillMemory(rstring, (*lpcbData) + 2, 0);
                            memcpy(rstring, lpData, *lpcbData);
                            LogString(RegLocalInstance, L"RegQueryValueExW: Returning success with value", rstring);
                        }
                        else
                        {
                            Log(L"[%d] RegQueryValueExW:  Returning success with no data", RegLocalInstance);
                        }
                        break;
                    case REG_DWORD:
                        if (lpData != NULL)
                        {
                            Log(L"[% d] RegQueryValueExW:  Returning success with DWORD x0x%x", RegLocalInstance, *((DWORD*)lpData));
                        }
                        else
                        {
                            Log(L"[% d] RegQueryValueExW:  Returning success with DWORD", RegLocalInstance);
                        }

                        break;
                    default:
                        Log(L"[%d] RegQueryValueExW:  Returning success of type 0x%x", RegLocalInstance, dwType);
                        break;
                    }
                }
                else
                {
                    Log(L"[%d] RegQueryValueExW:  Returning success", RegLocalInstance);
                }
#endif                
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
#if _DEBUG
                Log(L"[%d] RegQueryValueExW:  DeletionMarker Blocking this call.", RegLocalInstance);
#endif                

            }
        }
        else
        {
#if _DEBUG
            Log(L"[%d] RegQueryValueExW:  Returning normal failure 0x%x.", RegLocalInstance, result);
#endif                
        }
    }
    catch (...)
    {
        Log(L"[%d] RegQueryValueEx:  Exception thrown.", RegLocalInstance);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegQueryValueExW, RegQueryValueExWFixup);

#endif