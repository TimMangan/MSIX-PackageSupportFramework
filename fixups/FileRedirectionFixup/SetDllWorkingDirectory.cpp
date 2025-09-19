//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>


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
                LogStringWA(LogLevel_DebugBasic, g_FrfModuleName, SetCurrentDirectoryInstance,L"SetWorkingDirectoryInstance A input is", (const char *)filePath);
            }
            else
            {
                LogStringWW(LogLevel_DebugBasic, g_FrfModuleName, SetCurrentDirectoryInstance,L"SetCurrentDirectoryFixup W input is", filePath);
            }
            std::wstring wFilePath = widen(filePath);
            LogString(LogLevel_DebugBasic, g_FrfModuleName, SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup ", wFilePath.c_str());
            if (!path_relative_to(wFilePath.c_str(), psf::current_package_path()))
            {
                normalized_path normalized = NormalizePath(wFilePath.c_str(), SetCurrentDirectoryInstance);
                normalized_path virtualized = VirtualizePath(normalized, SetCurrentDirectoryInstance);
                if (impl::PathExists(virtualized.full_path.c_str()))
                {
                    LogString(LogLevel_DebugBasic, g_FrfModuleName, SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup Use Folder", virtualized.full_path.c_str());

                    return impl::SetCurrentDirectoryW(virtualized.full_path.c_str());
                }
                else
                {
                    // Fall through to original call
                    LogString(LogLevel_DebugBasic, g_FrfModuleName, SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup ", L"Virtualized folder not in package, use requested folder.");
                }
            }
            else
            {
                // Fall through to original call
                LogString(LogLevel_DebugBasic, g_FrfModuleName, SetCurrentDirectoryInstance, L"SetCurrentDirectoryFixup ", L"Requested folder is part of package, use requested folder.");
            }
            return ::SetCurrentDirectoryW(wFilePath.c_str());
        }

    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, SetCurrentDirectoryInstance, L"SetCurrentDirectory")



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

            Log(LogLevel_DebugBasic, L"[%s%d]GetCurrentDirectory: returns 0x%x", g_FrfModuleName, GetCurrentDirectoryInstance, dRet);
            if (dRet != 0)
            {
                if (nBufferLength >= dRet)
                {
                    try
                    {
                        LogString(LogLevel_DebugBasic, g_FrfModuleName, GetCurrentDirectoryInstance, L"GetCurrentDirectory path", filePath);
                    }
                    catch (...)
                    {
                        Log(LogLevel_Exception, L"[%s%d] Exception printing", g_FrfModuleName, "GetCurrentDirectory");
                    }
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]GetCurrentDirectory but buffer was only 0x%x", g_FrfModuleName, GetCurrentDirectoryInstance, nBufferLength);
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d]GetCurrentDirectory Error = 0x%x", g_FrfModuleName, GetCurrentDirectoryInstance, GetLastError());
            }

            return dRet;
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN_ReturnError(LogLevel_Exception, g_FrfModuleName, GetCurrentDirectoryInstance, L"GetCurrentDirectory")

}
DECLARE_STRING_FIXUP(impl::GetCurrentDirectory, GetCurrentDirectoryFixup);

