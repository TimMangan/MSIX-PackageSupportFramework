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
#include <roapi.h>
#include <winstring.h>

#ifdef Intercept_CoCreateInstance
extern const wchar_t* g_PsfRunTimeName;
inline thread_local psf::reentrancy_guard g_reentrancyGuard;


namespace impl
{
    inline auto RoActivateInstance = &::RoActivateInstance;
    //inline auto GetActivationFactory = &::GetActivationFactory; This is async function and we don't know how to intercept (yet).
}

// Helper function to convert HSTRING to wstring
std::wstring HStringToWString(HSTRING hstr)
{
    UINT32 len = 0;
    const wchar_t* buf = WindowsGetStringRawBuffer(hstr, &len);
    return std::wstring(buf, len);
}

#include <sstream>
#include <iomanip>

extern DWORD g_CoCreateInstanceInterceptInstance;


HRESULT WINAPI RoActivateInstanceFixup(
    _In_  HSTRING  activatableClassId,
    _In_  IInspectable** instance
)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            DWORD CoCreateInstanceInterceptInstance = ++g_CoCreateInstanceInterceptInstance;

            std::wstring className = HStringToWString(activatableClassId);
            Log(LogLevel_DebugMaximum, L" [%s%d] RoActivateInstanceFixup: (Informational) Class=%s", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, className.c_str());

            HRESULT hr = impl::RoActivateInstance(activatableClassId, instance);

            if (SUCCEEDED(hr))
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] RoActivateInstanceFixup: Succeeded", g_PsfRunTimeName, CoCreateInstanceInterceptInstance);
            }
            else
            {
                Log(LogLevel_DebugMaximum, L" [%s%d] RoActivateInstanceFixup: FAILED with HResult=0x%x", g_PsfRunTimeName, CoCreateInstanceInterceptInstance, hr);
            }
            return hr;
        }
        else
        {
            return impl::RoActivateInstance(activatableClassId, instance);
        }
    }
    else
    {
        // Call the real function
        return impl::RoActivateInstance(activatableClassId, instance);
    }
}
DECLARE_FIXUP(impl::RoActivateInstance, RoActivateInstanceFixup);


#endif
