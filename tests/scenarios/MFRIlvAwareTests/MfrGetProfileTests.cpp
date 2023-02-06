//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrGetProfileTests.h"
#include "MfrCleanup.h"
#include <stdio.h>


std::vector< MfrGetProfileSectionNamesTest> MfrGetProfileSectionNamesTests;
std::vector< MfrGetProfileSectionTest> MfrGetProfileSectionTests;
std::vector<MfrGetProfileIntTest> MfrGetProfileIntTests;
std::vector< MfrGetProfileStringTest> MfrGetProfileStringTests;

int InitializeGetProfileTestsSectionNames()
{
    std::wstring temp;

    // Requests to Native File Locations for GetPrivateProfileSection via existing VFS file with value present
    MfrGetProfileSectionNamesTest ts_Native_PF1 = { "MFR+ILV Section Native-file VFS exists in package", true, true, true, (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),  27, 3 };
    MfrGetProfileSectionNamesTests.push_back(ts_Native_PF1);

    // Requests to Native File Locations for GetPrivateProfileSection via Missing VFS file with value present
    MfrGetProfileSectionNamesTest ts_Native_PF2 = { "MFR+ILV Section Native-file VFS missing in package", true, false, false, (g_NativePF + L"\\PlaceholderTest\\Nonesuch.ini"),  0, 0 };
    MfrGetProfileSectionNamesTests.push_back(ts_Native_PF2);


    int count = 0;
    for (MfrGetProfileSectionNamesTest t : MfrGetProfileSectionNamesTests)      if (t.enabled) { count++; }
    return count;
}

int InitializeGetProfileTestsSection()
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


    // Requests to Native File Locations for GetPrivateProfileSection via existing VFS file with value present
    MfrGetProfileSectionTest ts_Native_PF1 = { "MFR+ILV Section Native-file VFS exists in package", true, true, true, 
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"), L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_Native_PF1);

    // Requests to Native File Locations for GetPrivateProfileSection via Missing VFS file with value present
    MfrGetProfileSectionTest ts_Native_PF2 = { "MFR+ILV Section Native-file VFS missing in package", true, false, false, 
                                    (g_NativePF + L"\\PlaceholderTest\\Nonesuch.ini"), L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_Native_PF2);


    // Requests to Package PVAD File Locations for GetPrivateProfileSection via existing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD1.ini");
    MfrGetProfileSectionTest ts_PackagePVAD_F1 = { "MFR+ILV Section Package-file PVAD exists in package", true, false, false, 
                                    temp.c_str() ,  L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for GetPrivateProfileSection via Missing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\NonesuchPVAD1.ini");
    MfrGetProfileSectionTest ts_PackagePVAD_F2 = { "MFR+ILV Section Package-file PVAD missing in package", true, false, false,
                                    temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_PackagePVAD_F2);

    // Requests to Package VFS File Locations for GetPrivateProfileSection via existing VFS file with value present
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileSectionTest ts_PackageVFS_PF1 = { "MFR+ILV Section Package-file VFS exists in package", true, false, false, 
                                    temp.c_str() ,  L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_PackageVFS_PF1);

    // Requests to Package VFS File Locations for GetPrivateProfileSection via Missing VFS file with value present
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileSectionTest ts_PackageVFS_PF2 = { "MFR+ILV Section Package-file VFS missing in package", true, false, false, 
                                    temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for GetPrivateProfileSection via existing VFS file with value present
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileSectionTest ts_RedirectedVFS_PF1 = { "MFR+ILV Section Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() ,  L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for GetPrivateProfileSection via Missing VFS file with value present
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileSectionTest ts_RedirectedVFS_PF2 = { "MFR+ILV Section Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedVFS_PF2);


    // Requests to Redirected PVAD File Locations for GetPrivateProfileSection via existing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD2.ini");
    MfrGetProfileSectionTest ts_RedirectedPVAD_F1 = { "MFR+ILV Section Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for GetPrivateProfileSection via Missing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\NonesuchPVAD2.ini");
    MfrGetProfileSectionTest ts_RedirectedPVAD_F2 = { "MFR+ILV Section Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedPVAD_F2);



    // Requests to NULL FILE for GetPrivateProfileSection 
    std::wstring nullpathS;
    MfrGetProfileSectionTest ts_NULL = { "MFR+ILV Section NULL FILE", true, false, false, 
                                    nullpathS,  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_NULL);



    int count = 0;
    for (MfrGetProfileSectionTest t : MfrGetProfileSectionTests)      if (t.enabled) { count++; }
    return count;
}

int InitializeGetProfileTestsInt()
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

    // Requests to Native File Locations for GetPrivateProfileInt via existing VFS file with value present
    MfrGetProfileIntTest t_Native_PF1 = { "MFR+ILV Int Native-file VFS exists in package", true, true, true, 
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"), L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_Native_PF1);

    // Requests to Native File Locations for GetPrivateProfileInt via Missing VFS file with value present
    MfrGetProfileIntTest t_Native_PF2 = { "MFR+ILV Int Native-file VFS missing in package", true, false, false, 
                                    (g_NativePF + L"\\PlaceholderTest\\Nonesuch.ini"), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_Native_PF2);


    // Requests to Package PVAD File Locations for GetPrivateProfileInt via existing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD3.ini");
    MfrGetProfileIntTest t_PackagePVAD_F1 = { "MFR+ILV Int Package-file PVAD exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for GetPrivateProfileInt via Missing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\NonesuchPVAD3.ini");
    MfrGetProfileIntTest t_PackagePVAD_F2 = { "MFR+ILV Int Package-file PVAD missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_PackagePVAD_F2);


    // Requests to Package VFS File Locations for GetPrivateProfileInt via existing VFS file with value present
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileIntTest t_PackageVFS_PF1 = { "MFR+ILV Int Package-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_PackageVFS_PF1);

    // Requests to Package VFS File Locations for GetPrivateProfileInt via Missing VFS file with value present
    temp = VFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileIntTest t_PackageVFS_PF2 = { "MFR+ILV Int Package-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for GetPrivateProfileInt via existing VFS file with value present
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileIntTest t_RedirectedVFS_PF1 = { "MFR+ILV Int Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for GetPrivateProfileInt via Missing VFS file with value present
    temp = REVFSPF;
    temp.append(L"\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileIntTest t_RedirectedVFS_PF2 = { "MFR+ILV Int Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_RedirectedVFS_PF2);


    // Requests to Redirected PVAD File Locations for GetPrivateProfileInt via existing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD4.ini");
    MfrGetProfileIntTest t_RedirectedPVAD_F1 = { "MFR+ILV Int Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for GetPrivateProfileInt via Missing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\NonesuchPVAD4.ini");
    MfrGetProfileIntTest t_RedirectedPVAD_F2 = { "MFR+ILV Int Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_RedirectedPVAD_F2);



    // Requests to NULL FILE for GetPrivateProfileInt 
    std::wstring nullpath;
    MfrGetProfileIntTest t_NULL = { "MFR+ILV Int NULL FILE", true, false, false, nullpath,  
                                    L"Section1", L"ItemInt", 888, 888 };
    MfrGetProfileIntTests.push_back(t_NULL);


    int count = 0;
    for (MfrGetProfileIntTest t : MfrGetProfileIntTests)      if (t.enabled) { count++; }
    return count;
}

int InitializeGetProfileTestsString()
{
    std::wstring temp;


    // Requests to Native File Locations for GetPrivateProfileString via existing VFS file with value present
    MfrGetProfileStringTest ts_Native_PF1 = { "MFR+ILV String Native-file VFS exists in package", true, true, true, 
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"), L"Section1", L"ItemString", L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_Native_PF1);

    // Requests to Native File Locations for GetPrivateProfileString via Missing VFS file with value present
    MfrGetProfileStringTest ts_Native_PF2 = { "MFR+ILV String Native-file VFS missing in package", true, false, false, 
                                    (g_NativePF + L"\\PlaceholderTest\\Nonesuch.ini"), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_Native_PF2);


    // Requests to Package PVAD File Locations for GetPrivateProfileString via existing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD5.ini");
    MfrGetProfileStringTest ts_PackagePVAD_F1 = { "MFR+ILV String Package-file PVAD exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString",  L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for GetPrivateProfileString via Missing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\NonesuchPVAD5.ini");
    MfrGetProfileStringTest ts_PackagePVAD_F2 = { "MFR+ILV String Package-file PVAD missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_PackagePVAD_F2);

    // Requests to Package VFS File Locations for GetPrivateProfileString via existing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileStringTest ts_PackageVFS_PF1 = { "MFR+ILV String Package-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString", L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_PackageVFS_PF1);

    // Requests to Package VFS File Locations for GetPrivateProfileString via Missing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileStringTest ts_PackageVFS_PF2 = { "MFR+ILV String Package-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for GetPrivateProfileString via existing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileStringTest ts_RedirectedVFS_PF1 = { "MFR+ILV String Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString",  L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for GetPrivateProfileString via Missing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileStringTest ts_RedirectedVFS_PF2 = { "MFR+ILV String Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedVFS_PF2);


    // Requests to Redirected PVAD File Locations for GetPrivateProfileString via existing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD6.ini");
    MfrGetProfileStringTest ts_RedirectedPVAD_F1 = { "MFR+ILV String Redirected-file VFS exists in package", true, false, false, 
                                    temp.c_str() , L"Section1", L"ItemString",  L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for GetPrivateProfileString via Missing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\NonesuchPVAD6.ini");
    MfrGetProfileStringTest ts_RedirectedPVAD_F2 = { "MFR+ILV String Redirected-file VFS missing in package", true, false, false, 
                                    temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedPVAD_F2);



    // Requests to NULL FILE for GetPrivateProfileString 
    std::wstring nullpathS;
    MfrGetProfileStringTest ts_NULL = { "MFR+ILV String NULL FILE", true, false, false, 
                                    nullpathS, L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_NULL);


    int count = 0;
    for (MfrGetProfileStringTest t : MfrGetProfileStringTests)      if (t.enabled) { count++; }
    return count;
}

int InitializeGetProfileTests()
{
    int count = 0;
    count += InitializeGetProfileTestsSectionNames();
    count += InitializeGetProfileTestsSection();
    count += InitializeGetProfileTestsInt();
    count += InitializeGetProfileTestsString();

    return count;
}


int RunGetProfileTests()
{

    int result = ERROR_SUCCESS;
    int   testLen = 2048 ;
    wchar_t* buffer = (wchar_t*)malloc(testLen * sizeof(wchar_t));
    if (buffer != NULL)
    {
        for (MfrGetProfileSectionNamesTest testInput : MfrGetProfileSectionNamesTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "MFR+ILV GetPrivateProfileSectionNames Test: ";
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

                auto testResult = GetPrivateProfileSectionNames( buffer, testLen, testInput.TestPath.c_str());
                // The return is a set of null terminated strings with an extra null termination at the end.
                // The count is the number of characters, including individual string null, but not the extra null at the end.
                DWORD count = 0;
                for (DWORD index = 0; index < testResult; index++)
                {
                    if (buffer[index] == NULL)
                        count++;
                }

                if (testResult == testInput.Expected_Result_Length &&
                    count == testInput.Expected_Result_NumberStrings)
                {
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    std::wstring detail1 = L"ERROR: Returned value/Length incorrect. Expected=";
                    detail1.append(std::to_wstring(testInput.Expected_Result_Length));
                    detail1.append(L" Received=");
                    detail1.append(std::to_wstring(testResult));
                    detail1.append(L", and #strings expected=");
                    detail1.append(std::to_wstring(testInput.Expected_Result_NumberStrings));
                    detail1.append(L" Received=");
                    detail1.append(std::to_wstring(count));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), error_color);
                    result = result ? result : -1;
                    test_end(-1);
                }
            }
        }

        for (MfrGetProfileSectionTest testInput : MfrGetProfileSectionTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "MFR+ILV GetPrivateProfileSection Test: ";
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

                auto testResult = GetPrivateProfileSection(testInput.Section.c_str(),  buffer, testLen, testInput.TestPath.c_str());
                if (testResult == testInput.Expected_Result_Length &&
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
                    detail1.append(std::to_wstring(testResult));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), error_color);
                    result = result ? result : -1;
                    test_end(-1);
                }
            }
        }

        for (MfrGetProfileIntTest testInput : MfrGetProfileIntTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "MFR+ILV GetPrivateProfileInt Test: ";
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

                auto testResult = GetPrivateProfileInt(testInput.Section.c_str(), testInput.KeyName.c_str(), testInput.nDefault, testInput.TestPath.c_str());
                if (testResult == testInput.Expected_Result)
                {
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                    detail1.append(std::to_wstring(testInput.Expected_Result));
                    detail1.append(L" Received=");
                    detail1.append(std::to_wstring(testResult));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), error_color);
                    result = result ? result : -1;
                    test_end(-1);
                }
            }
        }

        for (MfrGetProfileStringTest testInput : MfrGetProfileStringTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "MFR+ILV GetPrivateProfileString Test: ";
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

                auto testResult = GetPrivateProfileString(testInput.Section.c_str(), testInput.KeyName.c_str(), testInput.sDefault.c_str(), buffer, testLen, testInput.TestPath.c_str());
                if (testResult == testInput.Expected_Result_Length &&
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
                    detail1.append(std::to_wstring(testResult));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), error_color);
                    result = result ? result : -1;
                    test_end(-1);
                }
            }
        }


        free(buffer);
    }
    return result;
}