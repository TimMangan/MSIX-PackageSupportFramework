//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOLEAN __stdcall CreateSymbolicLinkFixup(
    _In_ const CharT* symlinkFileName,
    _In_ const CharT* targetFileName,
    _In_ DWORD flags) noexcept
{
    DWORD CreateSymbolicLinkInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(CreateSymbolicLinkInstance,L"CreateSymbolicLinkFixup for", symlinkFileName);
            LogString(CreateSymbolicLinkInstance,L"CreateSymbolicLinkFixup target",  targetFileName);
#endif
            path_redirect_info  priSource = ShouldRedirectV2(symlinkFileName, redirect_flags::ensure_directory_structure, CreateSymbolicLinkInstance);
            path_redirect_info  priTarget = ShouldRedirectV2(targetFileName, redirect_flags::copy_on_read, CreateSymbolicLinkInstance);
            if (priSource.should_redirect || priTarget.should_redirect)
            {
                // NOTE: If the target is a directory, then ideally we would recursively copy its contents to its
                //       redirected location (since future accesses may want to read/write to files that originated from
                //       the package). However, doing so would be quite a bit of work, so we'll defer doing so until
                //       later when we have evidence that this could be an issue.
                std::wstring rldFileName = TurnPathIntoRootLocalDevice(priSource.should_redirect ? priSource.redirect_path.c_str() : widen_argument(symlinkFileName).c_str());
                std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(priTarget.should_redirect ? priTarget.redirect_path.c_str() : widen_argument(targetFileName).c_str());
                return impl::CreateSymbolicLink(rldFileName.c_str(), rldExistingFileName.c_str(), flags);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(CreateSymbolicLinkInstance, L"CreateSymbolicLink")
#else
    catch (...)
    {
        Log(L"[%d] CreateSymbolicLink Exception=0x%x", CreateSymbolicLinkInstance, GetLastError());
    }
#endif


    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(symlinkFileName).c_str());
    std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(widen_argument(targetFileName).c_str());
    return impl::CreateSymbolicLink(rldFileName.c_str(), rldExistingFileName.c_str(), flags);

}
DECLARE_STRING_FIXUP(impl::CreateSymbolicLink, CreateSymbolicLinkFixup);
