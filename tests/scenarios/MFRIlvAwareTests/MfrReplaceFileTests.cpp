//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrReplaceFileTests.h"
#include "MfrConsts.h"
#include "MfrCleanup.h"
#include <thread>
#include <cstdlib>


std::vector<MfrReplaceFileTest> MfrReplaceFileTests1;

std::vector<MfrReplaceFileTest> MfrReplaceFileTests2;


int InitializeReplaceFileTest1()
{
    std::wstring tempReplaced;
    std::wstring tempReplacement;
    std::wstring tempNoFile;
    std::wstring tempNoPath;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    std::wstring NullBackup;

    // Requests to Native File Locations for ReplaceFile via VFS
    MfrReplaceFileTest ts_Native_PF1 = { "MFR+ILV ReplaceFile Native-file VFS both exist in package (allowed)",             
                                        true, true,  true,
                                        (g_NativePF + L"\\PlaceholderTestR\\TestIniFileVfsPF1.ini"),
                                        (g_NativePF + L"\\PlaceholderTestR\\TestIniFileVfsPFGoodSource.txt"),
                                        NullBackup, 0,
                                        true, ERROR_SUCCESS };
    MfrReplaceFileTests1.push_back(ts_Native_PF1);

    MfrReplaceFileTest ts_Native_PF1E1 = { "MFR+ILV ReplaceFile Native-file VFS folder exists in package with missing file (not allowed)", true, false, false,
                                        (g_NativePF + L"\\PlaceholderTestR\\TestIniFileVfsPF2.ini"),
                                        (g_NativePF + L"\\PlaceholderTestR\\MissingTestIniFileVfsPF2.ini"),
                                        NullBackup, 0,
                                        false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Native_PF1E1);

    MfrReplaceFileTest ts_Native_PF1E2 = { "MFR+ILV ReplaceFile Native-file VFS exists in package with missing folder (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestR\\TestIniFileVfsPF3.ini"),
                                    (g_NativePF + L"\\PlaceholderTestR\\NoSuchFolder3\\CopiedTestIniFileVfsPF3.ini"),
                                    NullBackup, 0,
                                    false, ERROR_PATH_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Native_PF1E2);

    MfrReplaceFileTest t_Native_PF2 = { "MFR+ILV ReplaceFile Native-file VFS missing in package with file (not allowed)",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestR\\MissingNativePlaceholder4.txt"),
                                    (g_NativePF + L"\\PlaceholderTestR\\TestIniFileVfsPFGoodSource.txt"),
                                    NullBackup, 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Native_PF2);

    MfrReplaceFileTest t_Native_PF3 = { "MFR+ILV ReplaceFile Native-file VFS parent-folder missing in package with missing path-file (not allowed)",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTestR\\NoSuchFolder5A\\MissingNativePlaceholder5A.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTestR\\NoSuchFolder5B\\CopiedMissingNarivePlaceholder5B.txt"),
                                    NullBackup, 0,
                                    false, ERROR_PATH_NOT_FOUND, true, ERROR_FILE_NOT_FOUND };  // Natively this would have been path not found, but this is reasonable.
    MfrReplaceFileTests1.push_back(t_Native_PF3);


    // Requests to Package File Locations for ReplaceFile using VFS
#if _M_IX86
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#else
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestR\\Placeholder6.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestR\\TestIniFileVfsPF6.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestR\\NoneSuchFile6.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestR\\NoSuchPath6\\NoneSuchFile6.ini";
#endif
    MfrReplaceFileTest t_Vfs_PF1 = { "MFR+ILV ReplaceFile Package-file VFS both exist in package (allowed)",                      true, true,  true,
                                tempReplaced, tempReplacement, NullBackup, 0, true, ERROR_SUCCESS };
    MfrReplaceFileTests1.push_back(t_Vfs_PF1);

    MfrReplaceFileTest ts_Vfs_PF1E1 = { "MFR+ILV ReplaceFile Package-file VFS exists in package with missing file (not allowed)", true, true, true,
                                tempReplaced, tempNoFile, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Vfs_PF1E1);


    MfrReplaceFileTest t_Vfs_PF2 = { "MFR+ILV ReplaceFile Package-file VFS missing in package with file (not allowed)",              true, true, true,
                                tempNoFile, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Vfs_PF2);

    MfrReplaceFileTest t_Vfs_PF3 = { "MFR+ILV ReplaceFile Package-file VFS parent-folder missing in package with file (not allowed)",            true, true, true,
                                tempNoPath, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND,
                                                                            true, ERROR_PATH_NOT_FOUND};
    MfrReplaceFileTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for FileReplace using VFS
#if _M_IX86
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX86\\PlaceholderTestR\\Placeholder7.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestR\\TestIniFileVfsPF7.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestR\\NoneSuchFile7.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestR\\NoSuchPath7\\NoneSuchFile7.ini";
#else
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX64\\PlaceholderTestR\\Placeholder7.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestR\\TestIniFileVfsPF7.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestR\\NoneSuchFile7.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestR\\NoSuchPath7\\NoneSuchFile7.ini";
#endif
    MfrReplaceFileTest t_Redir_PF1 = { "MFR+ILV ReplaceFile Redirected-file VFS exists in package with package file (allowed)",      true, true,  true,
                                tempReplaced, tempReplacement, NullBackup, 0, true, ERROR_SUCCESS };
    MfrReplaceFileTests1.push_back(t_Redir_PF1);

    MfrReplaceFileTest ts_Redir_PF1E1 = { "MFR+ILV ReplaceFile Redirected Package-file VFS exists in package with missing file (not allowed)", true, true, true,
                                tempReplaced, tempNoFile, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Redir_PF1E1);

    MfrReplaceFileTest ts_Redir_PF1E2 = { "MFR+ILV ReplaceFile Redirected Package-file VFS exists in package with missing folder (not allowed)", true, true, true,
                                tempReplaced, tempNoPath, NullBackup, 0, false, ERROR_PATH_NOT_FOUND,
                                                                          true, ERROR_FILE_NOT_FOUND};
    MfrReplaceFileTests1.push_back(ts_Redir_PF1E2);

    MfrReplaceFileTest t_Redir_PF2E1 = { "MFR+ILV ReplaceFile Redirected Package-file VFS missing in package with file (not allowed)",       true, true, true,
                                tempNoFile, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Redir_PF2E1);

    MfrReplaceFileTest t_Redir_PF2E2 = { "MFR+ILV ReplaceFile Redirected Package-file VFS missing folder in package with file (not allowed)",       true, true, true,
                                tempNoPath, tempReplacement, NullBackup, 0, false, ERROR_PATH_NOT_FOUND, true, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Redir_PF2E2);


    // Local Documents test
    tempReplaced = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_RF1.txt";
    tempReplacement = psf::known_folder(FOLDERID_Documents);
    tempReplacement.append(L"\\");
    tempReplacement.append(MFRTESTDOCS);
    tempReplacement.append(L"\\Copy_RF1\\CopiedPresonalFile_RF1.txt");
    MfrReplaceFileTest t_LocalDoc_1 = { "MFR+ILV ReplaceFile Package-file VFS exists with missing path Local Documents  (not allowed)",              true, true,  true,
                                tempReplaced, tempReplacement, NullBackup, 0, false, ERROR_PATH_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_LocalDoc_1);

    MfrReplaceFileTest t_LocalDoc_2 = { "MFR+ILV ReplaceFile Package-file VFS exists with missing file Local Documents (not allowed)",                 true, true, true,
                                tempReplaced, tempNoFile, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_LocalDoc_2);

    int count = 0;
    for (MfrReplaceFileTest t : MfrReplaceFileTests1)      if (t.enabled) { count++; }
    return count;
}


int InitializeReplaceFileTest2()
{
    std::wstring tempReplaced;
    std::wstring tempReplacement;
    std::wstring tempNoFile;
    std::wstring tempNoPath;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    // Requests to Native File Locations for ReplaceFile via VFS
    MfrReplaceFileTest ts_Native_PF1 = { "MFR+ILV ReplaceFile w/Backup Native-file VFS both exist in package, no Backup exists (allowed)",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest_R2\\TestIniFileVfsPF_R21.ini"),
                                    (g_NativePF + L"\\PlaceholderTest_R2\\TestIniFileVfsPFGoodSource.ini"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R21.ini"), 0,
                                    true, ERROR_SUCCESS };
    MfrReplaceFileTests2.push_back(ts_Native_PF1);

    MfrReplaceFileTest ts_Native_PF1E1 = { "MFR+ILV ReplaceFile w/Backup Native-file VFS both exist in package, backup exists (allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest_R2\\TestIniFileVfsPF_R22.ini"),
                                    (g_NativePF + L"\\PlaceholderTest_R2\\TestIniFileVfsPFGoodSource.ini"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R21.ini"), 0,
                                    true, ERROR_SUCCESS };
    MfrReplaceFileTests2.push_back(ts_Native_PF1E1);

    MfrReplaceFileTest ts_Native_PF1E2 = { "MFR+ILV ReplaceFile w/Backup Native-file VFS dest exists in package with missing source folder (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest_R2\\TestIniFileVfsPF_R23.ini"),
                                    (g_NativePF + L"\\PlaceholderTest_R2\\NoSuchFolder_R2\\CopiedTestIniFileVfsPF_R23.ini"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R23.ini"), 0,
                                    false, ERROR_PATH_NOT_FOUND,
                                    true, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Native_PF1E2);

    MfrReplaceFileTest t_Native_PF2 = { "MFR+ILV ReplaceFile w/Backup Native-file VFS missing file in source and dest in package  (not allowed)",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest_R2\\MissingNativePlaceholder_R24.txt"),
                                    (g_NativePF + L"\\PlaceholderTest_R2\\MissingNativePlaceholder_R24.txt"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R24.ini"), 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Native_PF2);

    MfrReplaceFileTest t_Native_PF3 = { "MFR+ILV ReplaceFile w/Backup Native-file VFS missing parent-folder in source and dest in package (not allowed)",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest_R2\\MissingNativePlaceholder_R25.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest_R2\\CopiedMissingNarivePlaceholder_R25.txt"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R25.ini"), 0,
                                    false, ERROR_PATH_NOT_FOUND,
                                    true, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Native_PF3);


    // Requests to Package File Locations for ReplaceFile using VFS
#if _M_IX86
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\Placeholder_R26.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\TestIniFileVfsPFGoodSource.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\NoneSuchFile_R26.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\NoSuchPath_R2\\NoneSuchFile_R26.ini";
#else
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest_R2\\Placeholder_R26.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest_R2\\TestIniFileVfsPFGoodSource.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest_R2\\NoneSuchFile_R26.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\NoSuchPath_R2\\NoneSuchFile_R26.ini";
#endif
    MfrReplaceFileTest t_Vfs_PF1 = { "MFR+ILV ReplaceFile w/Backup Package-file VFS both exist in package (allowed)",                           true, true,  true,
                                tempReplaced, tempReplacement, 
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R26.ini"), 0,
                                true, ERROR_SUCCESS };
    MfrReplaceFileTests2.push_back(t_Vfs_PF1);

    MfrReplaceFileTest ts_Vfs_PF1E1 = { "MFR+ILV ReplaceFile w/Backup Package-file VFS exists in package with missing file (not allowed)", true, false, false,
                                tempReplaced, tempNoFile, 
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R27.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Vfs_PF1E1);


    MfrReplaceFileTest t_Vfs_PF2 = { "MFR+ILV ReplaceFile w/Backup Package-file VFS missing in package with file (not allowed)",                          true, true, true,
                                tempNoFile, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R28.ini"), 0, 
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Vfs_PF2);

    MfrReplaceFileTest t_Vfs_PF3 = { "MFR+ILV ReplaceFile w/Backup Package-file VFS parent-folder missing in package with file (not allowed)",            true, true, true,
                                tempNoPath, tempReplacement, 
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R29.ini"), 0,
                                false, ERROR_PATH_NOT_FOUND,
                                    true, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for ReplaceFile using VFS
#if _M_IX86
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\Placeholder_R30.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\TestIniFileVfsPFGoodSource.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\NoneSuchFile_R30.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest_R2\\NoSuchPath_R2\\NoneSuchFile_R30.ini";
#else
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX64\\PlaceholderTest_R2\\Placeholder_R30.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest_R2\\TestIniFileVfsPFGoodSource.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest_R2\\NoneSuchFile_R30.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest_R2\\NoSuchPath_R2\\NoneSuchFile_R30.ini";
#endif
    MfrReplaceFileTest t_Redir_PF1 = { "MFR+ILV ReplaceFile w/Backup Redirected-file VFS exists in package with package file (allowed)",      true, true,  true,
                                tempReplaced, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R30.ini"), 0,
                                true, ERROR_SUCCESS };
    MfrReplaceFileTests2.push_back(t_Redir_PF1);

    MfrReplaceFileTest ts_Redir_PF1E1 = { "MFR+ILV ReplaceFile w/Backup Redirected Package-file VFS exists in package with missing file (not allowed)", true, true, true,
                                tempReplaced, tempNoFile,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R31.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Redir_PF1E1);

    MfrReplaceFileTest ts_Redir_PF1E2 = { "MFR+ILV ReplaceFile w/Backup Redirected Package-file VFS exists in package with missing folder (not allowed)", true, true, true,
                                tempReplaced, tempNoPath,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R32.ini"), 0,
                                false, ERROR_PATH_NOT_FOUND,
                                    true, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Redir_PF1E2);

    MfrReplaceFileTest t_Redir_PF2E1 = { "MFR+ILV ReplaceFile w/Backup Redirected Package-file VFS missing in package with file (not allowed)",       true, true, true,
                                tempNoFile, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R33.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Redir_PF2E1);

    MfrReplaceFileTest t_Redir_PF2E2 = { "MFR+ILV ReplaceFile w/Backup Redirected Package-file VFS missing folder in package with file (not allowed)",       true, true, true,
                                tempNoPath, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF_R34.ini"), 0,
                                false, ERROR_PATH_NOT_FOUND,
                                    true, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Redir_PF2E2);


    // Local Documents test
    tempReplaced = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_RF2.txt";
    tempReplacement = psf::known_folder(FOLDERID_Documents);
    tempReplacement.append(L"\\");
    tempReplacement.append(MFRTESTDOCS);
    tempReplacement.append(L"\\Copy_RF2\\CopiedPresonalFile_FR2.txt");
    MfrReplaceFileTest t_LocalDoc_1 = { "MFR+ILV ReplaceFile w/Backup Package-file VFS exists with missing path Local Documents  (not allowed)",              true, true,  true,
                                tempReplaced, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_PATH_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_LocalDoc_1);

    MfrReplaceFileTest t_LocalDoc_2 = { "MFR+ILV ReplaceFile w/Backup Package-file VFS exists with missing file Local Documents (not allowed)",                 true, true, true,
                                tempReplaced, tempNoFile,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_LocalDoc_2);


    int count = 0;
    for (MfrReplaceFileTest t : MfrReplaceFileTests2)      if (t.enabled) { count++; }
    return count;

}

int InitializeReplaceFileTests()
{
    int count = 0;
    count += InitializeReplaceFileTest1();
    count += InitializeReplaceFileTest2();
    return count;
}


int RunReplaceFileTests()
{

    int result = ERROR_SUCCESS;

    for (MfrReplaceFileTest testInput : MfrReplaceFileTests1)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR+ILV ReplaceFile File Test (#1): ";
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

            auto testResult = ReplaceFile(testInput.TestPathReplaced.c_str(), testInput.TestPathReplacement.c_str(),
                                          testInput.TestPathBackup.c_str(), testInput.ReplaceFlags, nullptr, nullptr);
            auto eCode = GetLastError();
            std::wstring GetCode = L"   GetLastError=";
            GetCode.append(std::to_wstring(eCode));
            GetCode.append(L"\n");
            trace_message(GetCode.c_str(), info_color);
            if (testResult == 0)
            {
                // return was failure
                if (!testInput.Expected_Success)
                {
                    // failure was expected
                    if (eCode == testInput.Expected_LastError ||
                        (testInput.AllowAlternateError && testInput.AlternateError == eCode))
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Replace failed as expected, but with incorrect error.\n", error_color);
                        std::wstring detail1 = L"     Expected Error=";
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
                        if (testInput.AllowAlternateError)
                        {
                            detail1.append(L" or ");
                            detail1.append(std::to_wstring(testInput.AlternateError));
                        }
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), info_color);
                        std::wstring detail2 = L"       Actual Error=";
                        detail2.append(std::to_wstring(eCode));
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), error_color);

                        result = result ? result : eCode;
                        test_end(eCode);
                    }
                }
                else
                {
                    trace_message(L"ERROR: Replace failed unexpectedly, with incorrect error.\n", error_color);
                    std::wstring detail1 = L"     Expected Error=";
                    detail1.append(std::to_wstring(testInput.Expected_LastError));
                    if (testInput.AllowAlternateError)
                    {
                        detail1.append(L" or ");
                        detail1.append(std::to_wstring(testInput.AlternateError));
                    }
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual Error=";
                    detail2.append(std::to_wstring(eCode));
                    detail2.append(L"\n");
                    trace_message(detail2.c_str(), error_color);
                    result = result ? result : eCode;
                    test_end(eCode);
                }
            }
            else
            {
                // return was success
                if (!testInput.Expected_Success)
                {
                    trace_message(L"ERROR: Replace unexpected successful return.\n", error_color);
                    std::wstring detail1 = L"       Intended: return=";
                    detail1.append(std::to_wstring(!testInput.Expected_Success));
                    detail1.append(L" Error=");
                    detail1.append(std::to_wstring(testInput.Expected_LastError));
                    if (testInput.AllowAlternateError)
                    {
                        detail1.append(L" or ");
                        detail1.append(std::to_wstring(testInput.AlternateError));
                    }
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: return=";
                    detail2.append(std::to_wstring(testResult));
                    detail2.append(L" Error=");
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
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
        }
    }

    for (MfrReplaceFileTest testInput : MfrReplaceFileTests2)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR+ILV ReplaceFile File Test (#2): ";
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

            auto testResult = ReplaceFile(testInput.TestPathReplaced.c_str(), testInput.TestPathReplacement.c_str(),
                testInput.TestPathBackup.c_str(), testInput.ReplaceFlags, nullptr, nullptr);
            auto eCode = GetLastError();
            std::wstring GetCode = L"   GetLastError=";
            GetCode.append(std::to_wstring(eCode));
            GetCode.append(L"\n");
            trace_message(GetCode.c_str(), info_color);
            if (testResult == 0)
            {
                // return was failure
                if (!testInput.Expected_Success)
                {
                    // failure was expected
                    if (eCode == testInput.Expected_LastError ||
                        (testInput.AllowAlternateError && testInput.AlternateError == eCode))
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Replace failed, but with incorrect error.\n", error_color);
                        std::wstring detail1 = L"     Expected Error=";
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
                        if (testInput.AllowAlternateError)
                        {
                            detail1.append(L" or ");
                            detail1.append(std::to_wstring(testInput.AlternateError));
                        }
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
                }
                else
                {
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                // return was success
                if (!testInput.Expected_Success)
                {
                    trace_message(L"ERROR: Replace unexpected return value.\n", error_color);
                    std::wstring detail1 = L"       Intended: return=";
                    detail1.append(std::to_wstring(!testInput.Expected_Success));
                    detail1.append(L" Error=");
                    detail1.append(std::to_wstring(testInput.Expected_LastError));
                    if (testInput.AllowAlternateError)
                    {
                        detail1.append(L" or ");
                        detail1.append(std::to_wstring(testInput.AlternateError));
                    }
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: return=";
                    detail2.append(std::to_wstring(testResult));
                    detail2.append(L" Error=");
                    detail2.append(std::to_wstring(eCode));
                    detail2.append(L"\n");
                    trace_message(detail2.c_str(), error_color);

                    result = result ? result : eCode;
                    test_end(eCode);
                }
                else
                {
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
        }
    }


    return result;
}
