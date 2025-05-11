//-------------------------------------------------------------------------------------------------------
// Copyright (C) Rafael Rivera, Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
DWORD __stdcall GetPrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* defaultString,
    _Out_writes_to_opt_(returnStringSizeInChars, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD GetPrivateProfileStringInstance = ++g_FileIntceptInstance;
#if _DEBUG
    Log(L"[%s%d] GetPrivateProfileStringFixup", g_FrfModuleName, GetPrivateProfileStringInstance);
#endif
    try
    {
        if (guard)
        {
#if _DEBUG
            if constexpr (psf::is_ansi<CharT>)
            {
                if (fileName != NULL)
                {
                    LogString(g_FrfModuleName, GetPrivateProfileStringInstance,L"GetPrivateProfileStringFixup (A) for fileName", widen(fileName, CP_ACP).c_str());
                }
                else
                {
                    Log(L"[%s%d] GetPrivateProfileStringFixup for null file.", g_FrfModuleName, GetPrivateProfileStringInstance);
                }
                if (appName != NULL)
                {

                    LogString(g_FrfModuleName, GetPrivateProfileStringInstance,L" Section", widen_argument(appName).c_str());
                }
                if (keyName != NULL)
                {
                        LogString(g_FrfModuleName, GetPrivateProfileStringInstance,L" Key", widen_argument(keyName).c_str());
                }
            }
            else
            {
                if (fileName != NULL)
                {
                    LogString(g_FrfModuleName, GetPrivateProfileStringInstance,L"GetPrivateProfileStringFixup (W) for fileName", widen(fileName, CP_ACP).c_str());
                }
                else
                {
                    Log(L"[%s%d] GetPrivateProfileStringFixup for null file.", g_FrfModuleName, GetPrivateProfileStringInstance);
                }
                if (appName != NULL)
                {

                    LogString(g_FrfModuleName, GetPrivateProfileStringInstance,L" Section", appName);
                }
                if (keyName != NULL)
                {
                        LogString(g_FrfModuleName, GetPrivateProfileStringInstance,L" Key", keyName);
                }
            }
#endif
            if (fileName != NULL)
            {
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    path_redirect_info  pri = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, GetPrivateProfileStringInstance);
                    if (pri.should_redirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            
                            auto realRetValue = impl::GetPrivateProfileString(appName, keyName,
                                                                               defaultString, string, stringLength, 
                                                                               narrow(pri.redirect_path.c_str()).c_str() );
#if _DEBUG
                            Log(L"[%s%d] Ansi Returned length=0x%x", g_FrfModuleName, GetPrivateProfileStringInstance, realRetValue);
                            if (realRetValue > 0)
                                LogString(g_FrfModuleName, GetPrivateProfileStringInstance, L" Ansi Returned string", string);
#endif
                            return realRetValue;
                        }
                        else
                        {
                            auto realRetValue = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, pri.redirect_path.c_str());
#if _DEBUG
                            if (realRetValue > 0)
                                LogString(g_FrfModuleName, GetPrivateProfileStringInstance, L" Returned string", string);
                            else
                                Log(L"[%s%d] Returned string zero length", g_FrfModuleName, GetPrivateProfileStringInstance);
#endif
                            return realRetValue;
                        }
                    }
                }
                else
                {
#if _DEBUG
                    Log(L"[%s%d]  Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, GetPrivateProfileStringInstance);
#endif
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%s%d]  null fileName, don't redirect as may be registry based or default.", g_FrfModuleName, GetPrivateProfileStringInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(g_FrfModuleName, GetPrivateProfileStringInstance, L"GetPrivateProfileString")
#else
    catch (...)
    {
        Log(L"[%s%d] GetPrivateProfileString Exception=0x%x", g_FrfModuleName, GetPrivateProfileStringInstance, GetLastError());
    }
#endif


    DWORD dRet =  impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if _DEBUG
    LogString(g_FrfModuleName, GetPrivateProfileStringInstance, L" Returning ", string);
#endif
    return dRet;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileString, GetPrivateProfileStringFixup);
