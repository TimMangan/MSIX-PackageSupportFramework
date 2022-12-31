//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexa


#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

#if FIXUP_UCRTMOVE

template <typename CharT>
int  __cdecl wrenameFixup(
    _In_ const CharT* oldName,
    _In_ const CharT* newName)
{
    [[maybe_unused]] DWORD dllInstance = ++g_InterceptInstance;
    int ret;

    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        LogString(dllInstance, L"wrename Fixup oldName", oldName);
        LogString(dllInstance, L"wrename Fixup newName", newName);
#endif
        if (guard)
        {
            ; // if needed
        }

        if constexpr (psf::is_ansi<CharT>)
        {
            ret = impl::Rename(oldName, newName);
        }
        else
        {
            ret = impl::Rename(oldName, newName);
        }
#if _DEBUG
        if (ret == 0)
        {
            Log(L"[%d]\twrename returns SUCCESS", dllInstance);
        }
        else
        {
            Log(L"[%d]\twrename returns 0x", dllInstance,GetLastError());
        }
#endif
        return ret;
    }
    catch (...)
    {
        return -1;
    }
}
DECLARE_STRING_FIXUP(impl::Rename, wrenameFixup);







#endif