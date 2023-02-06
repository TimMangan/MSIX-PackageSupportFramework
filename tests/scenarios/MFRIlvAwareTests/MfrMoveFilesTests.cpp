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

    // Requests to Native File Locations for MoveFile via VFS
    MfrMoveFileTest ts_Native_PF1 = { "MFR+ILV MoveFile Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF1.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedTestIniFileVfsPF_MF1A.ini"),
                                    true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(ts_Native_PF1);

    MfrMoveFileTest ts_Native_PF1E1 = { "MFR+ILV MoveFile Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF2.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedTestIniFileVfsPF_MF1A.ini"),
                                    false, ERROR_FILE_EXISTS,
                                    true, ERROR_ALREADY_EXISTS};
    MfrMoveFileTests1.push_back(ts_Native_PF1E1);

    MfrMoveFileTest ts_Native_PF1E2 = { "MFR+ILV MoveFile Native-file VFS exists in package and not dest subdir (allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF3.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\NoSuchFolder_MF1C\\CopiedTestIniFileVfsPF_MF1C.ini"),
                                    true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(ts_Native_PF1E2);

    MfrMoveFileTest t_Native_PF2 = { "MFR+ILV MoveFile Native-file VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMF\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedMissingNativePlaceholder_MF1D.txt"),
                                    false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Native_PF2);

    MfrMoveFileTest t_Native_PF3 = { "MFR+ILV MoveFile Native-file VFS parent-folder missing in package",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder_MF1E.txt"),
                                    false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Native_PF3);


    // Requests to Package File Locations for MoveFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\Placeholder1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\CopiedPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\Placeholder1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\CopiedPlaceholder_MF1F.txt";
#endif
    MfrMoveFileTest t_Vfs_PF1 = { "MFR+ILV MoveFile Package-file VFS exists in package",                           true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(t_Vfs_PF1);
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\Placeholder2.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\Placeholder2.txt";
#endif
    MfrMoveFileTest ts_Vfs_PF1E1 = { "MFR+ILV MoveFile Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests1.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\CopiedMissingVFSPlaceholder_MF1H.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\CopiedMissingVFSPlaceholder_MF1H.txt";
#endif
    MfrMoveFileTest t_Vfs_PF2 = { "MFR+ILV MoveFile Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF1I1\\MissingVFSPlaceholder_MF1I1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF1I2\\CopiedMissingVFSPlaceholder_MF1I2.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF1I1\\MissingVFSPlaceholder_MF1I1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF1I2\\CopiedMissingVFSPlaceholder_MF1I2.txt";
#endif
    MfrMoveFileTest t_Vfs_PF3 = { "MFR+ILV MoveFile Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for MoveFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder3.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedRedirPlaceholder_MF1J.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\Placeholder3.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\CopiedRedirPlaceholder_MF1J.txt";
#endif
    MfrMoveFileTest t_Redir_PF1 = { "MFR+ILV MoveFile Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(t_Redir_PF1);

#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\Placeholder4.txt");
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\Placeholder4.txt");
#endif
    MfrMoveFileTest ts_Redir_PF1E1 = { "MFR+ILV MoveFile Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests1.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\MissingVFSPlaceholder_MF1K.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\CopiedMissingVFSPlaceholder_MF1K.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\MissingVFSPlaceholder_MF1K.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\CopiedMissingVFSPlaceholder_MF1K.txt";
#endif
    MfrMoveFileTest t_Redir_PF2 = { "MFR+ILV MoveFile Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF1L1\\MissingVFSPlaceholder_MF1L1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF1L2\\CopiedMissingVFSPlaceholder_MF1L2.txt";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF1L1\\MissingVFSPlaceholder_MF1L1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF1L2\\CopiedMissingVFSPlaceholder_MF1L2.txt";
#endif
    MfrMoveFileTest t_Redir_PF3 = { "MFR+ILV MoveFile Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests1.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_M1.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\MoveDocsM1\\CopiedPresonalFile_M1.txt");
    MfrMoveFileTest t_LocalDoc_1 = { "MFR+ILV MoveFile Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests1.push_back(t_LocalDoc_1);

    MfrMoveFileTest t_LocalDoc_2 = { "MFR+ILV MoveFile Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND};
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

    // Requests to Native File Locations for MoveFile via VFS
    MfrMoveFileTest ts_Native_PF1 = { "MFR+ILV MoveFile Native-folder VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTestMF1"),
                                    (g_NativePF + L"\\PlaceholderTestCopied_MF2A"),
                                    true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(ts_Native_PF1);

    MfrMoveFileTest ts_Native_PF1E = { "MFR+ILV MoveFile Native-folder VFS exists in package and now in dest",             true, false,  false,
                                (g_NativePF + L"\\PlaceholderTestMF2"),
                                (g_NativePF + L"\\PlaceholderTestCopied_MF2A"),
                                false, ERROR_FILE_EXISTS,
                                true, ERROR_ALREADY_EXISTS};
    MfrMoveFileTests2.push_back(ts_Native_PF1E);

    MfrMoveFileTest t_Native_PF2 = { "MFR+ILV MoveFile Native-folder VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest_MF2C"),
                                    (g_NativePF + L"\\PlaceholderTest\\CopiedMissingNativePlaceholder_MF2C"),
                                    false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Native_PF2);


    // Requests to Package File Locations for MoveFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM3";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedPlaceholderTest_MF2D";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM3";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest_MF2D";
#endif
    MfrMoveFileTest t_Vfs_PF1 = { "MFR+ILV MoveFile Package-folder VFS exists in package but not dest",              true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(t_Vfs_PF1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM4";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM4";
#endif
    MfrMoveFileTest ts_Vfs_PF1E1 = { "MFR+ILV MoveFile Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests2.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF2E";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest_MF2E";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF2E";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest_MF2E";
#endif
    MfrMoveFileTest t_Vfs_PF2 = { "MFR+ILV MoveFile Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF2F1\\MissingSubFolder_MF2F1";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF2F2\\CopiedMissingSubfolder_MF2F2";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF2F1\\MissingSubFolder_MF2F1";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF2F2\\CopiedMissingSubFolder_MF2F2";
#endif
    MfrMoveFileTest t_Vfs_PF3 = { "MFR+ILV MoveFile Package-folder VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Vfs_PF3);



    // Requests to Redirected File Locations for MoveFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestM5");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedRedirPlaceholderTest_MF2G";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestM5");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest_MF2G";
#endif
    MfrMoveFileTest t_Redir_PF1 = { "MFR+ILV MoveFile Redirected-folder VFS exists in package but not dest.",                     true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(t_Redir_PF1);

#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestM6");
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestM6");
#endif
    MfrMoveFileTest ts_Redir_PF1E1 = { "MFR+ILV MoveFile Redirected Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileTests2.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\MissingVFSPlaceholderTest_MF2H");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest_MF2H";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF2H");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest_MF2H";
#endif
    MfrMoveFileTest t_Redir_PF2 = { "MFR+ILV MoveFile Redirected Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MF2I\\MissingSubFolder_MF2I";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholder_MF2I";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MF2I\\MissingSubFolder_MF2I";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholder_MF2I";
#endif
    MfrMoveFileTest t_Redir_PF3 = { "MFR+ILV MoveFile Package-folder VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileTests2.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocsM2";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\MovedDocsM2");
    MfrMoveFileTest t_LocalDoc_1 = { "MFR+ILV MoveFile Package-folder VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, true, ERROR_SUCCESS };
    MfrMoveFileTests2.push_back(t_LocalDoc_1);

    MfrMoveFileTest t_LocalDoc_2 = { "MFR+ILV MoveFile Package-folder VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, false, ERROR_FILE_NOT_FOUND };
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


    // Requests to Native File Locations for MoveFileEx via VFS
    MfrMoveFileExTest ts_Native_PF1 = { "MFR+ILV MoveFileEx Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF7.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedTestIniFileVfsPF_MFEx1A.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(ts_Native_PF1);

    MfrMoveFileExTest ts_Native_PF1E1 = { "MFR+ILV MoveFileEx Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF8.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedTestIniFileVfsPF_MFEx1A.ini"),
                                    0, false, ERROR_FILE_EXISTS,
                                       true, ERROR_ALREADY_EXISTS};
    MfrMoveFileExTests1.push_back(ts_Native_PF1E1);

    MfrMoveFileExTest ts_Native_PF1E2 = { "MFR+ILV MoveFileEx Native-file VFS exists in package and not dest subdir (allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF9.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\NoSuchFolder_MFEx1C\\CopiedTestIniFileVfsPF_MFEx1C.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(ts_Native_PF1E2);

    MfrMoveFileExTest t_Native_PF2 = { "MFR+ILV MoveFileEx Native-file VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMF\\MissingNativePlaceholder_MFEx1D.txt"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedMissingNativePlaceholder_MFEx1D.txt"),
                                    0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Native_PF2);

    MfrMoveFileExTest t_Native_PF3 = { "MFR+ILV MoveFileEx Native-file VFS parent-folder missing in package",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest_MFEx1E1\\MissingNativePlaceholder_MFEx1E1.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest_MFEx1E2\\CopiedMissingNarivePlaceholder_MFEx1E2.txt"),
                                    0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Native_PF3);


    // Requests to Package File Locations for MoveFileEx using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF11\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF\\CopiedPlaceholder_MFEx1F.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF11\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF\\CopiedPlaceholder_MFEx1F.txt";
#endif
    MfrMoveFileExTest t_Vfs_PF1 = { "MFR+ILV MoveFileEx Package-file VFS exists in package",                           true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(t_Vfs_PF1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF12\\Placeholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF12\\Placeholder.txt";
#endif
    MfrMoveFileExTest ts_Vfs_PF1E1 = { "MFR+ILV MoveFileEx Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests1.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF\\MissingVFSPlaceholder_MFEx1G.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF\\CopiedMissingVFSPlaceholder_MFEx1G.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF\\MissingVFSPlaceholder_MFEx1G.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF\\CopiedMissingVFSPlaceholder_MFEx1G.txt";
#endif
    MfrMoveFileExTest t_Vfs_PF2 = { "MFR+ILV MoveFileEx Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx1H1\\MissingVFSPlaceholder_MFEx1H1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx1H2\\CopiedMissingVFSPlaceholder_MFEx1H2.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx1H1\\MissingVFSPlaceholder_MFEx1H1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx1H2\\CopiedMissingVFSPlaceholder_MFEx1H2.txt";
#endif
    MfrMoveFileExTest t_Vfs_PF3 = { "MFR+ILV MoveFileEx Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for MoveFileEx using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF13\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF\\CopiedRedirPlaceholder_MFEx1I.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF13\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF\\CopiedRedirPlaceholder_MFEx1I.txt";
#endif
    MfrMoveFileExTest t_Redir_PF1 = { "MFR+ILV MoveFileEx Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(t_Redir_PF1);

#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF14\\Placeholder.txt");
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF14\\Placeholder.txt");
#endif
    MfrMoveFileExTest ts_Redir_PF1E1 = { "MFR+ILV MoveFileEx Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests1.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\MissingVFSPlaceholder_MFEx1J.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM\\CopiedMissingVFSPlaceholder_MFEx1J.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\MissingVFSPlaceholder_MFEx1J.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestM\\CopiedMissingVFSPlaceholder_MFEx1J.txt";
#endif
    MfrMoveFileExTest t_Redir_PF2 = { "MFR+ILV MoveFileEx Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx1K1\\MissingVFSPlaceholder_MFEx1K1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx1K2\\CopiedMissingVFSPlaceholder_MFEx1K2.txt";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx1K1\\MissingVFSPlaceholder_MFEx1K1.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx1K2\\CopiedMissingVFSPlaceholder_MFEx1K2.txt";
#endif
    MfrMoveFileExTest t_Redir_PF3 = { "MFR+ILV MoveFileEx Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests1.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_M2.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\MoveFileM3\\CopiedPresonalFile_M2.txt");
    MfrMoveFileExTest t_LocalDoc_1 = { "MFR+ILV MoveFileEx Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests1.push_back(t_LocalDoc_1);

    MfrMoveFileExTest t_LocalDoc_2 = { "MFR+ILV MoveFileEx Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
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

    // Requests to Native File Locations for MoveFileEx via VFS
    MfrMoveFileExTest ts_Native_PF1 = { "MFR+ILV MoveFileEx Native-folder VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTestMF16"),
                                    (g_NativePF + L"\\PlaceholderTestCopied_MFEx2A"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(ts_Native_PF1);

    MfrMoveFileExTest ts_Native_PF1E = { "MFR+ILV MoveFileEx Native-folder VFS exists in package and now in dest",             true, false,  false,
                                (g_NativePF + L"\\PlaceholderTestMF17"),
                                (g_NativePF + L"\\PlaceholderTestCopied_MFEx2A"),
                                0, false, ERROR_FILE_EXISTS,
                                   true, ERROR_ALREADY_EXISTS};
    MfrMoveFileExTests2.push_back(ts_Native_PF1E);

    MfrMoveFileExTest t_Native_PF2 = { "MFR+ILV MoveFileEx Native-folder VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest_MFEx2B"),
                                    (g_NativePF + L"\\PlaceholderTestM\\CopiedMissingNativePlaceholder_MFEx2B"),
                                    0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Native_PF2);


    // Requests to Package File Locations for MoveFileEx using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM18";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedPlaceholderTest_MFEx2C";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF18";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest_MFEx2C";
#endif
    MfrMoveFileExTest t_Vfs_PF1 = { "MFR+ILV MoveFileEx Package-folder VFS exists in package but not dest",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(t_Vfs_PF1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestM19";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF19";
#endif
    MfrMoveFileExTest ts_Vfs_PF1E1 = { "MFR+ILV MoveFileEx Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests2.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx2D";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest_MFEx2D";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx2D";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest_MFEx2D";
#endif
    MfrMoveFileExTest t_Vfs_PF2 = { "MFR+ILV MoveFileEx Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx2E1\\MissingSubFolder_MFEx2E1";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx2E2\\CopiedMissingSubfolder_MFEx2E2";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx2E1\\MissingSubFolder_MFEx2E1";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx2E2\\CopiedMissingSubFolder_MFEx2E2";
#endif
    MfrMoveFileExTest t_Vfs_PF3 = { "MFR+ILV MoveFileEx Package-folder VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Vfs_PF3);



    // Requests to Redirected File Locations for MoveFileEx using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF20");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedRedirPlaceholderTest_MFEx2F";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF20");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedPlaceholderTest_MFEx2F";
#endif
    MfrMoveFileExTest t_Redir_PF1 = { "MFR+ILV MoveFileEx Redirected-folder VFS exists in package but not dest.",                     true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(t_Redir_PF1);

#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestMF21");
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestMF21");
#endif
    MfrMoveFileExTest ts_Redir_PF1E1 = { "MFR+ILV MoveFileEx Redirected Package-folder VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileExTests2.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\MissingVFSPlaceholderTest_MFEx2G");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholderTest_MFEx2G";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx2G");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholderTest_MFEx2G";
#endif
    MfrMoveFileExTest t_Redir_PF2 = { "MFR+ILV MoveFileEx Redirected Package-folder VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFEx2H\\MissingSubFolder_MFEx2H";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\CopiedMissingPlaceholder_MFEx2H";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFEx2H\\MissingSubFolder_MFEx2H";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\CopiedMissingPlaceholder_MFEx2H";
#endif
    MfrMoveFileExTest t_Redir_PF3 = { "MFR+ILV MoveFileEx Package-folder VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileExTests2.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocsM3";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\MovedDocsM3");
    MfrMoveFileExTest t_LocalDoc_1 = { "MFR+ILV MoveFileEx Package-folder VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileExTests2.push_back(t_LocalDoc_1);

    MfrMoveFileExTest t_LocalDoc_2 = { "MFR+ILV MoveFileEx Package-folder VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
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
    MfrMoveFileExTest ts_Native_PF1 = { "MFR+ILV MoveFileEx with Replace Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF4.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedTestIniFileVfsPF_MFEx3A.ini"),
                                    MOVEFILE_REPLACE_EXISTING, true, ERROR_SUCCESS };
    MfrMoveFileExTests3.push_back(ts_Native_PF1);

    MfrMoveFileExTest ts_Native_PF1E1 = { "MFR+ILV MoveFileEx with Replace Native-file VFS exists in package and dest (also allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMF\\TestIniFileVfsPF5.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMF\\CopiedTestIniFileVfsPF_MFEx3A.ini"),
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


    // Requests to Native File Locations for MoveFileWithProgress via VFS
    MfrMoveFileWithProgressTest ts_Native_PF1 = { "MFR+ILV MoveFileWithProgress Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\TestIniFileVfsPF_MFWP1.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\CopiedTestIniFileVfsPF_MFwp1.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native_PF1);

    MfrMoveFileWithProgressTest ts_Native_PF1E1 = { "MFR+ILV MoveFileWithProgress Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\TestIniFileVfsPF_MFWP2.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\CopiedTestIniFileVfsPF_MFwp1.ini"),
                                    0, false, ERROR_FILE_EXISTS ,
                                        true, ERROR_ALREADY_EXISTS};
    MfrMoveFileWithProgressTests1.push_back(ts_Native_PF1E1);

    MfrMoveFileWithProgressTest ts_Native_PF1E2 = { "MFR+ILV MoveFileWithProgress Native-file VFS exists in package but not dest subfolder (allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\TestIniFileVfsPF_MSWP3.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\NoSuchFolder_MFwp3\\CopiedTestIniFileVfsPF_MFwp3.ini"),
                                    0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native_PF1E2);

    MfrMoveFileWithProgressTest t_Native_PF2 = { "MFR+ILV MoveFileWithProgress Native-file VFS missing in package",                          true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\MissingNativePlaceholder_MFwp4.txt"),
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\CopiedMissingNativePlaceholder_MFwp4.txt"),
                                    0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Native_PF2);

    MfrMoveFileWithProgressTest t_Native_PF3 = { "MFR+ILV MoveFileWithProgress Native-file VFS parent-folder missing in package",            true, false, false,
                                    (g_NativePF + L"\\MissingPlaceholderTest_MFwp5a\\MissingNativePlaceholder_MFwp5a.txt"),
                                    (g_NativePF + L"\\MissingPlaceholderTest_MFwp5b\\CopiedMissingNarivePlaceholder_MFwp5b.txt"),
                                    0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Native_PF3);

    
    // Requests to Package File Locations for MoveFileWithProgress using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\Placeholder5.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\CopiedPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\Placeholder6.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\CopiedPlaceholder_MFwp6.txt";
#endif
    MfrMoveFileWithProgressTest t_Vfs_PF1 = { "MFR+ILV MoveFileWithProgress Package-file VFS exists in package",                           true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(t_Vfs_PF1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\Placeholder7.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\Placeholder7.txt";
#endif
    MfrMoveFileWithProgressTest ts_Vfs_PF1E1 = { "MFR+ILV MoveFileWithProgress Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileWithProgressTests1.push_back(ts_Vfs_PF1E1);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\MissingVFSPlaceholder_MFwp8.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\CopiedMissingVFSPlaceholder_MFwp8.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\MissingVFSPlaceholder_MFwp8.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\CopiedMissingVFSPlaceholder_MFwp8.txt";
#endif
    MfrMoveFileWithProgressTest t_Vfs_PF2 = { "MFR+ILV MoveFileWithProgress Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFwp9a\\MissingVFSPlaceholder_MFwp9a.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFwp9b\\CopiedMissingVFSPlaceholder_MFwp9b.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFwp9a\\MissingVFSPlaceholder_MFwp9a.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFwp9b\\CopiedMissingVFSPlaceholder_MFwp9b.txt";
#endif
    MfrMoveFileWithProgressTest t_Vfs_PF3 = { "MFR+ILV MoveFileWithProgress Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for MoveFileWithProgress using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\Placeholder9.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\CopiedRedirPlaceholder9.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\Placeholder9.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\CopiedRedirPlaceholder_MFwp9.txt";
#endif
    MfrMoveFileWithProgressTest t_Redir_PF1 = { "MFR+ILV MoveFileWithProgress Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(t_Redir_PF1);

#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\Placeholder10.txt");
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\Placeholder10.txt");
#endif
    MfrMoveFileWithProgressTest ts_Redir_PF1E1 = { "MFR+ILV MoveFileWithProgress Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, 0, true, ERROR_ALREADY_EXISTS };
    MfrMoveFileWithProgressTests1.push_back(ts_Redir_PF1E1);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\MissingVFSPlaceholder11.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTestMFWP\\CopiedMissingVFSPlaceholder11.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\MissingVFSPlaceholder_MFwp11.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTestMFWP\\CopiedMissingVFSPlaceholder_MFwp11.txt";
#endif
    MfrMoveFileWithProgressTest t_Redir_PF2 = { "MFR+ILV MoveFileWithProgress Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, 0, false, ERROR_FILE_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFwp12a\\MissingVFSPlaceholder_MFwp12a.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest_MFwp12b\\CopiedMissingVFSPlaceholder_MFwp12b.txt";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFwp12a\\MissingVFSPlaceholder_MFwp12a.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest_MFwp12b\\CopiedMissingVFSPlaceholder_MFwp12b.txt";
#endif
    MfrMoveFileWithProgressTest t_Redir_PF3 = { "MFR+ILV MoveFileWithProgress Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_M3.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\MoveFileM3\\CopiedPresonalFile_M3.txt");
    MfrMoveFileWithProgressTest t_LocalDoc_1 = { "MFR+ILV MoveFileWithProgress Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(t_LocalDoc_1);

    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_M3.txt";
    MfrMoveFileWithProgressTest t_LocalDoc_2 = { "MFR+ILV MoveFileWithProgress Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, 0, false, ERROR_PATH_NOT_FOUND };
    MfrMoveFileWithProgressTests1.push_back(t_LocalDoc_2);



    // Requests to Native File Locations for MoveFileWithProgress via VFS
    MfrMoveFileWithProgressTest ts_Native2_PF1 = { "MFR+ILV MoveFileWithProgress with Replace Native-file VFS exists in package but not dest",             true, true,  true,
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\TestIniFileVfsPF_MFWP15.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\CopiedTestIniFileVfsPF_MFwp15.ini"),
                                    MOVEFILE_REPLACE_EXISTING, true, ERROR_SUCCESS };
    MfrMoveFileWithProgressTests1.push_back(ts_Native2_PF1);

    MfrMoveFileWithProgressTest ts_Native2_PF1E1 = { "MFR+ILV MoveFileWithProgress with Replace Native-file VFS exists in package and dest (also allowed)", true, false, false,
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\TestIniFileVfsPF_MFWP16.ini"),
                                    (g_NativePF + L"\\PlaceholderTestMFWP\\CopiedTestIniFileVfsPF_MFwp15.ini"),
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
            std::string testname = "MFR+ILV MoveFile File Test (#1): ";
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

            auto testResult = MoveFile(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str());
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: Move failed, but with incorrect error.\n", error_color);
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
            std::string testname = "MFR+ILV MoveFile File Test (#2): ";
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

            auto testResult = MoveFile(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str());
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: Move failed, but with incorrect error.\n", error_color);
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
                    detail1.append(std::to_wstring(testInput.Expected_Success));
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
            std::string testname = "MFR+ILV MoveFileEx File Test (#1): ";
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

            auto testResult = MoveFileEx(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveEx failed, but with incorrect error.\n", error_color);
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
                    detail1.append(std::to_wstring(testInput.Expected_Success));
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
            std::string testname = "MFR+ILV MoveFileEx File Test (#2): ";
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

            auto testResult = MoveFileEx(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveEx failed, but with incorrect error.\n", error_color);
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
                    detail1.append(std::to_wstring(testInput.Expected_Success));
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
            std::string testname = "MFR+ILV MoveFileEx File Test (#3): ";
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

            auto testResult = MoveFileEx(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveEx failed, but with incorrect error.\n", error_color);
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
                    detail1.append(std::to_wstring(testInput.Expected_Success));
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
            std::string testname = "MFR+ILV MoveFileWithProgress File Test (#1): ";
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

            auto testResult = MoveFileWithProgress(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), nullptr, nullptr, testInput.flags);
            auto eCode = GetLastError();
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
                        trace_message(L"ERROR: MoveWithProgress failed, but with incorrect error.\n", error_color);
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
                    detail1.append(std::to_wstring(testInput.Expected_Success));
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
