//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrCreateOthersTests.h"
#include "MfrCleanup.h"
#include <stdio.h>

std::vector<MfrCreateHardLinkTest> MfrCreateHardLinkTests1;

std::vector<MfrCreateSymbolicLinkTest> MfrCreateSymbolicLinkTests1;

std::vector<MfrCreateSymbolicLinkTest> MfrCreateSymbolicLinkTests2;

int InitializeCreateHardLinkTest1()
{
    std::wstring tempNewFrom;
    std::wstring tempExistingTo;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    // Requests to Native File Locations for GetPrivateProfileSection via existing VFS file with value present
    MfrCreateHardLinkTest ts_Native_PF1f = { "MFR Native-file VFS present in package", true, true, true, 
                                            (g_NativePF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF.ini"),  
                                            (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                            true, ERROR_SUCCESS };
    MfrCreateHardLinkTests1.push_back(ts_Native_PF1f);


    // Requests to Native File Locations for GetPrivateProfileSection via existing VFS file with value present
    MfrCreateHardLinkTest ts_Native_PF1s = { "MFR Native-file VFS missing from package", true, true, true,
                                            (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                            (g_NativePF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF.ini"),
                                            false, ERROR_FILE_NOT_FOUND };
    MfrCreateHardLinkTests1.push_back(ts_Native_PF1s);

    tempNewFrom = VFSPF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF.ini";
    tempExistingTo = VFSPF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini";
    MfrCreateHardLinkTest ts_VFS_PF1f = { "MFR Package-file VFS present in package", true, true, true,
                                            tempNewFrom.c_str(),
                                            tempExistingTo.c_str(),
                                            true, ERROR_SUCCESS };
    MfrCreateHardLinkTests1.push_back(ts_VFS_PF1f);

    MfrCreateHardLinkTest ts_VFS_PF1s = { "MFR Package-file VFS missing from package", true, true, true,
                                        tempExistingTo.c_str(),
                                        tempNewFrom.c_str(),
                                        false, ERROR_FILE_NOT_FOUND };
    MfrCreateHardLinkTests1.push_back(ts_VFS_PF1s);


    tempNewFrom = REVFSPF;
    tempNewFrom.append(L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF.ini");
    tempExistingTo = REVFSPF;
    tempExistingTo.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrCreateHardLinkTest ts_REDIRECTED_PF1f = { "MFR Redirected-file VFS present in package", true, true, true,
                                            tempNewFrom.c_str(),
                                            tempExistingTo.c_str(),
                                            true, ERROR_SUCCESS };
    MfrCreateHardLinkTests1.push_back(ts_REDIRECTED_PF1f);

    MfrCreateHardLinkTest ts_REDIRECTED_PF1s = { "MFR Redirected-file VFS missing from package", true, true, true,
                                        tempExistingTo.c_str(),
                                        tempNewFrom.c_str(),
                                        false, ERROR_FILE_NOT_FOUND };
    MfrCreateHardLinkTests1.push_back(ts_REDIRECTED_PF1s);

    int count = 0;
    for (MfrCreateHardLinkTest t : MfrCreateHardLinkTests1)      if (t.enabled) { count++; }
    return count;
}


int InitializeCreateSymbolicLinkTest1()
{
    std::wstring tempNewTo;
    std::wstring tempExistingFrom;


    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    tempNewTo = VFSPF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF.ini";
    tempExistingFrom = VFSPF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini";
    MfrCreateSymbolicLinkTest ts_VFS_PF1f = { "MFR Package-file VFS present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests1.push_back(ts_VFS_PF1f);

    MfrCreateSymbolicLinkTest ts_VFS_PF1s = { "MFR Package-file VFS missing from package", true, true, true,
                                        tempExistingFrom.c_str(),
                                        tempNewTo.c_str(),
                                        0,
                                        true, ERROR_SUCCESS };  // target need not be present!
    MfrCreateSymbolicLinkTests1.push_back(ts_VFS_PF1s);


    tempNewTo = L"NewLinkTestPvadFile.ini"; 
    tempExistingFrom = L"PvadFile1.ini";
    MfrCreateSymbolicLinkTest ts_CWD_PF1f = { "MFR Package-file PVAD relative present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests1.push_back(ts_CWD_PF1f);

    MfrCreateSymbolicLinkTest ts_CWD_PF1s = { "MFR Package-file PVAD relative missing from package", true, true, true,
                                        tempExistingFrom.c_str(),
                                        tempNewTo.c_str(),
                                        0,
                                        true, ERROR_SUCCESS };  // target need not be present
    MfrCreateSymbolicLinkTests1.push_back(ts_CWD_PF1s);

    int count = 0;
    for (MfrCreateSymbolicLinkTest t : MfrCreateSymbolicLinkTests1)      if (t.enabled) { count++; }
    return count;
}


int InitializeCreateSymbolicLinkTest2()
{
    std::wstring tempNewTo;
    std::wstring tempExistingFrom;


    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    tempNewTo = VFSPF + L"\\NewPlaceholderTest";
    tempExistingFrom = VFSPF + L"\\PlaceholderTest";
    std::wstring secondmissing = VFSPF + L"\\MissingPlaceholderTest";
    MfrCreateSymbolicLinkTest ts_VFS_PF1f = { "MFR Package-folder VFS present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests2.push_back(ts_VFS_PF1f);

    MfrCreateSymbolicLinkTest ts_VFS_PF1s = { "MFR Package-folder VFS missing from package", true, true, true,
                                        tempNewTo.c_str(),
                                        secondmissing.c_str(),
                                        0,
                                        true, ERROR_SUCCESS };  // target need not be present!
    MfrCreateSymbolicLinkTests2.push_back(ts_VFS_PF1s);


    tempNewTo = L"NewPvadFolder";
    tempExistingFrom = L"PvadFolder";
    secondmissing = L"SecondMissingPavdFolder";
    MfrCreateSymbolicLinkTest ts_CWD_PF1f = { "MFR Package-folder PVAD relative present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests2.push_back(ts_CWD_PF1f);

    MfrCreateSymbolicLinkTest ts_CWD_PF1s = { "MFR Package-folder PVAD relative missing from package", true, true, true,
                                        tempNewTo.c_str(),
                                        secondmissing.c_str(),
                                        0,
                                        true, ERROR_SUCCESS };  // target need not be present
    MfrCreateSymbolicLinkTests2.push_back(ts_CWD_PF1s);

    int count = 0;
    for (MfrCreateSymbolicLinkTest t : MfrCreateSymbolicLinkTests2)      if (t.enabled) { count++; }
    return count;
}

int InitializeCreateOthersTests()
{
    int count = 0;
    count += InitializeCreateHardLinkTest1();

    count += InitializeCreateSymbolicLinkTest1();

    count += InitializeCreateSymbolicLinkTest2();

    return count;
}



int RunCreateOthersTests()
{
    int result = ERROR_SUCCESS;

    for (MfrCreateHardLinkTest testInput : MfrCreateHardLinkTests1)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR CreateHardLink Test(1): ";
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

            std::wstring msg;
            msg = L"      LinkNewFrom: ";
            msg.append(testInput.TestPathNewHardLinkFrom.c_str());
            msg.append(L"\n");
            trace_message(msg.c_str(), info_color);
            msg = L"   LinkExistingTo: ";
            msg.append(testInput.TestPathExistingTo.c_str());
            msg.append(L"\n");
            trace_message(msg.c_str(), info_color);

            BOOL Bret = CreateHardLink(testInput.TestPathNewHardLinkFrom.c_str(), testInput.TestPathExistingTo.c_str(),  nullptr);
            if (Bret == 0)
            {
                DWORD eCode = GetLastError();
                // call failed
                if (testInput.Expected_Result)
                {
                    // failed but should have succeeded
                    trace_message(L"ERROR: Expected success but failed\n", error_color);
                    std::wstring detail = L"       Intended: Error=";
                    detail.append(std::to_wstring(testInput.Expected_LastError));
                    detail.append(L" Rx Error=");
                    detail.append(std::to_wstring(eCode));
                    detail.append(L"\n");
                    trace_message(detail.c_str(), info_color);
                    result = eCode;
                    test_end(eCode);
                }
                else
                {
                    // expected failure, check cause
                    if (eCode == testInput.Expected_LastError)
                    {
                        result = ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Expected the fail but error incorrect\n", error_color);
                        std::wstring detail = L"       Intended: Error=";
                        detail.append(std::to_wstring(testInput.Expected_LastError));
                        detail.append(L" Rx Error=");
                        detail.append(std::to_wstring(eCode));
                        detail.append(L"\n");
                        trace_message(detail.c_str(), info_color);
                        result = eCode;
                        test_end(eCode);
                    }
                }
            }
            else
            {
                // call succeeded
                if (testInput.Expected_Result)
                {
                    // expected success
                    result = ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    // unexpected success
                    trace_message(L"ERROR: Unexpected Success\n", error_color);
                    result = -1;
                    test_end(-1);
                }
            }
        }
    }


    for (MfrCreateSymbolicLinkTest testInput : MfrCreateSymbolicLinkTests1)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR CreateSymbolicLink Test(1): ";
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

            std::wstring msg;
            msg = L"   LinkFrom: ";
            msg.append(testInput.TestPathNewSymbolicLinkFrom.c_str());
            msg.append(L"\n");
            trace_message(msg.c_str(), info_color);
            msg = L"     LinkTo: ";
            msg.append(testInput.TestPathExistingTo.c_str());
            msg.append(L"\n");
            trace_message(msg.c_str(), info_color);
            if (testInput.flag == 0)
            {
                trace_message(L"   Is link to file.\n", info_color);
            }
            else
            {
                trace_message(L"   Is link to directory.\n", info_color);
            }

            BOOLEAN Bret = CreateSymbolicLink(testInput.TestPathNewSymbolicLinkFrom.c_str(), testInput.TestPathExistingTo.c_str(), testInput.flag);
            if (Bret == 0)
            {
                DWORD eCode = GetLastError();
                // call failed
                if (testInput.Expected_Result)
                {
                    // failed but should have succeeded
                    trace_message(L"ERROR: Expected success but failed\n", error_color);
                    std::wstring detail = L"       Intended: Error=";
                    detail.append(std::to_wstring(testInput.Expected_LastError));
                    detail.append(L" Rx Error=");
                    detail.append(std::to_wstring(eCode));
                    detail.append(L"\n");
                    trace_message(detail.c_str(), info_color);
                    result = eCode;
                    test_end(eCode);
                }
                else
                {
                    // expected failure, check cause
                    if (eCode == testInput.Expected_LastError)
                    {
                        result = ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Expected the fail but error incorrect\n", error_color);
                        std::wstring detail = L"       Intended: Error=";
                        detail.append(std::to_wstring(testInput.Expected_LastError));
                        detail.append(L" Rx Error=");
                        detail.append(std::to_wstring(eCode));
                        detail.append(L"\n");
                        trace_message(detail.c_str(), info_color);
                        result = eCode;
                        test_end(eCode);
                    }
                }
            }
            else
            {
                // call succeeded
                if (testInput.Expected_Result)
                {
                    // expected success
                    result = ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    // unexpected success
                    trace_message(L"ERROR: Unexpected Success\n", error_color);
                    result = -1;
                    test_end(-1);
                }
            }

        }
    }

    for (MfrCreateSymbolicLinkTest testInput : MfrCreateSymbolicLinkTests2)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR CreateSymbolicLink Test(2): ";
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

            std::wstring msg;
            msg = L"   LinkFrom: ";
            msg.append(testInput.TestPathNewSymbolicLinkFrom.c_str());
            msg.append(L"\n");
            trace_message(msg.c_str(), info_color);
            msg = L"     LinkTo: ";
            msg.append(testInput.TestPathExistingTo.c_str());
            msg.append(L"\n");
            trace_message(msg.c_str(), info_color);
            if (testInput.flag == 0)
            {
                trace_message(L"   Is link to file.\n", info_color);
            }
            else
            {
                trace_message(L"   Is link to directory.\n", info_color);
            }

            BOOLEAN Bret = CreateSymbolicLink(testInput.TestPathNewSymbolicLinkFrom.c_str(), testInput.TestPathExistingTo.c_str(),  testInput.flag);
            if (Bret == 0)
            {
                DWORD eCode = GetLastError();
                // call failed
                if (testInput.Expected_Result)
                {
                    // failed but should have succeeded
                    trace_message(L"ERROR: Expected success but failed\n", error_color);
                    std::wstring detail = L"       Intended: Error=";
                    detail.append(std::to_wstring(testInput.Expected_LastError));
                    detail.append(L" Rx Error=");
                    detail.append(std::to_wstring(eCode));
                    detail.append(L"\n");
                    trace_message(detail.c_str(), info_color);
                    result = eCode;
                    test_end(eCode);
                }
                else
                {
                    // expected failure, check cause
                    if (eCode == testInput.Expected_LastError)
                    {
                        result = ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Expected the fail but error incorrect\n", error_color);
                        std::wstring detail = L"       Intended: Error=";
                        detail.append(std::to_wstring(testInput.Expected_LastError));
                        detail.append(L" Rx Error=");
                        detail.append(std::to_wstring(eCode));
                        detail.append(L"\n");
                        trace_message(detail.c_str(), info_color);
                        result = eCode;
                        test_end(eCode);
                    }
                }
            }
            else
            {
                // call succeeded
                if (testInput.Expected_Result)
                {
                    // expected success
                    result = ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    // unexpected success
                    trace_message(L"ERROR: Unexpected Success\n", error_color);
                    result = -1;
                    test_end(-1);
                }
            }

        }
    }



    return result;
}