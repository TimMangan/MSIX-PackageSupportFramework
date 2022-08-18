//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall CopyFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName, _In_ BOOL failIfExists) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(DllInstance, L"CopyFileFixup from", existingFileName);
            LogString(DllInstance, L"CopyFileFixup to", newFileName);
            Log(L"[%d] CopyFileFixup FileIfExists %d", DllInstance, failIfExists);
#endif
            // TODO
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"CopyFile")
#else
    catch (...)
    {
        Log(L"[%d] CopyFile Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    return impl::CopyFile(existingFileName, newFileName, failIfExists);
}
DECLARE_STRING_FIXUP(impl::CopyFile, CopyFileFixup);



template <typename CharT>
BOOL __stdcall CopyFileExFixup(
    _In_ const CharT* existingFileName,
    _In_ const CharT* newFileName,
    _In_opt_ LPPROGRESS_ROUTINE progressRoutine,
    _In_opt_ LPVOID data,
    _When_(cancel != NULL, _Pre_satisfies_(*cancel == FALSE)) _Inout_opt_ LPBOOL cancel,
    _In_ DWORD copyFlags) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(DllInstance, L"CopyFileExFixup from", existingFileName);
            LogString(DllInstance, L"CopyFileExFixup to", newFileName);
#endif
            // TODO
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"CopyFileEx")
#else
    catch (...)
    {
        Log(L"[%d] CopyFileEx Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    return impl::CopyFileEx(existingFileName, newFileName, progressRoutine, data, cancel, copyFlags);
}
DECLARE_STRING_FIXUP(impl::CopyFileEx, CopyFileExFixup);


HRESULT __stdcall CopyFile2Fixup(
    _In_ PCWSTR existingFileName,
    _In_ PCWSTR newFileName,
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(DllInstance, L"CopyFile2Fixup from", existingFileName);
            LogString(DllInstance, L"CopyFile2Fixup to", newFileName);
#endif
            // TODO
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"CopyFileEx")
#else
    catch (...)
    {
        Log(L"[%d] CopyFile2 Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    return impl::CopyFile2(existingFileName, newFileName, extendedParameters);
}
DECLARE_FIXUP(impl::CopyFile2, CopyFile2Fixup);