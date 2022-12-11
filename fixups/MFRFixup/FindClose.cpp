//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
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
#include <memory>
#include "FindData3.h"


BOOL __stdcall FindCloseFixup(_Inout_ HANDLE findHandle) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    if (!guard)
    {
#if _DEBUG
        Log(L"FindCloseFixup");
#endif
        return impl::FindClose(findHandle);
    }

#if _DEBUG
    DWORD dllInstance = ++g_InterceptInstance;
#endif
    if (findHandle == INVALID_HANDLE_VALUE)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

#if _DEBUG
    auto data = reinterpret_cast<FindData3*>(findHandle);
    Log(L"[%d][%d] FindCloseFixup.", data->RememberedInstance, dllInstance);
#endif

    delete reinterpret_cast<FindData3*>(findHandle);
    ::SetLastError(ERROR_SUCCESS);
    return TRUE;
}
DECLARE_FIXUP(impl::FindClose, FindCloseFixup);
