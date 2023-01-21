//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//#define Dowatch 1

#include "MfrReplaceFileTests.h"
#include "MfrConsts.h"
#include "MfrCleanup.h"
#include <thread>
#include <cstdlib>

int SpecialCounter = 0;

void WINAPI  CompletionRoutine(
    [[maybe_unused]] ULONG dwErrorCode,
    [[maybe_unused]] ULONG dwNumberOfBytesTransfered,
    [[maybe_unused]] LPOVERLAPPED lpOverlapped
)
{
    SpecialCounter++;
    trace_message(L"overlappedCompletionRoutine called.\n");
}


int InitializeOddDirectoryTests()
{
#if DOWATCH
    return 7;
#else
    return 4;
#endif
}

int RunOddDirectoryTests()
{
    int result = ERROR_SUCCESS;
    int testResult;
    std::wstring FolderTest = L"VFS\\Common AppData\\folder\\folder2";

    DWORD cleanupResult = MfrCleanupWritablePackageRoot();
    if (cleanupResult != 0)
    {
        trace_message("***** CLEANUP WritablePackageRoot ERROR *****\n", error_color);
    }
    else
    {
        trace_message("CLEANUP WritablePackageRoot SKIPPED\n", info_color);
    }


    test_begin("MFR+ILV OddDirectory #1 CreateDirectory for Testing a new directory (should succeed).");

    BOOL testB = CreateDirectory(FolderTest.c_str(),  NULL);
    if (testB == 0)
    {
        testResult = GetLastError();
    }
    else
    {
        testResult = ERROR_SUCCESS;
    }
    result = result ? result : testResult;
    test_end(testResult);


    test_begin("MFR+ILV OddDirectory #2 CreateDirectory for Testing a existing directory (should fail).");
    testB = CreateDirectory(FolderTest.c_str(), NULL);
    if (testB == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS)
        {
            testResult = ERROR_SUCCESS;
        }
        else
        {
            testResult = err;
        }
    }
    else
    {
        testResult = -1;
    }
    result = result ? result : testResult;
    test_end(testResult);



    test_begin("MFR+ILV OddDirectory #3 CreateDirectory for Testing a directory with no name (should fail).");
    testB = CreateDirectory(L"", NULL);
    if (testB == 0)
    {
        testResult = ERROR_SUCCESS;
    }
    else
    {
        testResult = -1;
    }
    result = result ? result : testResult;
    test_end(testResult);




    test_begin("MFR+ILV OddDirectory #4 CreateFile to open directory handle and ReadDirectoryChanges without changes.");
    SetLastError(0);
    SpecialCounter = 0;

    HANDLE testFileH = CreateFile(FolderTest.c_str(),
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

            BOOL bRet = ReadDirectoryChangesW(testFileH,
                pBuffer,
                BUFSIZE,
                TRUE,
                0x17F,
                &BytesIn,
                pOverlapped,
                (LPOVERLAPPED_COMPLETION_ROUTINE)CompletionRoutine);
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


#if DOWATCH

    test_begin("MFR+ILV OddDirectory #5 CreateFile to open directory handle and ReadDirectoryChangesEx for a VFS folder (with Notify) ");
    SetLastError(0);
    SpecialCounter = 0;

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
        trace_message(L"Unable to create handle before monitoring.\n", error_color);
    }
    else
    {
        trace_message(L"Created handle before monitoring.\n", info_color);
        ULONG BUFSIZE = 4096;
        VOID* pBuff2 = malloc(BUFSIZE);
        PFILE_NOTIFY_INFORMATION pBuffer = (PFILE_NOTIFY_INFORMATION)malloc(BUFSIZE);
        if (pBuffer != NULL && pBuff2 != NULL)
        {
            ZeroMemory(pBuff2, BUFSIZE);
            ZeroMemory(pBuffer, BUFSIZE);

            LPOVERLAPPED pOverlapped = (LPOVERLAPPED)pBuff2;


            BOOL bRet = ReadDirectoryChangesExW(testFileH,
                pBuffer,
                BUFSIZE, 
                TRUE,
                0x17F,
                NULL,
                pOverlapped,
                (LPOVERLAPPED_COMPLETION_ROUTINE)CompletionRoutine,
                ReadDirectoryNotifyExtendedInformation);
            if (bRet == 0)
            {
                testResult = GetLastError();
                trace_message(L"Unable to monitor directory.\n", error_color);
            }
            else
            {
                trace_message(L"Monitoring directory.\n", info_color);
                testResult = ERROR_SUCCESS;

                std::wstring aFile = FolderTest + L"\\testfile5.txt";
                HANDLE testH2 = CreateFile(aFile.c_str(),
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                            NULL,
                                            CREATE_NEW,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL);
                if (testH2 == INVALID_HANDLE_VALUE)
                {
                    testResult = GetLastError();
                    trace_message(L"Unable to create file during monitoring.\n", error_color);
                }
                else 
                {
                    trace_message(L"Text file opened.\n", info_color);
                    DWORD w;
                    WriteFile(testH2, L"hi", 4, &w, NULL);
                    trace_message(L"Text file written.\n", info_color);
                    CloseHandle(testH2);
                    trace_message(L"Text file closed.\n", info_color);
                }
            }
            Sleep(500);
            CloseHandle(testFileH);
            if (SpecialCounter == 1)
            {
                testResult = ERROR_SUCCESS;
            }
            else
            {
                testResult = -1;
                std::wstring msg = L"Incorrect monitor count=";
                msg.append(std::to_wstring(SpecialCounter));
                msg.append(L"\n");
                trace_message(msg.c_str(), error_color);
            }
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


    test_begin("MFR+ILV OddDirectory #6 CreateFile to open directory handle and ReadDirectoryChangesEx for a VFS folder (NotifyExtended) ");
    SetLastError(0);
    SpecialCounter = 0;

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

                std::wstring aFile = FolderTest + L"\\testfile6.txt";
                HANDLE testH2 = CreateFile(aFile.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
                if (testH2 == INVALID_HANDLE_VALUE)
                {
                    testResult = GetLastError();
                    trace_message(L"Unable to create file.\n");
                }
                else
                {
                    DWORD w;
                    WriteFile(testH2, L"hi", 4, &w, NULL);
                    CloseHandle(testH2);
                }
            }
            CloseHandle(testFileH);
            if (SpecialCounter == 1)
            {
                testResult = ERROR_SUCCESS;
            }
            else
            {
                testResult = -1;
                std::wstring msg = L"Incorrect monitor count=";
                msg.append(std::to_wstring(SpecialCounter));
                msg.append(L"\n");
                trace_message(msg.c_str(), error_color);
            }
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

    

    test_begin("MFR+ILV OddDirectory #7 CreateFile to open directory handle and ReadDirectoryChangesEx for a VFS folder (NotifyFull) ");
    SetLastError(0);
    SpecialCounter = 0;

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

                std::wstring aFile = FolderTest + L"\\testfile7.txt";
                HANDLE testH2 = CreateFile(aFile.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
                if (testH2 == INVALID_HANDLE_VALUE)
                {
                    testResult = GetLastError();
                    trace_message(L"Unable to create file.\n");
                }
                else
                {
                    DWORD w;
                    WriteFile(testH2, L"hi", 4, &w, NULL);
                    CloseHandle(testH2);
                }
            }
            CloseHandle(testFileH);
            if (SpecialCounter == 1)
            {
                testResult = ERROR_SUCCESS;
            }
            else
            {
                testResult = -1;
                std::wstring msg = L"Incorrect monitor count=";
                msg.append(std::to_wstring(SpecialCounter));
                msg.append(L"\n");
                trace_message(msg.c_str(), error_color);
            }
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
#endif

    return result;
}
