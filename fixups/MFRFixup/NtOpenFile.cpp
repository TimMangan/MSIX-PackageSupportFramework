//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api:https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntopenfile


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

#ifdef DO_Intercept_NtOpenFile


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:Ntdll_NtOpenFileFixup_Fixup=_NtDll_NtOpenFileFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#else
#pragma comment(linker, "/EXPORT:Ntdll_NtOpenFileFixup_Fixup=NtDll_NtOpenFileFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#endif


NTSTATUS __stdcall NtDll_NtOpenFileFixup(
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

    //auto guard = g_reentrancyGuard.enter();
    try
    {
        //if (guard)
        {
            // Release level logging for detection
            bool temp = g_psf_NoLogging;
            g_psf_NoLogging = false;
            Log(L"[%s%d] NtDll_NtOpenFileFixup unguarded and informational", g_MfrModuleName, dllInstance);
            if (ObjectAttributes->ObjectName != NULL)
            {
                Log(L"[%s%d] NtDll_NtOpenFileFixup RootDirectory=0x%x ObjectName=%ls", g_MfrModuleName, dllInstance, ObjectAttributes->RootDirectory, ObjectAttributes->ObjectName->Buffer);
            }
            else
            {
                Log(L"[%s%d] NtDll_NtOpenFileFixup RootDirectory=0x%x ObjectName=NULL", g_MfrModuleName, dllInstance, ObjectAttributes->RootDirectory);
            }
            LogCallingModuleInstance(g_MfrModuleName, dllInstance);
            g_psf_NoLogging = temp;
        }
        retfinal = ntdllimpl::NtOpenFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,ShareAccess, OpenOptions);
        Log(L"[%s%d] NtDll_NtOpenFileFixup result=0x%x", g_MfrModuleName, dllInstance, retfinal);
        return retfinal;
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(g_MfrModuleName, dllInstance, L"NtDll_NtOpenFileFixup")
#else
    catch (...)
    {
        Log(L"[%s%d] NtDll_NtOpenFileFixup Exception=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }
#endif
    retfinal = ntdllimpl::NtOpenFileImpl(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::NtOpenFileImpl, NtDll_NtOpenFileFixup);
#endif

#endif