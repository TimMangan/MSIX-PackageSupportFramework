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
    MfrReplaceFileTest ts_Native_PF1 = { "MFR ReplaceFile Native-file VFS both exist in package (allowed)",             
                                        true, true,  true,
                                        (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                        (g_NativePF + L"\\PlaceholderTest\\Placeholder.txt"),
                                        NullBackup, 0,
                                        true, ERROR_SUCCESS };
    MfrReplaceFileTests1.push_back(ts_Native_PF1);

    MfrReplaceFileTest ts_Native_PF1E1 = { "MFR ReplaceFile Native-file VFS exists in package with missing file (not allowed)", true, false, false,
                                        (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                        (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                        NullBackup, 0,
                                        false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Native_PF1E1);

    MfrReplaceFileTest ts_Native_PF1E2 = { "MFR ReplaceFile Native-file VFS exists in package with missing folder (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\NoSuchFolder\\CopiedTestIniFileVfsPF.ini"),
                                    NullBackup, 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Native_PF1E2);

    MfrReplaceFileTest t_Native_PF2 = { "MFR ReplaceFile Native-file VFS missing in package with file (not allowed)",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\PlaceholderTest\\Placeholder.txt"),
                                    NullBackup, 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Native_PF2);

    MfrReplaceFileTest t_Native_PF3 = { "MFR ReplaceFile Native-file VFS parent-folder missing in package with missing path-file (not allowed)",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder.txt"),
                                    NullBackup, 0,
                                    false, ERROR_FILE_NOT_FOUND };  // Natively this would have been path not found, but this is reasonable.
    MfrReplaceFileTests1.push_back(t_Native_PF3);


    // Requests to Package File Locations for ReplaceFile using VFS
#if _M_IX86
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#else
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#endif
    MfrReplaceFileTest t_Vfs_PF1 = { "MFR ReplaceFile Package-file VFS both exist in package (allowed)",                      true, true,  true,
                                tempReplaced, tempReplacement, NullBackup, 0, true, ERROR_SUCCESS };
    MfrReplaceFileTests1.push_back(t_Vfs_PF1);

    MfrReplaceFileTest ts_Vfs_PF1E1 = { "MFR ReplaceFile Package-file VFS exists in package with missing file (not allowed)", true, true, true,
                                tempReplaced, tempNoFile, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Vfs_PF1E1);


    MfrReplaceFileTest t_Vfs_PF2 = { "MFR ReplaceFile Package-file VFS missing in package with file (not allowed)",              true, true, true,
                                tempNoFile, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Vfs_PF2);

    MfrReplaceFileTest t_Vfs_PF3 = { "RMFR eplaceFile Package-file VFS parent-folder missing in package with file (not allowed)",            true, true, true,
                                tempNoPath, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for FileReplace using VFS
#if _M_IX86
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#else
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#endif
    MfrReplaceFileTest t_Redir_PF1 = { "MFR ReplaceFile Redirected-file VFS exists in package with package file (allowed)",      true, true,  true,
                                tempReplaced, tempReplacement, NullBackup, 0, true, ERROR_SUCCESS };
    MfrReplaceFileTests1.push_back(t_Redir_PF1);

    MfrReplaceFileTest ts_Redir_PF1E1 = { "MFR ReplaceFile Redirected Package-file VFS exists in package with missing file (not allowed)", true, true, true,
                                tempReplaced, tempNoFile, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Redir_PF1E1);

    MfrReplaceFileTest ts_Redir_PF1E2 = { "MFR ReplaceFile Redirected Package-file VFS exists in package with missing folder (not allowed)", true, true, true,
                                tempReplaced, tempNoPath, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(ts_Redir_PF1E2);

    MfrReplaceFileTest t_Redir_PF2E1 = { "MFR ReplaceFile Redirected Package-file VFS missing in package with file (not allowed)",       true, true, true,
                                tempNoFile, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Redir_PF2E1);

    MfrReplaceFileTest t_Redir_PF2E2 = { "MFR ReplaceFile Redirected Package-file VFS missing folder in package with file (not allowed)",       true, true, true,
                                tempNoPath, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_Redir_PF2E2);


    // Local Documents test
    tempReplaced = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile.txt";
    tempReplacement = psf::known_folder(FOLDERID_Documents);
    tempReplacement.append(L"\\");
    tempReplacement.append(MFRTESTDOCS);
    tempReplacement.append(L"\\Copy\\CopiedPresonalFile.txt");
    MfrReplaceFileTest t_LocalDoc_1 = { "MFR ReplaceFile Package-file VFS exists with missing path Local Documents  (not allowed)",              true, true,  true,
                                tempReplaced, tempReplacement, NullBackup, 0, false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests1.push_back(t_LocalDoc_1);

    MfrReplaceFileTest t_LocalDoc_2 = { "MFR ReplaceFile Package-file VFS exists with missing file Local Documents (not allowed)",                 true, true, true,
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
    MfrReplaceFileTest ts_Native_PF1 = { "MFR ReplaceFile w/Backup Native-file VFS both exist in package (allowed)",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\Placeholder.txt.ini"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                    true, ERROR_SUCCESS };
    MfrReplaceFileTests2.push_back(ts_Native_PF1);

    MfrReplaceFileTest ts_Native_PF1E1 = { "MFR ReplaceFile w/Backup Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Native_PF1E1);

    MfrReplaceFileTest ts_Native_PF1E2 = { "MFR ReplaceFile w/Backup Native-file VFS exists in package with missing folder (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\NoSuchFolder\\CopiedTestIniFileVfsPF.ini"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Native_PF1E2);

    MfrReplaceFileTest t_Native_PF2 = { "MFR ReplaceFile w/Backup Native-file VFS missing in package with file (not allowed)",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\PlaceholderTest\\Placeholder.txt"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Native_PF2);

    MfrReplaceFileTest t_Native_PF3 = { "MFR ReplaceFile w/Backup Native-file VFS parent-folder missing in package with missing path-file (not allowed)",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder.txt"),
                                    (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                    false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Native_PF3);


    // Requests to Package File Locations for ReplaceFile using VFS
#if _M_IX86
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#else
    tempReplaced = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#endif
    MfrReplaceFileTest t_Vfs_PF1 = { "MFR ReplaceFile w/Backup Package-file VFS both exist in package (allowed)",                           true, true,  true,
                                tempReplaced, tempReplacement, 
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                true, ERROR_SUCCESS };
    MfrReplaceFileTests2.push_back(t_Vfs_PF1);

    MfrReplaceFileTest ts_Vfs_PF1E1 = { "MFR ReplaceFile w/Backup Package-file VFS exists in package with missing file (not allowed)", true, false, false,
                                tempReplaced, tempNoFile, 
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Vfs_PF1E1);


    MfrReplaceFileTest t_Vfs_PF2 = { "MFR ReplaceFile w/Backup Package-file VFS missing in package with file (not allowed)",                          true, true, true,
                                tempNoFile, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0, 
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Vfs_PF2);

    MfrReplaceFileTest t_Vfs_PF3 = { "MFR ReplaceFile w/Backup Package-file VFS parent-folder missing in package with file (not allowed)",            true, true, true,
                                tempNoPath, tempReplacement, 
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for ReplaceFile using VFS
#if _M_IX86
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#else
    tempReplaced = (std::wstring)g_writablePackageRootPath.c_str() + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempReplacement = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\TestIniFileVfsPF.ini";
    tempNoFile = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoneSuchFile.ini";
    tempNoPath = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\NoSuchPath\\NoneSuchFile.ini";
#endif
    MfrReplaceFileTest t_Redir_PF1 = { "MFR ReplaceFile w/Backup Redirected-file VFS exists in package with package file (allowed)",      true, true,  true,
                                tempReplaced, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                true, ERROR_SUCCESS };
    MfrReplaceFileTests2.push_back(t_Redir_PF1);

    MfrReplaceFileTest ts_Redir_PF1E1 = { "MFR ReplaceFile w/Backup Redirected Package-file VFS exists in package with missing file (not allowed)", true, true, true,
                                tempReplaced, tempNoFile,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Redir_PF1E1);

    MfrReplaceFileTest ts_Redir_PF1E2 = { "MFR ReplaceFile w/Backup Redirected Package-file VFS exists in package with missing folder (not allowed)", true, true, true,
                                tempReplaced, tempNoPath,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(ts_Redir_PF1E2);

    MfrReplaceFileTest t_Redir_PF2E1 = { "MFR ReplaceFile w/Backup Redirected Package-file VFS missing in package with file (not allowed)",       true, true, true,
                                tempNoFile, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Redir_PF2E1);

    MfrReplaceFileTest t_Redir_PF2E2 = { "MFR ReplaceFile w/Backup Redirected Package-file VFS missing folder in package with file (not allowed)",       true, true, true,
                                tempNoPath, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_Redir_PF2E2);


    // Local Documents test
    tempReplaced = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile.txt";
    tempReplacement = psf::known_folder(FOLDERID_Documents);
    tempReplacement.append(L"\\");
    tempReplacement.append(MFRTESTDOCS);
    tempReplacement.append(L"\\Copy\\CopiedPresonalFile.txt");
    MfrReplaceFileTest t_LocalDoc_1 = { "MFR ReplaceFile w/Backup Package-file VFS exists with missing path Local Documents  (not allowed)",              true, true,  true,
                                tempReplaced, tempReplacement,
                                (REVFSPF + L"\\VFS\\Local AppData\\Backup\\TestIniFileVfsPF.ini"), 0,
                                false, ERROR_FILE_NOT_FOUND };
    MfrReplaceFileTests2.push_back(t_LocalDoc_1);

    MfrReplaceFileTest t_LocalDoc_2 = { "MFR ReplaceFile w/Backup Package-file VFS exists with missing file Local Documents (not allowed)",                 true, true, true,
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
            std::string testname = "MFR ReplaceFile File Test (#1): ";
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
                    if (eCode == testInput.Expected_LastError)
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Replace failed as expected, but with incorrect error.\n", error_color);
                        std::wstring detail1 = L"     Expected Error=";
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
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
            std::string testname = "MFR ReplaceFile File Test (#2): ";
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
                    if (eCode == testInput.Expected_LastError)
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Replace failed, but with incorrect error.\n", error_color);
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
