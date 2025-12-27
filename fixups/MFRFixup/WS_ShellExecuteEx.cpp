// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Microsoft documentation of this API: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecuteexa

// There are hints that this might be being called because the app called WIndows.System.Launcher.Launch{File,Uri}Async()
// Possibly this means the app made that call and then moved on to other things while we get around to processing this call.
// This might confuse your debugging.

#if _DEBUG
//#define MOREDEBUG 1 
#endif

#include <errno.h>
#include <psf_logging.h>
#include "FunctionImplementations.h"
#include "FunctionImplementations_WindowsStorage.h"

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"

#if Intercept_WindowsStorage

#ifdef DO_WS_ShexEx_A

BOOL __stdcall WS_ShellExecuteExAFixup(
    _In_ _Out_ SHELLEXECUTEINFOA    *pExecInfo) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] BOOL retfinal;


    try
    {
        auto guard = g_reentrancyGuard.enter();


        if (guard)
        {

            // Release level logging for detection
            bool temp = g_psf_NoLogging;
            g_psf_NoLogging = false;
            Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecutEx()", g_MfrModuleName, dllInstance);
            LogCallingModuleInstance(g_MfrModuleName, dllInstance);
            g_psf_NoLogging = temp;
            

            
            retfinal = ::ShellExecuteExA(pExecInfo);
            return retfinal;
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] (Windows.Storage)ShellExecute Exception=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }

    retfinal = ::ShellExecuteExA(pExecInfo);
    return retfinal;
}
DECLARE_FIXUP(windowsstorageimpl::ShellExecuteExAImpl, WS_ShellExecuteExAFixup);

#endif

#if DO_WS_ShexEx_W
BOOL __stdcall WS_ShellExecuteExWFixup(
    _In_ _Out_ SHELLEXECUTEINFOW* pExecInfo) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] BOOL retfinal;


    try
    {
        //auto guard = g_reentrancyGuard.enter();


        //if (guard)
        {
            // Release level logging for detection
            Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteExW() [Informational]", g_MfrModuleName, dllInstance);
            if (pExecInfo != NULL)
            {
                if (pExecInfo->lpDirectory != NULL)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteExW() dir=%s", g_MfrModuleName, dllInstance, pExecInfo->lpDirectory);
                }
                if (pExecInfo->lpFile != NULL)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteExW() file=%s", g_MfrModuleName, dllInstance, pExecInfo->lpFile);
                }
                if (pExecInfo->lpVerb != NULL)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteExW() verb=%s", g_MfrModuleName, dllInstance, pExecInfo->lpVerb);
                }
            }
            LogCallingModuleInstance(g_MfrModuleName, dllInstance);
            


            retfinal = ::ShellExecuteExW(pExecInfo);
            if (retfinal)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteExW() [Informational] returns SUCCESS", g_MfrModuleName, dllInstance);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteExW() [Informational] returns ERROR=0x%x", g_MfrModuleName, dllInstance, GetLastError());
            }
            return retfinal;
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] (Windows.Storage)ShellExecuteExW Exception=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }

    retfinal = ::ShellExecuteExW(pExecInfo);
    return retfinal;
}
DECLARE_FIXUP(windowsstorageimpl::ShellExecuteExWImpl, WS_ShellExecuteExWFixup);
#endif
#endif