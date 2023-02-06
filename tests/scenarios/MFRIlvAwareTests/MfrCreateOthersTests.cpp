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
    MfrCreateHardLinkTest ts_Native_PF1f = { "MFR+ILV Native new to Native-file VFS present in package", true, true, true, 
                                            (g_NativePF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1A.ini"),  
                                            (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                            true, ERROR_SUCCESS };
    MfrCreateHardLinkTests1.push_back(ts_Native_PF1f);


    // Requests to Native File Locations for GetPrivateProfileSection via existing VFS file with value present
    MfrCreateHardLinkTest ts_Native_PF1s = { "MFR+ILV Native new to Native-file VFS missing from package", true, true, true,
                                            (g_NativePF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1B1.ini"),
                                            (g_NativePF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1B2.ini"),
                                            false, ERROR_FILE_NOT_FOUND };
    MfrCreateHardLinkTests1.push_back(ts_Native_PF1s);

    tempNewFrom = VFSPF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1C.ini";
    tempExistingTo = VFSPF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini";
    MfrCreateHardLinkTest ts_VFS_PF1f = { "MFR+ILV VFS new to Package-file VFS present in package", true, true, true,
                                            tempNewFrom.c_str(),
                                            tempExistingTo.c_str(),
                                            true, ERROR_SUCCESS };
    MfrCreateHardLinkTests1.push_back(ts_VFS_PF1f);

    tempNewFrom = VFSPF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1D1.ini";
    tempExistingTo = VFSPF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1D2.ini";
    MfrCreateHardLinkTest ts_VFS_PF1s = { "MFR+ILV VFS new to Package-file VFS missing from package", true, true, true,
                                        tempNewFrom.c_str(),
                                        tempExistingTo.c_str(),
                                        false, ERROR_FILE_NOT_FOUND };
    MfrCreateHardLinkTests1.push_back(ts_VFS_PF1s);


    tempNewFrom = REVFSPF;
    tempNewFrom.append(L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1E.ini");
    tempExistingTo = REVFSPF;
    tempExistingTo.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrCreateHardLinkTest ts_REDIRECTED_PF1f = { "MFR+ILV Redirected new to Redirected-file VFS present in package", true, true, true,
                                            tempNewFrom.c_str(),
                                            tempExistingTo.c_str(),
                                            true, ERROR_SUCCESS };
    MfrCreateHardLinkTests1.push_back(ts_REDIRECTED_PF1f);

    tempNewFrom = REVFSPF;
    tempNewFrom.append(L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_HL1F1.ini");
    tempExistingTo = REVFSPF;
    tempExistingTo.append(L"\\PlaceholderTest\\TestIniFileVfsPF_HL1F1.ini");
    MfrCreateHardLinkTest ts_REDIRECTED_PF1s = { "MFR+ILV Redirected new to Redirected-file VFS missing from package", true, true, true,
                                        tempNewFrom.c_str(),
                                            tempExistingTo.c_str(),
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

    tempNewTo = VFSPF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_SL1A.ini";
    tempExistingFrom = VFSPF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini";
    MfrCreateSymbolicLinkTest ts_VFS_PF1f = { "MFR+ILV VFS new to Package-file VFS present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests1.push_back(ts_VFS_PF1f);

    tempNewTo = VFSPF + L"\\PlaceholderTest\\NewLinkTestIniFileVfsPF_SL1B1.ini";
    tempExistingFrom = VFSPF + L"\\PlaceholderTest\\TestIniFileVfsPF_FL1B2.ini";
    MfrCreateSymbolicLinkTest ts_VFS_PF1s = { "MFR+ILV VFS new to Package-file VFS missing from package", true, true, true,
                                        tempNewTo.c_str(),
                                        tempExistingFrom.c_str(),
                                        0,
                                        true, ERROR_SUCCESS };  // target need not be present!
    MfrCreateSymbolicLinkTests1.push_back(ts_VFS_PF1s);


    tempNewTo = L"NewLinkTestPvadFile1.ini";
    tempExistingFrom = L"PvadFile1.ini";
    MfrCreateSymbolicLinkTest ts_CWD_PF1f = { "MFR+ILV Package-file PVAD relative present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests1.push_back(ts_CWD_PF1f);

    tempNewTo = L"NewLinkTestPvadFile2.ini";
    tempExistingFrom = L"NonExistentPvadFile1.ini";
    MfrCreateSymbolicLinkTest ts_CWD_PF1s = { "MFR+ILV Package-file PVAD relative missing from package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
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

    tempNewTo = VFSPF + L"\\NewPlaceholderTest_SL2A";
    tempExistingFrom = VFSPF + L"\\PlaceholderTest";
    MfrCreateSymbolicLinkTest ts_VFS_PF1f = { "MFR+ILV VFS new to Package-folder VFS present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests2.push_back(ts_VFS_PF1f);

    tempNewTo = VFSPF + L"\\NewPlaceholderTest_SL2B1";
    tempExistingFrom = VFSPF + L"\\MissingPlaceholderTest_SL2B2";
    MfrCreateSymbolicLinkTest ts_VFS_PF1s = { "MFR+ILV VFS new to Package-folder VFS missing from package", true, true, true,
                                        tempNewTo.c_str(),
                                        tempExistingFrom.c_str(),
                                        0,
                                        true, ERROR_SUCCESS };  // target need not be present!
    MfrCreateSymbolicLinkTests2.push_back(ts_VFS_PF1s);


    tempNewTo = L"NewPvadFolder_HL1";
    tempExistingFrom = L"PvadFolder";
    MfrCreateSymbolicLinkTest ts_CWD_PF1f = { "MFR+ILV Package-folder PVAD relative present in package", true, true, true,
                                            tempNewTo.c_str(),
                                            tempExistingFrom.c_str(),
                                            0,
                                            true, ERROR_SUCCESS };
    MfrCreateSymbolicLinkTests2.push_back(ts_CWD_PF1f);

    tempNewTo = L"NewPvadFolder_HL2";
    std::wstring secondmissing = L"SecondMissingPavdFolder";
    MfrCreateSymbolicLinkTest ts_CWD_PF1s = { "MFR+ILV Package-folder PVAD relative missing from package", true, true, true,
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
            std::string testname = "MFR+ILV CreateHardLink Test(1): ";
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
            std::string testname = "MFR+ILV CreateSymbolicLink Test(1): ";
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
            std::string testname = "MFR+ILV CreateSymbolicLink Test(2): ";
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