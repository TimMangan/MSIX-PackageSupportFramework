//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

/// Consider adding AddDllDirectory hooking to also add virtual equivalents
/// There seems to be some dll loading that occurs outside of loadlibrary???
/// It would be done in FRF as only FRF knows the alternate paths.

template <typename CharT>
BOOL __stdcall SetCurrentDirectoryFixup(_In_ const CharT* filePath) noexcept
{
    DWORD SetCurrentDirectoryInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
   
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                LogStringWA(SetCurrentDirectoryInstance,L"SetWorkingDirectoryInstance A input is", (const char *)filePath);
            }
            else
            {
#if _DEBUG
                LogStringWW(SetCurrentDirectoryInstance,L"SetCurrentDirectoryFixup W input is", filePath);
#endif
            }
            std::wstring wFilePath = widen(filePath);
#if _DEBUG
            LogString(SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup ", wFilePath.c_str());
#endif
            if (!path_relative_to(wFilePath.c_str(), psf::current_package_path()))
            {
                normalized_path normalized = NormalizePath(wFilePath.c_str(), SetCurrentDirectoryInstance);
                normalized_path virtualized = VirtualizePath(normalized, SetCurrentDirectoryInstance);
                if (impl::PathExists(virtualized.full_path.c_str()))
                {
#if _DEBUG
                    LogString(SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup Use Folder", virtualized.full_path.c_str());
#endif
                    return impl::SetCurrentDirectoryW(virtualized.full_path.c_str());
                }
                else
                {
                    // Fall through to original call
#if _DEBUG
                    LogString(SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup ", L"Virtualized folder not in package, use requested folder.");
#endif
                }
            }
            else
            {
                // Fall through to original call
#if _DEBUG
                LogString(SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup ", L"Requested folder is part of package, use requested folder.");
#endif
            }
            return ::SetCurrentDirectoryW(wFilePath.c_str());
        }

    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(SetCurrentDirectoryInstance, L"SetCurrentDirectory")
#else
    catch (...)
    {
        Log(L"[%d] SetCurrentDirectory Exception=0x%x", SetCurrentDirectoryInstance, GetLastError());
    }
#endif 


    return impl::SetCurrentDirectory(filePath);
}
DECLARE_STRING_FIXUP(impl::SetCurrentDirectory, SetCurrentDirectoryFixup);


template <typename CharT>
DWORD __stdcall GetCurrentDirectoryFixup(_In_ DWORD nBufferLength, _Out_ CharT* filePath) noexcept 
{
    DWORD GetCurrentDirectoryInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (!guard)
        {
            return impl::GetCurrentDirectory(nBufferLength, filePath);
        }
        else
        {

            // This exists for debugging only.
            DWORD dRet = impl::GetCurrentDirectory(nBufferLength, filePath);

#if _DEBUG
            Log(L"[%d]GetCurrentDirectory: returns 0x%x", GetCurrentDirectoryInstance, dRet);
#endif
            if (dRet != 0)
            {
                if (nBufferLength >= dRet)
                {
                    try
                    {
#if _DEBUG
                        LogString(GetCurrentDirectoryInstance, L"GetCurrentDirectory path", filePath);
#endif
                    }
                    catch (...)
                    {
                        Log(L"[%d] Exception printing GetCurrentDirectory.");
                    }
                }
                else
                {
#if _DEBUG
                    Log(L"[%d]GetCurrentDirectory but buffer was only 0x%x", GetCurrentDirectoryInstance, nBufferLength);
#endif
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]GetCurrentDirectory Error = 0x%x", GetCurrentDirectoryInstance, GetLastError());
#endif
            }

            return dRet;
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_ReturnError(GetCurrentDirectoryInstance, L"GetCurrentDirectory")
#else
    catch (...)
    {
        int err = win32_from_caught_exception();
        Log(L"[%d] GetCurrentDirectory Exception=0x%x", GetCurrentDirectoryInstance, err);
        return err;
    }
#endif 
}
DECLARE_STRING_FIXUP(impl::GetCurrentDirectory, GetCurrentDirectoryFixup);

