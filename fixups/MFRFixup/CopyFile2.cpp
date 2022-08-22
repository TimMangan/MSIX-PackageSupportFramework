// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"

#include "FunctionImplementations.h"
#include <psf_logging.h>


HRESULT __stdcall CopyFile2Fixup(
    _In_ PCWSTR existingFileName,
    _In_ PCWSTR newFileName,
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] BOOL retfinal;

    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(DllInstance, L"CopyFile2Fixup from", existingFileName);
            LogString(DllInstance, L"CopyFile2Fixup to", newFileName);
#endif
            // TODO
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"CopyFileEx")
#else
    catch (...)
    {
        Log(L"[%d] CopyFile2 Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    return impl::CopyFile2(existingFileName, newFileName, extendedParameters);
}
DECLARE_FIXUP(impl::CopyFile2, CopyFile2Fixup);
