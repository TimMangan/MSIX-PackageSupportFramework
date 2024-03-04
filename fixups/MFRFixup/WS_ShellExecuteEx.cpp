// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Microsoft documentation of this API: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecuteexa


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

            if (moredebug)
            {
                // Release level logging for detection
                bool temp = g_psf_NoLogging;
                g_psf_NoLogging = false;
                Log(L"[%d] (Windows.Storage)ShellExecutEx()", dllInstance);
                LogCallingModule();
                g_psf_NoLogging = temp;
            }

            
            retfinal = ::ShellExecuteExA(pExecInfo);
            return retfinal;
        }
    }
    catch (...)
    {
        Log(L"[%d] (Windows.Storage)ShellExecute Exception=0x%x", dllInstance, GetLastError());
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
        auto guard = g_reentrancyGuard.enter();


        if (guard)
        {

            if (moredebug)
            {
                // Release level logging for detection
                bool temp = g_psf_NoLogging;
                g_psf_NoLogging = false; 
                Log(L"[%d] (Windows.Storage)ShellExecutExW()", dllInstance);
                Log(L"[%d] (Windows.Storage)ShellExecute() unfixed  dir=%ls, file=%ls, verb=%ls", dllInstance, pExecInfo->lpDirectory, pExecInfo->lpFile, pExecInfo->lpVerb);
                LogCallingModule();
                g_psf_NoLogging = temp;
            }


            retfinal = ::ShellExecuteExW(pExecInfo);
            return retfinal;
        }
    }
    catch (...)
    {
        Log(L"[%d] (Windows.Storage)ShellExecute Exception=0x%x", dllInstance, GetLastError());
    }

    retfinal = ::ShellExecuteExW(pExecInfo);
    return retfinal;
}
DECLARE_FIXUP(windowsstorageimpl::ShellExecuteExWImpl, WS_ShellExecuteExWFixup);
#endif
#endif