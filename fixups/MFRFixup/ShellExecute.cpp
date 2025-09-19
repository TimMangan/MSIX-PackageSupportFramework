//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea


#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include <psf_logging.h>
#include "FunctionImplementations.h"


#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"


#if FIXUP_ORIGINAL_SHELLEXECUTE
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
                // Release level logging for detection
                bool temp = g_psf_NoLogging;
                g_psf_NoLogging = false;
                Log(LogLevel_DebugBasic, L"[%s%d] ShellExecuteA unguarded. Known compatibility issues exist in certain usages!", g_MfrModuleName, dllInstance);
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteA: file", lpFile);
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteA: verb", lpOperation);
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteA: directory", lpDirectory);
                LogCallingModuleInstance(g_MfrModuleName, dllInstance);
                g_psf_NoLogging = temp;
            }
            retfinal = impl::ShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
            return retfinal;
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteA")

    retfinal = impl::ShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
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

    //auto guard = g_reentrancyGuard.enter();
    HINSTANCE  retfinal;

    try
    {
        //if (guard)
        {
            if (lpOperation)
            {
                // Release level logging for detection
                bool temp = g_psf_NoLogging;
                g_psf_NoLogging = false;
                Log(LogLevel_DebugBasic, L"[%s%d] ShellExecuteW unguarded informational. Known compatibility issues exist in certain usages!", g_MfrModuleName, dllInstance);
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteW: file", lpFile);
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteW: verb", lpOperation);
                LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteW: directory", lpDirectory);
                LogCallingModuleInstance(LogLevel_DebugBasic, g_MfrModuleName, dllInstance);
                g_psf_NoLogging = temp;
            }
            retfinal = impl::ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
            return retfinal;
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"ShellExecuteW")

    retfinal = impl::ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory,nShowCmd);
    return retfinal;
}
DECLARE_FIXUP(impl::ShellExecuteW, ShellExecuteWFixup);
#endif