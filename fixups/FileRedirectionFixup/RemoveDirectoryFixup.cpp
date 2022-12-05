//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall RemoveDirectoryFixup(_In_ const CharT* pathName) noexcept
{
    DWORD RemoveDirectoryInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;
    try
    {
        if (guard)
        {
            std::wstring wPathName = widen(pathName);
#if _DEBUG
            LogString(RemoveDirectoryInstance,L"RemoveDirectoryFixup for pathName", wPathName.c_str());
#endif
            
            if (!IsUnderUserAppDataLocalPackages(wPathName.c_str()))
            {
                // NOTE: See commentary in DeleteFileFixup for limitations on deleting files/directories
                //auto [shouldRedirect, redirectPath, shouldReadonly, exist1, exist2] 
                path_redirect_info  pri = ShouldRedirectV2(wPathName.c_str(), redirect_flags::check_file_presence | redirect_flags::ok_if_parent_in_pkg, RemoveDirectoryInstance);
                if (pri.should_redirect)
                {
                    std::wstring rldPathName = TurnPathIntoRootLocalDevice(wPathName.c_str());
                    std::wstring rldRedirectPath = TurnPathIntoRootLocalDevice(widen_argument(pri.redirect_path.c_str()).c_str());
                    if (!pri.doesRedirectedExist)
                    {
                        if (pri.doesRequestedExist)
                        {
                            // If the directory does not exist in the redirected location, but does in the non-redirected
                            // location, then we want to give the "illusion" that the delete succeeded
#if _DEBUG
                            LogString(RemoveDirectoryInstance, L"RemoveDirectoryFixup In package but not redirected area.", L"Fake return true.");
#endif
                            return TRUE;
                        }
                        else
                        {
#if _DEBUG
                            LogString(RemoveDirectoryInstance, L"RemoveDirectoryFixup Not present in redirected or requested path.", L"return false.");
#endif
                            SetLastError(ERROR_PATH_NOT_FOUND);
                            return FALSE;
                        }
                    }
                    else
                    {
#if _DEBUG
                        LogString(RemoveDirectoryInstance, L"RemoveDirectoryFixup Use Folder", pri.redirect_path.c_str());
#endif
                        BOOL bRet = impl::RemoveDirectory(rldRedirectPath.c_str());
#if _DEBUG
                        Log(L"[%d]RemoveDirectoryFixup deletes redirected with result: %d", RemoveDirectoryInstance, bRet);
#endif
                        return bRet;
                    }
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]RemoveDirectoryFixup Under LocalAppData\\Packages, don't redirect", RemoveDirectoryInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(RemoveDirectoryInstance, L"RemoveDirectory")
#else
    catch (...)
    {
        Log(L"[%d] RemoveDirectory Exception=0x%x", RemoveDirectoryInstance, GetLastError());
    }
#endif

    if (pathName != nullptr)
    {
        std::wstring rldPathName = TurnPathIntoRootLocalDevice(widen_argument(pathName).c_str());
        retfinal = impl::RemoveDirectory(rldPathName.c_str());
    }
    else
    {
        retfinal = impl::RemoveDirectory(pathName);
    }
#if _DEBUG
    Log(L"[%d] RemoveDirectoryFixup returns 0x%x", RemoveDirectoryInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::RemoveDirectory, RemoveDirectoryFixup);
