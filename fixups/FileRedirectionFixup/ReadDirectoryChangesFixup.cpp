//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>



// TODO: there are two functions  that the app may call to be notified about changes made under a directory.
//       ReadDirectoryChangesW
//       ReadDirectoryChangesExW
// NOTE: There is no corresponding "A" function.
//
// Both of these take in an open handle, so at first glance we might not need to worry,
// If the handle was opened in the package space, no changes will ever be recorded, which might be bad and we might want to be looking at the redirection folder.

// We should first create an intercept that logs any such calls so that we'd know if there is a problem.

// The best solution may be to use the handle to determine where the file is, perform a redirection, and open a handle for that.
// There is a function GetFileInformationByHandle() that might be useful
// This is also fraught with problems due to lingering handles and possibly the inability to cancel that redirection.

/***********************************************
BOOL ReadDirectoryChangesW(
    [in]                HANDLE                          hDirectory,
    [out]               LPVOID                          lpBuffer,
    [in]                DWORD                           nBufferLength,
    [in]                BOOL                            bWatchSubtree,
    [in]                DWORD                           dwNotifyFilter,
    [out, optional]     LPDWORD                         lpBytesReturned,
    [in, out, optional] LPOVERLAPPED                    lpOverlapped,
    [in, optional]      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);
*************************************************************************************/

BOOL _stdcall ReadDirectoryChangesWFixup(
    _In_        HANDLE                          hDirectory,
    _Out_       LPVOID                          lpBuffer,
    _In_        DWORD                           nBufferLength,
    _In_        BOOL                            bWatchSubtree,
    _In_        DWORD                           dwNotifyFilter,
    _Out_opt_   LPDWORD                         lpBytesReturned,
    _Inout_opt_ LPOVERLAPPED                    lpOverlapped,
    _In_opt_    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    DWORD Instance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        Log(LogLevel_DebugBasic, L"[%s%d] ReadDirectoryChangesW Fixup Handle=0x%x ", g_FrfModuleName, Instance, hDirectory);


        if (guard)
        {
          
            FILE_BASIC_INFO FileBasicInfoData;
            BOOL res1 = GetFileInformationByHandleEx(hDirectory, FileBasicInfo, &FileBasicInfoData, sizeof(FileBasicInfoData));
            if (res1 != 0)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RDCW Handle FileBasicInfo OK Attributes=0x%x", g_FrfModuleName, Instance, FileBasicInfoData.FileAttributes);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RDCW Handle FileBasicInfo Fail err=0x%x", g_FrfModuleName, Instance, GetLastError());
            }
            DWORD len = 512;
            PFILE_NAME_INFO pFileInformation = (PFILE_NAME_INFO)malloc(len);
            if (pFileInformation != NULL)
            {
                BOOL res2 = GetFileInformationByHandleEx(hDirectory, FileNameInfo, pFileInformation, len);
                if (res2 != 0)
                {
                    LogCountedStringW(LogLevel_DebugBasic, g_FrfModuleName, Instance, "       RDCW Handle Path is", pFileInformation->FileName, pFileInformation->FileNameLength / 2 );
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  RDCW Handle Path did not get returned err=0x%x.", g_FrfModuleName, Instance, GetLastError());
                }
                free(pFileInformation);
            }
            ; // if needed
        }

        BOOL bRet = impl::ReadDirectoryChangesW(hDirectory, lpBuffer, nBufferLength, bWatchSubtree, dwNotifyFilter, lpBytesReturned, lpOverlapped, lpCompletionRoutine);
        if (bRet == 0)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] ReadDirectoryChangesW returns Failerror=0x%x", g_FrfModuleName, Instance, GetLastError());
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] ReadDirectoryChangesW returns Success", g_FrfModuleName, Instance);
        }
        return bRet;
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] ReadDirectoryChangesW Exception Fail", g_FrfModuleName, Instance);
        return 0;
    }
}
DECLARE_FIXUP(impl::ReadDirectoryChangesW, ReadDirectoryChangesWFixup);


/************************************************************************************
BOOL ReadDirectoryChangesExW(
    [in]                HANDLE                                  hDirectory,
    [out]               LPVOID                                  lpBuffer,
    [in]                DWORD                                   nBufferLength,
    [in]                BOOL                                    bWatchSubtree,
    [in]                DWORD                                   dwNotifyFilter,
    [out, optional]     LPDWORD                                 lpBytesReturned,
    [in, out, optional] LPOVERLAPPED                            lpOverlapped,
    [in, optional]      LPOVERLAPPED_COMPLETION_ROUTINE         lpCompletionRoutine,
    [in]                READ_DIRECTORY_NOTIFY_INFORMATION_CLASS ReadDirectoryNotifyInformationClass
);
****************************************************************************************************/

BOOL _stdcall ReadDirectoryChangesExWFixup(
    _In_        HANDLE                                  hDirectory,
    _Out_       LPVOID                                  lpBuffer,
    _In_        DWORD                                   nBufferLength,
    _In_        BOOL                                    bWatchSubtree,
    _In_        DWORD                                   dwNotifyFilter,
    _Out_opt_   LPDWORD                                 lpBytesReturned,
    _Inout_opt_ LPOVERLAPPED                            lpOverlapped,
    _In_opt_    LPOVERLAPPED_COMPLETION_ROUTINE         lpCompletionRoutine,
    _In_        READ_DIRECTORY_NOTIFY_INFORMATION_CLASS  ReadDirectoryNotifyInformationClass
)
{
    DWORD Instance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        Log(LogLevel_DebugBasic, L"[%s%d] ReadDirectoryChangesExW Fixup Handle=0x%x ", g_FrfModuleName, Instance, hDirectory);
        if (guard)
        {
            FILE_BASIC_INFO FileBasicInfoData;
            BOOL res1 = GetFileInformationByHandleEx(hDirectory, FileBasicInfo, &FileBasicInfoData, sizeof(FileBasicInfoData));
            if (res1 != 0)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] RDCWEx RDCWEx Handle FileBasicInfo Fail err=0x%x", g_FrfModuleName, Instance, GetLastError());
            }
            DWORD len = 512;
            PFILE_NAME_INFO pFileInformation = (PFILE_NAME_INFO)malloc(len);
            if (pFileInformation != NULL)
            {
                BOOL res = GetFileInformationByHandleEx(hDirectory, FileNameInfo, pFileInformation, len);
                if (res != 0)
                {
                    LogCountedStringW(LogLevel_DebugBasic, g_FrfModuleName, Instance, "       RDCWEx Handle Path is", pFileInformation->FileName, pFileInformation->FileNameLength);
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  RDCWEx Path did not get returned.", g_FrfModuleName, Instance);
        }
                free(pFileInformation);
    }
            ; // if needed
        }
        BOOL bRet = impl::ReadDirectoryChangesExW(hDirectory, lpBuffer, nBufferLength, bWatchSubtree, dwNotifyFilter, lpBytesReturned, lpOverlapped, lpCompletionRoutine, ReadDirectoryNotifyInformationClass);
        if (bRet == 0)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] ReadDirectoryChangesExW returns Fail error=0x%x", g_FrfModuleName, Instance, GetLastError());
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] ReadDirectoryChangesExW returns Success", g_FrfModuleName, Instance);
        }
        return bRet;
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] ReadDirectoryChangesExW Exception Fail", g_FrfModuleName, Instance);
        return 0;
    }
}
DECLARE_FIXUP(impl::ReadDirectoryChangesExW, ReadDirectoryChangesExWFixup);