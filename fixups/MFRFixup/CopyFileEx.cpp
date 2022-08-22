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



template <typename CharT>
BOOL __stdcall CopyFileExFixup(
    _In_ const CharT* existingFileName,
    _In_ const CharT* newFileName,
    _In_opt_ LPPROGRESS_ROUTINE progressRoutine,
    _In_opt_ LPVOID data,
    _When_(cancel != NULL, _Pre_satisfies_(*cancel == FALSE)) _Inout_opt_ LPBOOL cancel,
    _In_ DWORD copyFlags) noexcept
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
            LogString(DllInstance, L"CopyFileExFixup from", existingFileName);
            LogString(DllInstance, L"CopyFileExFixup to", newFileName);
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
        Log(L"[%d] CopyFileEx Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    return impl::CopyFileEx(existingFileName, newFileName, progressRoutine, data, cancel, copyFlags);
}
DECLARE_STRING_FIXUP(impl::CopyFileEx, CopyFileExFixup);
