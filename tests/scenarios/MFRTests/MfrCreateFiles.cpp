//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrCreateFileTests.h"
#include "MfrCleanup.h"
#include <stdio.h>

std::vector<MfrCreateFileTest> MfrCreateFileTests1;
std::vector<MfrCreateFileTest> MfrCreateFileTests2;
std::vector<MfrCreateFileTest> MfrCreateFileTests3;
std::vector<MfrCreateFileTest> MfrCreateFileTests4;
std::vector<MfrCreateFileTest> MfrCreateFileTests5;
std::vector<MfrCreateFileTest> MfrCreateFileTests6;

std::vector<MfrCreateFile2Test> MfrCreateFile2Tests1;


int InitializeCreateFileTests1()
{
    std::wstring temp;

    // Requests to Native File Locations for CreateFile via VFS Read
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFR1 = { "Native-file VFS exists in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFR1);

    temp = g_NativePF + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFR2 = { "Native-file VFS missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests1.push_back(t_Native_PFR2);

    temp = g_NativePF + L"\\PlaceholderTest\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFR3 = { "Native-file VFS folder missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests1.push_back(t_Native_PFR3);


    // Requests to Native File Locations for CreateFile via VFS New
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFN1 = { "Native-file VFS exists in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_EXISTS };
    MfrCreateFileTests1.push_back(t_Native_PFN1);

    temp = g_NativePF + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFN2 = { "Native-file VFS missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFN2);

    temp = g_NativePF + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFN3 = { "Native-file VFS folder missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFN3);


    // Requests to Native File Locations for CreateFile via VFS TruncateExisting
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFT1 = { "Native-file VFS exists in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFT1);

    temp = g_NativePF + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFT2 = { "Native-file VFS missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests1.push_back(t_Native_PFT2);

    temp = g_NativePF + L"\\PlaceholderTest\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFT3 = { "Native-file VFS folder missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests1.push_back(t_Native_PFT3);



    // Requests to Native File Locations for CreateFile via VFS OpenAlways
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFOA1 = { "Native-file VFS exists in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFOA1);

    temp = g_NativePF + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFOA2 = { "Native-file VFS missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFOA2);

    temp = g_NativePF + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFOA3 = { "Native-file VFS folder missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFOA3);



    // Requests to Native File Locations for CreateFile via VFS CreateAlways
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFA1 = { "Native-file VFS exists in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFA1);

    temp = g_NativePF + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFA2 = { "Native-file VFS missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFA2);

    temp = g_NativePF + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFA3 = { "Native-file VFS folder missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests1.push_back(t_Native_PFA3);


    int count = 0;
    for (MfrCreateFileTest t : MfrCreateFileTests1)      if (t.enabled) { count++; }
    return count;
} // InitializeCreateFileTests1

int InitializeCreateFileTests2()
{
        std::wstring temp;
#if _M_IX86
        std::wstring VfsPf = g_Cwd + L"\\VFS\\ProgramFilesX86";
#else
        std::wstring VfsPf = g_Cwd + L"\\VFS\\ProgramFilesX64";
#endif
    // Requests to Package File Locations for CreateFile via VFS Read
    temp = VfsPf + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFR1 = { "Package-file VFS exists in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFR1);

    temp = VfsPf + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFR2 = { "Package-file VFS missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests2.push_back(t_Native_PFR2);

    temp = VfsPf + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFR3 = { "Package-file VFS folder missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests2.push_back(t_Native_PFR3);


    // Requests to Package File Locations for CreateFile via VFS New
    temp = VfsPf + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFN1 = { "Package-file VFS exists in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_EXISTS };
    MfrCreateFileTests2.push_back(t_Native_PFN1);

    temp = VfsPf + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFN2 = { "Package-file VFS missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFN2);

    temp = VfsPf + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFN3 = { "Package-file VFS folder missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFN3);


    // Requests to Native File Locations for CreateFile via VFS TruncateExisting
    temp = VfsPf + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFT1 = { "Package-file VFS exists in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFT1);

    temp = VfsPf + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFT2 = { "Package-file VFS missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests2.push_back(t_Native_PFT2);

    temp = VfsPf + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFT3 = { "Package-file VFS folder missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests2.push_back(t_Native_PFT3);



    // Requests to Package File Locations for CreateFile via VFS OpenAlways
    temp = VfsPf + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFOA1 = { "Package-file VFS exists in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFOA1);

    temp = VfsPf + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFOA2 = { "Package-file VFS missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFOA2);

    temp = VfsPf + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFOA3 = { "Package-file VFS folder missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFOA3);



    // Requests to Package File Locations for CreateFile via VFS CreateAlways
    temp = VfsPf + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFileTest t_Native_PFA1 = { "Package-file VFS exists in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFA1);

    temp = VfsPf + L"\\PlaceholderTest\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFA2 = { "Package-file VFS missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFA2);

    temp = VfsPf + L"\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFA3 = { "Package-file VFS folder missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests2.push_back(t_Native_PFA3);


    int count = 0;
    for (MfrCreateFileTest t : MfrCreateFileTests2)      if (t.enabled) { count++; }
    return count;
} // InitializeCreateFileTests2

int InitializeCreateFileTests3()
{
    std::wstring temp;

    // Requests to Redirected File Locations for CreateFile via VFS Read
    temp = g_writablePackageRootPath.c_str();

#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
#endif
    MfrCreateFileTest t_Native_PFR1 = { "Redirected-file VFS exists in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFR1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFR2 = { "Redirected-file VFS missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests3.push_back(t_Native_PFR2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\NoneSuchFolder\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\NoneSuchFolder\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFR3 = { "Redirected-file VFS folder missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests3.push_back(t_Native_PFR3);


    // Requests to Redirected File Locations for CreateFile via VFS New
    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
#endif
    MfrCreateFileTest t_Native_PFN1 = { "Redirected-file VFS exists in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_EXISTS };
    MfrCreateFileTests3.push_back(t_Native_PFN1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFN2 = { "Redirected-file VFS missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFN2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\NoneSuchFolder\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\NoneSuchFolder\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFN3 = { "Redirected-file VFS folder missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFN3);


    // Requests to Redirected File Locations for CreateFile via VFS TruncateExisting
    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
#endif
    MfrCreateFileTest t_Native_PFT1 = { "Redirected-file VFS exists in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFT1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFT2 = { "Redirected-file VFS missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests3.push_back(t_Native_PFT2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\NoneSuchFolder\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\NoneSuchFolder\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFT3 = { "Redirected-file VFS folder missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests3.push_back(t_Native_PFT3);



    // Requests to Redirected File Locations for CreateFile via VFS OpenAlways
    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
#endif
    MfrCreateFileTest t_Native_PFOA1 = { "Redirected-file VFS exists in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFOA1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFOA2 = { "Redirected-file VFS missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFOA2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\NoneSuchFolder\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\NoneSuchFolder\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFOA3 = { "Redirected-file VFS folder missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFOA3);



    // Requests to Redirected File Locations for CreateFile via VFS CreateAlways
    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
#endif
    MfrCreateFileTest t_Native_PFA1 = { "Redirected-file VFS exists in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFA1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFA2 = { "Redirected-file VFS missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFA2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\NoneSuchFolder\\NoneSuchFile.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\NoneSuchFolder\\NoneSuchFile.txt");
#endif
    MfrCreateFileTest t_Native_PFA3 = { "Redirected-file VFS folder missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests3.push_back(t_Native_PFA3);


    int count = 0;
    for (MfrCreateFileTest t : MfrCreateFileTests3)      if (t.enabled) { count++; }
    return count;
} // InitializeCreateFileTests3

int InitializeCreateFileTests4()
{
    std::wstring temp;

    // Requests to Package File Locations for CreateFile via VFS Read
    temp = g_Cwd + L"\\PvadFolder\\PvadFile2.txt";
    MfrCreateFileTest t_Native_PFR1 = { "Package-file VFS exists in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFR1);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFR2 = { "Package-file VFS missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests4.push_back(t_Native_PFR2);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFR3 = { "Package-file VFS folder missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests4.push_back(t_Native_PFR3);


    // Requests to Package File Locations for CreateFile via PVAD New
    temp = g_Cwd + L"\\PvadFolder\\PvadFile2.txt";
    MfrCreateFileTest t_Native_PFN1 = { "Package-file PVAD exists in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_EXISTS };
    MfrCreateFileTests4.push_back(t_Native_PFN1);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFN2 = { "Package-file PVAD missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFN2);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFN3 = { "Package-file PVAD folder missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFN3);


    // Requests to Native File Locations for CreateFile via PVAD TruncateExisting
    temp = g_Cwd + L"\\PvadFolder\\PvadFile2.txt";
    MfrCreateFileTest t_Native_PFT1 = { "Package-file PVAD exists in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFT1);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFT2 = { "Package-file PVAD missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests4.push_back(t_Native_PFT2);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFT3 = { "Package-file PVAD folder missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests4.push_back(t_Native_PFT3);



    // Requests to Package File Locations for CreateFile via PVAD OpenAlways
    temp = g_Cwd + L"\\PvadFolder\\PvadFile2.txt";
    MfrCreateFileTest t_Native_PFOA1 = { "Package-file PVAD exists in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFOA1);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFOA2 = { "Package-file PVAD missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFOA2);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFOA3 = { "Package-file PVAD folder missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFOA3);



    // Requests to Package File Locations for CreateFile via PVAD CreateAlways
    temp = g_Cwd + L"\\PvadFolder\\PvadFile2.txt";
    MfrCreateFileTest t_Native_PFA1 = { "Package-file PVAD exists in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFA1);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFA2 = { "Package-file PVAD missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFA2);

    temp = g_Cwd + L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt";
    MfrCreateFileTest t_Native_PFA3 = { "Package-file PVAD folder missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests4.push_back(t_Native_PFA3);

    int count = 0;
    for (MfrCreateFileTest t : MfrCreateFileTests4)      if (t.enabled) { count++; }
    return count;
} // InitializeCreateFileTests4

int InitializeCreateFileTests5()
{
    std::wstring temp;

    // Requests to Redirected File Locations for CreateFile via PVAD Read
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\PvadFile2.txt");
    MfrCreateFileTest t_Native_PFR1 = { "Redirected-file PVAD exists in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFR1);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFR2 = { "Redirected-file PVAD missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests5.push_back(t_Native_PFR2);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFR3 = { "Redirected-file PVAD folder missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests5.push_back(t_Native_PFR3);


    // Requests to Redirected File Locations for CreateFile via PVAD New
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\PvadFile2.txt");
    MfrCreateFileTest t_Native_PFN1 = { "Redirected-file PVAD exists in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_EXISTS };
    MfrCreateFileTests5.push_back(t_Native_PFN1);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFN2 = { "Redirected-file PVAD missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFN2);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFN3 = { "Redirected-file PVAD folder missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFN3);


    // Requests to Redirected File Locations for CreateFile via PVAD TruncateExisting
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\PvadFile2.txt");
    MfrCreateFileTest t_Native_PFT1 = { "Redirected-file PVAD exists in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFT1);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFT2 = { "Redirected-file PVAD missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests5.push_back(t_Native_PFT2);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFT3 = { "Redirected-file PVAD folder missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests5.push_back(t_Native_PFT3);



    // Requests to Redirected File Locations for CreateFile via PVAD OpenAlways
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\PvadFile2.txt");
    MfrCreateFileTest t_Native_PFOA1 = { "Redirected-file PVAD exists in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFOA1);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFOA2 = { "Redirected-file PVAD missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFOA2);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFOA3 = { "Redirected-file PVAD folder missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFOA3);



    // Requests to Redirected File Locations for CreateFile via PVAD CreateAlways
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\PvadFile2.txt");
    MfrCreateFileTest t_Native_PFA1 = { "Redirected-file PVAD exists in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFA1);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFA2 = { "Redirected-file PVAD missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFA2);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFolder\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFA3 = { "Redirected-file PVAD folder missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests5.push_back(t_Native_PFA3);

    int count = 0;
    for (MfrCreateFileTest t : MfrCreateFileTests5)      if (t.enabled) { count++; }
    return count;
} // InitializeCreateFileTests5

int InitializeCreateFileTests6()
{
    std::wstring temp;

    // Requests to Native File Locations for CreateFile via Local Read
    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\PresonalFile.txt");
    MfrCreateFileTest t_Native_PFR1 = { "Local Native-file VFS exists in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFR1);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFR2 = { "Local Native-file VFS missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests6.push_back(t_Native_PFR2);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFR3 = { "Local Native-file VFS folder missing in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests6.push_back(t_Native_PFR3);


    // Requests to Native File Locations for CreateFile via Local New
    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\PresonalFile.txt");
    MfrCreateFileTest t_Native_PFN1 = { "Local Native-file VFS exists in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_EXISTS };
    MfrCreateFileTests6.push_back(t_Native_PFN1);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFN2 = { "Local Native-file VFS missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFN2);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFN3 = { "Local Native-file VFS folder missing in package New existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFN3);


    // Requests to Native File Locations for CreateFile via Local TruncateExisting
    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\PresonalFile.txt");
    MfrCreateFileTest t_Native_PFT1 = { "Local Native-file VFS exists in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFT1);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFT2 = { "Local Native-file VFS missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests6.push_back(t_Native_PFT2);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFT3 = { "Local Native-file VFS folder missing in package Truncate existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       false, ERROR_FILE_NOT_FOUND };
    MfrCreateFileTests6.push_back(t_Native_PFT3);



    // Requests to Native File Locations for CreateFile via VFS OpenAlways
    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\PresonalFile.txt");
    MfrCreateFileTest t_Native_PFOA1 = { "Local Native-file VFS exists in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFOA1);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFOA2 = { "Local Native-file VFS missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFOA2);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFOA3 = { "Local Native-file VFS folder missing in package OpenAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFOA3);



    // Requests to Native File Locations for CreateFile via Local CreateAlways
    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\PresonalFile.txt");
    MfrCreateFileTest t_Native_PFA1 = { "Local Native-file VFS exists in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFA1);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFA2 = { "Local Native-file VFS missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFA2);

    temp = g_Documents.c_str();
    temp.append(L"\\MFRTestDocs\\NoneSuchFolder\\NoneSuchFile.txt");
    MfrCreateFileTest t_Native_PFA3 = { "Local Native-file VFS folder missing in package CreateAlways existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                       true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFA3);

    temp = L"C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\clr.dll";

    MfrCreateFileTest t_Native_PFD1 = { "Local Native-file DLL not in package", true, true, true,
                                   temp.c_str(),
                                   GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   true, ERROR_SUCCESS };
    MfrCreateFileTests6.push_back(t_Native_PFD1);

    int count = 0;
    for (MfrCreateFileTest t : MfrCreateFileTests6)      if (t.enabled) { count++; }
    return count;
} // InitializeCreateFileTests6


int InitializeCreateFile2Tests1()
{
    std::wstring temp;

   

    // Requests to Native File Locations for CreateFile via VFS Read
    temp = g_NativePF + L"\\PlaceholderTest\\Placeholder.txt";
    MfrCreateFile2Test t_Native_PFR1 = { "Native-file VFS exists in package Read existing", true, true, true,
                                       temp.c_str(),
                                       GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, FILE_FLAG_RANDOM_ACCESS, SECURITY_IMPERSONATION,
                                       true, ERROR_SUCCESS };
    MfrCreateFile2Tests1.push_back(t_Native_PFR1);

    int count = 0;
    for (MfrCreateFile2Test t : MfrCreateFile2Tests1)      if (t.enabled) { count++; }
    return count;
} // InitializeCreateFile2Tests1

int InitializeCreateFileTests()
{
    int count = 0;
    count += InitializeCreateFileTests1();
    count += InitializeCreateFileTests2();
    count += InitializeCreateFileTests3();
    count += InitializeCreateFileTests4();
    count += InitializeCreateFileTests5();
    count += InitializeCreateFileTests6();

    count += InitializeCreateFile2Tests1();
    return count;
} // InitializeAttributeTests()


std::wstring creation_fileaccess_text(DWORD desiredAccess)
{
    std::wstring result = L"";
    if ((desiredAccess & GENERIC_READ) != 0) { result.append(L" GenRead"); }
    if ((desiredAccess & GENERIC_WRITE) != 0) { result.append(L" GenWrite"); }
    if ((desiredAccess & GENERIC_EXECUTE) != 0) { result.append(L" GenExecute"); }
    if ((desiredAccess & GENERIC_ALL) != 0) { result.append(L" GenAll"); }
    if ((desiredAccess & ACCESS_SYSTEM_SECURITY) != 0) { result.append(L" AccSec"); }

    // standard rights
    if ((desiredAccess & STANDARD_RIGHTS_ALL) == STANDARD_RIGHTS_ALL) { result.append(L" StdAll"); }
    else if ((desiredAccess & STANDARD_RIGHTS_ALL) == STANDARD_RIGHTS_REQUIRED) { result.append(L" StdReq"); }
    else
    {
        if ((desiredAccess & DELETE) != 0) { result.append(L" Delete"); }
        if ((desiredAccess & READ_CONTROL) != 0) { result.append(L" RdWtEx"); }
        if ((desiredAccess & SYNCHRONIZE) != 0) { result.append(L" Sync"); }
        if ((desiredAccess & WRITE_DAC) != 0) { result.append(L" WtDac"); }
        if ((desiredAccess & WRITE_OWNER) != 0) { result.append(L" WtOwner"); }
    }
    return result;
}

const wchar_t* creation_disposition_text(DWORD creationDisposition)
{
    switch (creationDisposition)
    {
    case CREATE_ALWAYS: return L"CREATE_ALWAYS";
    case CREATE_NEW: return L"CREATE_NEW";
    case OPEN_ALWAYS: return L"OPEN_ALWAYS";
    case OPEN_EXISTING: return L"OPEN_EXISTING";
    case TRUNCATE_EXISTING: return L"TRUNCATE_EXISTING";
    }

    assert(false);
    return L"UNKNOWN";
}

std::wstring creation_share_text(DWORD sharemode)
{
    std::wstring result = L"";
    if ((sharemode & FILE_SHARE_READ) != 0) { result.append(L" Read"); }
    if ((sharemode & FILE_SHARE_WRITE) != 0) { result.append(L" Write"); }
    if ((sharemode & FILE_SHARE_DELETE) != 0) { result.append(L" DELETE"); }
    if ((sharemode == 0)) { result.append(L" None"); }
    return result;
}

std::wstring fileattrib_text(DWORD fileattrib)
{
    std::wstring result = L"";
    if (fileattrib == FILE_ATTRIBUTE_NORMAL) { result = L"FILE_ATTRIBUTE_NORMAL"; }
    else
    {
        std::stringstream ss;
        ss  << std::hex << fileattrib;
        result = widen( ss.str());
    }
    return result;
}


int RunACreateFileTest(MfrCreateFileTest testInput, int setnum)
{
    // Just open and close the file
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "CreateFile File Test (Set";
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


        std::wstring detail1 = L"       File:";
        detail1.append(testInput.TestPath.c_str());
        detail1.append(L"\n");
        trace_message(detail1.c_str(), info_color);

        std::wstring detail2 = L"       DesiredAccess:";
        detail2.append(creation_fileaccess_text(testInput.dwDesiredAccess).c_str());
        detail2.append(L"\n");
        trace_message(detail2.c_str(), info_color);
        
        std::wstring detail3 = L"       Share:";
        detail3.append(creation_share_text(testInput.dwShareMode));
        detail3.append(L"\n");
        trace_message(detail3.c_str(), info_color);

        std::wstring detail4 = L"       CreationDisposition:";
        detail4.append(creation_disposition_text(testInput.CreationDisposition));
        detail4.append(L"\n");
        trace_message(detail4.c_str(), info_color);

        std::wstring detail5 = L"       FlagsAndAttributes:";
        detail5.append(fileattrib_text(testInput.FlagAndAttributes));
        detail5.append(L"\n");
        trace_message(detail5.c_str(), info_color);

        std::wstring detail6 = L"       Expect:";
        if (testInput.Expected_Result)
        {
            detail6.append(L" opened.\n");
        }
        else
        {
            detail6.append(L" not opened=");
            detail6.append(std::to_wstring(testInput.Expected_LastError));
            detail6.append(L"\n");
        }
        trace_message(detail6.c_str(), info_color);

        auto testResult = CreateFile(testInput.TestPath.c_str(),
            testInput.dwDesiredAccess,
            testInput.dwShareMode,
            testInput.lpSecurityAttributes,
            testInput.CreationDisposition,
            testInput.FlagAndAttributes,
            nullptr);
        auto eCode = GetLastError();
        if (testResult != INVALID_HANDLE_VALUE)
        {
            // call success
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
            CloseHandle(testResult);
        }
        else
        {
            // call failed
            if (!testInput.Expected_Result)
            {
                // expected fail, but is it for the right reason?
                if (testInput.Expected_LastError == eCode)
                {
                    // as expected
                    result = ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    // expected a failure but with different cause
                    trace_message(L"ERROR: Expected fail but error incorrect\n", error_color);
                    std::wstring detail = L"       Intended: Error=";
                    detail.append(std::to_wstring(testInput.Expected_LastError));
                    detail.append(L" Error=");
                    detail.append(std::to_wstring(eCode));
                    detail.append(L"\n");
                    trace_message(detail.c_str(), info_color);
                    result = eCode;
                    test_end(eCode);
                }
            }
            else
            {
                // unexpected fail
                trace_message(L"ERROR: Unexpected fail\n", error_color);
                std::wstring detail = L"       Error=";
                detail.append(std::to_wstring(eCode));
                detail.append(L"\n");
                trace_message(detail.c_str(), info_color);
                result = eCode;
                test_end(eCode);
            }
        }
    }
    return result;
}


int RunACreateFile2Test(MfrCreateFile2Test testInput, int setnum)
{
    // Just open and close the file
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {

        CREATEFILE2_EXTENDED_PARAMETERS ExtParms {
               sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
               testInput.Attributes,
               testInput.Flags,
               testInput.dwSecurityQosFlags,
               nullptr,
               nullptr
        };
        std::string testname = "CreateFile2 File Test (Set";
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


        std::wstring detail1 = L"       File:";
        detail1.append(testInput.TestPath.c_str());
        detail1.append(L"\n");
        trace_message(detail1.c_str(), info_color);

        std::wstring detail2 = L"       DesiredAccess:";
        detail2.append(creation_fileaccess_text(testInput.dwDesiredAccess).c_str());
        detail2.append(L"\n");
        trace_message(detail2.c_str(), info_color);

        std::wstring detail3 = L"       Share:";
        detail3.append(creation_share_text(testInput.dwShareMode));
        detail3.append(L"\n");
        trace_message(detail3.c_str(), info_color);

        std::wstring detail4 = L"       CreationDisposition:";
        detail4.append(creation_disposition_text(testInput.CreationDisposition));
        detail4.append(L"\n");
        trace_message(detail4.c_str(), info_color);

        std::wstring detail5 = L"       Attributes:";
        detail5.append(fileattrib_text(testInput.Attributes));
        detail5.append(L"\n");
        trace_message(detail5.c_str(), info_color);

        std::wstring detail6 = L"       Expect:";
        if (testInput.Expected_Result)
        {
            detail6.append(L" opened.\n");
        }
        else
        {
            detail6.append(L" not opened=");
            detail6.append(std::to_wstring(testInput.Expected_LastError));
            detail6.append(L"\n");
        }
        trace_message(detail6.c_str(), info_color);

        auto testResult = CreateFile2(testInput.TestPath.c_str(),
            testInput.dwDesiredAccess,
            testInput.dwShareMode,
            testInput.CreationDisposition,
            &ExtParms);
        auto eCode = GetLastError();
        if (testResult != INVALID_HANDLE_VALUE)
        {
            // call success
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
            CloseHandle(testResult);
        }
        else
        {
            // call failed
            if (!testInput.Expected_Result)
            {
                // expected fail, but is it for the right reason?
                if (testInput.Expected_LastError == eCode)
                {
                    // as expected
                    result = ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
                else
                {
                    // expected a failure but with different cause
                    trace_message(L"ERROR: Expected fail but error incorrect\n", error_color);
                    std::wstring detail = L"       Intended: Error=";
                    detail.append(std::to_wstring(testInput.Expected_LastError));
                    detail.append(L" Error=");
                    detail.append(std::to_wstring(eCode));
                    detail.append(L"\n");
                    trace_message(detail.c_str(), info_color);
                    result = eCode;
                    test_end(eCode);
                }
            }
            else
            {
                // unexpected fail
                trace_message(L"ERROR: Unexpected fail\n", error_color);
                std::wstring detail = L"       Error=";
                detail.append(std::to_wstring(eCode));
                detail.append(L"\n");
                trace_message(detail.c_str(), info_color);
                result = eCode;
                test_end(eCode);
            }
        }
    }
    return result;
}


int RunCreateFileTests()
{

    int result = ERROR_SUCCESS;
    for (MfrCreateFileTest testInput : MfrCreateFileTests1)
    {
        int tresult = RunACreateFileTest(testInput,1);
        result = result ? result : tresult;
    }
    for (MfrCreateFileTest testInput : MfrCreateFileTests2)
    {
        int tresult = RunACreateFileTest(testInput, 2);
        result = result ? result : tresult;
    }

    for (MfrCreateFileTest testInput : MfrCreateFileTests3)
    {
        int tresult = RunACreateFileTest(testInput, 3);
        result = result ? result : tresult;
    }

    for (MfrCreateFileTest testInput : MfrCreateFileTests4)
    {
        int tresult = RunACreateFileTest(testInput, 4);
        result = result ? result : tresult;
    }

    for (MfrCreateFileTest testInput : MfrCreateFileTests5)
    {
        int tresult = RunACreateFileTest(testInput, 5);
        result = result ? result : tresult;
    }

    for (MfrCreateFileTest testInput : MfrCreateFileTests6)
    {
        int tresult = RunACreateFileTest(testInput, 6);
        result = result ? result : tresult;
    }


    for (MfrCreateFile2Test testInput : MfrCreateFile2Tests1)
    {
        int tresult = RunACreateFile2Test(testInput, 1);
        result = result ? result : tresult;
    }


    return result;
}
