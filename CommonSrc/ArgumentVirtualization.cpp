//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// ArgumentVirtualization is used to modify the command line parameters of new processes being started (via CreateProcess and friends) 
// with command line arruments that include file paths.  File paths included in the command line will be modified prior to applicatoin
// launch when an equivalent path within the package contains a copy of the file.
//
// NOTE: This function only modifies the file paths of arguments to point to the file in the package.  The application may also, and
// seperately, require the FileRedirectionFixup to be able to make use of the file, such as to modify it.
//

#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <known_folders.h>
#include <psf_constants.h>
#include <psf_framework.h>
#include <psf_logging.h>

#include "Config.h"
#include <StartInfo_helper.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <findStringIC.h>

using namespace std::literals;

//extern std::filesystem::path PackageRootPath();
struct vfs_folder_mapping
{
    std::filesystem::path path;
    std::filesystem::path package_vfs_relative_path; // E.g. "Windows"
}; 
std::vector<vfs_folder_mapping> g_vfsFolderMappings;




std::wstring CanReplaceWithVFS(const std::wstring input)
{
    std::wstring output = L"";
    std::filesystem::path testpath;

    const std::filesystem::path l_PackageRootPath = PSFQueryPackageRootPath();// PackageRootPath();

    // NOTE: Some of these don't exist on 32-bit OSs and that's OK
    try
    {
        testpath = psf::known_folder(FOLDERID_Documents);
        if (findStringIC(input, testpath))
        {
            LogString(L"PackageRootPath is ", l_PackageRootPath.c_str());
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Personal" + input.substr(testpath.wstring().length()).c_str();
            LogString(L"Replacement is ", output.c_str());
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_Documents");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_PublicDocuments);
        if (findStringIC(input, testpath))
        {
            LogString(L"PackageRootPath is ", l_PackageRootPath.c_str());
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Common Documents" + input.substr(testpath.wstring().length()).c_str();
            LogString(L"Replacement is ", output.c_str());
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_PublicDocuments");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_LocalAppData);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Local AppData" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_LocalAppData");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_RoamingAppData);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\AppData" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_RoamingAppData");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_ProgramFilesCommonX86);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\ProgramFilesCommonX86" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_ProgramFilesCommonX86");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_ProgramFilesX86);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\ProgramFilesX86" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_ProgramFilesX86");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_ProgramFilesCommonX64);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\ProgramFilesCommonX64" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_ProgramFilesCommonX64");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_ProgramFilesX64);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\ProgramFilesX64" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_ProgramFilesX64");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_System);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\SystemX64" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_System");
    }
    try
    {
        testpath = psf::known_folder(FOLDERID_SystemX86);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\SystemX86" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_SystemX86");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_Fonts);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Fonts" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_Fonts");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_Windows);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Windows" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_Windows");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_ProgramData);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Common AppData" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_ProgramData");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_PublicDesktop);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Common Desktop" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_PublicDesktop");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_CommonPrograms);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\Common Programs" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_CommonProgram");
    }

    try
    {
        testpath = psf::known_folder(FOLDERID_LocalAppDataLow);
        if (findStringIC(input, testpath))
        {
            output = ((std::wstring)l_PackageRootPath.c_str()) + L"\\VFS\\LOCALAPPDATALOW" + input.substr(testpath.wstring().length()).c_str();
            return output;
        }
    }
    catch (...)
    {
        Log(L"Warning: unknown known folder FOLDERID_LocalAppDataLow");
    }

    return output;
}

std::wstring ArgumentVirtualization(const std::wstring input)
{
    std::wstring output = L"";
    if (findStringIC(input, L"C:\\"))
    {
        size_t offset = 0;
        while (offset < input.length())
        {
            if (findStringIC(input.substr(offset,3), L"C:\\"))
            {

                size_t start = offset;
                size_t len = std::wstring::npos;
                if (offset > 0)
                {
                    if (input[offset - 1] == L'\'')
                    {
                        start = offset;
                        len = input.substr(offset).find_first_of(L'\'');                      
                    }
                    else if (input[offset - 1] == L'\"')
                    {
                        start = offset;
                        len = input.substr(offset).find_first_of(L'\"');
                    }
                    else
                    {
                        start = offset;
                        len = input.substr(offset).find_first_of(L' ');
                        if (len == std::wstring::npos)
                        {
                            len = input.length() - start;
                        }
                    }
                }
                else
                {
                    start = offset;
                    len = input.substr(offset).find_first_of(L' ');
                    if (len == std::wstring::npos)
                    {
                        len = input.length() - start;
                    }
                }
                if (len != std::wstring::npos)
                {
                    std::wstring replacement = CanReplaceWithVFS(input.substr(offset,len));
                    if (replacement.length() != 0)
                    {
                        output.append(replacement); 
                        offset += len;
                    }
                }
                else
                {
                    output.append(1, input[offset]);
                    offset++;
                }
            }
            else
            {
                output.append(1, input[offset]);
                offset++;
            }
            
        }
    }
    else
    {
        return input;
    }
    return output;
}
