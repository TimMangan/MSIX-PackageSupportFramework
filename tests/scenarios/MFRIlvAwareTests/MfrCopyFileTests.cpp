//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrCopyFileTests.h"
#include <stdio.h>
#include "MfrCleanup.h"

std::vector<MfrCopyFileTest> MfrCopyFileTests;
std::vector<MfrCopyFileExTest> MfrCopyFileExTests;
std::vector<MfrCopyFile2Test> MfrCopyFile2Tests;

COPYFILE2_EXTENDED_PARAMETERS extendedParameters1;
COPYFILE2_EXTENDED_PARAMETERS extendedParameters2;

int InitializeCopyFileTestArray()
{
    std::wstring tempS;
    std::wstring tempD;



    // Requests to Native File Locations for CopyFile via VFS
    tempS = g_NativePF;
    tempS.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    tempD = g_NativePF;
    tempD.append(L"\\PlaceholderTest\\CopiedTestIniFileVfsPF.ini");
    MfrCopyFileTest ts_Native_PF1 = { "MFR+ILV CopyFile Native-file VFS exists in package",                          true, true,  true,  
                                tempS, tempD, true, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Native_PF1);

    MfrCopyFileTest ts_Native_PF1E1 = { "MFR+ILV CopyFile Native-file VFS exists in package and dest (not allowed)", true, false, false, 
                                tempS, tempD, true, false, ERROR_FILE_EXISTS };
    MfrCopyFileTests.push_back(ts_Native_PF1E1);

    MfrCopyFileTest ts_Native_PF1E2 = { "MFR+ILV CopyFile Native-file VFS exists in package and dest (allowed)",     true, false, false, 
                                tempS, tempD, false, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Native_PF1E2);

    tempS = g_NativePF;
    tempS.append(L"\\PlaceholderTest\\MissingNativePlaceholder.txt");
    tempD = g_NativePF;
    tempD.append(L"\\PlaceholderTest\\CopiedMissingNativePlaceholder.txt");
    MfrCopyFileTest t_Native_PF2 = { "MFR+ILV CopyFile Native-file VFS missing in package",                          true, false, false, 
                                tempS, tempD, true, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Native_PF2);

    tempS = g_NativePF;
    tempS.append(L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt");
    tempD = g_NativePF;
    tempD.append(L"\\MissingPlaceholderTest\\CopiedMissingNarivePlaceholder.txt");
    MfrCopyFileTest t_Native_PF3 = { "MFR+ILV CopyFile Native-file VFS parent-folder missing in package",            true, false, false, 
                                tempS, tempD, true, false, ERROR_PATH_NOT_FOUND, true, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Native_PF3);


    // Requests to Package File Locations for CopyFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedPlaceholder.txt";
#endif
    MfrCopyFileTest t_Vfs_PF1 = { "MFR+ILV CopyFile Package-file VFS exists in package",                                    true, true,  true, 
                                tempS, tempD, true,true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(t_Vfs_PF1);

    MfrCopyFileTest ts_Vfs_PF1E1 = { "MFR+ILV CopyFile Package-file VFS exists in package and dest (not allowed)", true, false, false, 
                                tempS, tempD, true, false, ERROR_FILE_EXISTS };
    MfrCopyFileTests.push_back(ts_Vfs_PF1E1);

    MfrCopyFileTest ts_Vfs_PF1E2 = { "MFR+ILV CopyFile Package-file VFS exists in package and dest (allowed)",     true, false, false, 
                                tempS, tempD, false, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Vfs_PF1E2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt"; 
#endif
    MfrCopyFileTest t_Vfs_PF2 = { "MFR+ILV CopyFile Package-file VFS missing in package",                          true, false, false, 
                                tempS, tempD, true, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt"; 
#endif
    MfrCopyFileTest t_Vfs_PF3 = { "MFR+ILV CopyFile Package-file VFS parent-folder missing in package",            true, false, false, 
                                tempS, tempD, true, false, ERROR_PATH_NOT_FOUND,
                                true, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for CopyFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedRedirPlaceholder.txt";
#endif
    MfrCopyFileTest t_Redir_PF1 = { "MFR+ILV CopyFile Redirected-file VFS exists in package",                                   true, true,  true,  
                                tempS, tempD, true, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(t_Redir_PF1);

    MfrCopyFileTest ts_Redir_PF1E1 = { "MFR+ILV CopyFile Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false, 
                                tempS, tempD, true, false, ERROR_FILE_EXISTS };
    MfrCopyFileTests.push_back(ts_Redir_PF1E1);

    MfrCopyFileTest ts_Redir_PF1E2 = { "MFR+ILV CopyFile Redirected Package-file VFS exists in package and dest (allowed)",     true, false, false, 
                                tempS, tempD, false, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(ts_Redir_PF1E2);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrCopyFileTest t_Redir_PF2 = { "MFR+ILV CopyFile Redirected Package-file VFS missing in package",                          true, false, false, 
                                tempS, tempD, true, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#else

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedMissingVFSPlaceholder.txt";
#endif
    MfrCopyFileTest t_Redir_PF3 = { "MFR+ILV CopyFile Package-file VFS parent-folder missing in package",              true, false, false, 
                                tempS, tempD, true, false, ERROR_PATH_NOT_FOUND,
                                true, ERROR_FILE_NOT_FOUND };
    MfrCopyFileTests.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_C1.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\CopiedPresonalFile_C1.txt");
    MfrCopyFileTest t_LocalDoc_1 = { "MFR+ILV CopyFile Package-file VFS to Local Documents to succeed",              true, true,  true,  
                                tempS, tempD, true, TRUE, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(t_LocalDoc_1);

    MfrCopyFileTest t_LocalDoc_2 = { "MFR+ILV CopyFile Package-file VFS to Local Documents to fail",                 true, false, false, 
                                tempS, tempD, true, false, ERROR_FILE_EXISTS, true, ERROR_ALREADY_EXISTS };
    MfrCopyFileTests.push_back(t_LocalDoc_2);

    tempS = tempD;
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\ReCopiedPresonalFile_C1.txt");
    MfrCopyFileTest t_LocalDoc_3 = { "MFR+ILV CopyFile Local Documents to Local Documents to succeed",              true, false, false, 
                                tempS, tempD, true, true, ERROR_SUCCESS };
    MfrCopyFileTests.push_back(t_LocalDoc_3);

    int count = 0;
    for (MfrCopyFileTest      t : MfrCopyFileTests)      if (t.enabled) { count++; }

    return count;
}

int InitializeCopyFileExTestArray()
{
    std::wstring tempS;
    std::wstring tempD;


    // Requests to Native File Locations for CopyFile via VFS
    tempS = g_NativePF;
    tempS.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    tempD = g_NativePF;
    tempD.append(L"\\PlaceholderTest\\CopiedEx1TestIniFileVfsPF.ini");
    MfrCopyFileExTest ts_Native_PF1 = { "MFR+ILV CopyFileEx Native-file VFS exists in package",                          true, true,  true,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(ts_Native_PF1);

    MfrCopyFileExTest ts_Native_PF1E1 = { "MFR+ILV CopyFileEx Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_FILE_EXISTS };
    MfrCopyFileExTests.push_back(ts_Native_PF1E1);

    MfrCopyFileExTest ts_Native_PF1E2 = { "MFR+ILV CopyFileEx Native-file VFS exists in package and dest (allowed)",     true, false, false,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(ts_Native_PF1E2);

    tempS = g_NativePF;
    tempS.append(L"\\PlaceholderTest\\MissingNativePlaceholder.txt");
    tempD = g_NativePF;
    tempD.append(L"\\PlaceholderTest\\CopiedEx1MissingNativePlaceholder.txt");
    MfrCopyFileExTest t_Native_PF2 = { "MFR+ILV CopyFileEx Native-file VFS missing in package",                          true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileExTests.push_back(t_Native_PF2);

    tempS = g_NativePF;
    tempS.append(L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt");
    tempD = g_NativePF;
    tempD.append(L"\\MissingPlaceholderTest\\CopiedEx1MissingNarivePlaceholder.txt");
    MfrCopyFileExTest t_Native_PF3 = { "MFR+ILV CopyFileEx Native-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_PATH_NOT_FOUND,
                                true, ERROR_FILE_NOT_FOUND };
    MfrCopyFileExTests.push_back(t_Native_PF3);


    // Requests to Package File Locations for CopyFile using VFS
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedEx2Placeholder.txt";
    MfrCopyFileExTest t_Vfs_PF1 = { "MFR+ILV CopyFileEx Package-file VFS exists in package",                                    true, true,  true,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS,true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(t_Vfs_PF1);

    MfrCopyFileExTest ts_Vfs_PF1E1 = { "MFR+ILV CopyFileEx Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_FILE_EXISTS };
    MfrCopyFileExTests.push_back(ts_Vfs_PF1E1);

    MfrCopyFileExTest ts_Vfs_PF1E2 = { "MFR+ILV CopyFileEx Package-file VFS exists in package and dest (allowed)",     true, false, false,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(ts_Vfs_PF1E2);

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedEx2MissingVFSPlaceholder.txt";
    MfrCopyFileExTest t_Vfs_PF2 = { "MFR+ILV CopyFileEx Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileExTests.push_back(t_Vfs_PF2);

    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedEx2MissingVFSPlaceholder.txt";
    MfrCopyFileExTest t_Vfs_PF3 = { "MFR+ILV CopyFileEx Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_PATH_NOT_FOUND,
                                true, ERROR_FILE_NOT_FOUND };
    MfrCopyFileExTests.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for CopyFile using VFS
    tempS = g_writablePackageRootPath.c_str();
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedEx3RedirPlaceholder.txt";
    MfrCopyFileExTest t_Redir_PF1 = { "MFR+ILV CopyFileEx Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(t_Redir_PF1);

    MfrCopyFileExTest ts_Redir_PF1E1 = { "MFR+ILV CopyFileEx Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_FILE_EXISTS };
    MfrCopyFileExTests.push_back(ts_Redir_PF1E1);

    MfrCopyFileExTest ts_Redir_PF1E2 = { "MFR+ILV CopyFileEx Redirected Package-file VFS exists in package and dest (allowed)",     true, false, false,
                                tempS, tempD, 0, true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(ts_Redir_PF1E2);

    tempS = g_writablePackageRootPath.c_str();
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\CopiedEx3MissingVFSPlaceholder.txt";
    MfrCopyFileExTest t_Redir_PF2 = { "MFR+ILV CopyFileEx Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_FILE_NOT_FOUND };
    MfrCopyFileExTests.push_back(t_Redir_PF2);

    // Additional Failure request
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\CopiedEx3MissingVFSPlaceholder.txt";
    MfrCopyFileExTest t_Redir_PF3 = { "MFR+ILV CopyFileEx Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_PATH_NOT_FOUND,
                                true, ERROR_FILE_NOT_FOUND };
    MfrCopyFileExTests.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_C2.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\CopiedPresonalFile_C2.txt");
    MfrCopyFileExTest t_LocalDoc_1 = { "MFR+ILV CopyFileEx Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(t_LocalDoc_1);

    MfrCopyFileExTest t_LocalDoc_2 = { "MFR+ILV CopyFileEx Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, false, ERROR_FILE_EXISTS, true, ERROR_ALREADY_EXISTS };
    MfrCopyFileExTests.push_back(t_LocalDoc_2);

    tempS = tempD;
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\ReCopiedPresonalFile_C2.txt");
    MfrCopyFileExTest t_LocalDoc_3 = { "MFR+ILV CopyFileEx Local Documents to Local Documents to succeed",              true, false, false,
                                tempS, tempD, COPY_FILE_FAIL_IF_EXISTS, true, ERROR_SUCCESS };
    MfrCopyFileExTests.push_back(t_LocalDoc_3);

    int count = 0;
    for (MfrCopyFileExTest      t : MfrCopyFileExTests)      if (t.enabled) { count++; }

    return count;
}

int InitializeCopyFile2TestArray()
{
    std::wstring tempS;
    std::wstring tempD;


    extendedParameters1.dwSize = sizeof(extendedParameters1);
    extendedParameters1.dwCopyFlags = COPY_FILE_FAIL_IF_EXISTS;
    extendedParameters1.pfCancel = FALSE;
    extendedParameters1.pProgressRoutine = NULL;
    extendedParameters1.pvCallbackContext = NULL;

    extendedParameters2.dwSize = sizeof(extendedParameters2);
    extendedParameters2.dwCopyFlags = 0;
    extendedParameters2.pfCancel = FALSE;
    extendedParameters2.pProgressRoutine = NULL;
    extendedParameters2.pvCallbackContext = NULL;

    

    // Requests to Native File Locations for CopyFile via VFS
    tempS = g_NativePF;
    tempS.append(L"\\PlaceholderTest\\TestIniFileVfsPF.ini");
    tempD = g_NativePF;
    tempD.append(L"\\PlaceholderTest\\Copied21TestIniFileVfsPF.ini");
    MfrCopyFile2Test ts_Native_PF1 = { "MFR+ILV CopyFile2 Native-file VFS exists in package",                          true, true,  true,
                                tempS, tempD, &extendedParameters1, S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(ts_Native_PF1);

    MfrCopyFile2Test ts_Native_PF1E1 = { "MFR+ILV CopyFile2 Native-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, &extendedParameters1, HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), ERROR_FILE_EXISTS };
    MfrCopyFile2Tests.push_back(ts_Native_PF1E1);

    MfrCopyFile2Test ts_Native_PF1E2 = { "MFR+ILV CopyFile2 Native-file VFS exists in package and dest (allowed)",     true, false, false,
                                tempS, tempD, &extendedParameters2, S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(ts_Native_PF1E2);

    tempS = g_NativePF;
    tempS.append(L"\\PlaceholderTest\\MissingNativePlaceholder.txt");
    tempD = g_NativePF;
    tempD.append(L"\\PlaceholderTest\\Copied21MissingNativePlaceholder.txt");
    MfrCopyFile2Test t_Native_PF2 = { "MFR+ILV CopyFile2 Native-file VFS missing in package",                          true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND };
    MfrCopyFile2Tests.push_back(t_Native_PF2);

    tempS = g_NativePF;
    tempS.append(L"\\MissingPlaceholderTest\\MissingNativePlaceholder.txt");
    tempD = g_NativePF;
    tempD.append(L"\\MissingPlaceholderTest\\Copied21MissingNarivePlaceholder.txt");
    MfrCopyFile2Test t_Native_PF3 = { "MFR+ILV CopyFile2 Native-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_PATH_NOT_FOUND, ERROR_PATH_NOT_FOUND, true, ERROR_FILE_NOT_FOUND };
    MfrCopyFile2Tests.push_back(t_Native_PF3);


    // Requests to Package File Locations for CopyFile using VFS
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Copied3Placeholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Copied22Placeholder.txt";
#endif
    MfrCopyFile2Test t_Vfs_PF1 = { "MFR+ILV CopyFile2 Package-file VFS exists in package",                                    true, true,  true,
                                tempS, tempD, &extendedParameters1,S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(t_Vfs_PF1);

    MfrCopyFile2Test ts_Vfs_PF1E1 = { "MFR+ILV CopyFile2 Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_FILE_EXISTS, ERROR_FILE_EXISTS };
    MfrCopyFile2Tests.push_back(ts_Vfs_PF1E1);

    MfrCopyFile2Test ts_Vfs_PF1E2 = { "MFR+ILV CopyFile2 Package-file VFS exists in package and dest (allowed)",     true, false, false,
                                tempS, tempD, &extendedParameters2, S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(ts_Vfs_PF1E2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Copied22MissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Copied22MissingVFSPlaceholder.txt";
#endif
    MfrCopyFile2Test t_Vfs_PF2 = { "MFR+ILV CopyFile2 Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND };
    MfrCopyFile2Tests.push_back(t_Vfs_PF2);

#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\Copied3MissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\Copied22MissingVFSPlaceholder.txt";
#endif
    MfrCopyFile2Test t_Vfs_PF3 = { "MFR+ILV CopyFile2 Package-file VFS parent-folder missing in package",            true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_PATH_NOT_FOUND, ERROR_PATH_NOT_FOUND,
                                true, ERROR_FILE_NOT_FOUND };
    MfrCopyFile2Tests.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for CopyFile using VFS
    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Copied4RedirPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Copied23RedirPlaceholder.txt";
#endif
    MfrCopyFile2Test t_Redir_PF1 = { "MFR+ILV CopyFile2 Redirected-file VFS exists in package",                                   true, true,  true,
                                tempS, tempD, &extendedParameters1, S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(t_Redir_PF1);

    MfrCopyFile2Test ts_Redir_PF1E1 = { "MFR+ILV CopyFile2 Redirected Package-file VFS exists in package and dest (not allowed)", true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_FILE_EXISTS, ERROR_FILE_EXISTS };
    MfrCopyFile2Tests.push_back(ts_Redir_PF1E1);

    MfrCopyFile2Test ts_Redir_PF1E2 = { "MFR+ILV CopyFile2 Redirected Package-file VFS exists in package and dest (allowed)",     true, false, false,
                                tempS, tempD, &extendedParameters2, S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(ts_Redir_PF1E2);

    tempS = g_writablePackageRootPath.c_str();
#if _M_IX86
    tempS.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Copied23MissingVFSPlaceholder.txt";
#else
    tempS.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingVFSPlaceholder.txt");
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Copied23MissingVFSPlaceholder.txt";
#endif
    MfrCopyFile2Test t_Redir_PF2 = { "MFR+ILV CopyFile2 Redirected Package-file VFS missing in package",                          true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND };
    MfrCopyFile2Tests.push_back(t_Redir_PF2);

    // Additional Failure request
#if _M_IX86
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\Copied23MissingVFSPlaceholder.txt";
#else
    tempS = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingVFSPlaceholder.txt";
    tempD = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\Copied23MissingVFSPlaceholder.txt";
#endif
    MfrCopyFile2Test t_Redir_PF3 = { "MFR+ILV CopyFile2 Package-file VFS parent-folder missing in package",              true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_PATH_NOT_FOUND, ERROR_PATH_NOT_FOUND,
                                true, ERROR_FILE_NOT_FOUND };
    MfrCopyFile2Tests.push_back(t_Redir_PF3);

    // Local Documents test
    tempS = g_Cwd + L"\\VFS\\Personal\\MFRTestDocs\\PresonalFile_C3.txt";
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\CopiedPresonalFile_C3.txt");
    MfrCopyFile2Test t_LocalDoc_1 = { "MFR+ILV CopyFile2 Package-file VFS to Local Documents to succeed",              true, true,  true,
                                tempS, tempD, &extendedParameters1, S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(t_LocalDoc_1);

    MfrCopyFile2Test t_LocalDoc_2 = { "MFR+ILV CopyFile2 Package-file VFS to Local Documents to fail",                 true, false, false,
                                tempS, tempD, &extendedParameters1, ERROR_FILE_EXISTS, ERROR_FILE_EXISTS, true, ERROR_ALREADY_EXISTS };
    MfrCopyFile2Tests.push_back(t_LocalDoc_2);

    tempS = tempD;
    tempD = psf::known_folder(FOLDERID_Documents);
    tempD.append(L"\\");
    tempD.append(MFRTESTDOCS);
    tempD.append(L"\\ReCopiedPresonalFile_C3.txt");
    MfrCopyFile2Test t_LocalDoc_3 = { "MFR+ILV CopyFile2 Local Documents to Local Documents to succeed",              true, false, false,
                                tempS, tempD, &extendedParameters1, S_OK, ERROR_SUCCESS };
    MfrCopyFile2Tests.push_back(t_LocalDoc_3);


    int count = 0;
    for (MfrCopyFile2Test      t : MfrCopyFile2Tests)      if (t.enabled) { count++; }
    return count;
}

int InitializeCopyFileTests()
{
    int count = 0;
    count += InitializeCopyFileTestArray();
    count += InitializeCopyFileExTestArray();
    count += InitializeCopyFile2TestArray();
    return count;
}

BOOL CopyFileIndividualTest(MfrCopyFileTest testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "MFR+ILV CopyFile Test: ";
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

        auto testResult = CopyFile(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.FailIfExists);
        if (testResult == 0)
        {
            DWORD cErr = GetLastError();
            // call failed
            if (testInput.Expected_Result)
            {
                // should have succeeded
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L" GetLastError=";
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result =  cErr;
                test_end(cErr);
            }
            else if (cErr == testInput.Expected_LastError || 
                    (testInput.AllowAlternateError && testInput.AlternateError == cErr))
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // different reason, not cool
                std::wstring detail1 = L"ERROR: Although returned value correct. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L"      Expected GetLastError=";
                detail2.append(std::to_wstring(testInput.Expected_LastError));
                if (testInput.AllowAlternateError)
                {
                    detail1.append(L" or ");
                    detail1.append(std::to_wstring(testInput.AlternateError));
                }
                detail2.append(L"      Received GetLastError=");
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
            }
        }
        else
        {
            // call succeeded
            if (testInput.Expected_Result)
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // should have failed
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                result =  -1;
                test_end(-1);
            }
        }
    }
    return result;
}


int CopyFile2IndividualTest(MfrCopyFile2Test testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "MFR+ILV CopyFile2 Test: ";
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

        auto testResult = CopyFile2(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), testInput.extendedParameters);
        if (testResult != 0)
        {
            DWORD cErr = GetLastError();
            // call failed
            if (testInput.Expected_Result == ERROR_SUCCESS)
            {
                // should have succeeded
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L" GetLastError=";
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
            }
            else if (cErr == testInput.Expected_LastError ||
                (testInput.AllowAlternateError && testInput.AlternateError == cErr))
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // different reason, not cool
                std::wstring detail1 = L"ERROR: Although returned value correct. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L"      Expected GetLastError=";
                detail2.append(std::to_wstring(testInput.Expected_LastError));
                if (testInput.AllowAlternateError)
                {
                    detail1.append(L" or ");
                    detail1.append(std::to_wstring(testInput.AlternateError));
                }
                detail2.append(L"      Received GetLastError=");
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
            }
        }
        else
        {
            // call succeeded
            if (testInput.Expected_Result == ERROR_SUCCESS)
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // should have failed
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                result = -1;
                test_end(-1);
            }
        }
    }
    return result;
}


BOOL CopyFileExIndividualTest(MfrCopyFileExTest testInput)
{
    int result = ERROR_SUCCESS;
    if (testInput.enabled)
    {
        std::string testname = "MFR+ILV CopyFileEx Test: ";
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

        auto testResult = CopyFileEx(testInput.TestPathSource.c_str(), testInput.TestPathDestination.c_str(), nullptr, nullptr, nullptr, testInput.dwCopyFlags);
        if (testResult == 0)
        {
            DWORD cErr = GetLastError();
            // call failed
            if (testInput.Expected_Result)
            {
                // should have succeeded
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L" GetLastError=";
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
            }
            else if (cErr == testInput.Expected_LastError ||
                (testInput.AllowAlternateError && testInput.AlternateError == cErr))
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // different reason, not cool
                std::wstring detail1 = L"ERROR: Although returned value correct. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                std::wstring detail2 = L"      Expected GetLastError=";
                detail2.append(std::to_wstring(testInput.Expected_LastError));
                if (testInput.AllowAlternateError)
                {
                    detail1.append(L" or ");
                    detail1.append(std::to_wstring(testInput.AlternateError));
                }
                detail2.append(L"      Received GetLastError=");
                detail2.append(std::to_wstring(cErr));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);
                result = cErr;
                test_end(cErr);
            }
        }
        else
        {
            // call succeeded
            if (testInput.Expected_Result)
            {
                // cool!
                result = ERROR_SUCCESS;
                test_end(ERROR_SUCCESS);
            }
            else
            {
                // should have failed
                std::wstring detail1 = L"ERROR: Returned value incorrect. Expected=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Received=");
                detail1.append(std::to_wstring(testResult));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), error_color);
                result = -1;
                test_end(-1);
            }
        }
    }
    return result;
}


int RunCopyFileTests()
{
    int result = ERROR_SUCCESS;
    BOOL testResult;

    for (MfrCopyFileTest testInput : MfrCopyFileTests)
    {
        if (testInput.enabled)
        {
            testResult = CopyFileIndividualTest(testInput);
            result = result ? result : testResult;
        }
    }



    for (MfrCopyFile2Test testInput : MfrCopyFile2Tests)
    {
        if (testInput.enabled)
        {
            testResult = CopyFile2IndividualTest(testInput);
            result = result ? result : testResult;
        }
    }


    for (MfrCopyFileExTest testInput : MfrCopyFileExTests)
    {
        if (testInput.enabled)
        {
            testResult = CopyFileExIndividualTest(testInput);
            result = result ? result : testResult;
        }
    }


    return result;
}