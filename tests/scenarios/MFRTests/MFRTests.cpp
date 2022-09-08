//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//#define DEBUGPATHTESTING 1

#include <conio.h>
#include <fcntl.h>
#include <io.h>

#include <known_folders.h>
#include <psf_runtime.h>

#include <test_config.h>

///#include "common_paths.h"
#include "file_paths.h"
#include <iostream>

//#include "MfrAttributeTest.h"
//#include "MfrGetProfileTest.h"

namespace details
{
    inline std::wstring appmodel_string(LONG(__stdcall* AppModelFunc)(UINT32*, PWSTR))
    {
        // NOTE: `length` includes the null character both as input and output, hence the +1/-1 everywhere
        UINT32 length = MAX_PATH + 1;
        std::wstring result(length - 1, '\0');

        const auto err = AppModelFunc(&length, result.data());
        if ((err != ERROR_SUCCESS) && (err != ERROR_INSUFFICIENT_BUFFER))
        {
            throw_win32(err, "could not retrieve AppModel string");
        }

        assert(length > 0);
        result.resize(length - 1);
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            check_win32(AppModelFunc(&length, result.data()));
            result.resize(length - 1);
        }

        return result;
    }
}

std::filesystem::path g_PackageRootPath;
std::filesystem::path g_writablePackageRootPath;
std::wstring g_Cwd;
std::filesystem::path g_PFs;
std::filesystem::path g_Documents;
std::wstring g_NativePF;

#if DEBUGPATHTESTING
extern int PlaceholderTest();
#else
extern int InitializeAttributeTests();
extern int RunAttributeTests();

extern int InitializeGetProfileTests();
extern int RunGetProfileTests();

extern int InitializeWriteProfileTests();
extern int RunWriteProfileTests();

extern int InitializeCopyFileTests();
extern int RunCopyFileTests();

extern int InitializeCreateDirectoryTests();
extern int RunCreateDirectoryTests();

extern int InitializeCreateFileTests();
extern int RunCreateFileTests();

extern int InitializeCreateOthersTests();
extern int RunCreateOthersTests();

extern int InitializeMoveFilesTests();
extern int RunMoveFilesTests();

extern int InitializeReplaceFileTests();
extern int RunReplaceFileTests();

extern int InitializeRemoveDeleteTests();
extern int RunRemoveDeleteTests();


extern int InitializeFindFileTests();
extern int RunFindFileTests();

#endif

void InitializeGlobals()
{
    g_PackageRootPath = psf::current_package_path();
    g_writablePackageRootPath = std::filesystem::path(psf::known_folder(FOLDERID_LocalAppData).native()) / L"Packages" / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";
    g_Cwd = std::filesystem::current_path().c_str();
    g_PFs = psf::known_folder(FOLDERID_ProgramFiles);
    g_Documents = psf::known_folder(FOLDERID_Documents);

#if _M_IX86
    g_NativePF = L"C:\\Program Files (x86)";
#else
    g_NativePF = L"C:\\Program Files";
#endif

}

void InitializeFolderMappings()
{
    std::wstring temp;
    int count = 0;
    count += InitializeAttributeTests();
    count += InitializeGetProfileTests();
    count += InitializeWriteProfileTests();
    count += InitializeCopyFileTests();
    count += InitializeCreateDirectoryTests();
    count += InitializeCreateFileTests();
    count += InitializeCreateOthersTests();
    count += InitializeMoveFilesTests();
    count += InitializeReplaceFileTests();
    count += InitializeRemoveDeleteTests();
    count += InitializeFindFileTests();
    test_initialize("Managed File Redirection (MFR) Tests", count);
}


int run()
{
    int result = ERROR_SUCCESS;
    int testResult;

#if DEBUGPATHTESTING
    testResult = PlaceholderTest();
    result = result ? result : testResult;
    return result;
#else
    testResult = RunAttributeTests();
    result = result ? result : testResult;

    testResult = RunGetProfileTests();
    result = result ? result : testResult;

    testResult = RunWriteProfileTests();
    result = result ? result : testResult;

    testResult = RunCopyFileTests();
    result = result ? result : testResult;

    testResult = RunCreateDirectoryTests();
    result = result ? result : testResult;

    testResult = RunCreateFileTests();
    result = result ? result : testResult;

    testResult = RunCreateOthersTests();
    result = result ? result : testResult;

    testResult = RunMoveFilesTests();
    result = result ? result : testResult;

    testResult = RunReplaceFileTests();
    result = result ? result : testResult;

    testResult = RunRemoveDeleteTests();
    result = result ? result : testResult;


    testResult = RunFindFileTests();
    result = result ? result : testResult;


    return result;
#endif
}


int main(int argc, const char** argv)
{
    // Display UTF-16 correctly...
    [[maybe_unused]] auto iret = _setmode(_fileno(stdout), _O_U16TEXT);


    auto result = parse_args(argc, argv);
    if (result == ERROR_SUCCESS)
    {
        InitializeGlobals();

#if DEBUGPATHTESTING
        test_initialize("Managed File Redirection (MFR) Low level tests", 1);
#else
        InitializeFolderMappings();
#endif

        result = run();

        test_cleanup();
    }


    if (!g_testRunnerPipe)
    {
        system("pause");
    }
    return 0;
}



void Log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string str;
    str.resize(256);
    try
    {
        std::size_t count = std::vsnprintf(str.data(), str.size() + 1, fmt, args);
        assert(count >= 0);
        va_end(args);

        if (count > str.size())
        {
            str.resize(count);

            va_list args2;
            va_start(args2, fmt);
            count = std::vsnprintf(str.data(), str.size() + 1, fmt, args2);
            assert(count >= 0);
            va_end(args2);
        }

        str.resize(count);
    }
    catch (...)
    {
        str = fmt;
    }
    ::OutputDebugStringA(str.c_str());
}



