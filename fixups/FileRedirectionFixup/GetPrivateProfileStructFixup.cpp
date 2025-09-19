//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall GetPrivateProfileStructFixup(
    _In_opt_ const CharT* sectionName,
    _In_opt_ const CharT* key,
    _Out_writes_to_opt_(uSizeStruct, return) LPVOID structArea,
    _In_ UINT uSizeStruct,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD GetPrivateProfileStructInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            if (fileName != NULL)
            {
                LogString(LogLevel_DebugBasic, g_FrfModuleName, GetPrivateProfileStructInstance,L"GetPrivateProfileStructFixup for fileName", widen(fileName, CP_ACP).c_str());
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    path_redirect_info  pri = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, GetPrivateProfileStructInstance);
                    if (pri.should_redirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            return impl::GetPrivateProfileStructW(widen_argument(sectionName).c_str(), widen_argument(key).c_str(),
                                structArea, uSizeStruct, pri.redirect_path.c_str());
                        }
                        else
                        {
                            return impl::GetPrivateProfileStructW(sectionName, key, structArea, uSizeStruct, pri.redirect_path.c_str());
                        }
                    }
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, GetPrivateProfileStructInstance);
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d]  null fileName, don't redirect as may be registry based or default.", g_FrfModuleName, GetPrivateProfileStructInstance);

            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, GetPrivateProfileStructInstance, L"GetPrivateProfileStruct")


    return impl::GetPrivateProfileStruct(sectionName, key, structArea, uSizeStruct, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileStruct, GetPrivateProfileStructFixup);