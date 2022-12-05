//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea


#if _DEBUG
#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"



HINSTANCE __stdcall ShellExecuteAFixup(_In_opt_ HWND   hwnd,
                                _In_opt_  LPCSTR lpOperation,
                                _In_      LPCSTR lpFile,
                                _In_opt_  LPCSTR lpParameters,
                                _In_opt_  LPCSTR lpDirectory,
                                _In_      INT    nShowCmd) 
{
    DWORD dllInstance = g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    HINSTANCE  retfinal;

    try
    {
        if (guard)
        {
            if (lpOperation)
            {
                Log(L"[%d] ShellExecuteA intercepted without alteration. Known compatibility issues exist in certain usages!", dllInstance);

                LogString(dllInstance, L"ShellExecuteA: verb", lpOperation);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"ShellExecuteA")
#else
    catch (...)
    {
        Log(L"[%d] ShellExecuteA Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    Log(L"[%d] ShellExecuteA unguarded", dllInstance);
    LogString(dllInstance, L"ShellExecuteA: verb", lpOperation);


    retfinal = ::ShellExecuteA(hwnd,
                            lpOperation,
                            lpFile,
                            lpParameters,
                            lpDirectory,
                            nShowCmd);
    return retfinal;
}
DECLARE_FIXUP(impl::ShellExecuteA, ShellExecuteAFixup);




HINSTANCE __stdcall ShellExecuteWFixup(_In_opt_ HWND   hwnd,
    _In_opt_  LPCWSTR lpOperation,
    _In_      LPCWSTR lpFile,
    _In_opt_  LPCWSTR lpParameters,
    _In_opt_  LPCWSTR lpDirectory,
    _In_      INT    nShowCmd)
{
    DWORD dllInstance = g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    HINSTANCE  retfinal;

    try
    {
        if (guard)
        {
            if (lpOperation)
            {
                Log(L"[%d] ShellExecuteW intercepted without alteration. Known compatibility issues exist in certain usages!", dllInstance);

                LogString(dllInstance, L"ShellExecuteW: verb", lpOperation);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"ShellExecuteW")
#else
    catch (...)
    {
        Log(L"[%d] ShellExecuteW Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    Log(L"[%d] ShellExecuteW unguarded", dllInstance);
    LogString(dllInstance, L"ShellExecuteW: verb", lpOperation);

    retfinal = ::ShellExecuteW(hwnd,
        lpOperation,
        lpFile,
        lpParameters,
        lpDirectory,
        nShowCmd);
    return retfinal;
}
DECLARE_FIXUP(impl::ShellExecuteW, ShellExecuteWFixup);
