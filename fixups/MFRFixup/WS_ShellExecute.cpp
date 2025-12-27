// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Microsoft documentation of this API: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew


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

#ifdef DO_WS_Shex_A

HINSTANCE __stdcall WS_ShellExecuteAFixup(
    _In_opt_ HWND    hwnd,
    _In_opt_ const char* lpOperation,
    _In_     const char* lpFile,
    _In_opt_ const char* lpParameters,
    _In_opt_ const char* lpDirectory,
    _In_     INT     nShowCmd) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] HINSTANCE retfinal;

    try
    {
        //auto guard = g_reentrancyGuard.enter();
        

        //if (guard)
        {
            dllInstance = ++g_InterceptInstance;

            Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA [Informational]", g_MfrModuleName, dllInstance);
            if (lpOperation != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA() operation=%S", g_MfrModuleName, dllInstance, lpOperation);
                std::wstring pwszOperation = widen(lpOperation);
                if (pwszOperation._Equal(L"find"))
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] may need to debug find operation further...", g_MfrModuleName, dllInstance);
                }
            }
            if (lpFile != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA() file=%S", g_MfrModuleName, dllInstance, lpFile);
            }
            if (lpParameters != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA() parameters=%S", g_MfrModuleName, dllInstance, lpParameters);
            }
            if (lpDirectory != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA() directory=%S", g_MfrModuleName, dllInstance, lpDirectory);
            }
            Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA() nShowCmd=%d", g_MfrModuleName, dllInstance, nShowCmd);

            LogCallingModuleInstance(g_MfrModuleName, dllInstance);
            
            
            retfinal = ::ShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
            if (retfinal)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA() [Informational] returns SUCCESS", g_MfrModuleName, dllInstance);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteA() [Informational] returns ERROR=0x%x", g_MfrModuleName, dllInstance, GetLastError());
            }
            return retfinal;
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] (Windows.Storage)ShellExecuteA Exception=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }
   
    retfinal = ::ShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
    return retfinal;
}
DECLARE_FIXUP(windowsstorageimpl::ShellExecuteAImpl, WS_ShellExecuteAFixup);

#endif
#if DO_WS_Shex_W

HINSTANCE __stdcall WS_ShellExecuteWFixup(
    _In_opt_ HWND    hwnd,
    _In_opt_ LPCWSTR lpOperation,
    _In_     LPCWSTR  lpFile,
    _In_opt_ LPCWSTR lpParameters,
    _In_opt_ LPCWSTR lpDirectory,
    _In_     INT     nShowCmd) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] HINSTANCE retfinal;

   

    try
    {
        //auto guard = g_reentrancyGuard.enter();

        //if (guard)
        {
            dllInstance = ++g_InterceptInstance;

            // Release level logging for detection
            Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW [Informational]", g_MfrModuleName, dllInstance);
            if (lpOperation != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW() operation=%s", g_MfrModuleName, dllInstance, lpOperation);
                std::wstring pwszOperation = widen(lpOperation);
                if (pwszOperation._Equal(L"find"))
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] may need to debug find operation further...", g_MfrModuleName, dllInstance);
                }
            }
            if (lpFile != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW() file=%s", g_MfrModuleName, dllInstance, lpFile);
            }
            if (lpParameters != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW() parameters=%s", g_MfrModuleName, dllInstance, lpParameters);
            }
            if (lpDirectory != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW() directory=%s", g_MfrModuleName, dllInstance, lpDirectory);
            }
            Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW() nShowCmd=%d", g_MfrModuleName, dllInstance, nShowCmd);



            LogCallingModuleInstance(g_MfrModuleName, dllInstance);

            retfinal = ::ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
            if (retfinal)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW() [Informational] returns SUCCESS", g_MfrModuleName, dllInstance);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] (Windows.Storage)ShellExecuteW() [Informational] returns ERROR=0x%x", g_MfrModuleName, dllInstance, GetLastError());
            }
            return retfinal;
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] (Windows.Storage)ShellExecute Exception=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }

    retfinal = ::ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
    return retfinal;
}
DECLARE_FIXUP(windowsstorageimpl::ShellExecuteWImpl, WS_ShellExecuteWFixup);
#endif
#endif