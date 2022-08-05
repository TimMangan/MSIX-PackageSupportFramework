//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <windows.h> 
#include <winbase.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

extern void Log(const char* fmt, ...);

/**********************************************
typedef struct _FILE_NOTIFY_INFORMATION {
    DWORD NextEntryOffset;
    DWORD Action;
    DWORD FileNameLength;
    WCHAR FileName[1];
} FILE_NOTIFY_INFORMATION, * PFILE_NOTIFY_INFORMATION;

typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        } DUMMYSTRUCTNAME;
        PVOID Pointer;
    } DUMMYUNIONNAME;
    HANDLE    hEvent;
} OVERLAPPED, * LPOVERLAPPED;
***************************************************/

//LPOVERLAPPED_COMPLETION_ROUTINE LPoverlappedCompletionRoutine;
void WINAPI  CompletionRoutine(
        [[maybe_unused]] ULONG dwErrorCode,
        [[maybe_unused]]  ULONG dwNumberOfBytesTransfered,
        [[maybe_unused]] LPOVERLAPPED lpOverlapped
)
{
    trace_message(L"overlappedCompletionRoutine called.\n");
}


int OddDirectoryTests([[maybe_unused]] int TestNum)
{
    int SubTestNum = 1;
    int result = ERROR_SUCCESS;
    int testResult;
    std::wstring FolderTest = L"VFS\\ProgramFilesCommonX64";

#if _DEBUG
    Log("[%d] [%d] Starting OddDirectoryTests ********************************************************************************************", TestNum, 0);
#endif

    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    //clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d1]  Cleanup End.", TestNum, SubTestNum);
#endif


    test_begin("CreateFile for Testing a real directory");

    HANDLE testFileH = CreateFile(FolderTest.c_str(), 0x0, 0x7, NULL, 0x3, 0x2000000, NULL);
    if (testFileH == INVALID_HANDLE_VALUE)
    {
        testResult = GetLastError();
    }
    else
    {
        testResult = ERROR_SUCCESS;
        CloseHandle(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;


    test_begin("CreateFile for Testing a real directory2");
    testFileH = CreateFile(FolderTest.c_str(), 0x80000000, 0x7, NULL, 0x3, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (testFileH == INVALID_HANDLE_VALUE)
    {
        testResult = GetLastError();
    }
    else
    {
        testResult = ERROR_SUCCESS;
        CloseHandle(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    test_begin("CreateFile for Testing a directory with no name should fail");

    testFileH = CreateFile(L"", 0x0, 0x7, NULL, 0x3, 0x2000000,NULL);
    if (testFileH != INVALID_HANDLE_VALUE)
    {
        testResult = -1;
        CloseHandle(testFileH);
    }
    else
    {
        testResult = ERROR_SUCCESS;
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    test_begin("CreateFile for Testing a directory with no name 2");
    testFileH = CreateFile(L"", FILE_LIST_DIRECTORY, 0x7, NULL, 0x3, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (testFileH != INVALID_HANDLE_VALUE)
    {
        testResult = -1;
        CloseHandle(testFileH);
    }
    else
    {
        testResult = ERROR_SUCCESS;
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    test_begin("ReadDirectoryChanges for a VFS folder ");
    SetLastError(0);
    testFileH = CreateFile(FolderTest.c_str(), 
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
        NULL, 
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 
        NULL);
    if (testFileH == INVALID_HANDLE_VALUE)
    {
        testResult = GetLastError();
        trace_message(L"Unable to create file before monitoring.\n");
    }
    else
    {
        
        ULONG BUFSIZE = 4096;
        VOID * pBuff2 = malloc(BUFSIZE);
        DWORD BytesIn;
        PFILE_NOTIFY_INFORMATION pBuffer = (PFILE_NOTIFY_INFORMATION)malloc(BUFSIZE);
        if (pBuffer != NULL && pBuff2 != NULL)
        {
            pBuffer->NextEntryOffset = 0;

            LPOVERLAPPED pOverlapped = (LPOVERLAPPED)pBuff2;

            BOOL bRet = ReadDirectoryChangesW(testFileH,
                pBuffer,
                BUFSIZE,
                TRUE,
                0x17F,
                &BytesIn,
                pOverlapped,
                (LPOVERLAPPED_COMPLETION_ROUTINE) CompletionRoutine); 
            if (bRet == 0)
            {
                testResult = GetLastError();
                trace_message(L"Unable to monitor directory.\n");
            }
            else
            {
                testResult = ERROR_SUCCESS;
            }
            CloseHandle(testFileH);
            free(pBuffer);
            free(pBuff2);
        }
        else
        {
            CloseHandle(testFileH);
            testResult = -1;
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    test_begin("ReadDirectoryChangesEx for a VFS folder (Notify) ");
    SetLastError(0);
    testFileH = CreateFile(FolderTest.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);
    if (testFileH == INVALID_HANDLE_VALUE)
    {
        testResult = GetLastError();
        trace_message(L"Unable to create file before monitoring.\n");
    }
    else
    {

        ULONG BUFSIZE = 4096;
        VOID* pBuff2 = malloc(BUFSIZE);
        DWORD BytesIn;
        PFILE_NOTIFY_INFORMATION pBuffer = (PFILE_NOTIFY_INFORMATION)malloc(BUFSIZE);
        if (pBuffer != NULL && pBuff2 != NULL)
        {
            pBuffer->NextEntryOffset = 0;

            LPOVERLAPPED pOverlapped = (LPOVERLAPPED)pBuff2;


            BOOL bRet = ReadDirectoryChangesExW(testFileH,
                pBuffer,
                BUFSIZE,
                TRUE,
                0x17F,
                &BytesIn,
                pOverlapped,
                (LPOVERLAPPED_COMPLETION_ROUTINE)CompletionRoutine,
                ReadDirectoryNotifyInformation); 
            if (bRet == 0)
            {
                testResult = GetLastError();
                trace_message(L"Unable to monitor directory.\n");
            }
            else
            {
                testResult = ERROR_SUCCESS;
            }
            CloseHandle(testFileH);
            free(pBuffer);
            free(pBuff2);
        }
        else
        {
            CloseHandle(testFileH);
            testResult = -1;
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;


    test_begin("ReadDirectoryChangesEx for a VFS folder (NotifyExtended) ");
    SetLastError(0);
    testFileH = CreateFile(FolderTest.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);
    if (testFileH == INVALID_HANDLE_VALUE)
    {
        testResult = GetLastError();
        trace_message(L"Unable to create file before monitoring.\n");
    }
    else
    {

        ULONG BUFSIZE = 4096;
        VOID* pBuff2 = malloc(BUFSIZE);
        DWORD BytesIn;
        PFILE_NOTIFY_INFORMATION pBuffer = (PFILE_NOTIFY_INFORMATION)malloc(BUFSIZE);
        if (pBuffer != NULL && pBuff2 != NULL)
        {
            pBuffer->NextEntryOffset = 0;

            LPOVERLAPPED pOverlapped = (LPOVERLAPPED)pBuff2;


            BOOL bRet = ReadDirectoryChangesExW(testFileH,
                pBuffer,
                BUFSIZE,
                TRUE,
                0x17F,
                &BytesIn,
                pOverlapped,
                (LPOVERLAPPED_COMPLETION_ROUTINE)CompletionRoutine,
                ReadDirectoryNotifyExtendedInformation);
            if (bRet == 0)
            {
                testResult = GetLastError();
                trace_message(L"Unable to monitor directory.\n");
            }
            else
            {
                testResult = ERROR_SUCCESS;
            }
            CloseHandle(testFileH);
            free(pBuffer);
            free(pBuff2);
        }
        else
        {
            CloseHandle(testFileH);
            testResult = -1;
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;


    test_begin("ReadDirectoryChangesEx for a VFS folder (NotifyFull) ");
    SetLastError(0);
    testFileH = CreateFile(FolderTest.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);
    if (testFileH == INVALID_HANDLE_VALUE)
    {
        testResult = GetLastError();
        trace_message(L"Unable to create file before monitoring.\n");
    }
    else
    {

        ULONG BUFSIZE = 4096;
        VOID* pBuff2 = malloc(BUFSIZE);
        DWORD BytesIn;
        PFILE_NOTIFY_INFORMATION pBuffer = (PFILE_NOTIFY_INFORMATION)malloc(BUFSIZE);
        if (pBuffer != NULL && pBuff2 != NULL)
        {
            pBuffer->NextEntryOffset = 0;

            LPOVERLAPPED pOverlapped = (LPOVERLAPPED)pBuff2;


            BOOL bRet = ReadDirectoryChangesExW(testFileH,
                pBuffer,
                BUFSIZE,
                TRUE,
                0x17F,
                &BytesIn,
                pOverlapped,
                (LPOVERLAPPED_COMPLETION_ROUTINE)CompletionRoutine,
                ReadDirectoryNotifyFullInformation);
            if (bRet == 0)
            {
                testResult = GetLastError();
                trace_message(L"Unable to monitor directory.\n");
            }
            else
            {
                testResult = ERROR_SUCCESS;
            }
            CloseHandle(testFileH);
            free(pBuffer);
            free(pBuff2);
        }
        else
        {
            CloseHandle(testFileH);
            testResult = -1;
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



#if _DEBUG
    Log("[%d] [%d] Ending OddDirectoryTests ********************************************************************************************", TestNum, 0);
#endif

    return result;
}
//OddDirectoryTests = 8
