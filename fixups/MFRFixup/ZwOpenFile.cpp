//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwopenfile


#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include <psf_logging.h>
#include "FunctionImplementations.h"
#include "FunctionImplementations_ntdll.h"


#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

#if Intercept_NTDLL

#ifdef DO_Intercept_ZwOpenFile

NTSTATUS __stdcall NtDll_ZwOpenFileFixup(
    _Out_          PHANDLE            FileHandle,
    _In_           ACCESS_MASK        DesiredAccess,
    _In_           POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_          PIO_STATUS_BLOCK   IoStatusBlock,
    _In_           ULONG              ShareAccess,
    _In_           ULONG              OpenOptions
)
{
    NTSTATUS retfinal;
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // Release level logging for detection
            bool temp = g_psf_NoLogging;
            g_psf_NoLogging = false;
            Log(L"[%d] NtDll_ZwOpenFileFixup unguarded", dllInstance);
            if (ObjectAttributes->ObjectName != NULL)
            {
                Log(L"[%d] NtDll_ZwOpenFileFixup RootDirectory=0x%x ObjectName=%ls", dllInstance, ObjectAttributes->RootDirectory, ObjectAttributes->ObjectName->Buffer);
            }
            else
            {
                Log(L"[%d] NtDll_ZwOpenFileFixup RootDirectory=0x%x ObjectName=NULL", dllInstance, ObjectAttributes->RootDirectory);
            }
            LogCallingModule();
            g_psf_NoLogging = temp;
        }
        retfinal = ntdllimpl::ZwOpenFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
        return retfinal;
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"NtDll_ZwOpenFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] NtDll_ZwOpenFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    retfinal = ntdllimpl::ZwOpenFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,ShareAccess, OpenOptions);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::ZwOpenFileImpl, NtDll_ZwOpenFileFixup);
#endif

#endif