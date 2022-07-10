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
        Log(L"[%d] ReadDirectoryChangesW Fixup Handle=0x%x ", Instance, hDirectory);


        if (guard)
        {
          
#if _DEBUG
            FILE_BASIC_INFO FileBasicInfoData;
            BOOL res1 = GetFileInformationByHandleEx(hDirectory, FileBasicInfo, &FileBasicInfoData, sizeof(FileBasicInfoData));
            if (res1 != 0)
            {
                Log(L"[%d] RDCW Handle FileBasicInfo OK Attributes=0x%x", Instance, FileBasicInfoData.FileAttributes);
            }
            else
            {
                Log(L"[%d] RDCW Handle FileBasicInfo Fail err=0x%x", Instance, GetLastError());
            }
            DWORD len = 512;
            PFILE_NAME_INFO pFileInformation = (PFILE_NAME_INFO)malloc(len);
            if (pFileInformation != NULL)
            {
                BOOL res2 = GetFileInformationByHandleEx(hDirectory, FileNameInfo, pFileInformation, len);
                if (res2 != 0)
                {
                    wchar_t* wfn = (wchar_t*)malloc(pFileInformation->FileNameLength + 1);
                    if (wfn != NULL)
                    {
                        for (int inx = 0; inx <(int)pFileInformation->FileNameLength; inx++) { wfn[inx] = pFileInformation->FileName[inx]; }
                        wfn[pFileInformation->FileNameLength] = 0;
                        Log(L"[%d]       RDCW Handle Path is %s", Instance, wfn);
                        free(wfn);
                    }
                }
                else
                {
                    Log(L"[%d]  RDCW Handle Path did not get returned err=0x%x.", Instance, GetLastError());
                }
                free(pFileInformation);
            }
#endif
            ; // if needed
        }

        BOOL bRet = impl::ReadDirectoryChangesW(hDirectory, lpBuffer, nBufferLength, bWatchSubtree, dwNotifyFilter, lpBytesReturned, lpOverlapped, lpCompletionRoutine);
#if _DEBUG
        if (bRet == 0)
        {
            Log(L"[%d] ReadDirectoryChangesW returns Failerror=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d] ReadDirectoryChangesW returns Success", Instance);
        }
#endif
        return bRet;
    }
    catch (...)
    {
        Log(L"[%d] ReadDirectoryChangesW Exception Fail", Instance);
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
        Log(L"[%d] ReadDirectoryChangesExW Fixup Handle=0x%x ", Instance, hDirectory);
        if (guard)
        {
#if _DEBUG
            DWORD len = 512;
            PFILE_NAME_INFO pFileInformation = (PFILE_NAME_INFO)malloc(len);
            if (pFileInformation != NULL)
            {
                BOOL res = GetFileInformationByHandleEx(hDirectory, FileNameInfo, pFileInformation, len);
                if (res == ERROR_SUCCESS)
                {
                    LogCountedStringW("       RDCWEx Handle Path is", pFileInformation->FileName, pFileInformation->FileNameLength);
                }
                else
                {
                    Log(L"[%d]  RDCWEx Path did not get returned.", Instance);
        }
                free(pFileInformation);
    }
#endif
            ; // if needed
        }
        BOOL bRet = impl::ReadDirectoryChangesExW(hDirectory, lpBuffer, nBufferLength, bWatchSubtree, dwNotifyFilter, lpBytesReturned, lpOverlapped, lpCompletionRoutine, ReadDirectoryNotifyInformationClass);
#if _DEBUG
        if (bRet == 0)
        {
            Log(L"[%d] ReadDirectoryChangesExW returns Fail error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d] ReadDirectoryChangesExW returns Success", Instance);
        }
#endif
        return bRet;
    }
    catch (...)
    {
        Log(L"[%d] ReadDirectoryChangesExW Exception Fail", Instance);
        return 0;
    }
}
DECLARE_FIXUP(impl::ReadDirectoryChangesExW, ReadDirectoryChangesExWFixup);