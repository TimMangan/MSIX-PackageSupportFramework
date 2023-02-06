//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrWriteProfileTests.h"
#include "MfrCleanup.h"
#include <stdio.h>

std::vector< MfrWriteProfileSectionTest> MfrWriteProfileSectionTests;
std::vector< MfrWriteProfileStringTest> MfrWriteProfileStringTests;
std::vector<MfrWriteProfileStructTest> MfrWriteProfileStructTests;

wchar_t* makesectionbuffer(std::wstring test)
{
    wchar_t* buffer = (wchar_t*)malloc((92*sizeof(wchar_t)));  // yeah, we loose this memory but it's only a test.
    if (buffer != NULL)
    {
        memcpy_s(buffer, (DWORD)(92*sizeof(wchar_t)), test.c_str(), (DWORD)(test.length()*sizeof(wchar_t)));
        buffer[(DWORD)test.length()] = 0;
        buffer[(DWORD)test.length() + 1] = 0;
    }
    return buffer;
}

int InitializeWriteProfileTestsSection()
{
    std::wstring temp;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif


    // Requests to Native File Locations for WritePrivateProfileSection via existing VFS file with value present
    std::wstring test1 = L"UnusedItem=Nothing1";
    wchar_t* buffer1 = makesectionbuffer(test1);
    if (buffer1 == NULL)
        return 0;
    MfrWriteProfileSectionTest ts_Native_PF1 = { "MFR+ILV Section Native-file VFS exists in package", true, true, true, 
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"), L"Section4", (DWORD)test1.length()+1, buffer1};
    MfrWriteProfileSectionTests.push_back(ts_Native_PF1);


    // Requests to Native File Locations for WritePrivateProfileSection via Missing VFS file with value present
    std::wstring test2 = L"UnusedItem=Nothing12";
    wchar_t* buffer2 = makesectionbuffer(test2);
    if (buffer2 == NULL)
        return 0;
    MfrWriteProfileSectionTest ts_Native_PF2 = { "MFR+ILV Section Native-file VFS missing in package", true, false, false, 
                                    (g_NativePF + L"\\PlaceholderTest\\Nonesuch.ini"), L"Section4",  (DWORD)test2.length() + 1, buffer2 };
    MfrWriteProfileSectionTests.push_back(ts_Native_PF2);


    // Requests to Package PVAD File Locations for WritePrivateProfileSection via existing file with value present
    std::wstring test3 = L"UnusedItem=Nothing123";
    wchar_t* buffer3 = makesectionbuffer(test3);
    if (buffer3 == NULL)
        return 0;
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVADW1.ini");
    MfrWriteProfileSectionTest ts_PackagePVAD_F1 = { "MFR+ILV Section Package-file PVAD exists in package", true, false, false, 
                                    temp.c_str() ,  L"Section4", (DWORD)test3.length() + 1, buffer3 };
    MfrWriteProfileSectionTests.push_back(ts_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for WritePrivateProfileSection via Missing file with value present
    std::wstring test4 = L"UnusedItem=Nothing1234";
    wchar_t* buffer4 = makesectionbuffer(test4);
    if (buffer4 == NULL)
        return 0;
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\NonesuchW1.ini");
    MfrWriteProfileSectionTest ts_PackagePVAD_F2 = { "MFR+ILV Section Package-file PVAD missing in package", true, false, false, 
                                    temp.c_str(),  L"Section4", (DWORD)test4.length() + 1, buffer4 };
    MfrWriteProfileSectionTests.push_back(ts_PackagePVAD_F2);



    // Requests to Package VFS File Locations for WritePrivateProfileSection via existing VFS file with value present
    std::wstring test5 = L"UnusedItem=Nothing12345";
    wchar_t* buffer5 = makesectionbuffer(test5);
    if (buffer5 == NULL)
        return 0;
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrWriteProfileSectionTest ts_PackageVFS_PF1 = { "MFR+ILV Section Package-file VFS exists in package", true, false, false, 
                                    temp.c_str() ,  L"Section4", (DWORD)test5.length() + 1, buffer5 };
    MfrWriteProfileSectionTests.push_back(ts_PackageVFS_PF1);

    // Requests to Package VFS File Locations for WritePrivateProfileSection via Missing VFS file with value present
    std::wstring test6 = L"UnusedItem=Nothing123456";
    wchar_t* buffer6 = makesectionbuffer(test6);
    if (buffer6 == NULL)
        return 0;
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrWriteProfileSectionTest ts_PackageVFS_PF2 = { "MFR+ILV Section Package-file VFS missing in package", true, false, false, 
                                    temp.c_str(),  L"Section4",  (DWORD)test6.length() + 1, buffer6 };
    MfrWriteProfileSectionTests.push_back(ts_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for WritePrivateProfileSection via existing VFS file with value present
    std::wstring test7 = L"UnusedItem=Nothing1234567";
    wchar_t* buffer7 = makesectionbuffer(test7);
    if (buffer7 == NULL)
        return 0;
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrWriteProfileSectionTest ts_RedirectedVFS_PF1 = { "MFR+ILV Section Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() ,  L"Section4", (DWORD)test7.length() + 1, buffer7 };
    MfrWriteProfileSectionTests.push_back(ts_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for WritePrivateProfileSection via Missing VFS file with value present
    std::wstring test8 = L"UnusedItem=Nothing12345678";
    wchar_t* buffer8 = makesectionbuffer(test8);
    if (buffer8 == NULL)
        return 0;
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrWriteProfileSectionTest ts_RedirectedVFS_PF2 = { "MFR+ILV Section Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(),  L"Section4",  (DWORD)test8.length() + 1, buffer8 };
    MfrWriteProfileSectionTests.push_back(ts_RedirectedVFS_PF2);


    // Requests to Redirected PVAD File Locations for WritePrivateProfileSection via existing file with value present
    std::wstring test9 = L"UnusedItem=Nothing123456789";
    wchar_t* buffer9 = makesectionbuffer(test9);
    if (buffer9 == NULL)
        return 0;
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVADW2.ini");
    MfrWriteProfileSectionTest ts_RedirectedPVAD_F1 = { "MFR+ILV Section Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section4", (DWORD)test9.length() + 1, buffer9 };
    MfrWriteProfileSectionTests.push_back(ts_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for WritePrivateProfileSection via Missing file with value present
    std::wstring test10 = L"UnusedItem=Nothing12345678910";
    wchar_t* buffer10 = makesectionbuffer(test10);
    if (buffer10 == NULL)
        return 0;
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\NonesuchW2.ini");
    MfrWriteProfileSectionTest ts_RedirectedPVAD_F2 = { "MFR+ILV Section Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(),  L"Section4", (DWORD)test10.length() + 1, buffer10 };
    MfrWriteProfileSectionTests.push_back(ts_RedirectedPVAD_F2);



    // Requests to NULL FILE for WritePrivateProfileSection 
    std::wstring testNull = L"";
    wchar_t* bufferNull = makesectionbuffer(testNull);
    if (bufferNull == NULL)
        return 0;
    std::wstring nullpathS;
    MfrWriteProfileSectionTest ts_NULL = { "MFR+ILV Section NULL FILE", true, false, false,
                                    nullpathS,  L"Section4", 0, bufferNull };
    MfrWriteProfileSectionTests.push_back(ts_NULL);



    int count = 0;
    for (MfrWriteProfileSectionTest t : MfrWriteProfileSectionTests)      if (t.enabled) { count++; }
    return count; 
}

int InitializeWriteProfileTestsString()
{
    std::wstring temp;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    // Requests to Native File Locations for WritePrivateProfileString via existing VFS file with value present
    MfrWriteProfileStringTest ts_Native_PF1 = { "MFR+ILV String Native-file VFS exists in package", true, true, true, 
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_Native_PF1);

    // Requests to Native File Locations for WritePrivateProfileString via Missing VFS file with value present
    MfrWriteProfileStringTest ts_Native_PF2 = { "MFR+ILV String Native-file VFS missing in package", true, false, false, 
                                    (g_NativePF + L"\\PlaceholderTest\\Nonesuch.ini"), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_Native_PF2);


    // Requests to Package PVAD File Locations for WritePrivateProfileString via existing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVADW3.ini");
    MfrWriteProfileStringTest ts_PackagePVAD_F1 = { "MFR+ILV String Package-file PVAD exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString",  L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for WritePrivateProfileString via Missing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\NonesuchW3.ini");
    MfrWriteProfileStringTest ts_PackagePVAD_F2 = { "MFR+ILV String Package-file PVAD missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_PackagePVAD_F2);


    // Requests to Package VFS File Locations for WritePrivateProfileString via existing VFS file with value present
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrWriteProfileStringTest ts_PackageVFS_PF1 = { "MFR+ILV String Package-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_PackageVFS_PF1);

    // Requests to Package VFS File Locations for WritePrivateProfileString via Missing VFS file with value present
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrWriteProfileStringTest ts_PackageVFS_PF2 = { "MFR+ILV String Package-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for WritePrivateProfileString via existing VFS file with value present
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrWriteProfileStringTest ts_RedirectedVFS_PF1 = { "MFR+ILV String Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString",  L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for WritePrivateProfileString via Missing VFS file with value present
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrWriteProfileStringTest ts_RedirectedVFS_PF2 = { "MFR+ILV String Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_RedirectedVFS_PF2);

    // Requests to Redirected PVAD File Locations for WritePrivateProfileString via existing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVADW4.ini");
    MfrWriteProfileStringTest ts_RedirectedPVAD_F1 = { "MFR+ILV String Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString",  L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for WritePrivateProfileString via Missing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\NonesuchW4.ini");
    MfrWriteProfileStringTest ts_RedirectedPVAD_F2 = { "MFR+ILV String Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrWriteProfileStringTests.push_back(ts_RedirectedPVAD_F2);




    // Requests to NULL FILE for WritePrivateProfileString 
    std::wstring nullpathS;
    MfrWriteProfileStringTest ts_NULL = { "MFR+ILV String NULL FILE", true, false, false, 
                                    nullpathS, L"Section1", L"ItemString", L"InvalidString", 0, L"" };
    MfrWriteProfileStringTests.push_back(ts_NULL);



    int count = 0;
    for (MfrWriteProfileStringTest t : MfrWriteProfileStringTests)      if (t.enabled) { count++; }
    return count;
}

int InitializeWriteProfileTestsStruct()
{
    std::wstring temp;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    return (int)MfrWriteProfileStructTests.size();
}



int InitializeWriteProfileTests()
{
    int count = 0;
    count += InitializeWriteProfileTestsSection();
    count += InitializeWriteProfileTestsString();
    count += InitializeWriteProfileTestsStruct();

    return count;
}



int RunWriteProfileTests()
{

    int result = ERROR_SUCCESS;
    int   testLen = 2048;
    wchar_t* buffer = (wchar_t*)malloc(testLen * sizeof(wchar_t));
    if (buffer != NULL)
    {

        for (MfrWriteProfileSectionTest testInput : MfrWriteProfileSectionTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "MFR+ILV WritePrivateProfileSection Test: ";
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


                auto testResult = WritePrivateProfileSection(testInput.Section.c_str(), testInput.Expected_Result_String, testInput.TestPath.c_str());
                if (testResult != 0)
                {
                    auto testResult2 = GetPrivateProfileSection(testInput.Section.c_str(), buffer, testLen, testInput.TestPath.c_str());
                    if (testResult2 == testInput.Expected_Result_Length) // &&
                        //testInput.Expected_Result_String.compare(buffer) == 0)
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        std::wstring detail1 = L"ERROR: Returned value/Length incorrect. Expected=";
                        detail1.append(std::to_wstring(testInput.Expected_Result_Length));
                        detail1.append(L" Received=");
                        detail1.append(std::to_wstring(testResult2));
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), error_color);
                        result = result ? result : -1;
                        test_end(-1);
                    }
                }
                else if (testInput.TestPath.length() == 0)
                {
                    // Test should fail with a null file.
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    std::wstring detail1 = L"ERROR: Data not written. Error=";
                    detail1.append(std::to_wstring(GetLastError()));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), error_color);
                    result = result ? result : -1;
                    test_end(-1);
                }
            }
        }


        for (MfrWriteProfileStringTest testInput : MfrWriteProfileStringTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "MFR+ILV WritePrivateProfileString Test: ";
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

                auto testResult = WritePrivateProfileString(testInput.Section.c_str(), testInput.KeyName.c_str(), testInput.sDefault.c_str(), testInput.TestPath.c_str());
                if (testResult != 0)
                {
                    auto testResult2 = GetPrivateProfileString(testInput.Section.c_str(), testInput.KeyName.c_str(), testInput.sDefault.c_str(), buffer, testLen, testInput.TestPath.c_str());
                    if (testResult2 == testInput.Expected_Result_Length &&
                        testInput.Expected_Result_String.compare(buffer) == 0)
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        std::wstring detail1 = L"ERROR: Returned value/Length incorrect. Expected=";
                        detail1.append(std::to_wstring(testInput.Expected_Result_Length));
                        detail1.append(L" Received=");
                        detail1.append(std::to_wstring(testResult2));
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), error_color);
                        result = result ? result : -1;
                        test_end(-1);
                    }
                }
                else if (testInput.TestPath.length() == 0)
                {
                    // Test should fail with a null file.
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    std::wstring detail1 = L"ERROR: Data not written. Error=";
                    detail1.append(std::to_wstring(GetLastError()));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), error_color);
                    result = result ? result : -1;
                    test_end(-1);
                }
            }
        }

        for (MfrWriteProfileStructTest testInput : MfrWriteProfileStructTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "MFR+ILV WritePrivateProfileStruct Test: ";
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

                
                // not implemented 
                std::wstring detail1 = L"ERROR: Test not implimented.";
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                result = result ? result : -1;
                test_end(-1);
            }
        }


        free(buffer);
    }
    return result;
}