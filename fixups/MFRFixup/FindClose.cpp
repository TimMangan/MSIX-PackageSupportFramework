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

//Names on non-pairs not working?
#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:FindCloseFixup_Fixup=_FindCloseFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:FindCloseFixup_Fixup=FindCloseFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif

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

    auto data3 = reinterpret_cast<FindData3*>(findHandle);
#if _DEBUG
    Log(L"[%d][%d] FindCloseFixup handle=0x%x.", data3->RememberedInstance, dllInstance, findHandle);
#endif
    if ((int)data3->RememberedInstance > 60000)
    {
        if (data3->find_handles[Result_Redirected])
        {
            data3->find_handles[Result_Redirected].release();
        }
        if (data3->find_handles[Result_Package])
        {
            data3->find_handles[Result_Package].release();
        }
        if (data3->find_handles[Result_Native])
        {
            data3->find_handles[Result_Native].release();
        }
    }
    else
    {
        // This is a case where we got the guard, but it doesn't look like our FindFirst structure, so maybe from an unhandled NtQueryDirectoryFile or other source, so  impl:FindClose without interpretation?
        impl::FindClose(findHandle);
    }
    ::SetLastError(ERROR_SUCCESS);
    return TRUE;
}
DECLARE_FIXUP(impl::FindClose, FindCloseFixup);
