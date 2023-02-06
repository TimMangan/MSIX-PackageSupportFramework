//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrAttributeTests.h"
#include "MfrCleanup.h"
#include <stdio.h>

std::vector<MfrAttributeTest> MfrAttributeFileTests;
std::vector<MfrAttributeTest> MfrAttributeFolderTests;
std::vector<MfrAttributeExTest> MfrAttributeExFileTests;
std::vector<MfrSetAttributeTest> MfrSetAttributeFileTests;
std::vector<MfrSetAttributeTest> MfrSetAttributeFolderTests;



int InitializeAttributeTests()
{
    std::wstring temp;

    // Short Name request to native file.  NOTE: AppInstaller will create shortnames for package folders,
    //  but not files so ANY app shortenting the filename will fail.
    temp = L"C:\\Progra~1\\Shorte~1\\Subdir\\DiffTestFileVfsPF3.ini";
    MfrAttributeTest t_Native_Short1 = { "MFR Native-shortnamefile VFS exists in package", true, true, true,
                                    temp.c_str(), 0x20, ERROR_SUCCESS };
    MfrAttributeFileTests.push_back(t_Native_Short1);


    // Requests to Native File Locations for GetFileAttributes via VFS
    temp = g_NativePF;
    temp.append(L"\\PlaceholderTest\\Placeholder.txt");
    MfrAttributeTest t_Native_PF1 = { "MFR Native-file VFS exists in package", true, false, false, 
                                    temp.c_str(), 0x20, ERROR_SUCCESS };
    MfrAttributeFileTests.push_back(t_Native_PF1);

    temp = g_NativePF;
    temp.append(L"\\PlaceholderTest\\MissingPlaceholder.txt");
    MfrAttributeTest t_Native_PF2 = { "MFR Native-file VFS missing in package", true, false, false,
                                    temp.c_str(), INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Native_PF2);

    temp = g_NativePF;
    temp.append(L"\\MissingPlaceholderTest\\MissingPlaceholder.txt");
    MfrAttributeTest t_Native_PF3 = { "MFR Native-file VFS parent-folder missing in package", true, false, false,
                                    temp.c_str(), INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Native_PF3);

    // Requests to Package File Locations for GetFileAttributes using VFS
#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
#endif
    MfrAttributeTest t_Vfs_PF1 = { "MFR Package-file VFS exists in package", true, false, false, 
                                    temp, 0x20, ERROR_SUCCESS };
    MfrAttributeFileTests.push_back(t_Vfs_PF1);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingPlaceholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingPlaceholder.txt";
#endif
    MfrAttributeTest t_Vfs_PF2 = { "MFR Package-file VFS missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Vfs_PF2);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingPlaceholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingPlaceholder.txt";
#endif
    MfrAttributeTest t_Vfs_PF3 = { "MFR Package-file VFS parent-folder missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Vfs_PF3);


    // Requests to Redirected File Locations for GetFileAttributes using VFS
    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
#endif
    MfrAttributeTest t_Redir_PF1 = { "MFR Redirected-file VFS exists in package", true, true, true, 
                                    temp, 0x20, ERROR_SUCCESS };
    MfrAttributeFileTests.push_back(t_Redir_PF1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingPlaceholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingPlaceholder.txt");
#endif
    MfrAttributeTest t_Redir_PF2 = { "MFR Redirected-file VFS missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Redir_PF2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingPlaceholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingPlaceholder.txt");
#endif
    MfrAttributeTest t_Redir_PF3 = { "MFR Redirected-file VFS parent-folder missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Redir_PF3);



    // Requests to Package File Locations for GetFileAttributes using PVAD
    temp = g_Cwd + L"\\PvadFile1.txt";
    MfrAttributeTest t_Package_PV1 = { "MFR Package-file PVAD exists in package", true, true, true, 
                                    temp, 0x20, ERROR_SUCCESS };
    MfrAttributeFileTests.push_back(t_Package_PV1);

    temp = g_Cwd + L"\\NonExistent1.txt";
    MfrAttributeTest t_Package_PV2 = { "MFR Package-file PVAD missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Package_PV2);

    temp = g_Cwd + L"\\NonExistentFolder\\NonExistent1.txt";
    MfrAttributeTest t_Package_PV3 = { "MFR Package-file PVAD parent-folder missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Package_PV3);



    // Requests to Redirect File Locations for GetFileAttributes using PVAD
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFile1.txt");
    MfrAttributeTest t_Redir_PV1 = { "MFR Redirected-file PVAD exists in package", true, true, true, 
                                    temp, 0x20, ERROR_SUCCESS };
    MfrAttributeFileTests.push_back(t_Redir_PV1);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\NonExistent1.txt");
    MfrAttributeTest t_Redir_PV2 = { "MFR Redirected-file PVAD missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Redir_PV2);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\NonExistentFolder\\NonExistent1.txt");
    MfrAttributeTest t_Redir_PV3 = { "MFR Redirected-file PVAD parent-folder missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFileTests.push_back(t_Redir_PV3);








    // Requests to Native folder Locations for GetFileAttributes via VFS
    temp = g_NativePF;
    temp.append(L"\\PlaceholderTest");
    MfrAttributeTest t_Native_PFo1 = { "MFR Native-folder VFS exists in package", true, true, true,
                                    temp.c_str(), 0x10, ERROR_SUCCESS };
    MfrAttributeFolderTests.push_back(t_Native_PFo1);

    temp = g_NativePF;
    temp.append(L"\\MissingPlaceholderTest");
    MfrAttributeTest t_Native_PFo2 = { "MFR Native-folder VFS missing in package", true, false, false,
                                    temp.c_str(), INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFolderTests.push_back(t_Native_PFo2);

    temp = g_NativePF;
    temp.append(L"\\MissingPlaceholderTest\\MissingPlaceholder2");
    MfrAttributeTest t_Native_PFo3 = { "MFR Native-folder VFS parent-folder missing in package", true, false, false,
                                    temp.c_str(), INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFolderTests.push_back(t_Native_PFo3);


    // Requests to Package Folder Locations for GetFileAttributes using VFS
#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest";
#endif
    MfrAttributeTest t_Vfs_PFo1 = { "MFR Package-folder VFS exists in package", true, true, true, 
                                    temp, 0x10, ERROR_SUCCESS };
    MfrAttributeFolderTests.push_back(t_Vfs_PFo1);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest";
#endif
    MfrAttributeTest t_Vfs_PFo2 = { "MFR Package-folder VFS missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFolderTests.push_back(t_Vfs_PFo2);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingPlaceholder2";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingPlaceholder2";
#endif
    MfrAttributeTest t_Vfs_PFo3 = { "MFR Package-folder VFS parent-folder missing in package", true, false, false,
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFolderTests.push_back(t_Vfs_PFo3);

    // Requests to Redir Folder Locations for GetFileAttributes using VFS
    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest");
#endif
    MfrAttributeTest t_Redir_PFo1 = { "MFR Redirected-folder VFS exists in package", true, true, true, 
                                    temp, 0x10, ERROR_SUCCESS };
    MfrAttributeFolderTests.push_back(t_Redir_PFo1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest");
#endif
    MfrAttributeTest t_Redir_PFo2 = { "MFR Redirected-folder VFS missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrAttributeFolderTests.push_back(t_Redir_PFo2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingPlaceholder2");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingPlaceholder2");
#endif
    MfrAttributeTest t_Redir_PFo3 = { "MFR Redirected-folder VFS parent-folder missing in package", true, false, false, 
                                    temp, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrAttributeFolderTests.push_back(t_Redir_PFo3);




    // Requests to Native File Locations for GetFileAttributesEx via VFS
    temp = g_NativePF;
    temp.append(L"\\PlaceholderTest\\Placeholder.txt");
    MfrAttributeExTest t_Native_ExPF1 = { "MFR Ex Native-file VFS exists in package", true, true, true,
                                    temp.c_str(), true, ERROR_SUCCESS };
    MfrAttributeExFileTests.push_back(t_Native_ExPF1);

    temp = g_NativePF;
    temp.append(L"\\PlaceholderTest\\MissingPlaceholder.txt");
    MfrAttributeExTest t_Native_ExPF2 = { "MFR Ex Native-file VFS missing in package", true, false, false,
                                    temp.c_str(), false, ERROR_FILE_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Native_ExPF2);

    temp = g_NativePF;
    temp.append(L"\\MissingPlaceholderTest\\MissingPlaceholder.txt");
    MfrAttributeExTest t_Native_ExPF3 = { "MFR Ex Native-file VFS parent-folder missing in package", true, false, false,
                                    temp.c_str(), false, ERROR_PATH_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Native_ExPF3);


    // Requests to Package File Locations for GetFileAttributesEx using VFS
#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
#endif
    MfrAttributeExTest t_Vfs_ExPF1 = { "MFR Ex Package-file VFS exists in package", true, true, true, 
                                    temp, true, ERROR_SUCCESS };
    MfrAttributeExFileTests.push_back(t_Vfs_ExPF1);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingPlaceholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingPlaceholder.txt";
#endif
    MfrAttributeExTest t_Vfs_ExPF2 = { "MFR Ex Package-file VFS missing in package", true, false, false, 
                                    temp, false, ERROR_FILE_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Vfs_ExPF2);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingPlaceholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingPlaceholder.txt";
#endif
    MfrAttributeExTest t_Vfs_ExPF3 = { "MFR Ex Package-file VFS parent-folder missing in package", true, false, false, 
                                    temp, false, ERROR_PATH_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Vfs_ExPF3);


    // Requests to Redirected File Locations for GetFileAttributesEx using VFS
    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt");
#endif
    MfrAttributeExTest t_Redir_ExPF1 = { "MFR Ex Redirected-file VFS exists in package", true, true, true, 
                                    temp, true, ERROR_SUCCESS };
    MfrAttributeExFileTests.push_back(t_Redir_ExPF1);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingPlaceholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingPlaceholder.txt");
#endif
    MfrAttributeExTest t_Redir_ExPF2 = { "MFR Ex Redirected-file VFS missing in package", true, false, false,
                                    temp, false, ERROR_FILE_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Redir_ExPF2);

    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingPlaceholder.txt");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingPlaceholder.txt");
#endif
    MfrAttributeExTest t_Redir_ExPF3 = { "MFR Ex Redirected-file VFS parent-folder missing in package", true, false, false, 
                                    temp, false, ERROR_PATH_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Redir_ExPF3);


    // Requests to Package File Locations for GetFileAttributes using PVAD
    temp = g_Cwd + L"\\PvadFile1.txt";
    MfrAttributeExTest t_Package_ExPV1 = { "MFR Package-file PVAD exists in package", true, true, true, 
                                        temp, true, ERROR_SUCCESS };
    MfrAttributeExFileTests.push_back(t_Package_ExPV1);

    temp = g_Cwd + L"\\NonExistent1.txt";
    MfrAttributeExTest t_Package_ExPV2 = { "MFR Package-file PVAD missing in package", true, false, false, 
                                        temp, false, ERROR_FILE_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Package_ExPV2);

    temp = g_Cwd + L"\\NonExistentFolder\\NonExistent1.txt";
    MfrAttributeExTest t_Package_ExPV3 = { "MFR Package-file PVAD parent-folder missing in package", true, false, false, 
                                        temp, false, ERROR_PATH_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Package_ExPV3);



    // Requests to Redirect File Locations for GetFileAttributes using PVAD
    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\PvadFile1.txt");
    MfrAttributeExTest t_Redir_ExPV1 = { "MFR Redirected-file PVAD exists in package", true, true, true, 
                                    temp, true, ERROR_SUCCESS };
    MfrAttributeExFileTests.push_back(t_Redir_ExPV1);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\NonExistent1.txt");
    MfrAttributeExTest t_Redir_ExPV2 = { "MFR Redirected-file PVAD missing in package", true, false, false, 
                                    temp, false, ERROR_FILE_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Redir_ExPV2);

    temp = g_writablePackageRootPath.c_str();
    temp.append(L"\\VFS\\NonExistentFolder\\NonExistent1.txt");
    MfrAttributeExTest t_Redir_ExPV3 = { "MFR Redirected-file PVAD parent-folder missing in package", true, false, false, 
                                    temp, false, ERROR_PATH_NOT_FOUND };
    MfrAttributeExFileTests.push_back(t_Redir_ExPV3);




    // Requests to Native File Locations for SetFileAttributes via VFS
    temp = g_NativePF;
    temp.append(L"\\PlaceholderTest\\Placeholder.txt");
    MfrSetAttributeTest t_Native_SetPF1 = { "MFR Set Native-file VFS exists in package", true, true, true,
                                    temp.c_str(), FILE_ATTRIBUTE_NORMAL, true, 0x20, ERROR_SUCCESS };
    MfrSetAttributeFileTests.push_back(t_Native_SetPF1);

    temp = g_NativePF;
    temp.append(L"\\PlaceholderTest\\MissingPlaceholder.txt");
    MfrSetAttributeTest t_Native_SetPF2 = { "MFR Set Native-file VFS missing in package", true, false, false,
                                    temp.c_str(), FILE_ATTRIBUTE_NORMAL, false, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrSetAttributeFileTests.push_back(t_Native_SetPF2);

    temp = g_NativePF;
    temp.append(L"\\MissingPlaceholderTest\\MissingPlaceholder.txt");
    MfrSetAttributeTest t_Native_SetPF3 = { "MFR Set Native-file VFS parent-folder missing in package", true, false, false,
                                    temp.c_str(), FILE_ATTRIBUTE_NORMAL, false, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrSetAttributeFileTests.push_back(t_Native_SetPF3);

    // Requests to Package File Locations for SetFileAttributes using VFS
#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Placeholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Placeholder.txt";
#endif
    MfrSetAttributeTest t_Vfs_SetPF1 = { "MFR Set Package-file VFS exists in package", true, true, true, 
                                    temp, FILE_ATTRIBUTE_NORMAL, true, 0x20, ERROR_SUCCESS };
    MfrSetAttributeFileTests.push_back(t_Vfs_SetPF1);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\MissingPlaceholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\MissingPlaceholder.txt";
#endif
    MfrSetAttributeTest t_Vfs_SetPF2 = { "MFR Set Package-file VFS missing in package", true, false, false, 
                                    temp, FILE_ATTRIBUTE_NORMAL, false, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrSetAttributeFileTests.push_back(t_Vfs_SetPF2);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\MissingPlaceholderTest\\MissingPlaceholder.txt";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\MissingPlaceholderTest\\MissingPlaceholder.txt";
#endif
    MfrSetAttributeTest t_Vfs_SetPF3 = { "MFR Set Package-file VFS parent-folder missing in package", true, false, false, 
                                    temp, FILE_ATTRIBUTE_NORMAL, false, INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND };
    MfrSetAttributeFileTests.push_back(t_Vfs_SetPF3);

    // Request to set hidden file
    MfrSetAttributeTest t_Native_SetPF1H = { "MFR Set Hidden Native-file VFS exists in package", true, false, false, 
                                    (g_NativePF + L"\\PlaceholderTest\\Placeholder.txt"), FILE_ATTRIBUTE_HIDDEN, true, FILE_ATTRIBUTE_HIDDEN, ERROR_SUCCESS };
    MfrSetAttributeFileTests.push_back(t_Native_SetPF1H);


    // Requests to create folders using SetFileAttribute which is not really a thing?
    MfrSetAttributeTest t_Native_SetPF1D = { "MFR Set Directory Native-folder VFS not in package", true, false, false, 
                                    (g_NativePF + L"\\PlaceholderTest\\SubFolder1"), FILE_ATTRIBUTE_DIRECTORY, false, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrSetAttributeFolderTests.push_back(t_Native_SetPF1D);

#if _M_IX86
    temp = g_Cwd + L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Subfolder2";
#else
    temp = g_Cwd + L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Subfolder2";
#endif
    MfrSetAttributeTest t_Vfs_SetPF1D = { "MFR Set Directory Package-folder VFS not in package", true, false, false, 
                                    temp, FILE_ATTRIBUTE_DIRECTORY, false, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrSetAttributeFolderTests.push_back(t_Vfs_SetPF1D);


    temp = g_writablePackageRootPath.c_str();
#if _M_IX86
    temp.append(L"\\VFS\\ProgramFilesX86\\PlaceholderTest\\Subfolder3");
#else
    temp.append(L"\\VFS\\ProgramFilesX64\\PlaceholderTest\\Subfolder3");
#endif
    MfrSetAttributeTest t_Redir_PF1D = { "MFR Set Directory Redirected-folder VFS not in package", true, false, false, 
                                    temp, FILE_ATTRIBUTE_DIRECTORY, false, INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND };
    MfrSetAttributeFolderTests.push_back(t_Redir_PF1D);

    // This attempt should be blocked by the default configuration PE filetype exclusions on COW
    temp = g_Cwd + L"\\PsfLauncher.exe";
    MfrSetAttributeTest t_Exe_PVAD1 = { "MFR Set Package-file PVAD Exe to Hidden", true, false, false, 
                                    temp, FILE_ATTRIBUTE_HIDDEN, false, INVALID_FILE_ATTRIBUTES, ERROR_ACCESS_DENIED };
    MfrSetAttributeFileTests.push_back(t_Exe_PVAD1);


    int count = 0;
    for (MfrAttributeTest    t : MfrAttributeFileTests)      if (t.enabled) { count++; }
    for (MfrAttributeTest    t : MfrAttributeFolderTests)    if (t.enabled) { count++; }
    for (MfrAttributeExTest  t : MfrAttributeExFileTests)    if (t.enabled) { count++; }
    for (MfrSetAttributeTest t : MfrSetAttributeFileTests)   if (t.enabled) { count++; }
    for (MfrSetAttributeTest t : MfrSetAttributeFolderTests) if (t.enabled) { count++; }
    return count;
} // InitializeAttributeTests()


int RunAttributeTests()
{

    int result = ERROR_SUCCESS;
    for (MfrAttributeTest testInput : MfrAttributeFileTests)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR GetFileAttributes File Test: ";
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

            auto testResult = GetFileAttributes(testInput.TestPath.c_str());
            auto eCode = GetLastError();
            if (testResult == testInput.Expected_Result)
            {
                if (testResult == INVALID_FILE_ATTRIBUTES)
                {
                    if (eCode == testInput.Expected_LastError)
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Setting attributes or error incorrect\n", error_color);
                        std::wstring detail1 = L"       Intended: Att=";
                        detail1.append(std::to_wstring(testInput.Expected_Result));
                        detail1.append(L" Error=");
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), info_color);
                        std::wstring detail2 = L"       Actual: Att=";
                        detail2.append(std::to_wstring(testResult));
                        detail2.append(L" Error=");
                        detail2.append(std::to_wstring(eCode));
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), error_color);

                        result = result ? result : eCode;
                        if (eCode != ERROR_SUCCESS)
                        {
                            test_end(GetLastError());
                        }
                        else
                        {
                            test_end(-1);
                        }
                    }
                }
                else
                {
                    // LastError not reset by native GetFileAttributes call when successful
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                trace_message(L"ERROR: Getting attributes or error incorrect\n", error_color);
                std::wstring detail1 = L"       Intended: Att=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Error=");
                detail1.append(std::to_wstring(testInput.Expected_LastError));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), info_color);
                std::wstring detail2 = L"       Actual: Att=";
                detail2.append(std::to_wstring(testResult));
                detail2.append(L" Error=");
                detail2.append(std::to_wstring(eCode));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);

                result = result ? result : eCode;
                if (eCode != ERROR_SUCCESS)
                {
                    test_end(GetLastError());
                }
                else
                {
                    test_end(-1);
                }
            }
        }
    }

    for (MfrAttributeTest testInput : MfrAttributeFolderTests)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR SetFileAttributes Folder Test: ";
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

            auto testResult = GetFileAttributes(testInput.TestPath.c_str());
            auto eCode = GetLastError();
            if (testResult == testInput.Expected_Result)
            {
                if (testResult == INVALID_FILE_ATTRIBUTES)
                {
                    if (eCode == testInput.Expected_LastError)
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Getting attributes or error incorrect\n", error_color);
                        std::wstring detail1 = L"       Intended: Att=";
                        detail1.append(std::to_wstring(testInput.Expected_Result));
                        detail1.append(L" Error=");
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), info_color);
                        std::wstring detail2 = L"       Actual: Att=";
                        detail2.append(std::to_wstring(testResult));
                        detail2.append(L" Error=");
                        detail2.append(std::to_wstring(eCode));
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), error_color);

                        result = result ? result : eCode;
                        if (eCode != ERROR_SUCCESS)
                        {
                            test_end(GetLastError());
                        }
                        else
                        {
                            test_end(-1);
                        }
                    }
                }
                else
                {
                    // LastError not reset by native GetFileAttributes call when successful
                    result = result ? result : ERROR_SUCCESS;
                    test_end(ERROR_SUCCESS);
                }
            }
            else
            {
                trace_message(L"ERROR: Getting attributes or error incorrect\n", error_color);
                std::wstring detail1 = L"       Intended: Att=";
                detail1.append(std::to_wstring(testInput.Expected_Result));
                detail1.append(L" Error=");
                detail1.append(std::to_wstring(testInput.Expected_LastError));
                detail1.append(L"\n");
                trace_message(detail1.c_str(), info_color);
                std::wstring detail2 = L"       Actual: Att=";
                detail2.append(std::to_wstring(testResult));
                detail2.append(L" Error=");
                detail2.append(std::to_wstring(eCode));
                detail2.append(L"\n");
                trace_message(detail2.c_str(), error_color);

                result = result ? result : eCode;
                if (eCode != ERROR_SUCCESS)
                {
                    test_end(GetLastError());
                }
                else
                {
                    test_end(-1);
                }
            }
        }
    }

    LPVOID fileInformation = malloc(sizeof(WIN32_FILE_ATTRIBUTE_DATA));
    if (fileInformation != NULL)
    {
        for (MfrAttributeExTest testInput : MfrAttributeExFileTests)
        {
            if (testInput.enabled)
            {
                std::string testname = "GetFileAttributesEx File Test: ";
                testname.append(testInput.TestName);
                test_begin(testname);
                auto testResult = GetFileAttributesEx(testInput.TestPath.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, fileInformation );
                auto eCode = GetLastError();
                if (!testInput.Expect_Success)
                {
                    if (testResult == 0)
                    {
                        if (eCode == testInput.Expected_LastError)
                        {
                            result = result ? result : ERROR_SUCCESS;
                            test_end(ERROR_SUCCESS);
                        }
                        else
                        {
                            trace_message(L"ERROR: Getting attributes or error incorrect\n", error_color);
                            std::wstring detail1 = L"       Intended: Att=";
                            detail1.append(std::to_wstring(!testInput.Expect_Success));
                            detail1.append(L" Error=");
                            detail1.append(std::to_wstring(testInput.Expected_LastError));
                            detail1.append(L"\n");
                            trace_message(detail1.c_str(), info_color);
                            std::wstring detail2 = L"       Actual: Att=";
                            detail2.append(std::to_wstring(testResult));
                            detail2.append(L" Error=");
                            detail2.append(std::to_wstring(eCode));
                            detail2.append(L"\n");
                            trace_message(detail2.c_str(), error_color);

                            result = result ? result : eCode;
                            if (eCode != ERROR_SUCCESS)
                            {
                                test_end(GetLastError());
                            }
                            else
                            {
                                test_end(-1);
                            }
                        }
                    }
                    else
                    {
                        // LastError not reset by native GetFileAttributes call when successful
                        result = result ? result : -1;
                        test_end(-1);
                    }
                }
                else
                {
                    if (testResult != 0)
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                    else
                    {
                        trace_message(L"ERROR: Getting attributes or error incorrect\n", error_color);
                        std::wstring detail1 = L"       Intended: ret=";
                        detail1.append(std::to_wstring(!testInput.Expect_Success));
                        detail1.append(L" Error=");
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), info_color);
                        std::wstring detail2 = L"       Actual: ret=";
                        detail2.append(std::to_wstring(testResult));
                        detail2.append(L" Error=");
                        detail2.append(std::to_wstring(eCode));
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), error_color);

                        result = result ? result : eCode;
                        if (eCode != ERROR_SUCCESS)
                        {
                            test_end(GetLastError());
                        }
                        else
                        {
                            test_end(-1);
                        }
                    }
                }
            }
        }
        free(fileInformation);
    }

    for (MfrSetAttributeTest testInput : MfrSetAttributeFileTests)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR SetFileAttributes File Test: ";
            testname.append(testInput.TestName);

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

            test_begin(testname);
            auto testResult = SetFileAttributes(testInput.TestPath.c_str(), testInput.fileAttributes);
            if (testResult == 0)
            {
                // Did not set
                DWORD eCode = GetLastError();
                if (testInput.Expect_Success)
                {
                    trace_message(L"ERROR: Setting attributes or error incorrect\n", error_color);
                    std::wstring detail1 = L"       Intended: Att=";
                    detail1.append(std::to_wstring(testInput.Expect_Success));
                    detail1.append(L" Error=");
                    detail1.append(std::to_wstring(testInput.Expected_LastError));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: Att=";
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
            else
            {
                if (!testInput.Expect_Success)
                {
                    trace_message(L"ERROR: Setting attributes or error incorrect\n", error_color);
                    std::wstring detail1 = L"       Intended: Att=";
                    detail1.append(std::to_wstring(testInput.Expect_Success));
                    //detail1.append(L" Error=");
                    //detail1.append(std::to_wstring(testInput.Expected_LastError));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: Att=";
                    detail2.append(std::to_wstring(testResult));
                    //detail2.append(L" Error=");
                    //detail2.append(std::to_wstring(eCode));
                    detail2.append(L"\n");
                    trace_message(detail2.c_str(), error_color);

                    result = result ? result : -1;
                    test_end(-1);
                }
                else
                {
                    // Verify
                    auto verifyResult = GetFileAttributes(testInput.TestPath.c_str());
                    if (verifyResult == INVALID_FILE_ATTRIBUTES)
                    {
                        DWORD eCode = GetLastError();
                        trace_message(L"ERROR: Re-testing attributes or error incorrect\n", error_color);
                        std::wstring detail1 = L"       Intended: Att=";
                        detail1.append(std::to_wstring(testInput.Expect_Success));
                        detail1.append(L" Error=");
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), info_color);
                        std::wstring detail2 = L"       Actual: Att=";
                        detail2.append(std::to_wstring(verifyResult));
                        detail2.append(L" Error=");
                        detail2.append(std::to_wstring(eCode));
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), error_color);

                        result = result ? result : eCode;
                        test_end(eCode);
                    }
                    else
                    {
                        // If we created the file, the attribute will be FILE_ATTRIBUTE_NORMAL,
                        // but if it already existed as ARCHIVE, for example, it will stay as ARCHIVE if we
                        // asked for NORMAL
                        if (verifyResult == testInput.Expected_AttributeWhenTested ||
                            verifyResult == FILE_ATTRIBUTE_NORMAL)
                        {
                            result = result ? result : ERROR_SUCCESS;
                            test_end(ERROR_SUCCESS);
                        }
                        else
                        {
                            trace_message(L"ERROR: Re-testing attributes or error incorrect\n", error_color);
                            std::wstring detail1 = L"       Intended: Att=";
                            detail1.append(std::to_wstring(testInput.Expected_AttributeWhenTested));
                            detail1.append(L" Error=");
                            //detail1.append(std::to_wstring(testInput.Expected_LastError));
                            //detail1.append(L"\n");
                            trace_message(detail1.c_str(), info_color);
                            std::wstring detail2 = L"       Actual: Att=";
                            detail2.append(std::to_wstring(verifyResult));
                            detail2.append(L" Error=");
                            //detail2.append(std::to_wstring(eCode));
                            //detail2.append(L"\n");
                            trace_message(detail2.c_str(), error_color);

                            result = result ? result : -1;
                            test_end(-1);
                        }
                    }
                }
            }
        }
    }


    for (MfrSetAttributeTest testInput : MfrSetAttributeFolderTests)
    {
        if (testInput.enabled)
        {
            std::string testname = "MFR SetFileAttributes Folder Test: ";
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

            auto testResult = SetFileAttributes(testInput.TestPath.c_str(), testInput.fileAttributes);
            if (testResult == 0)
            {
                // Did not set
                DWORD eCode = GetLastError();
                if (testInput.Expect_Success)
                {
                    trace_message(L"ERROR: Setting attributes or error incorrect\n", error_color);
                    std::wstring detail1 = L"       Intended: Att=";
                    detail1.append(std::to_wstring(testInput.Expect_Success));
                    detail1.append(L" Error=");
                    detail1.append(std::to_wstring(testInput.Expected_LastError));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: Att=";
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
            else
            {
                if (!testInput.Expect_Success)
                {
                    trace_message(L"ERROR: Setting attributes or error incorrect\n", error_color);
                    std::wstring detail1 = L"       Intended: Att=";
                    detail1.append(std::to_wstring(testInput.Expect_Success));
                    //detail1.append(L" Error=");
                    //detail1.append(std::to_wstring(testInput.Expected_LastError));
                    detail1.append(L"\n");
                    trace_message(detail1.c_str(), info_color);
                    std::wstring detail2 = L"       Actual: Att=";
                    detail2.append(std::to_wstring(testResult));
                    //detail2.append(L" Error=");
                    //detail2.append(std::to_wstring(eCode));
                    detail2.append(L"\n");
                    trace_message(detail2.c_str(), error_color);

                    result = result ? result : -1;
                    test_end(-1);
                }
                else
                {
                    // Verify
                    auto verifyResult = GetFileAttributes(testInput.TestPath.c_str());
                    if (verifyResult == INVALID_FILE_ATTRIBUTES)
                    {
                        DWORD eCode = GetLastError();
                        trace_message(L"ERROR: Re-testing attributes or error incorrect\n", error_color);
                        std::wstring detail1 = L"       Intended: Att=";
                        detail1.append(std::to_wstring(testInput.Expect_Success));
                        detail1.append(L" Error=");
                        detail1.append(std::to_wstring(testInput.Expected_LastError));
                        detail1.append(L"\n");
                        trace_message(detail1.c_str(), info_color);
                        std::wstring detail2 = L"       Actual: Att=";
                        detail2.append(std::to_wstring(verifyResult));
                        detail2.append(L" Error=");
                        detail2.append(std::to_wstring(eCode));
                        detail2.append(L"\n");
                        trace_message(detail2.c_str(), error_color);

                        result = result ? result : eCode;
                        test_end(eCode);
                    }
                    else if (verifyResult != testInput.Expected_AttributeWhenTested)
                    {
                        trace_message(L"ERROR: Re-testing attributes or error incorrect\n", error_color);
                        std::wstring detail1 = L"       Intended: Att=";
                        detail1.append(std::to_wstring(testInput.Expected_AttributeWhenTested));
                        detail1.append(L" Error=");
                        //detail1.append(std::to_wstring(testInput.Expected_LastError));
                        //detail1.append(L"\n");
                        trace_message(detail1.c_str(), info_color);
                        std::wstring detail2 = L"       Actual: Att=";
                        detail2.append(std::to_wstring(verifyResult));
                        detail2.append(L" Error=");
                        //detail2.append(std::to_wstring(eCode));
                        //detail2.append(L"\n");
                        trace_message(detail2.c_str(), error_color);

                        result = result ? result : -1;
                        test_end(-1);
                    }
                    else
                    {
                        result = result ? result : ERROR_SUCCESS;
                        test_end(ERROR_SUCCESS);
                    }
                }
            }
        }
    }
    return result;
}
