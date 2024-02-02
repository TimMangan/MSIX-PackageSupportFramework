//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwquerydirectoryfile


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

#ifdef DO_Intercept_ZwQueryDirectoryFile

NTSTATUS __stdcall
    NtDll_ZwQueryDirectoryFileFixup(
    IN             HANDLE                 FileHandle,
    IN OPTIONAL    HANDLE                 Event,
    IN OPTIONAL    PIO_APC_ROUTINE        ApcRoutine,
    IN OPTIONAL    PVOID                  ApcContext,
    OUT            PIO_STATUS_BLOCK       IoStatusBlock,
    OUT            PVOID                  FileInformation,
    IN             ULONG                  Length,
    IN             FILE_INFORMATION_CLASS FileInformationClass,
    IN             BOOLEAN                ReturnSingleEntry,
    IN OPTIONAL    PUNICODE_STRING        FileName,
    IN             BOOLEAN                RestartScan
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
            Log(L"[%d] NtDll_ZwQueryDirectoryFileFixup unguarded", dllInstance);
            if (Event != NULL || ApcRoutine != NULL)
            {
                Log(L"[%d] NtDll_ZwQueryDirectoryFileFixup isAsync", dllInstance);
            }
            if (FileName != NULL)
            {
                Log(L"[%d] NtDll_ZwQueryDirectoryFileFixup RootDirectory=0x%x FileName=%ls", dllInstance, FileHandle, FileName->Buffer);
            }
            else
            {
                Log(L"[%d] NtDll_ZwQueryDirectoryFileFixup RootDirectory=0x%x FileName=NULL", dllInstance, FileHandle);
            }
            LogCallingModule();
            g_psf_NoLogging = temp;
        }
        retfinal = ntdllimpl::ZwQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
        return retfinal;
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"NtDll_ZwQueryDirectoryFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] NtDll_ZwQueryDirectoryFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    retfinal = ntdllimpl::ZwQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::ZwQueryDirectoryFileImpl, NtDll_ZwQueryDirectoryFileFixup);
#endif

#endif