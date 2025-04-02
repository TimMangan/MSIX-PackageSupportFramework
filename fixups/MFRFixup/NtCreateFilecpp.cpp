//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api:https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ntcreatefile


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

#ifdef DO_Intercept_NtCreateFile


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:NtDll_NtCreateFileFixup_Fixup=_NtDll_NtCreateFileFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#else
#pragma comment(linker, "/EXPORT:Ntdll_NtDll_NtCreateFileFixup_Fixup=NtDll_NtCreateFileFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#endif


NTSTATUS __stdcall NtDll_NtCreateFileFixup(
    _Out_          PHANDLE            FileHandle,
    _In_           ACCESS_MASK        DesiredAccess,
    _In_           POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_          PIO_STATUS_BLOCK   IoStatusBlock,
    _In_opt_       PLARGE_INTEGER     AllocationSize,
    _In_           ULONG              FileAttributes,
    _In_           ULONG              ShareAccess,
    _In_           ULONG              CreateDisposition,
    _In_           ULONG              CreateOptions,
    _In_opt_       PVOID              EaBuffer,
    _In_           ULONG              EaLength
)
{
    NTSTATUS retfinal;
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    //auto guard = g_reentrancyGuard.enter();
    try
    {
        //if (guard)
        {
            // Release level logging for detection
            bool temp = g_psf_NoLogging;
            g_psf_NoLogging = false;
            Log(L"[%d] NtDll_NtCreateFileFixup unguarded and informational", dllInstance);
            if (ObjectAttributes->ObjectName != NULL)
            {
                Log(L"[%d] NtDll_NtCreateFileFixup RootDirectory=0x%x ObjectName=%ls", dllInstance, ObjectAttributes->RootDirectory, ObjectAttributes->ObjectName->Buffer);
            }
            else
            {
                Log(L"[%d] NtDll_NtCreateFileFixup RootDirectory=0x%x ObjectName=NULL", dllInstance, ObjectAttributes->RootDirectory);
            }
            LogCallingModule();
            g_psf_NoLogging = temp;
        }
        retfinal = ntdllimpl::NtCreateFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
        return retfinal;
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"NtDll_NtCreateFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] NtDll_NtCreateFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    retfinal = ntdllimpl::NtCreateFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::NtCreateFileImpl, NtDll_NtCreateFileFixup);
#endif

#endif