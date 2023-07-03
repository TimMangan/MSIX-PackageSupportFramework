//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrCreateDirectoryTests.h"
#include <stdio.h>
#include "MfrCleanup.h"

std::vector<MfrCreateDirectoryTest> MfrCreateDirectoryTests;
std::vector<MfrCreateDirectoryExTest> MfrCreateDirectoryExTests;

int InitializeCreateDirectoryTestArray()
{
    std::wstring tempD;

    // Requests to Native File Locations for CreateDirectory via VFS
    tempD = g_NativePF;
    tempD.append(L"\\PlaceholderTest");
    MfrCreateDirectoryTest ts_Native_PF1 = { "MFR+ILV CreateDirectory Native-folder VFS exists in package (not allowed)",  true, true,  true,
                                tempD, FALSE, ERROR_ALREADY_EXISTS };
    MfrCreateDirectoryTests.push_back(ts_Native_PF1);

    // Requests to Package File Locations for CreateDirectory VFS
    // 
    tempD = g_NativePF;

#if _M_IX86
    tempD.append(L"\\NotPreviousExistingDirectoryTest");
#else
    tempD.append(L"\\NotPreviousExistingDirectoryTest");
#endif
    MfrCreateDirectoryTest ts_Package_PF1 = { "MFR+ILV CreateDirectory Non-Existing Directory in Package VFS Test (allowed)",  true, true,  true,
                                tempD, true, ERROR_SUCCESS };
    MfrCreateDirectoryTests.push_back(ts_Package_PF1);

    // Requests to Package File Locations for CreateDirectory PVAD
    tempD = g_Cwd;
    tempD.append(L"\\DoesNotPreExistFolderTest_PVAD");
    MfrCreateDirectoryTest ts_PVAD_1 = { "MFR+ILV CreateDirectory Package-folder PVAD not exists in package",  true, true,  true,
                                tempD, true, ERROR_SUCCESS };
    MfrCreateDirectoryTests.push_back(ts_PVAD_1);

    // Requests to Redirected File Locations for CreateDirectory using VFS
    tempD = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempD.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NewFolder1");
#else
    tempD.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NewFolder1");
#endif
    MfrCreateDirectoryTest t_Redir_PF1 = { "MFR+ILV Redirected-file VFS missing in package with parent present (allowed)", true, true, true, 
                                    tempD, true, ERROR_SUCCESS };
    MfrCreateDirectoryTests.push_back(t_Redir_PF1);

    tempD = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempD.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NewFolder2\\NewFolder3");
#else
    tempD.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NewFolder2\\NewFolder3");
#endif
    MfrCreateDirectoryTest t_Redir_PF2 = { "MFR+ILV Redirected-file VFS missing in package with parent missing (not allowed normally but OK)", true, true, true, 
                                  //  tempD, FALSE, ERROR_PATH_NOT_FOUND };
                                      tempD, TRUE, ERROR_SUCCESS };
MfrCreateDirectoryTests.push_back(t_Redir_PF2);





    int count = 0;
    for (MfrCreateDirectoryTest t : MfrCreateDirectoryTests)      if (t.enabled) { count++; }

    return count;
}


int InitializeCreateDirectoryExTestArray()
{
    std::wstring tempS;
    std::wstring tempD;

    // Requests to Native File Locations for CopyFile via VFS
    tempS = g_NativePF;
    tempS.append(L"\\PlaceholderTest");
    tempD = g_NativePF;;
    tempD.append(L"\\PlaceholderTest4");
    MfrCreateDirectoryExTest ts_Native_PF1 = { "MFR+ILV CreateDirectoryEx from Native-folder to VFS exists in package (not allowed)", true, true,  true,
                                tempS, tempD, FALSE, ERROR_ALREADY_EXISTS };
    MfrCreateDirectoryExTests.push_back(ts_Native_PF1);


    // Requests to Package File Locations for CreateDirectory VFS
    // 
    tempD = g_NativePF;
    tempD.append(L"\\NotPreviousExistingDirectoryTestEx");
    MfrCreateDirectoryExTest ts_Package_PF1 = { "MFR+ILV CreateDirectoryEx to Non-Existing Directory in Package VFS Test (allowed)",  true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrCreateDirectoryExTests.push_back(ts_Package_PF1);

    // Requests to Package File Locations for Directory PVAD
    tempD = g_Cwd;
    tempD.append(L"\\DoesNotPreExistFolderTest_PVADEX");
    MfrCreateDirectoryExTest ts_PVAD_1 = { "MFR+ILV CreateDirectory Package-folder PVAD not exists in package",  true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrCreateDirectoryExTests.push_back(ts_PVAD_1);


    // Requests to Redirected File Locations for CreateDirectory using VFS
    tempD = g_writablePackageRootPath.c_str();
    tempD.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NewFolderEx1");
    MfrCreateDirectoryExTest t_Redir_PF1 = { "MFR+ILV Redirected-file VFS missing in package with parent present (allowed)", true, true, true,
                                    tempS, tempD, true, ERROR_SUCCESS };
    MfrCreateDirectoryExTests.push_back(t_Redir_PF1);

    tempD = g_writablePackageRootPath.c_str();
    tempD.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NewFolderEx2\\NewFolderEx3");
    tempD.append(L"\\DoesNotPreExistFolderTest");
    MfrCreateDirectoryExTest t_Redir_PF2 = { "MFR+ILV Redirected-file VFS missing in package with parent missing (not allowed normally but OK)", true, true, true,
                                //  tempS, tempD, FALSE, ERROR_PATH_NOT_FOUND };
                                    tempS, tempD, TRUE, ERROR_SUCCESS };
    MfrCreateDirectoryExTests.push_back(t_Redir_PF2);



    int count = 0;
    for (MfrCreateDirectoryExTest t : MfrCreateDirectoryExTests)      if (t.enabled) { count++; }

    return count;
}


int InitializeCreateDirectoryTests()
{
    int count = 0;
    count += InitializeCreateDirectoryTestArray();
    count += InitializeCreateDirectoryExTestArray();
    return count;
}


BOOL CreateDirectoryIndividualTest(MfrCreateDirectoryTest testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "MFR+ILV CreateDirectory Test: ";
        testname.append(testInput.TestName);
        test_begin(testname);

        if (testInput.cleanupWritablePackageRoot)
        {
            DWORD cleanupResult = MfrCleanupWritablePackageRoot();
            if (cleanupResult != 0)
            {
                trace_message("***** CLEANUP WritablePackageRoot ERROR *****\n", error_color);
            }
            else
            {
                trace_message("CLEANUP WritablePackageRoot SKIPPED\n", info_color);
            }
        }
        if (testInput.cleanupDocumentsSubfolder)
        {
            DWORD cleanupResult = MfrCleanupLocalDocuments(MFRTESTDOCS);
            if (cleanupResult != 0)
            {
                trace_message("***** CLEANUP LocalDocuments ERROR *****\n", error_color);
            }
            else
            {
                trace_message("CLEANUP LocalDocuments SUCCESS\n", info_color);
            }
        }

        auto testResult = CreateDirectory(testInput.DirectoryPath.c_str(),  nullptr);
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
                result = cErr;
                test_end(cErr);
            }
            else if (cErr == testInput.Expected_LastError ||
                (testInput.AllowAlternateError && testInput.AlternateError == cErr))
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
                result = -1;
                test_end(-1);
            }
        }
    }
    return result;
}


BOOL CreateDirectoryExIndividualTest(MfrCreateDirectoryExTest testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "MFR+ILV CreateDirectoryEx Test: ";
        testname.append(testInput.TestName);
        test_begin(testname);

        if (testInput.cleanupWritablePackageRoot)
        {
            DWORD cleanupResult = MfrCleanupWritablePackageRoot();
            if (cleanupResult != 0)
            {
                trace_message("***** CLEANUP WritablePackageRoot ERROR *****\n", error_color);
            }
            else
            {
                trace_message("CLEANUP WritablePackageRoot SKIPPED\n", info_color);
            }
        }
        if (testInput.cleanupDocumentsSubfolder)
        {
            DWORD cleanupResult = MfrCleanupLocalDocuments(MFRTESTDOCS);
            if (cleanupResult != 0)
            {
                trace_message("***** CLEANUP LocalDocuments ERROR *****\n", error_color);
            }
            else
            {
                trace_message("CLEANUP LocalDocuments SUCCESS\n", info_color);
            }
        }

        auto testResult = CreateDirectoryEx(testInput.TemplatePath.c_str(), testInput.DirectoryPath.c_str(), nullptr);
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
                result = cErr;
                test_end(cErr);
            }
            else if (cErr == testInput.Expected_LastError ||
                (testInput.AllowAlternateError && testInput.AlternateError == cErr))
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
                result = -1;
                test_end(-1);
            }
        }
    }
    return result;
}


int RunCreateDirectoryTests()
{
    int result = ERROR_SUCCESS;
    BOOL testResult;

    for (MfrCreateDirectoryTest testInput : MfrCreateDirectoryTests)
    {
        if (testInput.enabled)
        {
            testResult = CreateDirectoryIndividualTest(testInput);
            result = result ? result : testResult;
        }
    }


    for (MfrCreateDirectoryExTest testInput : MfrCreateDirectoryExTests)
    {
        if (testInput.enabled)
        {
            testResult = CreateDirectoryExIndividualTest(testInput);
            result = result ? result : testResult;
        }
    }

    return result;
}
