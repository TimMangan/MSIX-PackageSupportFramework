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
LSTATUS __stdcall RegQueryValueExAFixup(
    _In_ HKEY key,
    _In_opt_ LPCSTR lpValueName,
    LPDWORD lpReservered,
    _Out_opt_ LPDWORD lpDwType,
    _Out_opt_ PVOID lpData,
    _In_opt_ _Out_opt_ LPDWORD lpcbData)
{
    // Copilot suggested this, but it breaks stuff.
    //if (lpDwType) *lpDwType = 0;
    //if (lpcbData) *lpcbData = 0;
    //if (lpData && lpcbData) memset(lpData, 0, *lpcbData);

    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;

    try
    {
        std::string keyonlypath = InterpretKeyPath(key);


        std::string sValueName = "NULL";
        if (lpValueName != NULL)
            sValueName = lpValueName;
        Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA:  key=0x%x keyname=%S ValueName=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), sValueName.c_str());

        DWORD dwType;
        result = impl::KernelBaseRegQueryValueExA(key, lpValueName, lpReservered, &dwType, lpData, lpcbData);
        if (lpDwType != NULL)
        {
            *lpDwType = dwType;
        }
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = "";
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
                try
                {
                    switch (dwType)
                    {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                    case REG_MULTI_SZ:
                        if (lpData != NULL)
                        {
                            if (lpcbData != NULL)
                            {
                                char* rstring = new char[(*lpcbData) + 1];
                                FillMemory(rstring, (*lpcbData) + 1, 0);
                                memcpy(rstring, lpData, *lpcbData);
                                LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"RegQueryValueExA: Returning success with value", rstring);
                            }
                        }
                        else
                        {
                            if (lpcbData != NULL)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  Returning success with string no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, *lpcbData);
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  Returning success with string no data", g_RegModuleName, RegLocalInstance);
                            }
                        }
                        break;
                    case REG_DWORD:
                        if (lpData != NULL)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  Returning success with DWORD 0x%x", g_RegModuleName, RegLocalInstance, *((DWORD*)lpData));
                        }
                        else
                        {
                            if (lpcbData != NULL)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  Returning success with DWORD, len needed=0x%x", g_RegModuleName, RegLocalInstance, *lpcbData);
                            }
                            else
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  Returning success with DWORD no data", g_RegModuleName, RegLocalInstance);
                        }
                        break;
                    default:
                        if (lpData != NULL)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  Returning success of type 0x%x", g_RegModuleName, RegLocalInstance, dwType);
                        }
                        else
                        {
                            if (lpcbData != NULL)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  Returning success of type 0x%x no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, dwType, *lpcbData);
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, "[%s%d] RegQueryValueExA:  Returning success of type 0x%x no data", g_RegModuleName, RegLocalInstance, dwType);
                            }
                        }
                        break;
                    }
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegQueryValueExA:  Exception thrown reading data.", g_RegModuleName, RegLocalInstance);
                }
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExA:  DeletionMarker Blocking this call.", g_RegModuleName, RegLocalInstance);
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExA:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
        }
    }
    catch (...)
    {
        // Copilot suggested this, but it breaks stuff.
        //if (lpDwType) *lpDwType = 0;
        //if (lpcbData) *lpcbData = 0;
        //if (lpData && lpcbData) memset(lpData, 0, *lpcbData);

        Log(LogLevel_Exception, L"[%s%d] RegQueryValueExA:  Exception thrown.", g_RegModuleName, RegLocalInstance);
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
    DWORD RegLocalInstance = ++g_RegInterceptInstance;
    LSTATUS result = -1;

    try
    {
        std::string keyonlypath = InterpretKeyPath(key);


        std::string sValueName = "NULL";
        if (lpValueName != NULL)
            sValueName = narrow(lpValueName);
        Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW:  key=0x%x keyname=%S ValueName=%S", g_RegModuleName, RegLocalInstance, (ULONG)(ULONG_PTR)key, keyonlypath.c_str(), sValueName.c_str());

        DWORD dwType;
        result = impl::KernelBaseRegQueryValueExW(key, lpValueName, lpReservered, &dwType, lpData, lpcbData);
        if (lpDwType != NULL)
        {
            *lpDwType = dwType;
        }
        if (result == ERROR_SUCCESS)
        {
            std::string sskey = "";
            result = RegFixupDeletionMarker(LogLevel_DebugMaximum, keyonlypath, sskey, RegLocalInstance);
            if (result == ERROR_SUCCESS)
            {
                try
                {
                    switch (dwType)
                    {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                    case REG_MULTI_SZ:
                        if (lpData != NULL)
                        {
                            if (lpcbData != NULL)
                            {
                                wchar_t* rstring = new wchar_t[(*lpcbData) + 2];
                                FillMemory(rstring, (*lpcbData) + 2, 0);
                                memcpy(rstring, lpData, *lpcbData);
                                LogString(LogLevel_DebugIntermediate, g_RegModuleName, RegLocalInstance, L"RegQueryValueExW: Returning success with value", rstring);
                            }
                        }
                        else
                        {
                            if (lpcbData != NULL)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success with string no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, *lpcbData);
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success with string no data", g_RegModuleName, RegLocalInstance);
                            }
                        }
                        break;
                    case REG_DWORD:
                        if (lpData != NULL)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success with DWORD 0x%x", g_RegModuleName, RegLocalInstance, *((DWORD*)lpData));
                        }
                        else
                        {
                            if (lpcbData != NULL)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success with DWORD no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, *lpcbData);
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success with DWORD no data", g_RegModuleName, RegLocalInstance);
                            }
                        }
                        break;
                    default:
                        if (lpData != NULL)
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success of type 0x%x", g_RegModuleName, RegLocalInstance, dwType);
                        }
                        else
                        {
                            if (lpcbData != NULL)
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success of type 0x%x no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, dwType, *lpcbData);
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  Returning success of type 0x%x no data", g_RegModuleName, RegLocalInstance, dwType);
                            }
                        }
                        break;
                    }
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L"[%s%d] RegQueryValueExW:  Exception thrown during debug logging.", g_RegModuleName, RegLocalInstance);
                }  
            }
            else
            {
                // We have a deletion marker on this particular item, so we need to skip it.
                // When we return this value, a subsequent call by the app might ask for this new index, but we can probably assume it's OK to return it twice
                // because we do not have a way to remember this, like done in FindFirstFile.
                Log(LogLevel_DebugIntermediate, L"[%s%d] RegQueryValueExW:  DeletionMarker Blocking this call.", g_RegModuleName, RegLocalInstance);

            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] RegQueryValueExW:  Returning normal failure 0x%x.", g_RegModuleName, RegLocalInstance, result);
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] RegQueryValueEx:  Exception thrown.", g_RegModuleName, RegLocalInstance);
    }
    return result;
}
DECLARE_FIXUP(impl::KernelBaseRegQueryValueExW, RegQueryValueExWFixup);

#endif