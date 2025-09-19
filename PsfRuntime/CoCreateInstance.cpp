//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// The PsfRuntime intercepts all AddDirectory and SetDirectory[AW] calls so that paths may be fixed up. 

#define Intercept_CoCreateInstance 1

#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <psf_constants.h>
#include <psf_framework.h>
#include <psf_logging.h>

#include "Config.h"
#include <StartInfo_helper.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <findStringIC.h>

#include <shobjidl.h> 

using namespace std::literals;


#include <reentrancy_guard.h>
#include <psf_framework.h>

#ifdef Intercept_CoCreateInstance
extern const wchar_t* g_PsfRunTimeName;
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    inline auto CLSIDFromProgID = &::CLSIDFromProgID;
    inline auto CLSIDFromProgIDEx = &::CLSIDFromProgIDEx;
    // ProgIdFromCLSID is not intercepted here, but could be if needed.
    inline auto CoCreateInstance = &::CoCreateInstance;
    inline auto CoCreateInstanceEx = &::CoCreateInstanceEx;
    inline auto CoCreateInstanceFromApp = &::CoCreateInstanceFromApp;
    inline auto CoGetClassObject = &::CoGetClassObject;
}

// Helper function to convert GUID to wstring
#include <sstream>
#include <iomanip>

inline std::wstring GuidToString(const GUID& guid)
{
    std::wstringstream ss;
    try
    {
        ss << std::hex << std::setfill(L'0')
            << L'{'
            << std::setw(8) << guid.Data1 << L'-'
            << std::setw(4) << guid.Data2 << L'-'
            << std::setw(4) << guid.Data3 << L'-'
            << std::setw(2) << static_cast<int>(guid.Data4[0])
            << std::setw(2) << static_cast<int>(guid.Data4[1]) << L'-'
            << std::setw(2) << static_cast<int>(guid.Data4[2])
            << std::setw(2) << static_cast<int>(guid.Data4[3])
            << std::setw(2) << static_cast<int>(guid.Data4[4])
            << std::setw(2) << static_cast<int>(guid.Data4[5])
            << std::setw(2) << static_cast<int>(guid.Data4[6])
            << std::setw(2) << static_cast<int>(guid.Data4[7])
            << L'}';
        return ss.str();
    }
    catch (...)
    {
        return L"{BAD}";
    }
}

std::wstring ContextToString(DWORD     dwClsContext)
{
    std::wstring result = L"";
    if (dwClsContext & CLSCTX_INPROC_SERVER) result += L"CLSCTX_INPROC_SERVER|";
    if (dwClsContext & CLSCTX_INPROC_HANDLER) result += L"CLSCTX_INPROC_HANDLER|";
    if (dwClsContext & CLSCTX_LOCAL_SERVER) result += L"CLSCTX_LOCAL_SERVER|";
    if (dwClsContext & CLSCTX_REMOTE_SERVER) result += L"CLSCTX_REMOTE_SERVER|";
    if (dwClsContext & CLSCTX_INPROC_SERVER16) result += L"CLSCTX_INPROC_SERVER16|";
    if (dwClsContext & CLSCTX_INPROC_HANDLER16) result += L"CLSCTX_INPROC_HANDLER16|";
    if (dwClsContext & CLSCTX_RESERVED1) result += L"CLSCTX_RESERVED1|";
    if (dwClsContext & CLSCTX_RESERVED2) result += L"CLSCTX_RESERVED2|";
    if (dwClsContext & CLSCTX_RESERVED3) result += L"CLSCTX_RESERVED3|";
    if (dwClsContext & CLSCTX_RESERVED4) result += L"CLSCTX_RESERVED4|";
    if (dwClsContext & CLSCTX_NO_CODE_DOWNLOAD) result += L"CLSCTX_NO_CODE_DOWNLOAD|";
    if (dwClsContext & CLSCTX_RESERVED5) result += L"CLSCTX_RESERVED5|";
    if (dwClsContext & CLSCTX_NO_CUSTOM_MARSHAL) result += L"CLSCTX_NO_CUSTOM_MARSHAL|";
    if (dwClsContext & CLSCTX_ENABLE_CODE_DOWNLOAD) result += L"CLSCTX_ENABLE_CODE_DOWNLOAD|";
    if (dwClsContext & CLSCTX_NO_FAILURE_LOG) result += L"CLSCTX_NO_FAILURE_LOG|";
    if (dwClsContext & CLSCTX_DISABLE_AAA) result += L"CLSCTX_DISABLE_AAA|";
    if (dwClsContext & CLSCTX_ENABLE_AAA) result += L"CLSCTX_ENABLE_AAA|";
    if (dwClsContext & CLSCTX_FROM_DEFAULT_CONTEXT) result += L"CLSCTX_FROM_DEFAULT_CONTEXT|";
    if (dwClsContext & CLSCTX_ACTIVATE_X86_SERVER) result += L"CLSCTX_ACTIVATE_X86_SERVER|";
    if (dwClsContext & CLSCTX_ACTIVATE_32_BIT_SERVER) result += L"CLSCTX_ACTIVATE_32_BIT_SERVER|";
    if (dwClsContext & CLSCTX_ACTIVATE_64_BIT_SERVER) result += L"CLSCTX_ACTIVATE_64_BIT_SERVER|";
    if (dwClsContext & CLSCTX_ENABLE_CLOAKING) result += L"CLSCTX_ENABLE_CLOAKING|";
    if (dwClsContext & CLSCTX_APPCONTAINER) result += L"CLSCTX_APPCONTAINER|";
    if (dwClsContext & CLSCTX_ACTIVATE_AAA_AS_IU) result += L"CLSCTX_ACTIVATE_AAA_AS_IU|";
    if (dwClsContext & CLSCTX_RESERVED6) result += L"CLSCTX_RESERVED6|";
    if (dwClsContext & CLSCTX_ACTIVATE_ARM32_SERVER) result += L"CLSCTX_ACTIVATE_ARM32_SERVER|";
    if (dwClsContext & CLSCTX_ALLOW_LOWER_TRUST_REGISTRATION) result += L"CLSCTX_ALLOW_LOWER_TRUST_REGISTRATION|";
    if (dwClsContext & CLSCTX_PS_DLL) result += L"CLSCT|";
    if (!result.empty())    
        result.pop_back(); // Remove trailing |
    return result;
}


DWORD g_CoCreateInstanceInterceptInstance = 22000;

//Looks up a CLSID in the registry, given a ProgID.
HRESULT WINAPI CLSIDFromProgIDFixup(
    _In_  LPCOLESTR lpszProgID,
    _Out_ LPCLSID   lpclsid)
{

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // We want to be able to evaluate if COM instance creation is funtional for all of the COM interface calls being made by the application,
        // but only in extreme debugging scenarios for now.
        // So we will log the call parameters and the result, but not modify any of the parameters or the result.
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            DWORD CoCreateInstanceInterceptInstance = ++g_CoCreateInstanceInterceptInstance;
            Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDFixup: (Informational) ProgId=%s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, lpszProgID);
            Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, g_CoCreateInstanceInterceptInstance);
            HRESULT hr = impl::CLSIDFromProgID(lpszProgID, lpclsid);
            if (SUCCEEDED(hr))
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDFixup:     CLSID=%s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, GuidToString(*lpclsid).c_str());
                Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDFixup: Succeeded", g_PsfRunTimeName, CoCreateInstanceInterceptInstance);
            }
            else
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDFixup: FAILED with HResult=0x%x", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, hr);
            }
            return hr;
        }
    }
    return impl::CLSIDFromProgID(lpszProgID, lpclsid);
}
DECLARE_FIXUP(impl::CLSIDFromProgID, CLSIDFromProgIDFixup);


//Triggers automatic installation if the COMClassStore policy is enabled.
//This is analogous to the behavior of CoCreateInstance when neither CLSCTX_ENABLE_CODE_DOWNLOAD nor CLSCTX_NO_CODE_DOWNLOAD are specified.
HRESULT WINAPI CLSIDFromProgIDExFixup(
    _In_  LPCOLESTR lpszProgID,
    _Out_ LPCLSID   lpclsid)
{

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // We want to be able to evaluate if COM instance creation is funtional for all of the COM interface calls being made by the application,
        // but only in extreme debugging scenarios for now.
        // So we will log the call parameters and the result, but not modify any of the parameters or the result.
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            DWORD CoCreateInstanceInterceptInstance = ++g_CoCreateInstanceInterceptInstance;
            Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDExFixup: (Informational) ProgId=%s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, lpszProgID);
            Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDExFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, g_CoCreateInstanceInterceptInstance);
            HRESULT hr = impl::CLSIDFromProgIDEx(lpszProgID, lpclsid);
            if (SUCCEEDED(hr))
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDExFixup:     CLSID=%s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, GuidToString(*lpclsid).c_str());
                Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDExFixup: Succeeded", g_PsfRunTimeName, CoCreateInstanceInterceptInstance);
            }
            else
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CLSIDFromProgIDExFixup: FAILED with HResult=0x%x", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, hr);
            }
            return hr;
        }
    }
    return impl::CLSIDFromProgIDEx(lpszProgID, lpclsid);
}
DECLARE_FIXUP(impl::CLSIDFromProgIDEx, CLSIDFromProgIDExFixup);


// Creates and default-initializes a single object of the class associated with a specified CLSID.
// Call CoCreateInstance when you want to create only one object on the local system.To create a single object on a remote system, call the CoCreateInstanceEx function.To create multiple objects based on a single CLSID, call the CoGetClassObject function.
HRESULT WINAPI CoCreateInstanceFixup(
    _In_  REFCLSID  rclsid,
    _In_  LPUNKNOWN pUnkOuter,
    _In_  DWORD     dwClsContext,
    _In_  REFIID    riid,
    _Out_ LPVOID* ppv
)
{
    
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // We want to be able to evaluate if COM instance creation is funtional for all of the COM interface calls being made by the application,
        // but only in extreme debugging scenarios for now.
        // So we will log the call parameters and the result, but not modify any of the parameters or the result.
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            DWORD CoCreateInstanceInterceptInstance = ++g_CoCreateInstanceInterceptInstance;
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFixup: (Informational) CLSID=%s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, GuidToString(rclsid).c_str());
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, g_CoCreateInstanceInterceptInstance);
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFixup:     Context=0x%x %s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, dwClsContext, ContextToString(dwClsContext).c_str());
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFixup:     Interface=%s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, GuidToString(riid).c_str());

            HRESULT hr = impl::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
            if (SUCCEEDED(hr))
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFixup: Succeeded", g_PsfRunTimeName, CoCreateInstanceInterceptInstance);
            }
            else
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFixup: FAILED with HResult=0x%x", g_PsfRunTimeName, CoCreateInstanceInterceptInstance,hr);
            }
            return hr;
        }
    }
    
    return impl::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}
DECLARE_FIXUP(impl::CoCreateInstance, CoCreateInstanceFixup);



// Creates an instance of a specific class on a specific computer.
HRESULT WINAPI CoCreateInstanceExFixup(
    _In_  REFCLSID  rclsid,
    _In_  IUnknown   *pUnkOuter,
    _In_  DWORD     dwClsCtx,
    _In_  COSERVERINFO* pServerInfo,
    _In_  DWORD     dwCount,
    _Inout_  MULTI_QI* pResults
)
{

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // We want to be able to evaluate if COM instance creation is funtional for all of the COM interface calls being made by the application,
        // but only in extreme debugging scenarios for now.
        // So we will log the call parameters and the result, but not modify any of the parameters or the result.
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            DWORD CoCreateInstanceInterceptExInstance = ++g_CoCreateInstanceInterceptInstance;
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup: (Informational) CLSID=%s", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance, GuidToString(rclsid).c_str());
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, g_CoCreateInstanceInterceptInstance);
            ///Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup:     IUnknown=%s", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance, pUnkOuter->QueryInterface->riid);
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup:     Context=0x%x %s", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance, dwClsCtx, ContextToString(dwClsCtx).c_str());
            try
            {
                if (pServerInfo != NULL && pServerInfo->pwszName != NULL)
                    Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup:     ServerInfo=%s", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance, pServerInfo->pwszName);
            }
            catch (...)
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup:     ServerInfo=EXCEPTION", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance);
            }
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup:     Count=%d", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance, dwCount);

            HRESULT hr = impl::CoCreateInstanceEx(rclsid, pUnkOuter, dwClsCtx, pServerInfo, dwCount, pResults);
            if (SUCCEEDED(hr))
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup: Succeeded", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance);
            }
            else
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceExFixup: FAILED with HResult=0x%x", g_PsfRunTimeName, CoCreateInstanceInterceptExInstance, hr);
            }
            return hr;
        }
    }

    return impl::CoCreateInstanceEx(rclsid, pUnkOuter, dwClsCtx, pServerInfo, dwCount, pResults);
}
DECLARE_FIXUP(impl::CoCreateInstanceEx, CoCreateInstanceExFixup);


// 
// Creates an instance of a specific class on a specific computer from within an app container.
HRESULT WINAPI CoCreateInstanceFromAppFixup(
    _In_  REFCLSID  rclsid,
    _In_  IUnknown* pUnkOuter,
    _In_  DWORD     dwClsCtx,
    _In_opt_  PVOID reserved,
    _In_  DWORD     dwCount,
    _Inout_  MULTI_QI* pResults
)
{

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // We want to be able to evaluate if COM instance creation is functional for all of the COM interface calls being made by the application,
        // but only in extreme debugging scenarios for now.
        // So we will log the call parameters and the result, but not modify any of the parameters or the result.
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            DWORD CoCreateInstanceInterceptFromAppInstance = ++g_CoCreateInstanceInterceptInstance;
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFromAppFixup: (Informational) CLSID=%s", g_PsfRunTimeName, CoCreateInstanceInterceptFromAppInstance, GuidToString(rclsid).c_str());
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFromAppFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, g_CoCreateInstanceInterceptInstance);
            ///Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFromAppFixup:     IUnknown=%s", g_PsfRunTimeName, CoCreateInstanceInterceptFromAppInstance, pUnkOuter->QueryInterface->riid);
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFromAppFixup:     Context=0x%x %s", g_PsfRunTimeName, CoCreateInstanceInterceptFromAppInstance, dwClsCtx, ContextToString(dwClsCtx).c_str());
            Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFromAppFixup:     Count=%d", g_PsfRunTimeName, CoCreateInstanceInterceptFromAppInstance, dwCount);

            HRESULT hr = impl::CoCreateInstanceFromApp(rclsid, pUnkOuter, dwClsCtx, reserved, dwCount, pResults);
            if (SUCCEEDED(hr))
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFromAppFixup: Succeeded", g_PsfRunTimeName, CoCreateInstanceInterceptFromAppInstance);
            }
            else
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoCreateInstanceFromAppFixup: FAILED with HResult=0x%x", g_PsfRunTimeName, CoCreateInstanceInterceptFromAppInstance, hr);
            }
            return hr;
        }
    }

    return impl::CoCreateInstanceFromApp(rclsid, pUnkOuter, dwClsCtx, reserved, dwCount, pResults);
}
DECLARE_FIXUP(impl::CoCreateInstanceFromApp, CoCreateInstanceFromAppFixup);



// Provides a pointer to an interface on a class object associated with a specified CLSID. 
// CoGetClassObject locates, and if necessary, dynamically loads the executable code required to do this.
HRESULT WINAPI CoGetClassObjectFixup(
    _In_  REFCLSID  rclsid,
    _In_  DWORD     dwClsContext,
    _In_opt_  LPVOID pvReserved,
    _In_  REFIID     riid,
    _Out_  LPVOID *ppv
)
{

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        // We want to be able to evaluate if COM instance creation is functional for all of the COM interface calls being made by the application,
        // but only in extreme debugging scenarios for now.
        // So we will log the call parameters and the result, but not modify any of the parameters or the result.
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            DWORD CoGetClassObjectInstance = ++g_CoCreateInstanceInterceptInstance;
            Log(LogLevel_DebugMaximum, L" [%s%d] CoGetClassObjectFixup: (Informational) CLSID=%s", g_PsfRunTimeName, CoGetClassObjectInstance, GuidToString(rclsid).c_str());
            Log(LogLevel_DebugMaximum, L" [%s%d] CoGetClassObjectFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, g_CoCreateInstanceInterceptInstance);
            Log(LogLevel_DebugMaximum, L" [%s%d] CoGetClassObjectFixup:     Context=0x%x %s", g_PsfRunTimeName, CoGetClassObjectInstance, dwClsContext, ContextToString(dwClsContext).c_str());
            Log(LogLevel_DebugMaximum, L" [%s%d] CoGetClassObjectFixup:     riid=%s", g_PsfRunTimeName, CoGetClassObjectInstance, GuidToString(riid).c_str());

            HRESULT hr = impl::CoGetClassObject(rclsid, dwClsContext, pvReserved, riid, ppv);
            if (SUCCEEDED(hr))
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoGetClassObjectFixup: Succeeded", g_PsfRunTimeName, CoGetClassObjectInstance);
            }
            else
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] CoGetClassObjectFixup: FAILED with HResult=0x%x", g_PsfRunTimeName, CoGetClassObjectInstance, hr);
            }
            return hr;
        }
    }

    return impl::CoGetClassObject(rclsid, dwClsContext, pvReserved, riid, ppv);
}
DECLARE_FIXUP(impl::CoGetClassObject, CoGetClassObjectFixup);



#endif