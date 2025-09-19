//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionNamesFixup(
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD GetPrivateProfileSectionNamesInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            if (fileName != NULL)
            {
                LogString(LogLevel_DebugBasic, g_FrfModuleName, GetPrivateProfileSectionNamesInstance,L"GetPrivateProfileSectionNamesFixup for fileName", widen(fileName, CP_ACP).c_str());
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    path_redirect_info  pri = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, GetPrivateProfileSectionNamesInstance);
                    if (pri.should_redirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            auto wideString = std::make_unique<wchar_t[]>(stringLength);
                            auto realRetValue = impl::GetPrivateProfileSectionNamesW(wideString.get(), stringLength, pri.redirect_path.c_str());

                            if (_doserrno != ENOENT)
                            {
                                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr);
                                return realRetValue;
                            }
                        }
                        else
                        {
                            return impl::GetPrivateProfileSectionNamesW(string, stringLength, pri.redirect_path.c_str());
                        }
                    }
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, GetPrivateProfileSectionNamesInstance);
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d]  null fileName, don't redirect as may be registry based or default.", g_FrfModuleName, GetPrivateProfileSectionNamesInstance);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, GetPrivateProfileSectionNamesInstance, L"GetPrivateProfileSectionNames")



    return impl::GetPrivateProfileSectionNames(string, stringLength, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSectionNames, GetPrivateProfileSectionNamesFixup);
