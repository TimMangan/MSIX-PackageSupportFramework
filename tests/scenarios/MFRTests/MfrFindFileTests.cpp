// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrFindFileTests.h"
#include "MfrCleanup.h"
#include <stdio.h>

std::vector<MfrFindFileTest> MfrFindFileTests1;
std::vector<MfrFindFileTest> MfrFindFileTests2;
std::vector<MfrFindFileTest> MfrFindFileTests3;
std::vector<MfrFindFileTest> MfrFindFileTests4;
std::vector<MfrFindFileTest> MfrFindFileTests5;
std::vector<MfrFindFileTest> MfrFindFileTests6;

std::vector<MfrFindFileExTest> MfrFindFileExTests1;


int InitializeFindFileTests1()
{
    std::wstring temp;

    std::wstring WriteRoot = g_writablePackageRootPath.c_str();

#if _M_IX86
    std::wstring VfsPf = g_Cwd + L"\\VFS\\ProgramFilesX86";
    std::wstring WriteRootPF = WriteRoot + L"\\VFS\\ProgramFilesX86";
#else
    std::wstring VfsPf = g_Cwd + L"\\VFS\\ProgramFilesX64";
    std::wstring WriteRootPF = WriteRoot + L"\\VFS\\ProgramFilesX64";
#endif

    // Request to Native File/Folder Locations for FindFile for single file
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrFindFileTest t_Native_PFF1 = { "MFR Native-file VFS single file exists in package", true, true, true,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Native_PFF1);

    temp = g_NativePF + L"\\PlaceholderTest";
    MfrFindFileTest t_Native_PFF2 = { "MFR Native-file VFS folder exists in package that has files", true, false, false,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Native_PFF2);

    temp = g_NativePF + L"\\PlaceholderTest\\MissingPlaceholder.txt";
    MfrFindFileTest t_Native_PFF1E = { "MFR Native-file VFS single file missing in package (expect fail)", true, false, false,
                                       temp.c_str(),
                                       false, 0,  ERROR_FILE_NOT_FOUND };
    MfrFindFileTests1.push_back(t_Native_PFF1E);

    temp = g_NativePF + L"\\MissingPlaceholderTest";
    MfrFindFileTest t_Native_PFF2E = { "MFR Native-file VFS folder missing in package (expect fail)", true, false, false,
                                       temp.c_str(),
                                       false, 0,  ERROR_PATH_NOT_FOUND };
    MfrFindFileTests1.push_back(t_Native_PFF2E);


    // Request to Native File/Folder Locations for FindFile for multiple files
    temp = g_NativePF + L"\\PlaceholderTest2\\Test*";
    MfrFindFileTest t_Native_PFF3 = { "MFR Native-file VFS multiple file exists in package", true, false, false,
                                       temp.c_str(),
                                       true, 2,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Native_PFF3);

    temp = g_NativePF + L"\\PlaceholderTest2\\*";
    MfrFindFileTest t_Native_PFF4 = { "MFR Native-file VFS folder with wildcard exists in package with files", true, false, false,
                                       temp.c_str(),
                                       true, 5,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Native_PFF4);



    // Request to Package File/Folder Locations for FindFile for single file
    temp = VfsPf + L"\\PlaceholderTest\\Placeholder.txt";
    MfrFindFileTest t_Package_PFF1 = { "MFR Package-file VFS single file exists in package", true, true, true,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Package_PFF1);


    temp = VfsPf + L"\\PlaceholderTest";
    MfrFindFileTest t_Package_PFF2 = { "MFR Package-file VFS folder exists in package that has files", true, false, false,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Package_PFF2);

    temp = VfsPf + L"\\PlaceholderTest\\MissingPlaceholder.txt";
    MfrFindFileTest t_Package_PFF1E = { "MFR Package-file VFS single file missing in package (expect fail)", true, false, false,
                                       temp.c_str(),
                                       false, 0,  ERROR_FILE_NOT_FOUND };
    MfrFindFileTests1.push_back(t_Package_PFF1E);

    temp = VfsPf + L"\\MissingPlaceholderTest";
    MfrFindFileTest t_Package_PFF2E = { "MFR Package-file VFS folder missing in package (expect fail)", true, false, false,
                                       temp.c_str(),
                                       false, 0,  ERROR_PATH_NOT_FOUND };
    MfrFindFileTests1.push_back(t_Package_PFF2E);


    // Request to Native File/Folder Locations for FindFile for multiple files
    temp = VfsPf + L"\\PlaceholderTest2\\Test*";
    MfrFindFileTest t_Package_PFF3 = { "MFR Package-file VFS multiple file exists in package", true, false, false,
                                       temp.c_str(),
                                       true, 2,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Package_PFF3);

    temp = VfsPf + L"\\PlaceholderTest2\\*";
    MfrFindFileTest t_Package_PFF4 = { "MFR Package-file VFS folder with wildcard exists in package with files", true, false, false,
                                       temp.c_str(),
                                       true, 5,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Package_PFF4);



    // Request to Redirected File/Folder Locations for FindFile for single file
    temp = WriteRootPF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrFindFileTest t_Redirected_PFF1 = { "MFR Redirected-file VFS single file exists in package", true, true, true,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Redirected_PFF1);


    temp = WriteRootPF + L"\\PlaceholderTest";
    MfrFindFileTest t_Redirected_PFF2 = { "MFR Redirected-file VFS folder exists in package that has files", true, false, false,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Redirected_PFF2);

    temp = WriteRootPF + L"\\PlaceholderTest\\MissingPlaceholder.txt";
    MfrFindFileTest t_Redirected_PFF1E = { "MFR Redirected-file VFS single file missing in package (expect fail)", true, false, false,
                                       temp.c_str(),
                                       false, 0,  ERROR_FILE_NOT_FOUND };
    MfrFindFileTests1.push_back(t_Redirected_PFF1E);

    temp = WriteRootPF + L"\\MissingPlaceholderTest";
    MfrFindFileTest t_Redirected_PFF2E = { "MFR Redirected-file VFS folder missing in package (expect fail)", true, false, false,
                                       temp.c_str(),
                                       false, 0,  ERROR_PATH_NOT_FOUND };
    MfrFindFileTests1.push_back(t_Redirected_PFF2E);


    // Request to Redirected File/Folder Locations for FindFile for multiple files
    temp = WriteRootPF + L"\\PlaceholderTest2\\Test*";
    MfrFindFileTest t_Redirected_PFF3 = { "MFR Redirected-file VFS multiple file exists in package", true, false, false,
                                       temp.c_str(),
                                       true, 2,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Redirected_PFF3);

    temp = WriteRootPF + L"\\PlaceholderTest2\\*";
    MfrFindFileTest t_Redirected_PFF4 = { "MFR Redirected-file VFS folder with wildcard exists in package with files", true, false, false,
                                       temp.c_str(),
                                       true, 5,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Redirected_PFF4);


    temp =  L"C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\*.xsd";
    MfrFindFileTest t_Redirected_NET1 = { "MFR Native-folder present but not in package", true, false, false,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Redirected_NET1);



    temp = L"C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\\\*.xsd";
    MfrFindFileTest t_Redirected_NET2 = { "MFR Native-folder present but not in package (with extra backslash)", true, false, false,
                                       temp.c_str(),
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileTests1.push_back(t_Redirected_NET2);

    int count = 0;
    for (MfrFindFileTest t : MfrFindFileTests1)      if (t.enabled) { count++; }
    return count;
} // InitializeFindFileTests1


int InitializeFindFileExTests1()
{
    std::wstring temp;

    // Programing note: The AdditionalFlags FIND_FIRST_EX_CASE_SENSITIVE does not work on most systems, as the underlying file system uses a registry value
    //                  to disable case sensitivity in the file system and pretty much every running system has it set to 0 (disable) or nothing works.
    //                  So setting this value in the call should have no effect.

    std::wstring WriteRoot = g_writablePackageRootPath.c_str();

#if _M_IX86
    std::wstring VfsPf = g_Cwd + L"\\VFS\\ProgramFilesX86";
    std::wstring WriteRootPF = WriteRoot + L"\\VFS\\ProgramFilesX86";
#else
    std::wstring VfsPf = g_Cwd + L"\\VFS\\ProgramFilesX64";
    std::wstring WriteRootPF = WriteRoot + L"\\VFS\\ProgramFilesX64";
#endif


    // Request to Native File/Folder Locations for FindFile for single file
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrFindFileExTest t_Native_NF1 = { "MFR Ex: Native-file VFS single file exists in package; basic namematch 0", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoBasic, FINDEX_SEARCH_OPS::FindExSearchNameMatch, 0,
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileExTests1.push_back(t_Native_NF1);

    temp = g_NativePF + L"\\PlaceholderTest\\PLACEHOLDER.txt";
    MfrFindFileExTest t_Native_NF2 = { "MFR Ex: Native-file VFS single file exists in package; basic namematch casesensitive (normally succeeds anyway)", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoBasic, FINDEX_SEARCH_OPS::FindExSearchNameMatch, FIND_FIRST_EX_CASE_SENSITIVE,
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileExTests1.push_back(t_Native_NF2);

    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrFindFileExTest t_Native_NF3 = { "MFR Ex: Native-file VFS single file exists in package; standard namematch casesensitive", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoStandard, FINDEX_SEARCH_OPS::FindExSearchNameMatch, FIND_FIRST_EX_CASE_SENSITIVE,
                                       true, 1,  ERROR_SUCCESS };
    MfrFindFileExTests1.push_back(t_Native_NF3);


    temp = g_NativePF + L"\\PlaceholderTest3\\*";
    MfrFindFileExTest t_Native_NF4 = { "MFR Ex: Native-folder VFS folder exists in package with files standard directorymatch", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoStandard, FINDEX_SEARCH_OPS::FindExSearchLimitToDirectories, 0,
                                       true, 3,  ERROR_SUCCESS };
    MfrFindFileExTests1.push_back(t_Native_NF4);

    temp = g_NativePF + L"\\MissingFolderPlaceholderTest\\*";
    MfrFindFileExTest t_Native_NF5e = { "MFR Ex: Native-folder VFS folder missing from package with files standard directorymatch", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoStandard, FINDEX_SEARCH_OPS::FindExSearchLimitToDirectories, 0,
                                       false, 0,  ERROR_PATH_NOT_FOUND };
    MfrFindFileExTests1.push_back(t_Native_NF5e);


    // Request to Package locations for FindFile for missing file/folder
    temp = VfsPf + L"\\PlaceholderTest\\MissingPlaceholder.txt";
    MfrFindFileExTest t_Package_PFF1E = { "MFR Ex: Package-file VFS single file missing in package standard namematch (expect fail)", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoStandard, FINDEX_SEARCH_OPS::FindExSearchNameMatch, 0,
                                       false, 0,  ERROR_FILE_NOT_FOUND };
    MfrFindFileExTests1.push_back(t_Package_PFF1E);

    temp = VfsPf + L"\\MissingPlaceholderTest";
    MfrFindFileExTest t_Package_PFF2E = { "MFR Ex: Package-file VFS folder missing in package standard namematch (expect fail)", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoStandard, FINDEX_SEARCH_OPS::FindExSearchNameMatch, 0,
                                       false, 0,  ERROR_PATH_NOT_FOUND };
    MfrFindFileExTests1.push_back(t_Package_PFF2E);



    // Request to Native File/Folder Locations for FindFile for multiple files
    temp = WriteRootPF + L"\\PlaceholderTest2\\Test*";
    MfrFindFileExTest t_Redirected_RF1 = { "MFR Ex: Redirected-file VFS multiple file exists in package standard namematch", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoStandard, FINDEX_SEARCH_OPS::FindExSearchNameMatch, 0,
                                       true, 2,  ERROR_SUCCESS };
    MfrFindFileExTests1.push_back(t_Redirected_RF1);

    temp = WriteRootPF + L"\\PlaceholderTest2\\*";
    MfrFindFileExTest t_Redirected_RF2 = { "MFR Ex: Redirected-file VFS folder with wildcard exists in package with files standard namematch", true, false, false,
                                       temp.c_str(), FINDEX_INFO_LEVELS::FindExInfoStandard, FINDEX_SEARCH_OPS::FindExSearchNameMatch, 0,
                                       true, 5,  ERROR_SUCCESS };
    MfrFindFileExTests1.push_back(t_Redirected_RF2);


    int count = 0;
    for (MfrFindFileExTest t : MfrFindFileExTests1)      if (t.enabled) { count++; }
    return count;
} // InitializeFindFileExTests1()

int InitializeFindFileTests()
{
    int count = 0;
    count += InitializeFindFileTests1();
    count += InitializeFindFileExTests1();

    return count;
}


int RunFindFileIndividualTest(MfrFindFileTest testInput, int setnum)
{
    int result = ERROR_SUCCESS;

    if (testInput.enabled)
    {
        std::string testname = "MFR FindFirst/Next File Test set";
        testname.append(std::to_string(setnum));
        testname.append(": ");
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

        WIN32_FIND_DATA FindFileData;

        HANDLE FindHandle = ::FindFirstFile(testInput.TestPath.c_str(), &FindFileData);
        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            auto eCode = GetLastError();
            if (testInput.Expected_Result)
            {
                // unexpected failure
                trace_message(L"ERROR: FindFirst failed unexpectedly.\n", error_color);
                std::wstring detail1 = L"     Expected Error=";
                detail1.append(std::to_wstring(testInput.Expected_LastError));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), info_color);
                std::wstring detail2 = L"       Actual Error=";
                detail2.append(std::to_wstring(eCode));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                if (eCode != 0)
                {
                    result = result ? result : eCode;
                    test_end(eCode);
                }
                else
                {
                    result = result ? result : ERROR_ASSERTION_FAILURE;
                    test_end(ERROR_ASSERTION_FAILURE);
                }
            }
            else
            {
                // Expected failure
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
        }
        else
        {
            if (testInput.Expected_Result)
            {
                // expected success (so far)
                DWORD count = 1;
                std::wstring detail1 = L"     First=";
                detail1.append(FindFileData.cFileName);
                detail1.append(L"\n");
                trace_message(detail1.c_str(), info_color);
                // TODO: FindNext until done, check count
                bool stillSearching = true;
                while (stillSearching)
                {
                    BOOL next = FindNextFile(FindHandle, &FindFileData);
                    if (next != 0)
                    {
                        std::wstring detail2 = L"      Next=";
                        detail2.append(FindFileData.cFileName);
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), info_color);
                        count++;
                    }
                    else
                    {
                        stillSearching = false;
                        DWORD fnErr = GetLastError();
                        if (fnErr != ERROR_NO_MORE_FILES)
                        {
                            std::wstring detail2e = L"   Error on next=";
                            detail2e.append(std::to_wstring(fnErr));
                            detail2e.append(L"\n");
                            trace_message(detail2e.c_str(), error_color);
                            FindClose(FindHandle);
                            result = fnErr;
                            test_end(fnErr);
                            return result;
                        }
                        // fall through when no more found
                    }
                }
                if (testInput.Expected_FilesFound == count)
                {
                    result = ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    std::wstring detail3 = L"  Incorrect count.  Expected=";
                    detail3.append(std::to_wstring(testInput.Expected_FilesFound));
                    detail3.append(L" Actual=");
                    detail3.append(std::to_wstring(count));
                    detail3.append(L"\n");
                    trace_message(detail3.c_str(), error_color);
                    result = result ? result : ERROR_BAD_LENGTH;
                    test_end(ERROR_BAD_LENGTH);
                }

            }
            else
            {
                // unexpected success
                trace_message(L"ERROR: FindFirstFile succeeded unexpectedly.\n", error_color);
                result = result ? result : ERROR_ASSERTION_FAILURE;
                test_end(ERROR_ASSERTION_FAILURE);
            }

            FindClose(FindHandle);
        }
    }
    return result;
}


int RunFindFileExIndividualTest(MfrFindFileExTest testInput, int setnum)
{
    int result = ERROR_SUCCESS;

    if (testInput.enabled)
    {
        std::string testname = "MFR FindFirstEx/Next File Test set";
        testname.append(std::to_string(setnum));
        testname.append(": ");
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

        WIN32_FIND_DATA FindFileData;

        HANDLE FindHandle = ::FindFirstFileEx(testInput.TestPath.c_str(), testInput.info_levels, &FindFileData, testInput.search_ops, nullptr, testInput.additional_flags);
        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            auto eCode = GetLastError();
            if (testInput.Expected_Result)
            {
                // unexpected failure
                trace_message(L"ERROR: FindFirstEx failed unexpectedly.\n", error_color);
                std::wstring detail1 = L"     Expected Error=";
                detail1.append(std::to_wstring(testInput.Expected_LastError));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), info_color);
                std::wstring detail2 = L"       Actual Error=";
                detail2.append(std::to_wstring(eCode));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                if (eCode != 0)
                {
                    result = result ? result : eCode;
                    test_end(eCode);
                }
                else
                {
                    result = result ? result : ERROR_ASSERTION_FAILURE;
                    test_end(ERROR_ASSERTION_FAILURE);
                }
            }
            else
            {
                // Expected failure
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
        }
        else
        {
            if (testInput.Expected_Result)
            {
                // expected success (so far)
                DWORD count = 1;
                std::wstring detail1 = L"     First=";
                detail1.append(FindFileData.cFileName);
                detail1.append(L"\n");
                trace_message(detail1.c_str(), info_color);
                // TODO: FindNext until done, check count
                bool stillSearching = true;
                while (stillSearching)
                {
                    BOOL next = FindNextFile(FindHandle, &FindFileData);
                    if (next != 0)
                    {
                        std::wstring detail2 = L"      Next=";
                        detail2.append(FindFileData.cFileName);
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), info_color);
                        count++;
                    }
                    else
                    {
                        stillSearching = false;
                        DWORD fnErr = GetLastError();
                        if (fnErr != ERROR_NO_MORE_FILES)
                        {
                            std::wstring detail2e = L"   Error on next=";
                            detail2e.append(std::to_wstring(fnErr));
                            detail2e.append(L"\n");
                            trace_message(detail2e.c_str(), error_color);
                            FindClose(FindHandle);
                            result = fnErr;
                            test_end(fnErr);
                            return result;
                        }
                        // fall through when no more found
                    }
                }
                if (testInput.Expected_FilesFound == count)
                {
                    result = ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    std::wstring detail3 = L"  Incorrect count.  Expected=";
                    detail3.append(std::to_wstring(testInput.Expected_FilesFound));
                    detail3.append(L" Actual=");
                    detail3.append(std::to_wstring(count));
                    detail3.append(L"\n");
                    trace_message(detail3.c_str(), error_color);
                    result = result ? result : ERROR_BAD_LENGTH;
                    test_end(ERROR_BAD_LENGTH);
                }

            }
            else
            {
                // unexpected success
                [[maybe_unused]] DWORD what = GetLastError();
                trace_message(L"ERROR: FindFirstFileEx succeeded unexpectedly.\n", error_color);
                result = result ? result : ERROR_ASSERTION_FAILURE;
                test_end(ERROR_ASSERTION_FAILURE);
            }

            FindClose(FindHandle);
        }
    }
    return result;
}


int RunFindFileTests()
{
    int result = ERROR_SUCCESS;
    BOOL testResult;

    for (MfrFindFileTest testInput : MfrFindFileTests1)
    {
        if (testInput.enabled)
        {
            testResult = RunFindFileIndividualTest(testInput, 1);
            result = result ? result : testResult;
        }
    }


    for (MfrFindFileExTest testInput : MfrFindFileExTests1)
    {
        if (testInput.enabled)
        {
            testResult = RunFindFileExIndividualTest(testInput, 1);
            result = result ? result : testResult;
        }
    }

    return result;
}