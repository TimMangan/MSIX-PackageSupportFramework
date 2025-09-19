//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall DeleteFileFixup(_In_ const CharT* fileName) noexcept
{
    DWORD DeleteFileInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            LogString(LogLevel_DebugBasic, g_FrfModuleName, DeleteFileInstance,L"DeleteFileFixup for fileName", fileName);
            
            if (!IsUnderUserAppDataLocalPackages(fileName))
            {

                // NOTE: This will only delete the redirected file. If the file previously existed in the package path, then
                //       it will remain there and a later attempt to open, etc. the file will succeed. In the future, if
                //       this proves to be an issue, we could maintain a collection of package files that have been
                //       "deleted" and then just pretend like they've been deleted. Such a change would be rather large and
                //       disruptful and probably fairly inefficient as it would impact virtually every code path, so we'll
                //       put it off for now.
                path_redirect_info  pri = ShouldRedirectV2(fileName, redirect_flags::none, DeleteFileInstance);
                if (pri.should_redirect)
                {
                    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
                    std::wstring rldRedirPath = TurnPathIntoRootLocalDevice(widen_argument(pri.redirect_path.c_str()).c_str());
                    if (!impl::PathExists(rldRedirPath.c_str()) && impl::PathExists(rldFileName.c_str()))
                    {
                        // If the file does not exist in the redirected location, but does in the non-redirected location,
                        // then we want to give the "illusion" that the delete succeeded
                        Log(LogLevel_DebugBasic, L"[%s%d]DeleteFileFixup Exists in package but not redir, so fake success.", g_FrfModuleName, DeleteFileInstance);
                        SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                    else
                    {
                        BOOL bRet = impl::DeleteFile(rldRedirPath.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]DeleteFileFixup deletes from redir with result: %d %ls", g_FrfModuleName, DeleteFileInstance,bRet, rldRedirPath.c_str());
                        return bRet;
                    }
                }
            }
            else
            {
                std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
                BOOL bRet = impl::DeleteFile(rldFileName.c_str());
                Log(LogLevel_DebugBasic, L"[%s%d]DeleteFileFixup Under LocalAppData\\Packages, don't redirect. deletes with result: %d", g_FrfModuleName, DeleteFileInstance, bRet);
                return bRet;
            }
        }
        else
        {
            LogString(LogLevel_DebugBasic, g_FrfModuleName, 0, L"DeleteFileFixup Unguarded for fileName", fileName);
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, DeleteFileInstance, L"DeleteFile")


    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
    return impl::DeleteFile(rldFileName.c_str());
}
DECLARE_STRING_FIXUP(impl::DeleteFile, DeleteFileFixup);