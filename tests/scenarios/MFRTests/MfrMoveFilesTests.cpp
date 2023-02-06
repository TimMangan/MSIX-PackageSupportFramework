//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrMoveFilesTests.h"
#include "MfrConsts.h"
#include "MfrCleanup.h"
#include <thread>
#include <cstdlib>


std::vector<MfrMoveFileTest> MfrMoveFileTests1;

std::vector<MfrMoveFileTest> MfrMoveFileTests2;


std::vector<MfrMoveFileExTest> MfrMoveFileExTests1;

std::vector<MfrMoveFileExTest> MfrMoveFileExTests2;

std::vector<MfrMoveFileExTest> MfrMoveFileExTests3;


std::vector<MfrMoveFileWithProgressTest> MfrMoveFileWithProgressTests1;

int InitializeMoveFileTest1()
{
    std::wstring tempS;
    std::wstring tempD;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    // Requests to Native File Locations for CopyFile via VFS
    MfrMoveFileTest ts_Native_PF1 = { "MFR MoveFile Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(ts_Native_PF1);

    MfrMoveFileTest ts_Native_PF1E1 = { "MFR MoveFile Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    false, ERROR_FILE_EXISTS };
    MfrMoveFileTests1.push_back(ts_Native_PF1E1);

    MfrMoveFileTest ts_Native_PF1E2 = { "MFR MoveFile Native-file VFS exists in package and not dest subdir (allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\NoSuchFolder\\CopiedTestIniFileVfsPF.ini"),
                                    true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(ts_Native_PF1E2);

    MfrMoveFileTest t_Native_PF2 = { "MFR MoveFile Native-file VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedMissingNativePlaceholder.txt"),
                                    false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Native_PF2);

    MfrMoveFileTest t_Native_PF3 = { "MFR MoveFile Native-file VFS parent-folder missing in package",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder.txt"),
                                    false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Native_PF3);


    // Requests to Package File Locations for MoveFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedPlaceholder.txt";
#endif
    MfrMoveFileTest t_Vfs_PF1 = { "MFR MoveFile Package-file VFS exists in package",                           true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(t_Vfs_PF1);

    MfrMoveFileTest ts_Vfs_PF1E1 = { "MFR MoveFile Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests1.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileTest t_Vfs_PF2 = { "MFR MoveFile Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileTest t_Vfs_PF3 = { "MFR MoveFile Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for CopyFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#endif
    MfrMoveFileTest t_Redir_PF1 = { "MFR MoveFile Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(t_Redir_PF1);

    MfrMoveFileTest ts_Redir_PF1E1 = { "MFR MoveFile Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests1.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileTest t_Redir_PF2 = { "MFR MoveFile Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileTest t_Redir_PF3 = { "MFR MoveFile Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\Copy\\CopiedPresonalFile.txt");
    MfrMoveFileTest t_LocalDoc_1 = { "MFR MoveFile Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(t_LocalDoc_1);

    MfrMoveFileTest t_LocalDoc_2 = { "MFR MoveFile Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, false, ERROR_FILE_EXISTS };
    MfrMoveFileTests1.push_back(t_LocalDoc_2);


    int count = 0;
    for (MfrMoveFileTest t : MfrMoveFileTests1)      if (t.enabled) { count++; }
    return count;
}

int InitializeMoveFileTest2()
{
    std::wstring tempS;
    std::wstring tempD;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    // Requests to Native File Locations for MoveCopyFile via VFS
    MfrMoveFileTest ts_Native_PF1 = { "MFR MoveFile Native-folder VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest"),
                                    (g_NativePF + L"\\PlaceholderTestCopied"),
                                    true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(ts_Native_PF1);

    MfrMoveFileTest ts_Native_PF1E = { "MFR MoveFile Native-folder VFS exists in package and now in dest",             true, false,  false,
                                (g_NativePF + L"\\PlaceholderTest"),
                                (g_NativePF + L"\\PlaceholderTestCopied"),
                                false, ERROR_FILE_EXISTS };
    MfrMoveFileTests2.push_back(ts_Native_PF1E);

    MfrMoveFileTest t_Native_PF2 = { "MFR MoveFile Native-folder VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedMissingNativePlaceholder"),
                                    false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Native_PF2);


    // Requests to Package File Locations for MoveFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedPlaceholderTest";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest";
#endif
    MfrMoveFileTest t_Vfs_PF1 = { "MFR MoveFile Package-folder VFS exists in package but not dest",              true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(t_Vfs_PF1);

    MfrMoveFileTest ts_Vfs_PF1E1 = { "MFR MoveFile Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests2.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest";
#endif
    MfrMoveFileTest t_Vfs_PF2 = { "MFR MoveFile Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingSubfolder";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingSubFolder";
#endif
    MfrMoveFileTest t_Vfs_PF3 = { "MFR MoveFile Package-folder VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Vfs_PF3);



    // Requests to Redirected File Locations for MoveFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedRedirPlaceholderTest";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest";
#endif
    MfrMoveFileTest t_Redir_PF1 = { "MFR MoveFile Redirected-folder VFS exists in package but not dest.",                     true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(t_Redir_PF1);

    MfrMoveFileTest ts_Redir_PF1E1 = { "MFR MoveFile Redirected Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests2.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\MissingVFSPlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest";
#endif
    MfrMoveFileTest t_Redir_PF2 = { "MFR MoveFile Redirected Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholder";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholder";
#endif
    MfrMoveFileTest t_Redir_PF3 = { "MFR MoveFile Package-folder VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\CopiedDocs");
    MfrMoveFileTest t_LocalDoc_1 = { "MFR MoveFile Package-folder VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(t_LocalDoc_1);

    MfrMoveFileTest t_LocalDoc_2 = { "MFR MoveFile Package-folder VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, false, ERROR_INVALID_PARAMETER };
    MfrMoveFileTests2.push_back(t_LocalDoc_2);



    int count = 0;
    for (MfrMoveFileTest t : MfrMoveFileTests2)      if (t.enabled) { count++; }
    return count;
}


int InitializeMoveFileExTest1()
{
    std::wstring tempS;
    std::wstring tempD;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif


    // Requests to Native File Locations for CopyFile via VFS
    MfrMoveFileExTest ts_Native_PF1 = { "MFR MoveFileEx Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(ts_Native_PF1);

    MfrMoveFileExTest ts_Native_PF1E1 = { "MFR MoveFileEx Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    0, false, ERROR_FILE_EXISTS };
    MfrMoveFileExTests1.push_back(ts_Native_PF1E1);

    MfrMoveFileExTest ts_Native_PF1E2 = { "MFR MoveFileEx Native-file VFS exists in package and not dest subdir (allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\NoSuchFolder\\CopiedTestIniFileVfsPF.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(ts_Native_PF1E2);

    MfrMoveFileExTest t_Native_PF2 = { "MFR MoveFileEx Native-file VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedMissingNativePlaceholder.txt"),
                                    0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Native_PF2);

    MfrMoveFileExTest t_Native_PF3 = { "MFR MoveFileEx Native-file VFS parent-folder missing in package",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder.txt"),
                                    0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Native_PF3);


    // Requests to Package File Locations for MoveFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedPlaceholder.txt";
#endif
    MfrMoveFileExTest t_Vfs_PF1 = { "MFR MoveFileEx Package-file VFS exists in package",                           true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(t_Vfs_PF1);

    MfrMoveFileExTest ts_Vfs_PF1E1 = { "MFR MoveFileEx Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests1.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileExTest t_Vfs_PF2 = { "MFR MoveFileEx Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileExTest t_Vfs_PF3 = { "MFR MoveFileEx Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for CopyFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#endif
    MfrMoveFileExTest t_Redir_PF1 = { "MFR MoveFileEx Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(t_Redir_PF1);

    MfrMoveFileExTest ts_Redir_PF1E1 = { "MFR MoveFileEx Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests1.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileExTest t_Redir_PF2 = { "MFR MoveFileEx Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileExTest t_Redir_PF3 = { "MFR MoveFileEx Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\Copy\\CopiedPresonalFile.txt");
    MfrMoveFileExTest t_LocalDoc_1 = { "MFR MoveFileEx Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(t_LocalDoc_1);

    MfrMoveFileExTest t_LocalDoc_2 = { "MFR MoveFileEx Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_EXISTS };
    MfrMoveFileExTests1.push_back(t_LocalDoc_2);


    int count = 0;
    for (MfrMoveFileExTest t : MfrMoveFileExTests1)      if (t.enabled) { count++; }
    return count;
}

int InitializeMoveFileExTest2()
{
    std::wstring tempS;
    std::wstring tempD;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif

    // Requests to Native File Locations for MoveCopyFile via VFS
    MfrMoveFileExTest ts_Native_PF1 = { "MFR MoveFileEx Native-folder VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest"),
                                    (g_NativePF + L"\\PlaceholderTestCopied"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(ts_Native_PF1);

    MfrMoveFileExTest ts_Native_PF1E = { "MFR MoveFileEx Native-folder VFS exists in package and now in dest",             true, false,  false,
                                (g_NativePF + L"\\PlaceholderTest"),
                                (g_NativePF + L"\\PlaceholderTestCopied"),
                                0, false, ERROR_FILE_EXISTS };
    MfrMoveFileExTests2.push_back(ts_Native_PF1E);

    MfrMoveFileExTest t_Native_PF2 = { "MFR MoveFileEx Native-folder VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedMissingNativePlaceholder"),
                                    0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Native_PF2);


    // Requests to Package File Locations for MoveFileEx using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedPlaceholderTest";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest";
#endif
    MfrMoveFileExTest t_Vfs_PF1 = { "MFR MoveFileEx Package-folder VFS exists in package but not dest",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(t_Vfs_PF1);

    MfrMoveFileExTest ts_Vfs_PF1E1 = { "MFR MoveFileEx Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests2.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest";
#endif
    MfrMoveFileExTest t_Vfs_PF2 = { "MFR MoveFileEx Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingSubfolder";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingSubFolder";
#endif
    MfrMoveFileExTest t_Vfs_PF3 = { "MFR MoveFileEx Package-folder VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Vfs_PF3);



    // Requests to Redirected File Locations for MoveFileEx using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedRedirPlaceholderTest";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest";
#endif
    MfrMoveFileExTest t_Redir_PF1 = { "MFR MoveFileEx Redirected-folder VFS exists in package but not dest.",                     true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(t_Redir_PF1);

    MfrMoveFileExTest ts_Redir_PF1E1 = { "MFR MoveFileEx Redirected Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests2.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\MissingVFSPlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest";
#endif
    MfrMoveFileExTest t_Redir_PF2 = { "MFR MoveFileEx Redirected Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholder";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingSubFolder";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholder";
#endif
    MfrMoveFileExTest t_Redir_PF3 = { "MFR MoveFileEx Package-folder VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\CopiedDocs");
    MfrMoveFileExTest t_LocalDoc_1 = { "MFR MoveFileEx Package-folder VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(t_LocalDoc_1);

    MfrMoveFileExTest t_LocalDoc_2 = { "MFR MoveFileEx Package-folder VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, 0, false, ERROR_INVALID_PARAMETER };
    MfrMoveFileExTests2.push_back(t_LocalDoc_2);



    int count = 0;
    for (MfrMoveFileExTest t : MfrMoveFileExTests2)      if (t.enabled) { count++; }
    return count;
}

int InitializeMoveFileExTest3()
{
    std::wstring tempS;
    std::wstring tempD;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif


    // Requests to Native File Locations for MoveFileEx via VFS
    MfrMoveFileExTest ts_Native_PF1 = { "MFR MoveFileEx with Replace Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    MOVEFILE_REPLACE_EXISTING, true, ERROR_SUCCESS };
    MfrMoveFileExTests3.push_back(ts_Native_PF1);

    MfrMoveFileExTest ts_Native_PF1E1 = { "MFR MoveFileEx with Replace Native-file VFS exists in package and dest (also allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    MOVEFILE_REPLACE_EXISTING, true, ERROR_SUCCESS };
    MfrMoveFileExTests3.push_back(ts_Native_PF1E1);


    int count = 0;
    for (MfrMoveFileExTest t : MfrMoveFileExTests3)      if (t.enabled) { count++; }
    return count;
}


int InitializeMoveFileWithProgressTest1()
{
    std::wstring tempS;
    std::wstring tempD;

    std::wstring VFSPF = g_Cwd;
    std::wstring REVFSPF = g_writablePackageRootPath.c_str();
#if _M_IX86
    VFSPF.append(L"\\VFS\\ProgramFilesX86");
    REVFSPF.append(L"\\VFS\\ProgramFilesX86");
#else
    VFSPF.append(L"\\VFS\\ProgramFilesX64");
    REVFSPF.append(L"\\VFS\\ProgramFilesX64");
#endif


    // Requests to Native File Locations for CopyFile via VFS
    MfrMoveFileWithProgressTest ts_Native_PF1 = { "MFR MoveFileWithProgress Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native_PF1);

    MfrMoveFileWithProgressTest ts_Native_PF1E1 = { "MFR MoveFileWithProgress Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    0, false, ERROR_FILE_EXISTS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native_PF1E1);

    MfrMoveFileWithProgressTest ts_Native_PF1E2 = { "MFR MoveFileWithProgress Native-file VFS exists in package but not dest subfolder (allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\NoSuchFolder\\CopiedTestIniFileVfsPF.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native_PF1E2);

    MfrMoveFileWithProgressTest t_Native_PF2 = { "MFR MoveFileWithProgress Native-file VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedMissingNativePlaceholder.txt"),
                                    0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Native_PF2);

    MfrMoveFileWithProgressTest t_Native_PF3 = { "MFR MoveFileWithProgress Native-file VFS parent-folder missing in package",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder.txt"),
                                    0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Native_PF3);


    // Requests to Package File Locations for MoveFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedPlaceholder.txt";
#endif
    MfrMoveFileWithProgressTest t_Vfs_PF1 = { "MFR MoveFileWithProgress Package-file VFS exists in package",                           true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(t_Vfs_PF1);

    MfrMoveFileWithProgressTest ts_Vfs_PF1E1 = { "MFR MoveFileWithProgress Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileWithProgressTests1.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileWithProgressTest t_Vfs_PF2 = { "MFR MoveFileWithProgress Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileWithProgressTest t_Vfs_PF3 = { "MFR MoveFileWithProgress Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for CopyFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#endif
    MfrMoveFileWithProgressTest t_Redir_PF1 = { "MFR MoveFileWithProgress Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(t_Redir_PF1);

    MfrMoveFileWithProgressTest ts_Redir_PF1E1 = { "MFR MoveFileWithProgress Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileWithProgressTests1.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileWithProgressTest t_Redir_PF2 = { "MFR MoveFileWithProgress Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrMoveFileWithProgressTest t_Redir_PF3 = { "MFR MoveFileWithProgress Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\Copy\\CopiedPresonalFile.txt");
    MfrMoveFileWithProgressTest t_LocalDoc_1 = { "MFR MoveFileWithProgress Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(t_LocalDoc_1);

    MfrMoveFileWithProgressTest t_LocalDoc_2 = { "MFR MoveFileWithProgress Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_EXISTS };
    MfrMoveFileWithProgressTests1.push_back(t_LocalDoc_2);



    // Requests to Native File Locations for MoveFileWithProgress via VFS
    MfrMoveFileWithProgressTest ts_Native2_PF1 = { "MFR MoveFileWithProgress with Replace Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    MOVEFILE_REPLACE_EXISTING, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native2_PF1);

    MfrMoveFileWithProgressTest ts_Native2_PF1E1 = { "MFR MoveFileWithProgress with Replace Native-file VFS exists in package and dest (also allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTest\\TestIniFileVfsPF.ini"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini"),
                                    MOVEFILE_REPLACE_EXISTING, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native2_PF1E1);


    int count = 0;
    for (MfrMoveFileWithProgressTest t : MfrMoveFileWithProgressTests1)      if (t.enabled) { count++; }
    return count;
}



int InitializeMoveFilesTests()
{
    int count = 0;
    count += InitializeMoveFileTest1();
    count += InitializeMoveFileTest2();
    count += InitializeMoveFileExTest1();
    count += InitializeMoveFileExTest2();
    count += InitializeMoveFileExTest3();
    count += InitializeMoveFileWithProgressTest1();

    return count;
}


int RunMoveFilesTests()
{

    int result = ERROR_SUCCESS;
    for (MfrMoveFileTest testInput : MfrMoveFileTests1)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR MoveFile File Test (#1): ";
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

            auto testResult = MoveFile(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str());
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: Move failed, but with incorrect error.\n", error_color);
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
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                // return was success
                if (!testInput.Expected_Success)
                {
                    trace_message(L"ERROR: Move unexpected return value.\n", error_color);
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
                        result = result ? result : -1;
                        test_end(-1);
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

    for (MfrMoveFileTest testInput : MfrMoveFileTests2)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR MoveFile File Test (#2): ";
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

            auto testResult = MoveFile(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str());
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: Move failed, but with incorrect error.\n", error_color);
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
                            result = result ? result : -1;
                            test_end(-1);
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
                    trace_message(L"ERROR: Move unexpected return value.\n", error_color);
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


    for (MfrMoveFileExTest testInput : MfrMoveFileExTests1)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR MoveFileEx File Test (#1): ";
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

            auto testResult = MoveFileEx(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveEx failed, but with incorrect error.\n", error_color);
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
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                // return was success
                if (!testInput.Expected_Success)
                {
                    trace_message(L"ERROR: MoveEx unexpected return value.\n", error_color);
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
                        result = result ? result : -1;
                        test_end(-1);
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

    for (MfrMoveFileExTest testInput : MfrMoveFileExTests2)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR MoveFileEx File Test (#2): ";
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

            auto testResult = MoveFileEx(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveEx failed, but with incorrect error.\n", error_color);
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
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                // return was success
                if (!testInput.Expected_Success)
                {
                    trace_message(L"ERROR: MoveEx unexpected return value.\n", error_color);
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
                        result = result ? result : -1;
                        test_end(-1);
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

    for (MfrMoveFileExTest testInput : MfrMoveFileExTests3)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR MoveFileEx File Test (#3): ";
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

            auto testResult = MoveFileEx(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveEx failed, but with incorrect error.\n", error_color);
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
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                // return was success
                if (!testInput.Expected_Success)
                {
                    trace_message(L"ERROR: MoveEx unexpected return value.\n", error_color);
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
                        result = result ? result : -1;
                        test_end(-1);
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


    for (MfrMoveFileWithProgressTest testInput : MfrMoveFileWithProgressTests1)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR MoveFileWithProgress File Test (#1): ";
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

            auto testResult = MoveFileWithProgress(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), nullptr, nullptr, testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveWithProgress failed, but with incorrect error.\n", error_color);
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
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                // return was success
                if (!testInput.Expected_Success)
                {
                    trace_message(L"ERROR: MoveWithProgress unexpected return value.\n", error_color);
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
                        result = result ? result : -1;
                        test_end(-1);
                    }
                }
                else
                {
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
        }
        Sleep(300);  // ensure previous call has a chance to finish since we aren't implementing the callback.
    }

    return result;
}
