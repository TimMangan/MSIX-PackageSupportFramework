
#include <test_config.h>
#include <appmodel.h>
#include <algorithm>
#include <ShlObj.h>
#include <filesystem>
#include <conio.h>
#include <fcntl.h>
#include <io.h>

using namespace std::literals;

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

// The RegLegacyFixup supports only a few intercepts.
// The tests here make routine registry calls that might have once worked but do not when running under MSIX without remediation.

// THe folllowing strings must match with registry keus present in the appropriate section of the package Registry.dat file.
#define TestKeyName_HKCU_Covered         L"Software\\Vendor_Covered"
#define TestKeyName_HKCU_NotCovered      L"Software\\Vendor_NotCovered"

#define TestKeyName_HKLM_Covered         L"SOFTWARE\\Vendor_Covered"        // Registry contains both regular and wow entries so this works.
#define TestKeyName_HKLM_NotCovered      L"SOFTWARE\\Vendor_NotCovered"

#define TestKeyName_Deletion_Allowed1      L"SOFTWARE\\Vendor_Deletion"
#define TestKeyName_Deletion_NotAllowed1  L"SOFTWARE\\Vendor_Deletion\\SubKey"
#define TestKeyName_Deletion_NotAllowed2  L"SOFTWARE\\Vendor_Deletion\\SubKey\\SubKey"

#define TestSubSubKey L"SubKey"
#define TestSubItem  L"SubItem"

#define TestKeyName_Java_Allowed1        L"SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-0016-0000-0001-ABCDEFFEDCBA}"
#define TestKeyName_Java_Allowed2        L"SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-0017-0000-0001-ABCDEFFEDCBA}"
#define TestKeyName_Java_NotAllowed      L"SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-0019-0000-0001-ABCDEFFEDCBA}"
#define TestKeyName_WOW_Java_Allowed1    L"SOFTWARE\\WOW6432NODE\\CLASSES\\CLSID\\{CAFEEFAC-0016-0000-0001-ABCDEFFEDCBA}"
#define TestKeyName_WOW_Java_Allowed2    L"SOFTWARE\\WOW6432NODE\\CLASSES\\CLSID\\{CAFEEFAC-0017-0000-0001-ABCDEFFEDCBA}"
#define TestKeyName_WOW_Java_NotAllowed  L"SOFTWARE\\WOW6432NODE\\CLASSES\\CLSID\\{CAFEEFAC-0019-0000-0001-ABCDEFFEDCBA}"

#define FULL_RIGHTS_ACCESS_REQUEST   KEY_ALL_ACCESS
#define RW_ACCESS_REQUEST            KEY_READ | KEY_WRITE


#pragma region Helper_Functions
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

std::wstring FormatHelperMsg(LPCWSTR testFunction, LPCWSTR path, LSTATUS ExpectedResult, LSTATUS Result)
{
    std::wstring msg = testFunction;
    msg += std::wstring(L" ") + std::wstring(path);
    if (ExpectedResult == ERROR_SUCCESS)
    {
        if (ExpectedResult == Result)
        {
            msg += L" Expected Success";
        }
        else
        {
            WCHAR buffer[128];
            _itow_s(Result, buffer, 128, 16);
            msg += L" Unexpected Failure 0x" + std::wstring(buffer);
        }
    }
    else
    {
        if (ExpectedResult == Result)
        {
            msg += L" Expected Failure";
        }
        else
        {
            WCHAR eBuffer[128];
            _itow_s(ExpectedResult, eBuffer, 128, 16);
            WCHAR rBuffer[128];
            _itow_s(Result, rBuffer, 128, 16);
            msg += L" Unexpected Result 0x" + std::wstring(rBuffer) + L" Expected 0x" + std::wstring(eBuffer);;
        }
    }
    if (ExpectedResult == Result)
        msg += L": Test PASSES.";
    else
        msg += L": Test FAILS.";
    return msg;
} // FormatHelperMsg()
#pragma endregion Helper_Functions


void NotCoveredTests()
{
    DWORD retval = 0;


    REGSAM samFull = FULL_RIGHTS_ACCESS_REQUEST;
    REGSAM sam2R = samFull & ~(DELETE|WRITE_DAC|WRITE_OWNER|KEY_CREATE_SUB_KEY|KEY_CREATE_LINK| KEY_SET_VALUE);
    REGSAM samRW = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY;
    REGSAM samR = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
    DWORD Dispo;


    [[maybe_unused]] HKEY HKCU_Attempt;
    HKEY HKLM_Attempt;
    [[maybe_unused]] HKEY HKCU_Verify;
    HKEY HKLM_Verify;

    test_begin("RegLegacy Test without changes HKCU");




    trace_message(L"The following tests avoid using the fixup and are allowed to fail. The results are dependent on OS version you run on.", console::color::blue, true);

    if (RegOpenKey(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, &HKCU_Attempt) == ERROR_SUCCESS)
    {
        trace_message(L"returned success", console::color::blue, true);
        RegCloseKey(HKCU_Attempt);

        if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, samFull , &HKCU_Attempt) == ERROR_SUCCESS)
        {
            trace_message(L"OpenKeyEx HKCU full rights SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"OpenKeyEx HKCU full rights FAIL", console::color::blue, true);

        }
        if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, sam2R, &HKCU_Attempt) == ERROR_SUCCESS)
        {
            trace_message(L"OpenKeyEx HKCU full rights-Delete SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"OpenKeyEx HKCU full rights-Delete FAIL", console::color::blue, true);

        }
        if (RegCreateKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, NULL, 0, samFull, NULL,&HKCU_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKCU full rights SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKCU full rights FAIL", console::color::blue, true);

        }

        if (RegCreateKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_NotCovered, 0, NULL, 0, sam2R, NULL, &HKCU_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKCU full rights-Delete SUCCESS", console::color::blue, true);

            RegCloseKey(HKCU_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKCU full rights-Delete FAIL", console::color::blue, true);

        }

    }
    else
    {
        trace_message(L"Test2CU key not found", console::color::red, true);
        retval = 2;
    }
    trace_message(L"ready for 2nd test", console::color::blue, true);

    if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, &HKLM_Verify) == ERROR_SUCCESS)
    {
        RegCloseKey(HKLM_Verify);

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, samFull, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM full rights SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM full rights FAIL", console::color::blue, true);

        }
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, sam2R, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM full rights-Delete SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM full rights-Delete FAIL", console::color::blue, true);

        }

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, samRW, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM RW SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM RW FAIL", console::color::blue, true);

        }
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_NotCovered, 0, NULL, 0, samR, NULL, &HKLM_Attempt, &Dispo) == ERROR_SUCCESS)
        {
            trace_message(L"CreateKeyEx HKLM R SUCCESS", console::color::blue, true);

            RegCloseKey(HKLM_Attempt);
        }
        else
        {
            trace_message(L"CreateKeyEx HKLM R FAIL", console::color::blue, true);

        }
    }
    else
    {
        trace_message(L"Test2LM key not found", console::color::red, true);
        retval = 2;
    }
    test_end(retval);
}



DWORD DeletionMarkerTestHelper(HKEY hKey, LPCWSTR path, LSTATUS ExpectedResult)
{
    HKEY HK_Attempt;
    DWORD retval = 0;  // number of unexpected results.
    LSTATUS result = 0;
    REGSAM samR = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
    std::wstring msg;

    result = RegOpenKey(hKey, path, &HK_Attempt);
    msg = FormatHelperMsg(L"RegOpenKey", path, ExpectedResult, result);
    if (result == ExpectedResult)
    {
        trace_message(msg, console::color::blue, true);
    }
    else
    {
        retval++;
        trace_message(msg, console::color::dark_red, true);
    }
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(HK_Attempt);
    }


    result = RegOpenKeyEx(hKey, path, 0, samR, &HK_Attempt);
    msg = FormatHelperMsg(L"RegOpenKeyEx", path, ExpectedResult, result);
    if (result == ExpectedResult)
    {
        trace_message(msg, console::color::blue, true);
    }
    else
    {
        retval++;
        trace_message(msg, console::color::dark_red, true);
    }
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(HK_Attempt);
    }

    std::wstring longerpathEx = path;
    longerpathEx += L"\\StupidNewNameEx";
    DWORD options = 0;
    result = RegCreateKeyEx(hKey, longerpathEx.c_str(), 0, NULL, options, samR, NULL, &HK_Attempt, NULL);
    msg = FormatHelperMsg(L"RegCreateKeyEx", path, ExpectedResult, result);
    if (result == ExpectedResult)
    {
        trace_message(msg, console::color::blue, true);
    }
    else
    {
        retval++;
        trace_message(msg, console::color::dark_red, true);
    }
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(HK_Attempt);
    }


    std::wstring blockedKeyNameOnly = L"HideThis";
    std::wstring blockedKeyFullPath = path;
    if (blockedKeyFullPath.back() != L'\\')
        blockedKeyFullPath += L"\\";
    blockedKeyFullPath += blockedKeyNameOnly;
    DWORD index = 0;
    DWORD nLen = 256;
    wchar_t subName[256];
    //DWORD Reserved = 0;

    bool found = false;
    result = RegOpenKeyEx(hKey, path, 0, samR, &HK_Attempt);
    if (result == ERROR_SUCCESS)
    {

        while ((result = RegEnumKeyEx(HK_Attempt, index, subName, &nLen, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
        {
            //trace_message(L"Debug found subName: " + std::wstring(subName), console::color::white, true);
            if (wcscmp(subName, blockedKeyNameOnly.c_str()) == 0)
            {
                if (result == ExpectedResult)
                {
                    trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" found.", console::color::blue, true);
                }
                else
                {
                    trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" found.", console::color::dark_red, true);
                    retval++;
                }
                found = true;
                break;
            }
            index++;
            nLen = 256;  // reset for next time.
        }
        switch (found)
        {
        case true:
            if (ExpectedResult == ERROR_SUCCESS)
            {
                trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" found.", console::color::blue, true);
            }
            else
            {
                trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" found.", console::color::dark_red, true);
                retval++;
            }
            break;
        case false:
            if (ExpectedResult == ERROR_SUCCESS)
            {
                trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" not found.", console::color::dark_red, true);
                retval++;
            }
            else
            {
                trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" not found.", console::color::blue, true);
            }
            break;
        }
        CloseHandle(HK_Attempt);
    }
    else
    {
        if (ExpectedResult != ERROR_SUCCESS)
        {
            trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" not not testable because parent key could not be opened.", console::color::blue, true);
        }
        else
        {
            trace_message(L"Deleted subkey name " + std::wstring(blockedKeyFullPath) + L" not found  because parent key could not be opened.", console::color::dark_red, true);
            retval++;
        }
    }

    return retval;
}



void DeletionMarkerTests()
{
    LSTATUS retval = 0;
    LSTATUS result = 0;


    test_begin("RegLegacy Test Deletion HKCU Allowed1");
    retval = 0;
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = DeletionMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Deletion_Allowed1, ERROR_SUCCESS);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);


    test_begin("RegLegacy Test Deletion HKCU NotAllowed1");
    retval = 0;
    trace_message(L"The following tests require using the fixup and should fail.", console::color::blue, true);
    result = DeletionMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Deletion_NotAllowed1, ERROR_PATH_NOT_FOUND);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);


    test_begin("RegLegacy Test Deletion HKCU NotAllowed2");
    retval = 0;
    trace_message(L"The following tests require using the fixup and should fail.", console::color::blue, true);
    result = DeletionMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Deletion_NotAllowed2, ERROR_PATH_NOT_FOUND);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);

}

DWORD JavaMarkerTestHelper(HKEY hKey, LPCWSTR path, LSTATUS ExpectedResult)
{
    HKEY HK_Attempt;
    DWORD retval = 0;  // number of unexpected results.
    LSTATUS result = 0;
    REGSAM samR = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE; 
    std::wstring msg;

    result = RegOpenKey(hKey, path, &HK_Attempt);
    msg = FormatHelperMsg(L"RegOpenKey", path, ExpectedResult, result);
    if (result == ExpectedResult)
    {
        trace_message(msg, console::color::blue, true);
    }
    else
    {
        retval++;
        trace_message(msg, console::color::dark_red, true);
    }
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(HK_Attempt);
    }

    result = RegOpenKeyEx(hKey, path, 0, samR, &HK_Attempt);
    msg = FormatHelperMsg(L"RegOpenKeyEx", path, ExpectedResult, result);
    if (result == ExpectedResult)
    {
        trace_message(msg, console::color::blue, true);
    }
    else
    {
        retval++;
        trace_message(msg, console::color::dark_red, true);
    }
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(HK_Attempt);
    }

   
    return retval;
} // JavaMarkerTestHelper()
void JavaMarkerTests()
{
    LSTATUS retval = 0;
    LSTATUS result = 0;


    test_begin("RegLegacy Test Java HKCU Allowed1");
    retval = 0;
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = JavaMarkerTestHelper(HKEY_CURRENT_USER,TestKeyName_Java_Allowed1, ERROR_SUCCESS);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    { 
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);



    test_begin("RegLegacy Test Java HKCU Allowed2");
    retval = 0;
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = JavaMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Java_Allowed2, ERROR_SUCCESS);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);



    test_begin("RegLegacy Test Java HKCU NOT Allowed");
    retval = 0; 
    trace_message(L"The following tests require using the fixup and might fail.", console::color::blue, true);

    result = JavaMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Java_NotAllowed, ERROR_PATH_NOT_FOUND);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);





    test_begin("RegLegacy Test Java HKLM Allowed1");
    retval = 0; 
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = JavaMarkerTestHelper(HKEY_LOCAL_MACHINE, TestKeyName_Java_Allowed1, ERROR_SUCCESS);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);
    

    test_begin("RegLegacy Test Java HKLM Allowed2");
    retval = 0; 
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = JavaMarkerTestHelper(HKEY_LOCAL_MACHINE, TestKeyName_Java_Allowed2, ERROR_SUCCESS);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);


    test_begin("RegLegacy Test Java HKLM NOT Allowed");
    retval = 0; 
    trace_message(L"The following tests require using the fixup and might fail.", console::color::blue, true);

    result = JavaMarkerTestHelper(HKEY_LOCAL_MACHINE, TestKeyName_Java_NotAllowed, ERROR_PATH_NOT_FOUND);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);



    test_begin("RegLegacy Test Java WOW HKLM Allowed1");
    retval = 0; 
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = JavaMarkerTestHelper(HKEY_LOCAL_MACHINE, TestKeyName_WOW_Java_Allowed1, ERROR_SUCCESS);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);


    test_begin("RegLegacy Test Java WOW HKLM Allowed2");
    retval = 0;
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = JavaMarkerTestHelper(HKEY_LOCAL_MACHINE, TestKeyName_WOW_Java_Allowed2, ERROR_SUCCESS);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);


    test_begin("RegLegacy Test Java WOW HKLM NOT Allowed");
    retval = 0; 
    trace_message(L"The following tests require using the fixup and might fail.", console::color::blue, true);

    result = JavaMarkerTestHelper(HKEY_LOCAL_MACHINE, TestKeyName_WOW_Java_NotAllowed, ERROR_PATH_NOT_FOUND);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);


 

}

int wmain(int argc, const wchar_t** argv)
{
    // Display UTF-16 correctly...
 // NOTE: The CRT will assert if we try and use 'cout' with this set
    _setmode(_fileno(stdout), _O_U16TEXT);
    
    auto result = parse_args(argc, argv);
    //std::wstring aumid = details::appmodel_string(&::GetCurrentApplicationUserModelId);
    test_initialize("RegLegacy Tests", 15);

    NotCoveredTests();   // 1 test

    DeletionMarkerTests(); // 3 Tests

    JavaMarkerTests();  //9 Tests

    test_begin("RegLegacy Test ModifyKeyAccess HKCU");
    Log("<<<<<RegLegacyTest ModifyKeyAccess HKCU");
    try
    {
        HKEY HKCU_Verify;
        if (RegOpenKey(HKEY_CURRENT_USER, TestKeyName_HKCU_Covered, &HKCU_Verify) == ERROR_SUCCESS)
        {
            RegCloseKey(HKCU_Verify);
            HKEY HKCU_Attempt;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, TestKeyName_HKCU_Covered, 0, FULL_RIGHTS_ACCESS_REQUEST, &HKCU_Attempt) == ERROR_SUCCESS)
            {
                DWORD size = 128;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                data[0] = 0;
                DWORD type;
                if (RegGetValue(HKCU_Attempt, L"", TestSubItem, RRF_RT_REG_SZ, &type, data, &size) == ERROR_SUCCESS)
                {
                    trace_message(data, console::color::gray, true);
                    trace_messages("HKCU Full Access Rights Request: NO ERROR OCCURED");
                    //print_last_error("NO ERROR OCCURED");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result = GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND;
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(HKCU_Attempt);
            }
            else
            {
                trace_message("Fail to open key. Remediation did not work.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_ACCESS_DENIED;
                print_last_error("Failed to open key");
            }
        }
        else
        {
            trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_PATH_NOT_FOUND;
            print_last_error("Failed to find key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to Modify HKCU Full Access case");
    }
    test_end(result);
    Log("RegLegacyTest ModifyKeyAccess HKCU>>>>>");

    test_begin("RegLegacy Test ModifyKeyAccess HKLM");
    Log("<<<<<RegLegacyTest ModifyKeyAccess HKLM");
    try
    {
        HKEY HKLM_Verify;
        if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, &HKLM_Verify) == ERROR_SUCCESS)
        {
            RegCloseKey(HKLM_Verify);
            HKEY HKLM_Attempt;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKLM_Covered, 0, RW_ACCESS_REQUEST, &HKLM_Attempt) == ERROR_SUCCESS)
            {
                DWORD size = 128;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                data[0] = 0;
                DWORD type;
                if (RegGetValue(HKLM_Attempt, L"", TestSubItem, RRF_RT_REG_SZ, &type, data, &size) == ERROR_SUCCESS)
                {
                    trace_message(data, console::color::gray, true);
                    trace_messages("HKLM RW Access Test: NO ERROR OCCURED");
                    result = 0;
                }
                else
                {
                    trace_message("Failed to find read subItem.", console::color::red, true);
                    result =  GetLastError();
                    if (result == 0)
                        result = ERROR_FILE_NOT_FOUND; 
                    print_last_error("Failed to find read subItem");
                }
                RegCloseKey(HKLM_Attempt);
            }
            else
            {
                trace_message("Fail to open key. Remediation did not work.", console::color::red, true);
                result = GetLastError();
                if (result == 0)
                    result = ERROR_ACCESS_DENIED;
                print_last_error("Failed to open key");
            }
        }
        else
        {
            trace_message("Failed to find key. Most likely a bug in the testing tool.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_PATH_NOT_FOUND;
            print_last_error("Failed to find key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to MOdify HKCU RW Access case");
    }

    test_end(result);
    Log("RegLegacyTest ModifyKeyAccess HKLM>>>>>");



    test_cleanup();
    Sleep(500);
    return result;
}