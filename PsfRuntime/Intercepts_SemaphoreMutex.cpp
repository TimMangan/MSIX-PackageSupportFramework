//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// // The PsfRuntime intercepts all Mutex and Semaphore calls so that paths may be fixed up. 

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

using namespace std::literals;


#include <reentrancy_guard.h>
#include <psf_framework.h>

extern const wchar_t* g_PsfRunTimeName;
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    auto CreateSemaphoreImpl   = psf::detoured_string_function(&::CreateSemaphoreA, &::CreateSemaphoreW);
    auto CreateSemaphoreExImpl = psf::detoured_string_function(&::CreateSemaphoreExA, &::CreateSemaphoreExW);
    auto OpenSemaphoreWImpl     = &::OpenSemaphoreW;

    auto CreateMutexImpl   = psf::detoured_string_function(&::CreateMutexA, &::CreateMutexW);
    auto CreateMutexExImpl = psf::detoured_string_function(&::CreateMutexExA, &::CreateMutexExW);
    auto OpenMutexWImpl     = &::OpenMutexW;
}

DWORD g_SemaphoreMutexInterceptInstance = 24000;



// Utility function
void LogHandleAndError(Json_Debug_Levels jsonDebugLevel, const wchar_t* moduleMoniker, DWORD instance, const wchar_t* functionName, HANDLE returnedHandle )
{
    if (returnedHandle == INVALID_HANDLE_VALUE || returnedHandle == NULL)
    {
        Log(jsonDebugLevel, L"[%s%d] %s: return Handle=0x%x error=0x%x", moduleMoniker, instance, functionName, returnedHandle, GetLastError());
    }
    else
    {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_ALREADY_EXISTS)
        {
            Log(jsonDebugLevel, L"[%s%d] %s: return Handle=0x%x SUCCESS, but ALREADY_EXISTS", moduleMoniker, instance, functionName, returnedHandle);
        }
        else
        {
            Log(jsonDebugLevel, L"[%s%d] %s: return Handle=0x%x SUCCESS", moduleMoniker, instance, functionName, returnedHandle);
        }
    }
}



template <typename CharT>
HANDLE WINAPI CreateSemaphoreFixup(
    _In_opt_    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_        LONG                  lInitialCount,
    _In_        LONG                  lMaximumCount,
    _In_opt_    const CharT*          lpName)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD SemaphoreMutexInstance = ++g_SemaphoreMutexInterceptInstance;

        Log(LogLevel_DebugBasic, L"[%s%d] CreateSemaphoreFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, SemaphoreMutexInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] CreateSemaphoreFixup:     InitialCount=0x%x MaximumCount=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, lInitialCount, lMaximumCount);
        if (lpSemaphoreAttributes != NULL)
        {
            Log(LogLevel_DebugBasic, L" [%s%d] CreateSemaphoreFixup:     Attributes bInheritHandle=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, lpSemaphoreAttributes->bInheritHandle);
        }
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateSemaphoreFixup:     Name", lpName);
        }

        HANDLE hRet = impl::CreateSemaphoreImpl(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
        LogHandleAndError(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateSemaphoreFixup", hRet);
        return hRet;
    }
    return impl::CreateSemaphoreImpl(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::CreateSemaphoreImpl, CreateSemaphoreFixup);


template <typename CharT>
HANDLE WINAPI CreateSemaphoreExFixup(
    _In_opt_    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_        LONG                  lInitialCount,
    _In_        LONG                  lMaximumCount,
    _In_opt_    const CharT*          lpName,
    _In_        DWORD                 dwFlags,                          // reserved=0
    _In_        DWORD                 dwDesiredAccess)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD SemaphoreMutexInstance = ++g_SemaphoreMutexInterceptInstance;

        Log(LogLevel_DebugBasic, L"[%s%d] CreateSemaphoreExFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, SemaphoreMutexInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] CreateSemaphoreExFixup:     InitialCount=0x%x MaximumCount=0x%x DesiredAccess=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, lInitialCount, lMaximumCount, dwDesiredAccess);
        if (lpSemaphoreAttributes != NULL)
        {
            Log(LogLevel_DebugBasic, L" [%s%d] CreateSemaphoreExFixup:     Attributes bInheritHandle=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, lpSemaphoreAttributes->bInheritHandle);
        }
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateSemaphoreExFixup:     Name", lpName);
        }

        HANDLE hRet = impl::CreateSemaphoreExImpl(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName, dwFlags, dwDesiredAccess);
        LogHandleAndError(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateSemaphoreExFixup", hRet);

        return hRet;
    }
    return impl::CreateSemaphoreExImpl(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName, dwFlags, dwDesiredAccess);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::CreateSemaphoreExImpl, CreateSemaphoreExFixup);


HANDLE WINAPI OpenSemaphoreWFixup(
    _In_    DWORD     dwDesiredAccess,
    _In_    BOOL      bInheritHandle,
    _In_    LPCWSTR   lpName)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD SemaphoreMutexInstance = ++g_SemaphoreMutexInterceptInstance;

        Log(LogLevel_DebugBasic, L"[%s%d] OpenSemaphoreWFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, SemaphoreMutexInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] OpenSemaphoreWFixup:     DesiredAccess=0x%x InheritHandle=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, dwDesiredAccess, bInheritHandle);
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"OpenSemaphoreWFixup:     Name", lpName);
        }

        HANDLE hRet = impl::OpenSemaphoreWImpl(dwDesiredAccess, bInheritHandle, lpName);
        LogHandleAndError(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"OpenSemaphoreWFixup", hRet);

        return hRet;
    }
    return impl::OpenSemaphoreWImpl(dwDesiredAccess, bInheritHandle, lpName);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_FIXUP(impl::OpenSemaphoreWImpl, OpenSemaphoreWFixup);



template <typename CharT>
HANDLE WINAPI CreateMutexFixup(
    _In_opt_    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    _In_        BOOL                  lInitialOwner,
    _In_opt_    const CharT* lpName)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD SemaphoreMutexInstance = ++g_SemaphoreMutexInterceptInstance;

        Log(LogLevel_DebugBasic, L"[%s%d] CreateMutexFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, SemaphoreMutexInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] CreateMutexFixup:     InitialOwner?=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, lInitialOwner);
        if (lpMutexAttributes != NULL)
        {
            Log(LogLevel_DebugBasic, L" [%s%d] CreateMutexFixup:     Attributes bInheritHandle=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, lpMutexAttributes->bInheritHandle);
        }
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateMutexFixup:     Name", lpName);
        }

        HANDLE hRet = impl::CreateMutexImpl(lpMutexAttributes, lInitialOwner, lpName);
        LogHandleAndError(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateMutexFixup", hRet);

        return hRet;
    }
    return impl::CreateMutexImpl(lpMutexAttributes, lInitialOwner, lpName);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::CreateMutexImpl, CreateMutexFixup);


template <typename CharT>
HANDLE WINAPI CreateMutexExFixup(
    _In_opt_    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    _In_opt_    const CharT* lpName,
    _In_        DWORD        dwFlags,
    _In_        DWORD        dwDesiredAccess)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD SemaphoreMutexInstance = ++g_SemaphoreMutexInterceptInstance;

        Log(LogLevel_DebugBasic, L"[%s%d] CreateMutexExFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, SemaphoreMutexInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] CreateMutexExFixup:     dwFlags=0x%x dwDesiredAccess=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, dwFlags, dwDesiredAccess);
        if (lpMutexAttributes != NULL)
        {
            Log(LogLevel_DebugBasic, L" [%s%d] CreateMutexExFixup:     Attributes bInheritHandle=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, lpMutexAttributes->bInheritHandle);
        }
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateMutexExFixup:     Name", lpName);
        }

        HANDLE hRet = impl::CreateMutexExImpl(lpMutexAttributes, lpName, dwFlags, dwDesiredAccess);
        LogHandleAndError(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"CreateMutexExFixup", hRet);

        return hRet;
    }
    return impl::CreateMutexExImpl(lpMutexAttributes, lpName, dwFlags, dwDesiredAccess);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_STRING_FIXUP(impl::CreateMutexExImpl, CreateMutexExFixup);


HANDLE WINAPI OpenMutexWFixup(
    _In_    DWORD   dwDesiredAccess,
    _In_    BOOL    bInheritHandle,
    _In_    LPCWSTR lpName)  noexcept try
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        DWORD SemaphoreMutexInstance = ++g_SemaphoreMutexInterceptInstance;

        Log(LogLevel_DebugBasic, L"[%s%d] OpenMutexWFixup: (Informational) This is an experimental intercept for logging purposes only; to evaluate if there is a need for an intercept of this API.", g_PsfRunTimeName, SemaphoreMutexInstance);
        Log(LogLevel_DebugBasic, L" [%s%d] OpenMutexWFixup:     DesiredAccess=0x%x InheritHandle=0x%x", g_PsfRunTimeName, SemaphoreMutexInstance, dwDesiredAccess, bInheritHandle);
        if (lpName != NULL)
        {
            LogString(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"OpenMutexWFixup:     Name", lpName);
        }

        HANDLE hRet = impl::OpenMutexWImpl(dwDesiredAccess, bInheritHandle, lpName);
        LogHandleAndError(LogLevel_DebugBasic, g_PsfRunTimeName, SemaphoreMutexInstance, L"OpenMutexWFixup", hRet);

        return hRet;
    }
    return impl::OpenMutexWImpl(dwDesiredAccess, bInheritHandle, lpName);
}
catch (...)
{
    ::SetLastError(win32_from_caught_exception());
    return FALSE;
}
DECLARE_FIXUP(impl::OpenMutexWImpl, OpenMutexWFixup);

