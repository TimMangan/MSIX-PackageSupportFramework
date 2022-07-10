//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan, TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall WritePrivateProfileStructFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const LPVOID structData,
    _In_opt_ const UINT   uSizeStruct,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD WritePrivateProfileStructInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {

            if (fileName != NULL)
            {
                LogString(WritePrivateProfileStructInstance,L"WritePrivateProfileStructFixup for fileName", fileName);
                if (!IsUnderUserAppDataLocalPackages(fileName))
                {
                    path_redirect_info  pri = ShouldRedirectV2(fileName, redirect_flags::copy_on_read, WritePrivateProfileStructInstance);
                    if (pri.should_redirect)
                    {
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            return impl::WritePrivateProfileStructW(widen_argument(appName).c_str(), widen_argument(keyName).c_str(),
                                structData, uSizeStruct, pri.redirect_path.c_str());
                        }
                        else
                        {
                            return impl::WritePrivateProfileStructW(appName, keyName, structData, uSizeStruct, pri.redirect_path.c_str());
                        }
                    }
                }
                else
                {
                    Log(L"[%d]Under LocalAppData\\Packages, don't redirect", WritePrivateProfileStructInstance);
                }
            }
            else
            {
                Log(L"[%d]null fileName, don't redirect", WritePrivateProfileStructInstance);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(WritePrivateProfileStructInstance, L"WritePrivateProfileStruct")
#else
    catch (...)
    {
        Log(L"[%d] WritePrivateProfileStruct Exception=0x%x", WritePrivateProfileStructInstance, GetLastError());
    }
#endif 


    return impl::WritePrivateProfileStruct(appName, keyName, structData, uSizeStruct, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileStruct, WritePrivateProfileStructFixup);
