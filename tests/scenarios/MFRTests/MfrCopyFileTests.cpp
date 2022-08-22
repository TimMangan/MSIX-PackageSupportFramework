//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrCopyFileTest.h"
#include <stdio.h>
#include "MfrCleanup.h"

std::vector<MfrCopyFileTest> MfrCopyFileTests;
std::vector<MfrCopyFileExTest> MfrCopyFileExTests;
std::vector<MfrCopyFile2Test> MfrCopyFile2Tests;

int InitializeCopyFileTestArray()
{
    std::wstring tempS;
    std::wstring tempD;

    // Requests to Native File Locations for CopyFile via VFS
    tempS = L"C:\\Program Files\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempD = L"C:\\Program Files\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini";
    MfrCopyFileTest ts_Native_PF1 = { "CopyFile Native-file VFS exists in package",                          true, true, tempS, tempD, true, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Native_PF1);

    MfrCopyFileTest ts_Native_PF1E1 = { "CopyFile Native-file VFS exists in package and dest (not allowed)", true, false, tempS, tempD, true, false, ERROR_FILE_EXISTS };
    MfrCopyFileTests.push_back(ts_Native_PF1E1);

    MfrCopyFileTest ts_Native_PF1E2 = { "CopyFile Native-file VFS exists in package and dest (allowed)",     true, false, tempS, tempD, false, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Native_PF1E2);

    tempS = L"C:\\Program Files\\PlaceholderTest\\MissingNativePlaceholder.txt";
    tempD = L"C:\\Program Files\\PlaceholderTest\\CopiedMissingNativePlaceholder.txt";
    MfrCopyFileTest t_Native_PF2 = { "CopyFile Native-file VFS missing in package",                          true, false, tempS, tempD, true, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Native_PF2);

    tempS = L"C:\\Program Files\\MissingPlaceholderTest\\MissingNativePlaceholder.txt";
    tempD = L"C:\\Program Files\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder.txt";
    MfrCopyFileTest t_Native_PF3 = { "CopyFile Native-file VFS parent-folder missing in package",            true, false, tempS, tempD, true, false, ERROR_PATH_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Native_PF3);


    // Requests to Package File Locations for CopyFile using VFS
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedPlaceholder.txt";
    MfrCopyFileTest t_Vfs_PF1 = { "Package-file VFS exists in package",                                    true, true, tempS, tempD, true,true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(t_Vfs_PF1);

    MfrCopyFileTest ts_Vfs_PF1E1 = { "CopyFile Package-file VFS exists in package and dest (not allowed)", true, false, tempS, tempD, true, false, ERROR_FILE_EXISTS };
    MfrCopyFileTests.push_back(ts_Vfs_PF1E1);

    MfrCopyFileTest ts_Vfs_PF1E2 = { "CopyFile Package-file VFS exists in package and dest (allowed)",     true, false, tempS, tempD, false, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Vfs_PF1E2);

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
    MfrCopyFileTest t_Vfs_PF2 = { "CopyFile Package-file VFS missing in package",                          true, false, tempS, tempD, true, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Vfs_PF2);

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
    MfrCopyFileTest t_Vfs_PF3 = { "CopyFile Package-file VFS parent-folder missing in package",            true, false, tempS, tempD, true, false, ERROR_PATH_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for CopyFile using VFS
    tempS = g_writablePackageRootPath.c_str();
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");  
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
    MfrCopyFileTest t_Redir_PF1 = { "Redirected-file VFS exists in package",                               true, true, tempS, tempD, true, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(t_Redir_PF1);


    MfrCopyFileTest ts_Redir_PF1E1 = { "Redirected Package-file VFS exists in package and dest (not allowed)", true, false, tempS, tempD, true, false, ERROR_FILE_EXISTS };
    MfrCopyFileTests.push_back(ts_Redir_PF1E1);

    MfrCopyFileTest ts_Redir_PF1E2 = { "Redirected Package-file VFS exists in package and dest (allowed)",     true, false, tempS, tempD, false, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Redir_PF1E2);

    tempS = g_writablePackageRootPath.c_str();
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
    MfrCopyFileTest t_Redir_PF2 = { "Redirected Package-file VFS missing in package",                          true, false, tempS, tempD, true, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Redir_PF2);

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
    MfrCopyFileTest t_Redir_PF3 = { "CopyFile Package-file VFS parent-folder missing in package",            true, false, tempS, tempD, true, false, ERROR_PATH_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Redir_PF3);


    return (int)(MfrCopyFileTests.size());
}

int InitializeCopyFileExTestArray()
{
    std::wstring temp;

    return (int)MfrCopyFileExTests.size();
}

int InitializeCopyFile2TestArray()
{
    std::wstring temp;

    return (int)MfrCopyFile2Tests.size();
}

int InitializeCopyFileTests()
{
    int count = 0;
    count += InitializeCopyFileTestArray();
    count += InitializeCopyFileExTestArray();
    count += InitializeCopyFile2TestArray();
    return count;
}

BOOL CopyFileIndividualTest(MfrCopyFileTest testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "CopyFile Test: ";
        testname.append(testInput.TestName);
        test_begin(testname);
        
        if (testInput.cleanup)
        {
            DWORD cleanupResult = MfrCleanup();
            if (cleanupResult != 0)
            {
                trace_message("***** CLEANUP ERROR *****", error_color);
            }
            else
            {
                trace_message("***** CLEANUP SUCCESS *****", error_color);
            }
        }

        auto testResult = CopyFile(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.FailIfExists);
        if (testResult == 0)
        {
            DWORD cErr = GetLastError();
            // call failed
            if (testInput.Expected_Result)
            {
                // should have succeeded
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L" GetLastError=";
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result =  cErr;
                test_end(cErr);
            }
            else if (cErr == testInput.Expected_LastError)
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // different reason, not cool
                std::wstring detail1 = L"ERROR: Although returned value correct. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L"      Expected GetLastError=";
                detail2.append(std::to_wstring(testInput.Expected_LastError));
                detail2.append(L"      Received GetLastError=");
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
            }
        }
        else
        {
            // call succeeded
            if (testInput.Expected_Result)
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // should have failed
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                result =  -1;
                test_end(-1);
            }
        }
    }
    return result;
}

int RunCopyFileTests()
{
    int result = ERROR_SUCCESS;
    BOOL testResult;

    for (MfrCopyFileTest testInput : MfrCopyFileTests)
    {
        if (testInput.enabled)
        {
            testResult = CopyFileIndividualTest(testInput);
            result = result ? result : testResult;
        }
    }



    return result;
}