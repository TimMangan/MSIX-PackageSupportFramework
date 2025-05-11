//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall WritePrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD WritePrivateProfileSectionInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {

            if (fileName != NULL)
            {
                LogString(g_FrfModuleName, WritePrivateProfileSectionInstance,L"WritePrivateProfileSectionFixup for fileName", fileName);
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    path_redirect_info  pri = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, WritePrivateProfileSectionInstance);
                    if (pri.should_redirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            return impl::WritePrivateProfileSectionW(widen_argument(appName).c_str(),
                                widen_argument(string).c_str(), pri.redirect_path.c_str());
                        }
                        else
                        {
                            return impl::WritePrivateProfileSection(appName, string, pri.redirect_path.c_str());
                        }
                    }
                }
                else
                {
                    Log(L"[%s%d]Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, WritePrivateProfileSectionInstance);
                }
            }
            else
            {
                Log(L"[%s%d]null fileName, don't redirect", g_FrfModuleName, WritePrivateProfileSectionInstance);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(g_FrfModuleName, WritePrivateProfileSectionInstance, L"WritePrivateProfileSection")
#else
    catch (...)
    {
        Log(L"[%s%d] WritePrivateProfileSection Exception=0x%x", g_FrfModuleName, WritePrivateProfileSectionInstance, GetLastError());
    }
#endif 


    return impl::WritePrivateProfileSection(appName, string, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileSection, WritePrivateProfileSectionFixup);
