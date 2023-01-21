//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrRemoveDeleteTests.h"
#include <stdio.h>
#include "MfrCleanup.h"

std::vector<MfrRemoveDirectoryTest> MfrRemoveDirectoryTests;
std::vector<MfrDeleteFileTest> MfrDeleteFileTests;


int InitializeRemoveDirectoryTestArray()
{
    std::wstring tempD;

    // Requests to Native File Locations for NativeDirectory via VFS
    tempD = g_NativePF + L"\\PlaceholderTest";
    MfrRemoveDirectoryTest ts_Native_P1 = { "MFR RemoveDirectory Native-folder VFS exists in package and redir",  true, true,  true,
                                tempD, tempD, true, ERROR_SUCCESS, 0x10, ERROR_SUCCESS };
    MfrRemoveDirectoryTests.push_back(ts_Native_P1);

   
    tempD = g_NativePF + L"\\NoneSuchDirTest";
    MfrRemoveDirectoryTest ts_Native_P2 = { "MFR RemoveDirectory Native-folder missing from package and is in redir",  true, false,  false,
                                tempD, tempD, true, ERROR_SUCCESS, INVALID_FILE_ATTRIBUTES,  ERROR_FILE_NOT_FOUND };
    MfrRemoveDirectoryTests.push_back(ts_Native_P2);





    int count = 0;
    for (MfrRemoveDirectoryTest t : MfrRemoveDirectoryTests)      if (t.enabled) { count++; }

    return count;
}


int InitializeDeleteFileTestArray()
{
    std::wstring tempD;


    // Requests to Native File Locations for NativeFile via VFS
    tempD = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrDeleteFileTest ts_Native_PF1 = { "MFR DeleteFile Native-file VFS exists in package and redir",  true, true,  true,
                                tempD, tempD, true, ERROR_SUCCESS, 0x20, ERROR_SUCCESS };
    MfrDeleteFileTests.push_back(ts_Native_PF1);

    tempD = g_NativePF + L"\\PlaceholderTest\\NoneSuch.txt";
    MfrDeleteFileTest ts_Native_PF2 = { "MFR DeleteFile Native-file VFS missing (folder exists) in package and is in redir",  true, false,  false,
                                tempD, tempD, true, ERROR_SUCCESS, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrDeleteFileTests.push_back(ts_Native_PF2);

    tempD = g_NativePF + L"\\NoneSuchDirTest\\NoneSuch.txt";
    MfrDeleteFileTest ts_Native_PF3 = { "MFR DeleteFile Native-folder missing from package and is in redir",  true, false,  false,
                                tempD, tempD, true, ERROR_SUCCESS, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrDeleteFileTests.push_back(ts_Native_PF3);




    int count = 0;
    for (MfrDeleteFileTest t : MfrDeleteFileTests)      if (t.enabled) { count++; }

    return count;
}



int InitializeRemoveDeleteTests()
{
    int count = 0;
    count += InitializeRemoveDirectoryTestArray();
    count += InitializeDeleteFileTestArray();
    return count;
}


BOOL RemoveDirectoryIndividualTest(MfrRemoveDirectoryTest testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "MFR RemoveDirectory Test: ";
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
                trace_message("CLEANUP WritablePackageRoot SUCCESS\n", info_color);
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

        if (testInput.OptionalPathPreCreate.length() > 0)
        {
            // Precreate
            auto testResult = CreateDirectory(testInput.OptionalPathPreCreate.c_str(), nullptr);
            if (testResult == 0)
            {
                DWORD cErr = GetLastError();
                // call failed
                    // should have succeeded
                std::wstring detail1 = L"ERROR: Pre-create returned value incorrect. Expected=non-zero";
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
                return result;
            }
        }

        // Now do the delete
        auto testResult = RemoveDirectory(testInput.TestPathRemove.c_str());
        if (testResult == 0)
        {
            DWORD cErr = GetLastError();
            // call failed
            if (testInput.Expected_Result_FromRemove)
            {
                // should have succeeded
                std::wstring detail1 = L"ERROR: Remove returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result_FromRemove));
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
                return result;
            }
            else if (cErr == testInput.Expected_LastError_FromRemove)
            {
                // cool!
                result = ERROR_SUCCESS;
                //test_end(ERROR_SUCCESS);
            }
            else
            {
                // different reason, not cool
                std::wstring detail1 = L"ERROR: Although Remove returned value correct. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result_FromRemove));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L"      Expected GetLastError=";
                detail2.append(std::to_wstring(testInput.Expected_LastError_FromRemove));
                detail2.append(L"      Received GetLastError=");
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
                return result;
            }
        }
        else
        {
            // call succeeded
            if (testInput.Expected_Result_FromRemove)
            {
                // cool!
                result = ERROR_SUCCESS;
                //test_end(ERROR_SUCCESS);
            }
            else
            {
                // should have failed
                std::wstring detail1 = L"ERROR: Remove returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result_FromRemove));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                result = -1;
                test_end(-1);
                return result;
            }
        }

        // If still here, verify existance
        auto verifyResult = GetFileAttributes(testInput.TestPathRemove.c_str());
        auto eCode = GetLastError();
        if (verifyResult == testInput.Expected_Result_FromVerify)
        {
            if (verifyResult == INVALID_FILE_ATTRIBUTES)
            {
                if (eCode == testInput.Expected_LastError_FromVerify)
                {
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    trace_message(L"ERROR: Verifying attributes or error incorrect\n", error_color);
                    std::wstring detail1 = L"       Intended: Att=";
                    detail1.append(std::to_wstring(testInput.Expected_Result_FromVerify));
                    detail1.append(L" Error=");
                    detail1.append(std::to_wstring(testInput.Expected_LastError_FromVerify));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: Att=";
                    detail2.append(std::to_wstring(verifyResult));
                    detail2.append(L" Error=");
                    detail2.append(std::to_wstring(eCode));
                    detail2.append(L"\n");
                    trace_message(detail2.c_str(), error_color);

                    result = result ? result : eCode;
                    if (eCode != ERROR_SUCCESS)
                    {
                        test_end(GetLastError());
                    }
                    else
                    {
                        test_end(-1);
                    }
                }
            }
            else
            {
                // LastError not reset by native GetFileAttributes call when successful
                result = result ? result : ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
        }
        else
        {
            trace_message(L"ERROR: Verifying attributes or error incorrect\n", error_color);
            std::wstring detail1 = L"       Intended: Att=";
            detail1.append(std::to_wstring(testInput.Expected_Result_FromVerify));
            detail1.append(L" Error=");
            detail1.append(std::to_wstring(testInput.Expected_LastError_FromVerify));
            detail1.append(L"\n");
            trace_message(detail1.c_str(), info_color);
            std::wstring detail2 = L"       Actual: Att=";
            detail2.append(std::to_wstring(verifyResult));
            detail2.append(L" Error=");
            detail2.append(std::to_wstring(eCode));
            detail2.append(L"\n");
            trace_message(detail2.c_str(), error_color);

            result = result ? result : eCode;
            if (eCode != ERROR_SUCCESS)
            {
                test_end(GetLastError());
            }
            else
            {
                test_end(-1);
            }
        }

    }
    return result;
}

BOOL DeleteFileIndividualTest(MfrDeleteFileTest testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "MFR DeleteFile Test: ";
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
                trace_message("CLEANUP WritablePackageRoot SUCCESS\n", info_color);
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

        if (testInput.OptionalPathPreCreate.length() > 0)
        {
            // Precreate
            auto createhandle = CreateFile(testInput.OptionalPathPreCreate.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (createhandle == INVALID_HANDLE_VALUE )
            {
                DWORD cErr = GetLastError();
                // call failed
                    // should have succeeded
                std::wstring detail1 = L"ERROR: Pre-create returned value incorrect. Expected=handle";
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L" GetLastError=";
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
                return result;
            }
            else
            {
                CloseHandle(createhandle);
            }
        }


        // Now do the delete
        auto testResult = DeleteFile(testInput.TestPathRemove.c_str());
        if (testResult == 0)
        {
            DWORD cErr = GetLastError();
            // call failed
            if (testInput.Expected_Result_FromRemove)
            {
                // should have succeeded
                std::wstring detail1 = L"ERROR: Delete returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result_FromRemove));
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
                return result;
            }
            else if (cErr == testInput.Expected_LastError_FromRemove)
            {
                // cool!
                result = ERROR_SUCCESS;
                //test_end(ERROR_SUCCESS);
            }
            else
            {
                // different reason, not cool
                std::wstring detail1 = L"ERROR: Although Delete returned value correct. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result_FromRemove));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L"      Expected GetLastError=";
                detail2.append(std::to_wstring(testInput.Expected_LastError_FromRemove));
                detail2.append(L"      Received GetLastError=");
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
                return result;
            }
        }
        else
        {
            // call succeeded
            if (testInput.Expected_Result_FromRemove)
            {
                // cool!
                result = ERROR_SUCCESS;
                //test_end(ERROR_SUCCESS);
            }
            else
            {
                // should have failed
                std::wstring detail1 = L"ERROR: Delete returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result_FromRemove));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                result = -1;
                test_end(-1);
                return result;
            }
        }

        // If still here, verify existance
        auto verifyResult = GetFileAttributes(testInput.TestPathRemove.c_str());
        auto eCode = GetLastError();
        if (verifyResult == testInput.Expected_Result_FromVerify)
        {
            if (verifyResult == INVALID_FILE_ATTRIBUTES)
            {
                if (eCode == testInput.Expected_LastError_FromVerify)
                {
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    trace_message(L"ERROR: Verifying attributes or error incorrect\n", error_color);
                    std::wstring detail1 = L"       Intended: Att=";
                    detail1.append(std::to_wstring(testInput.Expected_Result_FromVerify));
                    detail1.append(L" Error=");
                    detail1.append(std::to_wstring(testInput.Expected_LastError_FromVerify));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: Att=";
                    detail2.append(std::to_wstring(verifyResult));
                    detail2.append(L" Error=");
                    detail2.append(std::to_wstring(eCode));
                    detail2.append(L"\n");
                    trace_message(detail2.c_str(), error_color);

                    result = result ? result : eCode;
                    if (eCode != ERROR_SUCCESS)
                    {
                        test_end(GetLastError());
                    }
                    else
                    {
                        test_end(-1);
                    }
                }
            }
            else
            {
                // LastError not reset by native GetFileAttributes call when successful
                result = result ? result : ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
        }
        else
        {
            trace_message(L"ERROR: Verifying attributes or error incorrect\n", error_color);
            std::wstring detail1 = L"       Intended: Att=";
            detail1.append(std::to_wstring(testInput.Expected_Result_FromVerify));
            detail1.append(L" Error=");
            detail1.append(std::to_wstring(testInput.Expected_LastError_FromVerify));
            detail1.append(L"\n");
            trace_message(detail1.c_str(), info_color);
            std::wstring detail2 = L"       Actual: Att=";
            detail2.append(std::to_wstring(verifyResult));
            detail2.append(L" Error=");
            detail2.append(std::to_wstring(eCode));
            detail2.append(L"\n");
            trace_message(detail2.c_str(), error_color);

            result = result ? result : eCode;
            if (eCode != ERROR_SUCCESS)
            {
                test_end(GetLastError());
            }
            else
            {
                test_end(-1);
            }
        }

    }
    return result;
}

int RunRemoveDeleteTests()
{
    int result = ERROR_SUCCESS;
    BOOL testResult;

    for (MfrRemoveDirectoryTest testInput : MfrRemoveDirectoryTests)
    {
        if (testInput.enabled)
        {
            testResult = RemoveDirectoryIndividualTest(testInput);
            result = result ? result : testResult;
        }
    }


    for (MfrDeleteFileTest testInput : MfrDeleteFileTests)
    {
        if (testInput.enabled)
        {
            testResult = DeleteFileIndividualTest(testInput);
            result = result ? result : testResult;
        }
    }

    return result;
}
