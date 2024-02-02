//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwquerydirectoryfileex


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

#ifdef DO_Intercept_ZwQueryDirectoryFileEx

NTSTATUS __stdcall NtDll_ZwQueryDirectoryFileExFixup(
    IN             HANDLE                 FileHandle,
    IN OPTIONAL    HANDLE                 Event,
    IN OPTIONAL    PIO_APC_ROUTINE        ApcRoutine,
    IN OPTIONAL    PVOID                  ApcContext,
    OUT            PIO_STATUS_BLOCK       IoStatusBlock,
    OUT            PVOID                  FileInformation,
    IN             ULONG                  Length,
    IN             FILE_INFORMATION_CLASS FileInformationClass,
    _In_           ULONG                  QueryFlags,
    _In_opt_       PUNICODE_STRING        FileName
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
            // Release level logging for detection.
            bool temp = g_psf_NoLogging;
            g_psf_NoLogging = false;
            Log(L"[%d] NtDll_ZwQueryDirectoryFileExFixup unguarded", dllInstance);
            if (Event != NULL || ApcRoutine != NULL)
            {
                Log(L"[%d] NtDll_ZwQueryDirectoryFileExFixup isAsync", dllInstance);
            }
            if (FileName != NULL)
            {
                Log(L"[%d] NtDll_ZwQueryDirectoryFileFixup RootDirectory=0x%x FileName=%ls", dllInstance, FileHandle, FileName->Buffer);
            }
            else
            {
                Log(L"[%d] NtDll_ZwQueryDirectoryFileFixup RootDirectory=0x%x FileName=null", dllInstance, FileHandle);
            }
            LogCallingModule();
            g_psf_NoLogging = temp;
        }
        retfinal = ntdllimpl::ZwQueryDirectoryFileExImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
        return retfinal;
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"NtDll_ZwQueryDirectoryFileExFixup")
#else
    catch (...)
    {
        Log(L"[%d] NtDll_ZwQueryDirectoryFileExFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    retfinal = ntdllimpl::ZwQueryDirectoryFileExImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::ZwQueryDirectoryFileExImpl, NtDll_ZwQueryDirectoryFileExFixup);
#endif

#endif