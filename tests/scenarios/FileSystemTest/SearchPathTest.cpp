//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

auto SearchPathFunc = &::SearchPathW;


static int DoSearchPathTest(const std::function<DWORD(LPCWSTR,LPCWSTR, LPCWSTR,DWORD, LPWSTR, LPWSTR*)>& getFn, const char* getFnName, const std::filesystem::path& path)
{
    trace_messages(L"Querying for: ", info_color, path.native(), new_line);
    trace_messages(L"                  Using function: ", info_color, getFnName, new_line);
    std::wstring folder = path.parent_path().c_str();
    std::wstring filebase = path.filename().c_str();
    std::wstring fileext = filebase.substr(filebase.find_last_of(L"."));
    //filebase = filebase.substr(0, filebase.find_last_of(L"."));
    wchar_t* resultpath = new wchar_t[1024];

    auto dRet = getFn(folder.c_str(), filebase.c_str(), fileext.c_str(),1024,resultpath,NULL);
    if (dRet == 0)
    {
        trace_last_error(L"Failed to find the file");
    }
    else
    {
        std::wstring xx = L"";
        trace_messages(L"return  has value.", info_color, xx, new_line);
    }
    return dRet;
}

int SearchPathTests()
{
    int result = ERROR_SUCCESS;

    auto packageFilePath = g_packageRootPath / L"TestIniFile.ini"; //g_packageRootPath / g_packageFileName;
    std::filesystem::path vfsWindowsPath = L"C:\\Windows\\WindowsFïℓè.txt";

    test_begin("SearchPath Test in package");
    clean_redirection_path();
    auto testResult = DoSearchPathTest(SearchPathFunc, "SearchPath", packageFilePath);
    result = result ? result : testResult;
    test_end(testResult);

    test_begin("SearchPath Test via VFS");
    clean_redirection_path();
    testResult = DoSearchPathTest(SearchPathFunc, "SearchPath", vfsWindowsPath);
    result = result ? result : testResult;
    test_end(testResult);

    return result;
}