//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <iostream>

#include <Windows.h>

#include <error_logging.h>
#include <test_config.h>
#include <file_paths.h>

using namespace std::literals;

#define UNEXPECTED_RESULT -1

void PDT_Tests(const char* testType, const wchar_t* basePath, const wchar_t* folderName, const wchar_t* fileName, const char *expectedOriginalContents, bool ExpectedResultTest1, bool ExpectedResultTest2, bool ExpectedResultTest3)
{
    int result;
    std::wstring testPath = std::wstring(basePath).append(folderName).append(L"\\").append(fileName);

    std::string test1String = std::string(testType).append(" Presence Test on ").append(narrow(folderName));
    test_begin(test1String);
    result = ERROR_SUCCESS;
    auto test1 = ::GetFileAttributes(testPath.c_str());
    if (test1 == INVALID_FILE_ATTRIBUTES)
    {
        if (ExpectedResultTest1)
        {
            trace_message(L"ERROR: Did not find the file!\n", error_color);
            result = test1;
        }
        else
        { 
            trace_message(L"OK: Did not find the file but that was expected.\n", info_color);
        }
    }
    else
    {
        if (ExpectedResultTest1)
        {
            trace_message(L"OK: Found the file attributes.\n", info_color);
        }
        else
        {
            trace_message(L"ERROR: Found the file and should not have!\n", error_color);
            result = UNEXPECTED_RESULT;
        }
    }
    test_end(result);


    std::string test2String = std::string(testType).append(" Read Test on ").append(narrow(folderName));
    test_begin(test2String);
    result = ERROR_SUCCESS;
    auto test2 = ::CreateFile(testPath.c_str(), FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
    if (test2 == INVALID_HANDLE_VALUE)
    {
        if (ExpectedResultTest2)
        {
            trace_message(L"ERROR: Did not open the file for read!\n", error_color);
            result = GetLastError();
        }
        else
        {
            trace_message(L"OK: Did not open the file for read but that was expected.\n", info_color);
        }
    }
    else
    {
        if (ExpectedResultTest2)
        {
            trace_message(L"OK: Opened the file for read.\n", info_color);
        }
        else
        {
            trace_message(L"ERROR: Opened the file for read and should not have!\n", error_color);
            result = UNEXPECTED_RESULT;
        }
        char buffer[256];
        DWORD size;
        OVERLAPPED overlapped = {};
        overlapped.Offset = 0; // Always read from the start of the file
        if (!::ReadFile(test2, buffer, sizeof(buffer) - 1, &size, &overlapped) && (::GetLastError() != ERROR_HANDLE_EOF))
        {
            trace_last_error(L"Failed to read from file");
        }
        else
        {
            buffer[size] = '\0';
            if (std::strcmp(buffer, expectedOriginalContents) != 0)
            {
                trace_messages(error_color,
                    L"ERROR: File contents did not match the expected value\n",
                    L"ERROR: Expected contents: ", error_info_color, expectedOriginalContents, new_line, error_color,
                    L"ERROR: Actual contents:   ", error_info_color, buffer, new_line);
                result =  ERROR_ASSERTION_FAILURE;
            }
        }

        CloseHandle(test2);
    }
    test_end(result);


    std::string test3String = std::string(testType).append(" Write Test on ").append(narrow(folderName)); 
    test_begin(test3String);
    result = ERROR_SUCCESS;
    auto test3 = ::CreateFile(testPath.c_str(), FILE_GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (test3 == INVALID_HANDLE_VALUE)
    {
        if (ExpectedResultTest3)
        {
            trace_message(L"ERROR: Did not open the file for write!\n", error_color);
            result = GetLastError();
        }
        else
        {
            trace_message(L"OK: Did not open the file for write but that was expected.\n", info_color);
        }
    }
    else
    {
        if (ExpectedResultTest3)
        {
            trace_message(L"OK: Opened the file for write.\n", info_color);
        }
        else
        {
            trace_message(L"ERROR: Opened the file for write and should not have!\n", error_color);
            result = UNEXPECTED_RESULT;
        }
        const char* bufferW = "This is the updated content that is just awesome and wild.";
        DWORD sizeWin = (DWORD) strlen(bufferW);
        DWORD sizeWout;
        auto bResultW = ::WriteFile(test3, bufferW, sizeWin, &sizeWout,NULL);
        if (bResultW == false)
        {
            trace_last_error(L"Failed to write to file");
            result = GetLastError();
            CloseHandle(test3);
        }
        else
        {
            CloseHandle(test3);
            auto test3b = ::CreateFile(testPath.c_str(), FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (test3b != INVALID_HANDLE_VALUE)
            {
                char bufferR[512];
                DWORD sizeR;
                OVERLAPPED overlappedR = {};
                overlappedR.Offset = 0; // Always read from the start of the file
                if (!::ReadFile(test3b, bufferR, sizeof(bufferR) - 1, &sizeR, &overlappedR) && (::GetLastError() != ERROR_HANDLE_EOF))
                {
                    trace_last_error(L"Failed to read from file");
                    result = GetLastError();
                }
                else
                {
                    bufferR[sizeR] = '\0';
                    if (std::strcmp(bufferR, bufferW) != 0)
                    {
                        trace_messages(error_color,
                            L"ERROR: File contents did not match the expected value\n",
                            L"ERROR: Expected contents: ", error_info_color, bufferW, new_line, error_color,
                            L"ERROR: Actual contents:   ", error_info_color, bufferR, new_line);
                        result = ERROR_ASSERTION_FAILURE;
                    }
                }
                CloseHandle(test3b);
            }
            else
            {
                trace_last_error(L"Failed to reopen file to re-read.");
                result = GetLastError();
            }
        }
    }
    test_end(result);


}

int wmain(int argc, const wchar_t** argv)
{
    const char* expectedoriginalcontent = "You are reading the original file.";  // matches file.txt contents
    auto result = parse_args(argc, argv);
    if (result == ERROR_SUCCESS)
    {
        test_initialize("Package File Tests", 18);
        
        PDT_Tests("Native Requests", L"C:\\", L"TestDriveFolder1",                        L"DriveFile1a.txt", expectedoriginalcontent, true, true, true);
        PDT_Tests("Package Request", L"",     L"VFS\\AppVPackageDrive\\TestDriveFolder1", L"DriveFile1b.txt", expectedoriginalcontent, true, true, true);

        PDT_Tests("Native Request",  L"C:\\", L"TestDriveFolder2",                        L"DriveFile2a.txt", expectedoriginalcontent, true, true, true);
        PDT_Tests("Package Request", L"",     L"VFS\\AppVPackageDrive\\TestDriveFolder2", L"DriveFile2b.txt", expectedoriginalcontent, true, true, true);

        PDT_Tests("Native Request",  L"C:\\", L"TestDriveFolder3",                        L"DriveFile3a.txt", expectedoriginalcontent, true, true, true);
        PDT_Tests("Package Request", L"",     L"VFS\\AppVPackageDrive\\TestDriveFolder3", L"DriveFile3b.txt", expectedoriginalcontent, true, true, true);

        test_cleanup();
    }

    if (!g_testRunnerPipe)
    {
        system("pause");
    } 

    return 0;
}
