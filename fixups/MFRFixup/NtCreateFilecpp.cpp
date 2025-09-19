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
            Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtCreateFileFixup unguarded and informational", g_MfrModuleName, dllInstance);
            if (ObjectAttributes->ObjectName != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtCreateFileFixup RootDirectory=0x%x ObjectName=%ls", g_MfrModuleName, dllInstance, ObjectAttributes->RootDirectory, ObjectAttributes->ObjectName->Buffer);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtCreateFileFixup RootDirectory=0x%x ObjectName=NULL", g_MfrModuleName, dllInstance, ObjectAttributes->RootDirectory);
            }
            LogCallingModuleInstance(g_MfrModuleName, dllInstance);
            g_psf_NoLogging = temp;
        }
        retfinal = ntdllimpl::NtCreateFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
        Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtCreateFileFixup result=0x%x", g_MfrModuleName, dllInstance, retfinal);
        return retfinal;
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"NtDll_NtCreateFileFixup")

    retfinal = ntdllimpl::NtCreateFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::NtCreateFileImpl, NtDll_NtCreateFileFixup);
#endif

#endif