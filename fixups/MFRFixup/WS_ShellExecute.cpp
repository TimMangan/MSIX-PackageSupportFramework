// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Microsoft documentation of this API: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew


#if _DEBUG
#define MOREDEBUG 1 
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
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] HINSTANCE retfinal;

    wchar_t* wcOperation = NULL;
    wchar_t* wcFile = NULL;
    wchar_t* wcParameters = NULL;
    wchar_t* wcDirectory = NULL;
    std::wstring wsOperation = L"";
    std::wstring wsFile = L"";
    std::wstring wsParameters = L"";
    std::wstring wsDirectory = L"";
    if (lpOperation != NULL)
    {
        wsOperation = widen(lpOperation);
        wcOperation = wsOperation.data();
    }
    if (lpFile != NULL)
    {
        wsFile = widen(lpFile);
        wcFile = const_cast<wchar_t*>(widen(lpFile).c_str());
    }
    if (lpParameters != NULL)
    {
        wsParameters = widen(lpParameters);
        wcParameters = const_cast<wchar_t*>(widen(lpParameters).c_str());
    }
    if (lpDirectory != NULL)
    {
        wsDirectory = widen(lpDirectory);
        wcDirectory = const_cast<wchar_t*>(widen(lpDirectory).c_str());
    }

    try
    {
        auto guard = g_reentrancyGuard.enter();
        

        if (guard)
        {

            if (moredebug)
            {
                // Release level logging for detection
                bool temp = g_psf_NoLogging;
                g_psf_NoLogging = false; Log(L"[%d] (Windows.Storage)ShellExecute(%ls, %ls, %ls, %ls, %d)", dllInstance, wcOperation, wcFile, wcParameters, wcDirectory, nShowCmd);
                LogCallingModule();
                g_psf_NoLogging = temp;
            }
            
            if (lpOperation != nullptr)
            {
                std::wstring pwszOperation = widen(lpOperation);
                if (pwszOperation._Equal(L"find"))
                {
                    Log(L"[%d] Debug here", dllInstance);
                }
            }
            retfinal = ::ShellExecuteW(hwnd, wcOperation, wcFile, wcParameters, wcDirectory, nShowCmd);
            return retfinal;
        }
    }
    catch (...)
    {
        Log(L"[%d] (Windows.Storage)ShellExecute Exception=0x%x", dllInstance, GetLastError());
    }
   
    retfinal = ::ShellExecute(hwnd, wcOperation, wcFile, wcParameters, wcDirectory, nShowCmd);
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
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] HINSTANCE retfinal;

    wchar_t* wcOperation = NULL;
    wchar_t* wcFile = NULL;
    wchar_t* wcParameters = NULL;
    wchar_t* wcDirectory = NULL;
    std::wstring wsOperation = L"";
    std::wstring wsFile = L"";
    std::wstring wsParameters = L"";
    std::wstring wsDirectory = L"";
    if (lpOperation != NULL)
    {
        wsOperation = widen(lpOperation);
        wcOperation = wsOperation.data();
    }
    if (lpFile != NULL)
    {
        wsFile = widen(lpFile);
        wcFile = const_cast<wchar_t*>(widen(lpFile).c_str());
    }
    if (lpParameters != NULL)
    {
        wsParameters = widen(lpParameters);
        wcParameters = const_cast<wchar_t*>(widen(lpParameters).c_str());
    }
    if (lpDirectory != NULL)
    {
        wsDirectory = widen(lpDirectory);
        wcDirectory = const_cast<wchar_t*>(widen(lpDirectory).c_str());
    }

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
                Log(L"[%d] (Windows.Storage)ShellExecute(%ls, %ls, %ls, %ls, %d) unfixed", dllInstance, wcOperation, wcFile, wcParameters, wcDirectory, nShowCmd);
                LogCallingModule();
                g_psf_NoLogging = temp;
            }

            if (lpOperation != nullptr)
            {
                std::wstring pwszOperation = widen(lpOperation);
                if (pwszOperation._Equal(L"find"))
                {
                    Log(L"[%d] Debug here", dllInstance);
                }
            }
            retfinal = ::ShellExecuteW(hwnd, wcOperation, wcFile, wcParameters, wcDirectory, nShowCmd);
            return retfinal;
        }
    }
    catch (...)
    {
        Log(L"[%d] (Windows.Storage)ShellExecute Exception=0x%x", dllInstance, GetLastError());
    }

    retfinal = ::ShellExecute(hwnd, wcOperation, wcFile, wcParameters, wcDirectory, nShowCmd);
    return retfinal;
}
DECLARE_FIXUP(windowsstorageimpl::ShellExecuteWImpl, WS_ShellExecuteWFixup);
#endif
#endif