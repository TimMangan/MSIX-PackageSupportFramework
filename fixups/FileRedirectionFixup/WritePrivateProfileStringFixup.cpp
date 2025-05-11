//-------------------------------------------------------------------------------------------------------
// Copyright (C) Rafael Rivera, Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall WritePrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD WritePrivateProfileStringInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            
            if (fileName != NULL)
            {
                LogString(g_FrfModuleName, WritePrivateProfileStringInstance,L"WritePrivateProfileStringFixup for fileName", fileName);
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    path_redirect_info  pri = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, WritePrivateProfileStringInstance);
                    if (pri.should_redirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            BOOL bRet = impl::WritePrivateProfileString(appName, keyName, string, ((std::filesystem::path)pri.redirect_path).string().c_str());
#if _DEBUG
                            Log(L"[%s%d] WritePrivateProfileString(A) returns %d", g_FrfModuleName, WritePrivateProfileStringInstance, bRet);
#endif
                            return bRet;
                        }
                        else
                        {
                            BOOL bRet = impl::WritePrivateProfileString(appName, keyName, string, pri.redirect_path.c_str());
#if _DEBUG
                            Log(L"[%s%d] WritePrivateProfileString(W) returns %d", g_FrfModuleName, WritePrivateProfileStringInstance, bRet);
#endif                            
                            return bRet;
                        }
                    }
                }
                else
                {
                    Log(L"[%s%d]Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, WritePrivateProfileStringInstance);
                }
            }
            else
            {
                Log(L"[%s%d]null fileName, don't redirect", g_FrfModuleName, WritePrivateProfileStringInstance);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(g_FrfModuleName, WritePrivateProfileStringInstance, L"WritePrivateProfileString")
#else
    catch (...)
    {
        Log(L"[%s%d] WritePrivateProfileString Exception=0x%x", g_FrfModuleName, WritePrivateProfileStringInstance, GetLastError());
    }
#endif 


    return impl::WritePrivateProfileString(appName, keyName, string, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileString, WritePrivateProfileStringFixup);
