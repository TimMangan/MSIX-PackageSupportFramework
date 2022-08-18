//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrGetProfileTest.h"
#include <stdio.h>

std::vector< MfrGetProfileSectionTest> MfrGetProfileSectionTests;
std::vector<MfrGetProfileIntTest> MfrGetProfileIntTests;
std::vector< MfrGetProfileStringTest> MfrGetProfileStringTests;


int InitializeGetProfileTestsSection()
{
    std::wstring temp;



    // Requests to Native File Locations for GetPrivateProfileSection via existing VFS file with value present
    MfrGetProfileSectionTest ts_Native_PF1 = { "Section Native-file VFS exists in package", true, L"C:\\Program Files\\PlaceholderTest\\TestIniFileVfsPF.ini", L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_Native_PF1);

    // Requests to Native File Locations for GetPrivateProfileSection via Missing VFS file with value present
    MfrGetProfileSectionTest ts_Native_PF2 = { "Section Native-file VFS missing in package", true, L"C:\\Program Files\\PlaceholderTest\\Nonesuch.ini", L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_Native_PF2);


    // Requests to Package PVAD File Locations for GetPrivateProfileSection via existing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD.ini");
    MfrGetProfileSectionTest ts_PackagePVAD_F1 = { "Section Package-file PVAD exists in package", true, temp.c_str() ,  L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for GetPrivateProfileSection via Missing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\Nonesuch.ini");
    MfrGetProfileSectionTest ts_PackagePVAD_F2 = { "Section Package-file PVAD missing in package", true, temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_PackagePVAD_F2);


    // Requests to Package VFS File Locations for GetPrivateProfileSection via existing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileSectionTest ts_PackageVFS_PF1 = { "Section Package-file VFS exists in package", true, temp.c_str() ,  L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_PackageVFS_PF1);

    // Requests to Package VFS File Locations for GetPrivateProfileSection via Missing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileSectionTest ts_PackageVFS_PF2 = { "Section Package-file VFS missing in package", true, temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for GetPrivateProfileSection via existing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileSectionTest ts_RedirectedVFS_PF1 = { "Section Redirected-file VFS exists in package", true, temp.c_str() ,  L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for GetPrivateProfileSection via Missing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileSectionTest ts_RedirectedVFS_PF2 = { "Section Redirected-file VFS missing in package", true, temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedVFS_PF2);


    // Requests to Redirected PVAD File Locations for GetPrivateProfileSection via existing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD.ini");
    MfrGetProfileSectionTest ts_RedirectedPVAD_F1 = { "Section Redirected-file VFS exists in package", true, temp.c_str() , L"Section2", 19, L"UnusedItem=Nothing" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for GetPrivateProfileSection via Missing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\Nonesuch.ini");
    MfrGetProfileSectionTest ts_RedirectedPVAD_F2 = { "Section Redirected-file VFS missing in package", true, temp.c_str(),  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_RedirectedPVAD_F2);



    // Requests to NULL FILE for GetPrivateProfileSection 
    std::wstring nullpathS;
    MfrGetProfileSectionTest ts_NULL = { "Section NULL FILE", true, nullpathS,  L"Section2", 0, L"" };
    MfrGetProfileSectionTests.push_back(ts_NULL);


    return (int)MfrGetProfileSectionTests.size();
}

int InitializeGetProfileTestsInt()
{
    std::wstring temp;

    // Requests to Native File Locations for GetPrivateProfileInt via existing VFS file with value present
    MfrGetProfileIntTest t_Native_PF1 = { "Int Native-file VFS exists in package", true, L"C:\\Program Files\\PlaceholderTest\\TestIniFileVfsPF.ini", L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_Native_PF1);

    // Requests to Native File Locations for GetPrivateProfileInt via Missing VFS file with value present
    MfrGetProfileIntTest t_Native_PF2 = { "Int Native-file VFS missing in package", true, L"C:\\Program Files\\PlaceholderTest\\Nonesuch.ini", L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_Native_PF2);


    // Requests to Package PVAD File Locations for GetPrivateProfileInt via existing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD.ini");
    MfrGetProfileIntTest t_PackagePVAD_F1 = { "Int Package-file PVAD exists in package", true, temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for GetPrivateProfileInt via Missing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\Nonesuch.ini");
    MfrGetProfileIntTest t_PackagePVAD_F2 = { "Int Package-file PVAD missing in package", true, temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_PackagePVAD_F2);


    // Requests to Package VFS File Locations for GetPrivateProfileInt via existing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileIntTest t_PackageVFS_PF1 = { "Int Package-file VFS exists in package", true, temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_PackageVFS_PF1);

    // Requests to Package VFS File Locations for GetPrivateProfileInt via Missing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileIntTest t_PackageVFS_PF2 = { "Int Package-file VFS missing in package", true, temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for GetPrivateProfileInt via existing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileIntTest t_RedirectedVFS_PF1 = { "Int Redirected-file VFS exists in package", true, temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for GetPrivateProfileInt via Missing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileIntTest t_RedirectedVFS_PF2 = { "Int Redirected-file VFS missing in package", true, temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_RedirectedVFS_PF2);


    // Requests to Redirected PVAD File Locations for GetPrivateProfileInt via existing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD.ini");
    MfrGetProfileIntTest t_RedirectedPVAD_F1 = { "Int Redirected-file VFS exists in package", true, temp.c_str() , L"Section1", L"ItemInt", 666, 1957 };
    MfrGetProfileIntTests.push_back(t_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for GetPrivateProfileInt via Missing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\Nonesuch.ini");
    MfrGetProfileIntTest t_RedirectedPVAD_F2 = { "Int Redirected-file VFS missing in package", true, temp.c_str(), L"Section1", L"ItemInt", 666, 666 };
    MfrGetProfileIntTests.push_back(t_RedirectedPVAD_F2);



    // Requests to NULL FILE for GetPrivateProfileInt 
    std::wstring nullpath;
    MfrGetProfileIntTest t_NULL = { "Int NULL FILE", true, nullpath, L"Section1", L"ItemInt", 888, 888 };
    MfrGetProfileIntTests.push_back(t_NULL);

    return (int)MfrGetProfileIntTests.size();
}

int InitializeGetProfileTestsString()
{
    std::wstring temp;


    // Requests to Native File Locations for GetPrivateProfileString via existing VFS file with value present
    MfrGetProfileStringTest ts_Native_PF1 = { "String Native-file VFS exists in package", true, L"C:\\Program Files\\PlaceholderTest\\TestIniFileVfsPF.ini", L"Section1", L"ItemString", L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_Native_PF1);

    // Requests to Native File Locations for GetPrivateProfileString via Missing VFS file with value present
    MfrGetProfileStringTest ts_Native_PF2 = { "String Native-file VFS missing in package", true, L"C:\\Program Files\\PlaceholderTest\\Nonesuch.ini", L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_Native_PF2);


    // Requests to Package PVAD File Locations for GetPrivateProfileString via existing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD.ini");
    MfrGetProfileStringTest ts_PackagePVAD_F1 = { "String Package-file PVAD exists in package", true, temp.c_str() , L"Section1", L"ItemString",  L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_PackagePVAD_F1);

    // Requests to Package PVAD File Locations for GetPrivateProfileString via Missing file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\Nonesuch.ini");
    MfrGetProfileStringTest ts_PackagePVAD_F2 = { "String Package-file PVAD missing in package", true, temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_PackagePVAD_F2);


    // Requests to Package VFS File Locations for GetPrivateProfileString via existing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileStringTest ts_PackageVFS_PF1 = { "String Package-file VFS exists in package", true, temp.c_str() , L"Section1", L"ItemString", L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_PackageVFS_PF1);

    // Requests to Package VFS File Locations for GetPrivateProfileString via Missing VFS file with value present
    temp = g_PackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileStringTest ts_PackageVFS_PF2 = { "String Package-file VFS missing in package", true, temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_PackageVFS_PF2);


    // Requests to Redirected VFS File Locations for GetPrivateProfileString via existing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini");
    MfrGetProfileStringTest ts_RedirectedVFS_PF1 = { "String Redirected-file VFS exists in package", true, temp.c_str() , L"Section1", L"ItemString",  L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedVFS_PF1);

    // Requests to Redirected VFS File Locations for GetPrivateProfileString via Missing VFS file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Nonesuch.ini");
    MfrGetProfileStringTest ts_RedirectedVFS_PF2 = { "String Redirected-file VFS missing in package", true, temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedVFS_PF2);


    // Requests to Redirected PVAD File Locations for GetPrivateProfileString via existing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\TestIniFilePVAD.ini");
    MfrGetProfileStringTest ts_RedirectedPVAD_F1 = { "String Redirected-file VFS exists in package", true, temp.c_str() , L"Section1", L"ItemString",  L"InvalidString",11, L"ValidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedPVAD_F1);

    // Requests to Redirected PVAD File Locations for GetPrivateProfileString via Missing file with value present
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\Nonesuch.ini");
    MfrGetProfileStringTest ts_RedirectedPVAD_F2 = { "String Redirected-file VFS missing in package", true, temp.c_str(), L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_RedirectedPVAD_F2);



    // Requests to NULL FILE for GetPrivateProfileString 
    std::wstring nullpathS;
    MfrGetProfileStringTest ts_NULL = { "String NULL FILE", true, nullpathS, L"Section1", L"ItemString", L"InvalidString", 13, L"InvalidString" };
    MfrGetProfileStringTests.push_back(ts_NULL);

    return (int)MfrGetProfileStringTests.size();
}

int InitializeGetProfileTests()
{
    int count = 0;
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
        for (MfrGetProfileSectionTest testInput : MfrGetProfileSectionTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "GetPrivateProfileSection Test: ";
                testname.append(testInput.TestName);
                test_begin(testname);
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
                std::string testname = "GetPrivateProfileInt Test: ";
                testname.append(testInput.TestName);
                test_begin(testname);
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
                std::string testname = "GetPrivateProfileString Test: ";
                testname.append(testInput.TestName);
                test_begin(testname);
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