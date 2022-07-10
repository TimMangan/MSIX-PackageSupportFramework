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
#if _DEBUG
                LogString(GetPrivateProfileStructInstance,L"GetPrivateProfileStructFixup for fileName", widen(fileName, CP_ACP).c_str());
#endif
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
#if _DEBUG
                    Log(L"[%d]  Under LocalAppData\\Packages, don't redirect", GetPrivateProfileStructInstance);
#endif
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]  null fileName, don't redirect as may be registry based or default.", GetPrivateProfileStructInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(GetPrivateProfileStructInstance, L"GetPrivateProfileStruct")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileStruct Exception=0x%x", GetPrivateProfileStructInstance, GetLastError());
    }
#endif


    return impl::GetPrivateProfileStruct(sectionName, key, structArea, uSizeStruct, fileName);
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileStruct, GetPrivateProfileStructFixup);