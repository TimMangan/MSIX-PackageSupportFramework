//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

#include <io.h>
#include <fcntl.h>
#include <share.h>

auto _wfopenFunc = &::_wfopen;
auto _wfopen_sFunc = &::_wfopen_s;

auto _wopenFunc = &::_wopen;
auto _wsopenFunc = &::_wsopen;
auto _wsopen_sFunc = &::_wsopen_s;

auto _wmkdirFunc = &::_wmkdir;
auto _wrmdirFunc = &::_wrmdir;


static FILE * DoFOpenTest(const std::function<FILE*(LPCWSTR, LPCWSTR)>& getFn, const char* getFnName, const std::filesystem::path& path, const wchar_t * mode)
{
    trace_messages(L"Open for: ", info_color, path.native(), new_line);
    trace_messages(L"             Using function: ", info_color, getFnName, new_line);
    std::wstring filepath = path.c_str();
    


    auto fRet = getFn(filepath.c_str(),mode);
    if (fRet == 0)
    {
        trace_last_error(L"Failed to open the file");
    }
    else
    {
        std::wstring xx = L"";
        trace_messages(L"return opened file.", info_color, xx, new_line);
    }
    return fRet;
}

static errno_t DoFOpen_sTest(const std::function<errno_t (FILE**, LPCWSTR, LPCWSTR)>& getFn, const char* getFnName, FILE** fileHandle, const std::filesystem::path& path, const wchar_t* mode)
{
    trace_messages(L"Open for: ", info_color, path.native(), new_line);
    trace_messages(L"             Using function: ", info_color, getFnName, new_line);
    std::wstring filepath = path.c_str();



    auto fRet = getFn(fileHandle, filepath.c_str(), mode);
    if (fRet != 0)
    {
        trace_last_error(L"Failed to open the file.");
    }
    else
    {
        std::wstring xx = L"";
        trace_messages(L"return opened file.", info_color, xx, new_line);
    }
    return fRet;
}


static int DoWOpenTest(const std::function<int (LPCWSTR, int, int)>& getFn, const char* getFnName, const std::filesystem::path& path, int oflag, int pmode)
{
    trace_messages(L"Open for: ", info_color, path.native(), new_line);
    trace_messages(L"             Using function: ", info_color, getFnName, new_line);
    std::wstring filepath = path.c_str();



    auto fRet = getFn(filepath.c_str(), oflag, pmode);
    if (fRet == 0)
    {
        trace_last_error(L"Failed to open the file");
    }
    else
    {
        std::wstring xx = L"";
        trace_messages(L"return opened file.", info_color, xx, new_line);
    }
    return fRet;
}

static int DoWSOpenTest(const std::function<int(LPCWSTR, int, int, int)>& getFn, const char* getFnName, const std::filesystem::path& path, int oflag, int shFlag, int pmode)
{
    trace_messages(L"Open for: ", info_color, path.native(), new_line);
    trace_messages(L"             Using function: ", info_color, getFnName, new_line);
    std::wstring filepath = path.c_str();



    auto fRet = getFn(filepath.c_str(), oflag, shFlag, pmode);
    if (fRet == 0)
    {
        trace_last_error(L"Failed to open the file");
    }
    else
    {
        std::wstring xx = L"";
        trace_messages(L"return opened file.", info_color, xx, new_line);
    }
    return fRet;
}

static errno_t DoWSOpen_sTest(const std::function<errno_t(int *, LPCWSTR, int, int, int)>& getFn, const char* getFnName, int * fHandle, const std::filesystem::path& path, int oflag, int shFlag, int pmode)
{
    trace_messages(L"Open for: ", info_color, path.native(), new_line);
    trace_messages(L"             Using function: ", info_color, getFnName, new_line);
    std::wstring filepath = path.c_str();



    auto eRet = getFn(fHandle, filepath.c_str(), oflag, shFlag, pmode);
    if (eRet != 0)
    {
        trace_last_error(L"Failed to open the file");
    }
    else
    {
        std::wstring xx = L"";
        trace_messages(L"return opened file.", info_color, xx, new_line);
    }
    return eRet;
}



static int DoWMkdirTest(const std::function<int (LPCWSTR)>& getFn, const char* getFnName, const std::filesystem::path& path, bool expectSuccess)
{
    trace_messages(L"mkdir for: ", info_color, path.native(), new_line);
    trace_messages(L"             Using function: ", info_color, getFnName, new_line);
    std::wstring filepath = path.c_str();



    auto iRet = getFn(filepath.c_str());
    if (iRet == -1)
    {
        if (expectSuccess)
        {
            trace_last_error(L"Failed to create the directory.");
        }
        else
        {
            trace_message(L"Expected failure.");
        }
    }
    else
    {
        if (!expectSuccess)
        {
            trace_messages(L"Directory created unepectedly.",error_color, L"", new_line);
        }
        else
        {
            trace_message(L"Directory created as expected.\n");
        }
    }
    return iRet;
}

static int DoWRmdirTest(const std::function<int(LPCWSTR)>& getFn, const char* getFnName, const std::filesystem::path& path, bool expectSuccess)
{
    trace_messages(L"rmdir for: ", info_color, path.native(), new_line);
    trace_messages(L"             Using function: ", info_color, getFnName, new_line);
    std::wstring filepath = path.c_str();



    auto iRet = getFn(filepath.c_str());
    if (iRet == -1)
    {
        if (expectSuccess)
        {
            trace_last_error(L"Failed to remove the directory unexpectedly.");
        }
        else
        {
            trace_message(L"Failed to remove the directory as expected.\n");
        }
    }
    else
    {
        if (expectSuccess)
        {
            trace_message(L"Directory removed as expected.");
        }
        else
        {
            trace_messages("Directory removed unexpectedly.", error_color, L"", new_line);
        }
    }
    return iRet;
}


static int DoWmkdirTest(const std::filesystem::path& path, bool expectSuccess)
{
    trace_messages(L"Creating directory: ", info_color, path.native(), new_line);
    trace_messages(L"   Expected result: ", info_color, (expectSuccess ? L"Success" : L"Failure"), new_line);
    trace_message(L"Calling with _wmkdir...\n");
    auto resp = _wmkdir(path.c_str());
    if (resp != -1)
    {
        if (!expectSuccess)
        {
            trace_message(L"ERROR: Successfully created directory, but we were expecting failure\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }

        // Trying to create the directory again should fail
        trace_message(L"Directory created successfully. Creating it again should fail...\n");
        resp = _wmkdir(path.c_str());
        if (resp != -1)
        {
            trace_message(L"ERROR: Successfully created directory a second time, but we were expecting failure\n", error_color);
            return ERROR_ASSERTION_FAILURE;
        }
        else
        {
            if (errno == EEXIST)
            {
                trace_message(L"Second attempt expected result ERROR_ALREADY_EXISTS received.\n");
            }
            else
            {
                return trace_last_error(L"Second attempt expected result unexpected: ");
            }
        }
    }
    else if (expectSuccess)
    {
        return trace_last_error(L"Failed to create directory, but we were expecting it to succeed.");
    }

    return ERROR_SUCCESS;
}



extern void Log(const char* fmt, ...);

int WfopenTests([[maybe_unused]] int TestNum)
{
    int SubTestNum = 1;
    int result = ERROR_SUCCESS;
    int testResult;
    auto packageFilePath = g_packageRootPath / L"UCTestIniFile.ini"; //g_packageRootPath / g_packageFileName;
    std::filesystem::path vfsWindowsPath = L"C:\\Windows\\UCWindowsFïℓè.txt";
    std::wstring modeR = L"r";
    std::wstring modeRW = L"r+";

#if _DEBUG
    Log("[%d] [%d] Starting WfopenTests ********************************************************************************************",TestNum, 0);
#endif
    
    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d1]  Cleanup End.", TestNum, SubTestNum);
#endif

    test_begin("_wfopen for Read Test in package");

    auto testFileH = DoFOpenTest(_wfopenFunc, "_wfopen", packageFilePath.c_str(), modeR.c_str());
    if (testFileH == NULL)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        fclose(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wfopen for Read Test via VFS");
    testFileH = DoFOpenTest(_wfopenFunc, "_wfopen", vfsWindowsPath.c_str(), modeR.c_str());
    if (testFileH == NULL)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        fclose(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wfopen for ReadWrite Test in package");
    testFileH = DoFOpenTest(_wfopenFunc, "_wfopen", packageFilePath.c_str(), modeRW.c_str());
    if (testFileH == NULL)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        fclose(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wfopen for ReadWrite Test via VFS");
    testFileH = DoFOpenTest(_wfopenFunc, "_wfopen", vfsWindowsPath.c_str(), modeRW.c_str());
    if (testFileH == NULL)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        fclose(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);



#if _DEBUG
    Log("[%d] [%d] Ending WfopenTests ********************************************************************************************", TestNum,0);
#endif

    return result;
}  
//WfopenTests = 4


int Wfopen_sTests([[maybe_unused]] int TestNum)
{
    int SubTestNum = 1;
    int result = ERROR_SUCCESS;
    int testResult;
    auto packageFilePath = g_packageRootPath / L"UCTestIniFile.ini"; //g_packageRootPath / g_packageFileName;
    std::filesystem::path vfsWindowsPath = L"C:\\Windows\\UCWindowsFïℓè.txt";
    std::wstring modeR = L"r";
    std::wstring modeRW = L"r+";

    FILE* testFileH;

    errno_t errnoS;
    int errFClose = 0;

#if _DEBUG
    Log("[%d] [%d] Starting Wfopen_sTests ********************************************************************************************", TestNum, 0);
#endif

    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wfopen_s for Read Test in package");
    errnoS = DoFOpen_sTest(_wfopen_sFunc, "_wfopen_s", &testFileH, packageFilePath.c_str(), modeR.c_str());
    if (errnoS != ERROR_SUCCESS)
    {
        testResult = errnoS;
        trace_error(errno, L"_wfopen_s failed.");
    }
    else
    {
        testResult = ERROR_SUCCESS;
        errFClose = fclose(testFileH);
        if (errFClose != 0)
        {
            trace_message(L"fclose_error", console::color::yellow, true);
        }
        else
        {
            trace_message("fclose_success", console::color::white, true);
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d] Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wfopen_s for Read Test via VFS");
    errnoS = DoFOpen_sTest(_wfopen_sFunc, "_wfopen_s", &testFileH, vfsWindowsPath.c_str(), modeR.c_str());
    if (errnoS != ERROR_SUCCESS)
    {
        testResult = errnoS;
        trace_error(errno, L"_wfopen_s failed.");
    }
    else
    {
        testResult = ERROR_SUCCESS;
        errFClose = fclose(testFileH);
        if (errFClose != 0)
        {
            trace_message(L"fclose_error", console::color::yellow, true);
        }
        else
        {
            trace_message("fclose_success", console::color::white, true);
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d] Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d] Cleanup End.", TestNum, SubTestNum);
    Log("TEST 3 Crash start.");
#endif
    test_begin("_wfopen_s for ReadWrite Test in package");
    errnoS = DoFOpen_sTest(_wfopen_sFunc, "_wfopen_s", &testFileH, packageFilePath.c_str(), modeRW.c_str());
#if _DEBUG
    Log("TEST 3 Crash return.");
#endif 
    if (errnoS != ERROR_SUCCESS)
    {
        testResult = errnoS;
        trace_error(errno, L"_wfopen_s failed.");
    }
    else
    {
        testResult = ERROR_SUCCESS;
        errFClose = fclose(testFileH);
        if (errFClose != 0)
        {
            trace_message(L"fclose_error", console::color::yellow, true);
        }
        else
        {
            trace_message("fclose_success", console::color::white, true);
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d] Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wfopen_s for ReadWrite Test via VFS");
    errnoS = DoFOpen_sTest(_wfopen_sFunc, "_wfopen_s", &testFileH, vfsWindowsPath.c_str(), modeRW.c_str());
    if (errnoS != ERROR_SUCCESS)
    {
        testResult = errnoS;
        trace_error(errno, L"_wfopen_s failed.");
    }
    else
    {
        testResult = ERROR_SUCCESS;
        errFClose = fclose(testFileH);
        if (errFClose != 0)
        {
            trace_message(L"fclose_error", console::color::yellow, true);
        }
        else
        {
            trace_message("fclose_success", console::color::white, true);
        }
    }
    result = result ? result : testResult;
    test_end(testResult);


#if _DEBUG
    Log("[%d] [%d] Ending Wfopen_sTests ********************************************************************************************", TestNum, 0);
#endif

    return result;
} 
// Wfopen_sTests = 4


int WopenTests([[maybe_unused]] int TestNum)
{
    int SubTestNum = 1;
    int result = ERROR_SUCCESS;
    int testResult;
    auto packageFilePath = g_packageRootPath / L"UCTestIniFile.ini"; //g_packageRootPath / g_packageFileName;
    std::filesystem::path vfsWindowsPath = L"C:\\Windows\\UCWindowsFïℓè.txt";
    std::wstring modeR = L"r";
    std::wstring modeRW = L"r+";

#if _DEBUG
    Log("[%d] [%d] Starting WopenTests ********************************************************************************************", TestNum, 0);
#endif

    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d1]  Cleanup End.", TestNum, SubTestNum);
#endif

    test_begin("_wopen for Read Test in package");

    auto testFileH = DoWOpenTest(_wopenFunc, "_wopen", packageFilePath.c_str(), _O_CREAT| _O_RDONLY, _S_IREAD);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wopen for Read Test via VFS");
    testFileH = DoWOpenTest(_wopenFunc, "_wopen", vfsWindowsPath.c_str(), _O_CREAT | _O_RDONLY, _S_IREAD);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wopen for ReadWrite Test in package");
    testFileH = DoWOpenTest(_wopenFunc, "_wfopen", packageFilePath.c_str(), _O_CREAT | _O_WRONLY | _O_APPEND, _S_IREAD | _S_IWRITE);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wopen for ReadWrite Test via VFS");
    testFileH = DoWOpenTest(_wopenFunc, "_wopen", vfsWindowsPath.c_str(), _O_CREAT | _O_WRONLY | _O_APPEND, _S_IREAD | _S_IWRITE);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);



#if _DEBUG
    Log("[%d] [%d] Ending WopenTests ********************************************************************************************", TestNum, 0);
#endif

    return result;
} 
// WopenTests = 4


int WSopenTests([[maybe_unused]] int TestNum)
{
    int SubTestNum = 1;
    int result = ERROR_SUCCESS;
    int testResult;
    auto packageFilePath = g_packageRootPath / L"UCTestIniFile.ini"; //g_packageRootPath / g_packageFileName;
    std::filesystem::path vfsWindowsPath = L"C:\\Windows\\UCWindowsFïℓè.txt";
    std::wstring modeR = L"r";
    std::wstring modeRW = L"r+";

#if _DEBUG
    Log("[%d] [%d] Starting WSopenTests ********************************************************************************************", TestNum, 0);
#endif

    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d1]  Cleanup End.", TestNum, SubTestNum);
#endif

    test_begin("_wsopen for Read Test in package");

    auto testFileH = DoWSOpenTest(_wsopenFunc, "_wsopen", packageFilePath.c_str(), _O_CREAT | _O_RDONLY, _SH_DENYNO, _S_IREAD);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wsopen for Read Test via VFS");
    testFileH = DoWSOpenTest(_wsopenFunc, "_wsopen", vfsWindowsPath.c_str(), _O_CREAT | _O_RDONLY, _SH_DENYNO, _S_IREAD);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wsopen for ReadWrite Test in package");
    testFileH = DoWSOpenTest(_wsopenFunc, "_wsopen", packageFilePath.c_str(), _O_CREAT | _O_WRONLY | _O_APPEND, _SH_DENYWR, _S_IREAD | _S_IWRITE);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wsopen for ReadWrite Test via VFS");
    testFileH = DoWSOpenTest(_wsopenFunc, "_wsopen", vfsWindowsPath.c_str(), _O_CREAT | _O_WRONLY | _O_APPEND, _SH_DENYWR, _S_IREAD | _S_IWRITE);
    if (testFileH == -1)
    {
        testResult = errno;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);



#if _DEBUG
    Log("[%d] [%d] Ending WSopenTests ********************************************************************************************", TestNum, 0);
#endif

    return result;
} 
// WSopenTests = 4


int WSopen_sTests([[maybe_unused]] int TestNum)
{
    int SubTestNum = 1;
    int result = ERROR_SUCCESS;
    int testResult;
    auto packageFilePath = g_packageRootPath / L"UCTestIniFile.ini"; //g_packageRootPath / g_packageFileName;
    std::filesystem::path vfsWindowsPath = L"C:\\Windows\\UCWindowsFïℓè.txt";
    std::wstring modeR = L"r";
    std::wstring modeRW = L"r+";
    int testFileH;

#if _DEBUG
    Log("[%d] [%d] Starting WSopen_sTests ********************************************************************************************", TestNum, 0);
#endif

    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d1]  Cleanup End.", TestNum, SubTestNum);
#endif

    test_begin("_wsopen_s for Read Test in package");

    auto eRet = DoWSOpen_sTest(_wsopen_sFunc, "_wsopen_s", &testFileH, packageFilePath.c_str(), _O_CREAT | _O_RDONLY, _SH_DENYNO, _S_IREAD);
    if (eRet != 0)
    {
        testResult = eRet;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wsopen_s for Read Test via VFS");
    eRet = DoWSOpen_sTest(_wsopen_sFunc, "_wsopen_s", &testFileH, vfsWindowsPath.c_str(), _O_CREAT | _O_RDONLY, _SH_DENYNO, _S_IREAD);
    if (eRet != 0)
    {
        testResult = eRet;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wsopen_s for ReadWrite Test in package");
    eRet = DoWSOpen_sTest(_wsopen_sFunc, "_wsopen_s", &testFileH, packageFilePath.c_str(), _O_CREAT | _O_WRONLY | _O_APPEND, _SH_DENYWR, _S_IREAD | _S_IWRITE);
    if (eRet != 0)
    {
        testResult = eRet;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d]   Cleanup End.", TestNum, SubTestNum);
#endif
    test_begin("_wsopen_s for ReadWrite Test via VFS");
    eRet = DoWSOpen_sTest(_wsopen_sFunc, "_wsopen_s", &testFileH, vfsWindowsPath.c_str(), _O_CREAT | _O_WRONLY | _O_APPEND, _SH_DENYWR, _S_IREAD | _S_IWRITE);
    if (eRet != 0)
    {
        testResult = eRet;
    }
    else
    {
        testResult = ERROR_SUCCESS;
        _close(testFileH);
    }
    result = result ? result : testResult;
    test_end(testResult); 



#if _DEBUG
    Log("[%d] [%d] Ending WSopen_sTests ********************************************************************************************", TestNum, 0);
#endif

    return result;
} 
// WSopen_sTests = 4


int MkDirTests([[maybe_unused]] int TestNum)
{
    // Note: mimics CreateDirectoryTest
    int result = ERROR_SUCCESS;
    int SubTestNum = 1;



#if _DEBUG
    Log("[%d] [%d] Starting wMkDirTests ********************************************************************************************", TestNum, 0);
#endif

    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d1]  Cleanup End.", TestNum, SubTestNum);
#endif

    // "Fôô" does not exist, we should try and redirect, so creation should succeed.
    test_begin("_wmkdir Create Non-Existing Directory in Package Root Test");

    auto testResult = DoWmkdirTest(L"Fôô", true);
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;

    // "VFS\LocalAppData\FileSystemTest" exists in the package and NewFolder doesn't, so creation should succeed.
    test_begin("_wmkdir Create Non-Existing Directory in Package VFS Test");
    testResult = DoWmkdirTest(L"VFS\\LocalAppData\\FileSystemTest\\NewFolder", true);
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;


    // NOTE: "Tèƨƭ" is a directory that exists in the package path. Therefore, it's reasonable to expect that an attempt
    //       to create that directory would fail. However, due to the limitations around file/directory deletion, we
    //       explicitly ensure that the opposite is true. That is, we give the application the benefit of the doubt that
    //       if it were to try and create the directory "Tèƨƭ" here that it may have previously tried to delete.
    test_begin("_wmkdir Create Existing Directory in Package Root Test");
    testResult = DoWmkdirTest(L"Tèƨƭ", true);
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;


#if _DEBUG
    Log("[%d] [%d] Ending wMkDirTests ********************************************************************************************", TestNum, 0);
#endif

    return result;
}
// wMkDirTests = 3


int MkRmDirTests([[maybe_unused]] int TestNum)
{
    int SubTestNum = 1;
    int result = ERROR_SUCCESS;
    int testResult;
    auto packageFilePath = g_packageRootPath / L"UCTestFolder";
    std::filesystem::path vfsWindowsPath = L"C:\\Windows\\UCTestFolder";
    std::filesystem::path vfsLadPath = psf::known_folder(FOLDERID_LocalAppData) / "FileSystemTest\\UCTestFolder";

#if _DEBUG
    Log("[%d] [%d] Starting MkRmDirTests ********************************************************************************************", TestNum, 0);
#endif

    trace_message(L"Cleanup start.\n");
#if _DEBUG
    Log("[%d] [%d]  Cleanup Start.", TestNum, SubTestNum);
#endif
    clean_redirection_path();
    trace_message(L"Cleanup end.\n");
#if _DEBUG
    Log("[%d] [%d1]  Cleanup End.", TestNum, SubTestNum);
#endif


    // "Fôô" does not exist, we should try and redirect, so creation should succeed.
    test_begin("_wmkdir/_wrmdir for Non-Existing Directory in Package Root Test.");

    auto eRet = DoWMkdirTest(_wmkdirFunc, "_wmkdir", L"Fôô", true);
    if (eRet != 0)
    {
        testResult = eRet;
        trace_message(L"wmkdir failed, skipping removal.\n");
    }
    else
    {
        testResult = ERROR_SUCCESS;
        eRet = DoWRmdirTest(_wrmdirFunc, "_wrmdir", L"Fôô", true);
        if (eRet != 0)
        {
            testResult = errno;
        }
        else
        {
            testResult = ERROR_SUCCESS;
        }
    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;



    // "VFS\LocalAppData\FileSystemTest" exists in the package and NewFolder doesn't, so creation should succeed.
    test_begin("_wmkdir Create Non-Existing Directory in Package VFS Test");
    testResult = DoWMkdirTest(_wmkdirFunc, "_wmkdir", L"VFS\\LocalAppData\\FileSystemTest\\NewFolder", true);
    if (eRet != 0)
    {
        testResult = eRet;
        trace_message(L"wmkdir failed, skipping removal.\n");
    }
    else
    {
        testResult = ERROR_SUCCESS;
        eRet = DoWRmdirTest(_wrmdirFunc, "_wrmdir", L"VFS\\LocalAppData\\FileSystemTest\\NewFolder", true);
        if (eRet != 0)
        {
            testResult = errno;
        }
        else
        {
            testResult = ERROR_SUCCESS;
        }

    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;


    // NOTE: "Tèƨƭ" is a directory that exists in the package path. Therefore, it's reasonable to expect that an attempt
    //       to create that directory would fail. However, due to the limitations around file/directory deletion, we
    //       explicitly ensure that the opposite is true. That is, we give the application the benefit of the doubt that
    //       if it were to try and create the directory "Tèƨƭ" here that it may have previously tried to delete.
    test_begin("_wmkdir Create Existing Directory in Package Root Test");
    eRet = DoWMkdirTest(_wmkdirFunc, "_wmkdir", L"Tèƨƭ", true);
    if (eRet != 0)
    {
        testResult = eRet;
        trace_message(L"wmkdir failed, skipping removal.\n");
    }
    else
    {
        testResult = ERROR_SUCCESS;
        eRet = DoWRmdirTest(_wrmdirFunc, "_wrmdir", L"Tèƨƭ", true);
        if (eRet != 0)
        {
            testResult = errno;
        }
        else
        {
            testResult = ERROR_SUCCESS;
        }

    }
    result = result ? result : testResult;
    test_end(testResult);
    SubTestNum++;


#if _DEBUG
    Log("[%d] [%d] Ending MkRmDirTests ********************************************************************************************", TestNum, 0);
#endif

    return result;
}
// MkRmDirTests = 3
