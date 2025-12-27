
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

// THe following strings must match with registry keys present in the appropriate section of the package Registry.dat file.
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
} // FormatHelperMsg() // FormatHelperMsg()
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
} // NotCoveredTests()


DWORD DeletionMarkerTestHelper(HKEY hKey, LPCWSTR path, LSTATUS ExpectedBasePathResult, LSTATUS StupidExpectedResult, LSTATUS HideExpectedResult)
{
    HKEY HK_Attempt;
    DWORD retval = 0;  // number of unexpected results.
    LSTATUS result = 0;
    REGSAM samR = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
    std::wstring msg;

    result = RegOpenKey(hKey, path, &HK_Attempt);
    msg = FormatHelperMsg(L"RegOpenKey", path, ExpectedBasePathResult, result);
    if (result == ExpectedBasePathResult)
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
    msg = FormatHelperMsg(L"RegOpenKeyEx", path, ExpectedBasePathResult, result);
    if (result == ExpectedBasePathResult)
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
    msg = FormatHelperMsg(L"RegCreateKeyEx", longerpathEx.c_str(), StupidExpectedResult, result);
    if (result == StupidExpectedResult)
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
    blockedKeyFullPath += L"\\" + blockedKeyNameOnly;

    DWORD index = 0;
    DWORD nLen = 256;
    wchar_t subName[256];
    //DWORD Reserved = 0;

    //trace_message(L"Debug about to test: " + blockedKeyNameOnly + L" ", console::color::white, true);

    bool found = false;
    result = RegOpenKeyEx(hKey, path, 0, samR, &HK_Attempt);
    if (result == ERROR_SUCCESS)
    {
        //trace_message(L"parent key did open...", console::color::white, true);
        DWORD n = 0;
        result = RegQueryInfoKey(HK_Attempt, NULL, NULL, NULL, &n, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        //WCHAR bufferN[128];
        //_itow_s(n, bufferN, 128, 16);
        //if (result == ERROR_SUCCESS)
        //    trace_message(L"subkeys under parent: " + std::wstring(bufferN), console::color::white, true);
        //else
        //    trace_message(L"RegQueryInfoKey failed to get subkey count", console::color::dark_red, true);

        while ((result = RegEnumKeyEx(HK_Attempt, index, subName, &nLen, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
        {
            //trace_message(L"Debug found subName: " + std::wstring(subName), console::color::white, true);
            if (wcscmp(subName, blockedKeyNameOnly.c_str()) == 0)
            {
                if (result == HideExpectedResult)
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
        //WCHAR buffer[128];
        //_itow_s(result, buffer, 128, 16);
        //trace_message(L" final enum result =0x" + std::wstring(buffer) + L"  (note 0=Success, 0xea=NO_MORE_DATA 0x103=NO_MORE_ITEMS)", console::color::white, true);

        switch (found)
        {
        case true:
            if (HideExpectedResult == ERROR_SUCCESS)
            {
                trace_message(L"Potentially hidden subkey name " + std::wstring(blockedKeyFullPath) + L" found.", console::color::blue, true);
            }
            else
            {
                trace_message(L"Potentially hidden subkey name " + std::wstring(blockedKeyFullPath) + L" found.", console::color::dark_red, true);
                retval++;
            }
            break;
        case false:
            if (HideExpectedResult == ERROR_SUCCESS)
            {
                trace_message(L"Potentially hidden subkey name " + std::wstring(blockedKeyFullPath) + L" not found.", console::color::dark_red, true);
                retval++;
            }
            else
            {
                trace_message(L"Potentially hidden subkey name " + std::wstring(blockedKeyFullPath) + L" not found.", console::color::blue, true);
            }
            break;
        }
        CloseHandle(HK_Attempt);
    }
    else
    {
        if (ExpectedBasePathResult != ERROR_SUCCESS)
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
} // DeletionMarkerTestHelper()

void DeletionMarkerTests()
{
    LSTATUS retval = 0;
    LSTATUS result = 0;


    test_begin("RegLegacy Test Deletion HKCU Allowed1");
    retval = 0;
    trace_message(L"The following tests avoid using the fixup and should succeed.", console::color::blue, true);
    result = DeletionMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Deletion_Allowed1, ERROR_SUCCESS, ERROR_SUCCESS, ERROR_SUCCESS);
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
    result = DeletionMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Deletion_NotAllowed1, ERROR_SUCCESS, ERROR_SUCCESS, ERROR_PATH_NOT_FOUND);
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
    result = DeletionMarkerTestHelper(HKEY_CURRENT_USER, TestKeyName_Deletion_NotAllowed2, ERROR_FILE_NOT_FOUND, ERROR_SUCCESS,ERROR_PATH_NOT_FOUND);
    if (result == 0)
    {
        retval = ERROR_SUCCESS;
    }
    else
    {
        retval = ERROR_UNIDENTIFIED_ERROR;
    }
    test_end(retval);

} // DeletionMarkerTests()


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
} // JavaMarkerTestHelper() // JavaMarkerTestHelper()
void JavaMarkerTests()
{
    LSTATUS retval = 0;
    LSTATUS result = 0;


    test_begin("RegLegacy Test Java HKCU Allowed1 (HKCU)");
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



    test_begin("RegLegacy Test Java HKCU Allowed2 (HKCU)");
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



    test_begin("RegLegacy Test Java HKCU NOT Allowed (HKCU)");
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





    test_begin("RegLegacy Test Java HKLM Allowed1 (HKLM)");
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
    

    test_begin("RegLegacy Test Java HKLM Allowed2 (HKLM)");
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


    test_begin("RegLegacy Test Java HKLM NOT Allowed (HKLM)");
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



    test_begin("RegLegacy Test Java WOW HKLM Allowed1 (HKLM)");
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


    test_begin("RegLegacy Test Java WOW HKLM Allowed2 (HKLM)");
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


    test_begin("RegLegacy Test Java WOW HKLM NOT Allowed (HKLM)");
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

} // JavaMarkerTests()


void StandardKeyAccessTests()
{
    LSTATUS result = 0;
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
                DWORD size = 256;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                for (DWORD index = 0; index < size; index++)
                    data[index] = 0;
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
                    result = GetLastError();
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

} // StandardKeyAccessTests()

void HKLMWriteTests()
{
    //LSTATUS retval = 0;
    LSTATUS result = 0;


    //REGSAM samFull = FULL_RIGHTS_ACCESS_REQUEST;
    //REGSAM sam2R = samFull & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK | KEY_SET_VALUE);
    //REGSAM samRW = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY;
    //REGSAM samR = READ_CONTROL | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
    //DWORD Dispo;



    test_begin("RegLegacy Test Enumerate HKLM subkeys (pre-writes)");
    Log("<<<<<RegLegacyTest Enumerate HKLM subkeys (pre-writes)");
    try
    {
        int count = 0;
        int index = 0;
        HKEY baseKey;
        result = RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Vendor_Covered", &baseKey);
        if (result == ERROR_SUCCESS)
        {
            bool done = false;
            wchar_t* lpName = (wchar_t*)malloc(256 * sizeof(wchar_t));
            DWORD  cchNameLen;
            while (!done)
            {
                cchNameLen = 255;
                result = RegEnumKey(baseKey, index, lpName, cchNameLen);
                if (result == ERROR_SUCCESS)
                {
                    count++;
                    index++;
                    trace_message("   KeyName=", console::color::white, false);
                    trace_message(lpName, console::color::white, true);
                }
                else if (result == ERROR_NO_MORE_ITEMS)
                {
                    done = true;
                }
                else
                {
                    trace_message("Fail to enumerate key. May be error in testing code.", console::color::red, true);
                    done = true;
                }
            }
            if (count == 1)
            {
                trace_message("Correct count of subKeys achieved.", console::color::blue, true);
                result = ERROR_SUCCESS;
            }
            else
            {
                trace_message("Fail to get correct count. Counted=", console::color::red, false);
                char sNum[16];
                _itoa_s((int)count, sNum, 16, 10);
                trace_message(sNum, console::color::red, true);
                result = -1;
            }
            free(lpName);
        }
        else
        {
            trace_message("Fail to open key. May be error in testing code.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_ACCESS_DENIED;
            print_last_error("Failed to open key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to Enumerate HKLM subkeys (pre-writes)");
    }
    test_end(result);
    Log("RegLegacyTest Enumerate HKLM subkeys (pre writes)>>>>>");



    test_begin("RegLegacy Test Enumerate HKLM subitems (pre-writes)");
    Log("<<<<<RegLegacyTest Enumerate HKLM subitems (pre-writes)");
    try
    {
        int count = 0;
        int index = 0;
        HKEY baseKey;
        result = RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Vendor_Covered", &baseKey);
        if (result == ERROR_SUCCESS)
        {
            bool done = false;
            DWORD MaxValueNameLen = 255;
            DWORD MaxValueDataLen = 1024;
            wchar_t* lpName = (wchar_t*)malloc((MaxValueNameLen+1) * sizeof(wchar_t));
            DWORD  cchNameLen;
            DWORD type;
            BYTE* data = (BYTE*)malloc(MaxValueDataLen);
            DWORD dataLen;
            while (!done)
            {
                cchNameLen = MaxValueNameLen;
                dataLen = MaxValueDataLen;
                result = RegEnumValue(baseKey, index, lpName, &cchNameLen, NULL, &type, data, &dataLen);
                if (result == ERROR_SUCCESS)
                {
                    count++;
                    index++;
                    trace_message("   ValueName=", console::color::white, false);
                    trace_message(lpName, console::color::white, false);
                    char sNum[16];
                    _itoa_s((int)type, sNum, 16, 10);
                    trace_message(" type=0x", console::color::white, false);
                    trace_message(sNum, console::color::white, true);
                }
                else if (result == ERROR_NO_MORE_ITEMS)
                {
                    done = true;
                }
                else
                {
                    trace_message("Fail to enumerate key. May be error in testing code.", console::color::red, true);
                    done = true;
                }
            }
            if (count == 2)
            {
                trace_message("Correct count of subItems achieved.", console::color::blue, true);
                result = ERROR_SUCCESS;
            }
            else
            {
                trace_message("Fail to get correct count. Counted=", console::color::red, false);
                char sNum[16];
                _itoa_s((int)count, sNum, 16, 10);
                trace_message( sNum, console::color::red, true);
                result = -1;
            }
        }
        else
        {
            trace_message("Fail to open key. May be error in testing code.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_ACCESS_DENIED;
            print_last_error("Failed to open key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to Enumerate HKLM subitems (pre-writes)");
    }
    test_end(result);
    Log("RegLegacyTest Enumerate HKLM subitems (pre writes)>>>>>");




    test_begin("RegLegacy Test ModifyKeyAccess HKLM");
    Log("<<<<<RegLegacyTest ModifyKeyAccess HKLM");
    try
    {
        HKEY HKLM_Verify;
        if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKCU_Covered, &HKLM_Verify) == ERROR_SUCCESS)
        {
            RegCloseKey(HKLM_Verify);
            HKEY HKLM_Attempt;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TestKeyName_HKCU_Covered, 0, FULL_RIGHTS_ACCESS_REQUEST, &HKLM_Attempt) == ERROR_SUCCESS)
            {
                DWORD size = 256;  // must be big enough for the test registry item string in the registry file.
                wchar_t* data = new wchar_t[size];
                for (DWORD index = 0; index < size; index++)
                    data[index] = 0;
                DWORD type;
                if (RegGetValue(HKLM_Attempt, L"", TestSubItem, RRF_RT_REG_SZ, &type, data, &size) == ERROR_SUCCESS)
                {
                    trace_message(data, console::color::gray, true);
                    trace_message("HKLM Full Access Rights Request: NO ERROR OCCURED", console::color::blue, true);
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
        print_last_error("Failed to Modify HKLM Full Access case");
    }
    test_end(result);
    Log("RegLegacyTest ModifyKeyAccess HKLM>>>>>");



    test_begin("RegLegacy Test Add Key HKLM");
    Log("<<<<<RegLegacyTest Add Key HKLM");
    try
    {
        HKEY HKLM_Verify;
        if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKCU_Covered, &HKLM_Verify) == ERROR_SUCCESS)
        {
            HKEY HKLM_AddedKey;
            DWORD dispo;
            result = RegCreateKeyEx(HKLM_Verify, L"AddedKey", 0, NULL, 0, FULL_RIGHTS_ACCESS_REQUEST, NULL, &HKLM_AddedKey, &dispo);
            if (result == ERROR_SUCCESS)
            {
                trace_message("Success in creating subkey under HKLM.", console::color::blue, true);
                RegCloseKey(HKLM_AddedKey);
            }
            else
            {
                trace_message("Fail to create key. Remediation did not work.", console::color::red, true);
                print_last_error("Failed to create key");
            }
            RegCloseKey(HKLM_Verify);
        }
        else
        {
            trace_message("Failed to open base key. Most likely a bug in the testing tool.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_PATH_NOT_FOUND;
            print_last_error("Failed to open base key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to create key due to exception");
    }
    test_end(result);
    Log("RegLegacyTest  Add Key HKLM>>>>>");


    test_begin("RegLegacy Test Add Item HKLM");
    Log("<<<<<RegLegacyTest Add Item HKLM");
    try
    {
        HKEY HKLM_Verify;
        if (RegOpenKey(HKEY_LOCAL_MACHINE, TestKeyName_HKCU_Covered, &HKLM_Verify) == ERROR_SUCCESS)
        {
            LPCTSTR data =  L"This is some added item value";
            //result = RegSetValue(HKLM_Verify, L"AddedExtraItem", REG_SZ, data, (DWORD)(wcslen(data)));
            result = RegSetValueEx(HKLM_Verify, L"AddedExtraItem", 0, REG_SZ, (BYTE*)data, (DWORD)(wcslen(data)));
            if (result == ERROR_SUCCESS)
            {
                trace_message("Success in creating item under HKLM.", console::color::blue, true);
            }
            else
            {
                trace_message("Fail to create item. Remediation did not work.", console::color::red, true);
                print_last_error("Failed to create item");
            }
            RegCloseKey(HKLM_Verify);
        }
        else
        {
            trace_message("Failed to open base key. Most likely a bug in the testing tool.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_PATH_NOT_FOUND;
            print_last_error("Failed to open base key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to create item due to exception");
    }
    test_end(result);
    Log("RegLegacyTest  Add  Item HKLM>>>>>");




    test_begin("RegLegacy Test Enumerate HKLM subkeys (post-writes)");
    Log("<<<<<RegLegacyTest Enumerate HKLM subkeys (post-writes)");
    try
    {
        int count = 0;
        int index = 0;
        HKEY baseKey;
        result = RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Vendor_Covered", &baseKey);
        if (result == ERROR_SUCCESS)
        {
            DWORD MaxValueNameLen = 255;
            wchar_t* lpName = (wchar_t*)malloc((MaxValueNameLen+1) * sizeof(wchar_t));
            DWORD  cchNameLen;
            bool done = false;
            while (!done)
            {
                cchNameLen = MaxValueNameLen;
                result = RegEnumKey(baseKey, index, lpName, cchNameLen);
                if (result == ERROR_SUCCESS)
                {
                    count++;
                    index++;
                    trace_message("   KeyName=", console::color::white, false);
                    trace_message(lpName, console::color::white, true);
                }
                else if (result == ERROR_NO_MORE_ITEMS)
                {
                    done = true;
                }
                else
                {
                    trace_message("Fail to enumerate key. May be error in testing code.", console::color::red, true);
                    done = true;
                }
            }
            if (count == 2)
            {
                trace_message("Correct count of subKeys achieved.", console::color::blue, true);
                result = ERROR_SUCCESS;
            }
            else
            {
                trace_message("Fail to get correct count. Counted=", console::color::red, false);
                char sNum[16];
                _itoa_s((int)count, sNum, 16, 10);
                trace_message(sNum, console::color::red, true);
                result = -1;
            }
            free(lpName);
        }
        else
        {
            trace_message("Fail to open key. May be error in testing code.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_ACCESS_DENIED;
            print_last_error("Failed to open key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to Enumerate HKLM subkeys (post-writes)");
    }
    test_end(result);
    Log("RegLegacyTest Enumerate HKLM subkeys (post writes)>>>>>");





    test_begin("RegLegacy Test Enumerate HKLM subitems (post-write)");
    Log("<<<<<RegLegacyTest Enumerate HKLM subitems (post-writes)");
    try
    {
        int count = 0;
        int index = 0;
        HKEY baseKey;
        result = RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Vendor_Covered", &baseKey);
        if (result == ERROR_SUCCESS)
        {
            bool done = false;
            DWORD MaxValueNameLen = 255;
            DWORD MaxValueDataLen = 2048;
            wchar_t* lpName = (wchar_t*)malloc((MaxValueNameLen + 1) * sizeof(wchar_t));
            DWORD  cchNameLen;
            DWORD type;
            BYTE* data = (BYTE*)malloc(MaxValueDataLen);
            DWORD dataLen;
            while (!done)
            {
                cchNameLen = MaxValueNameLen;
                dataLen = MaxValueDataLen;
                result = RegEnumValue(baseKey, index, lpName, &cchNameLen, NULL, &type, data, &dataLen);
                if (result == ERROR_SUCCESS)
                {
                    count++;
                    index++;
                    trace_message("   ValueName=", console::color::white, false);
                    trace_message(lpName, console::color::white, true);
                }
                else if (result == ERROR_NO_MORE_ITEMS)
                {
                    done = true;
                }
                else
                {
                    trace_message("Fail to enumerate key. May be error in testing code.", console::color::red, true);
                    done = true;
                }
            }
            if (count == 3)
            {
                trace_message("Correct count of subItems achieved.", console::color::blue, true);
                result = ERROR_SUCCESS;
            }
            else
            {
                trace_message("Fail to get correct count. Counted=", console::color::red, false);
                char sNum[16];
                _itoa_s((int)count, sNum, 16, 10);
                trace_message(sNum, console::color::red, true);
                result = -1;
            }
        }
        else
        {
            trace_message("Fail to open key. May be error in testing code.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_ACCESS_DENIED;
            print_last_error("Failed to open key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to Enumerate HKLM subitems (post-writes)");
    }
    test_end(result);
    Log("RegLegacyTest Enumerate HKLM subitems (post writes)>>>>>");


    test_begin("RegLegacy Test Enumerate HKLM subitems (system key)");
    Log("<<<<<RegLegacyTest Enumerate HKLM subitems (system key)");
    try
    {
        DWORD count = 0;
        DWORD index = 0;
        HKEY baseKey;
        result = RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink", &baseKey);
        if (result == ERROR_SUCCESS)
        {
            DWORD CountSubKeys = 0;
            DWORD CountSubItems = 0;
            DWORD MaxSubKeyNameLen = 0;
            DWORD MaxSubItemNameLen = 0;
            DWORD MaxSubItemValeDataLen = 0;
            result = RegQueryInfoKeyW(baseKey, NULL, NULL, NULL,
                                        &CountSubKeys, &MaxSubKeyNameLen, NULL,
                                        &CountSubItems, &MaxSubItemNameLen, &MaxSubItemValeDataLen,
                                        NULL, NULL);
            if (result == ERROR_SUCCESS)
            {
                char sNum[16];
                _itoa_s((int)CountSubItems, sNum, 16, 10);
                trace_message("Expected SubItem count=", console::color::blue, false);
                trace_message(sNum, console::color::blue, true);
            }
            else
            {
                trace_message("Failed to query key info.", console::color::red, true);
                print_last_error("Failed to query key info");
            }
            bool done = false;
            DWORD  MaximalNameLen = 0x20;
            if (MaxSubItemNameLen+1 > MaximalNameLen)
                MaximalNameLen = MaxSubItemNameLen + 1;
            wchar_t* lpName = (wchar_t*)malloc(MaxSubItemNameLen * sizeof(wchar_t));
            DWORD type;
            DWORD MaximalDatalen = 0x200;
            if (MaxSubItemValeDataLen > MaximalDatalen)
                MaximalDatalen = MaxSubItemValeDataLen;
            BYTE* data = (BYTE*)malloc(MaximalDatalen);
            while (!done)
            {
                DWORD local_cchNameLen = MaximalNameLen;
                DWORD local_dataLen = MaximalDatalen;
                result = RegEnumValue(baseKey, index, lpName, &local_cchNameLen, NULL, &type, data, &local_dataLen);
                if (result == ERROR_SUCCESS)
                {
                    count++;
                    index++;
                    trace_message("   SUCCESS ValueName=", console::color::white, false);
                    trace_message(lpName, console::color::white, true);
                }
                else if (result == ERROR_MORE_DATA)
                {
                    count++;
                    index++;
                    trace_message("   ERROR_MORE_DATA: ValueName=", console::color::white, false);
                    trace_message(lpName, console::color::white, true);
                }
                else if (result == ERROR_NO_MORE_ITEMS)
                {
                    trace_message("   NO_MORE_ITEMS.", console::color::white, true);
                    done = true;
                }
                else
                {
                    trace_message("Fail to enumerate key. May be error in testing code.", console::color::red, true);
                    done = true;
                }
            }
            if (count == CountSubItems)
            {
                trace_message("Correct count of subItems achieved.", console::color::blue, true);
                result = ERROR_SUCCESS;
            }
            else
            {
                trace_message("Fail to get correct count. Counted=", console::color::red, false);
                char sNum[16];
                _itoa_s((int)count, sNum, 16, 10);
                trace_message(sNum, console::color::red, true);
                result = -1;
            }
        }
        else
        {
            trace_message("Fail to open key. May be error in testing code.", console::color::red, true);
            result = GetLastError();
            if (result == 0)
                result = ERROR_ACCESS_DENIED;
            print_last_error("Failed to open key");
        }
    }
    catch (...)
    {
        trace_message("Unexpected error.", console::color::red, true);
        result = GetLastError();
        print_last_error("Failed to Enumerate HKLM subitems (system key)");
    }
    test_end(result);
    Log("RegLegacyTest Enumerate HKLM subitems (system key)>>>>>");


} // HKLMWriteTests()

int wmain(int argc, const wchar_t** argv)
{
    // Display UTF-16 correctly...
 // NOTE: The CRT will assert if we try and use 'cout' with this set
    _setmode(_fileno(stdout), _O_U16TEXT);
    
    auto result = parse_args(argc, argv);
    //std::wstring aumid = details::appmodel_string(&::GetCurrentApplicationUserModelId);
#if _M_IX86
    test_initialize("RegLegacy Tests", 14);
#else
    test_initialize("RegLegacy Tests", 23);
#endif
    NotCoveredTests();   // 1 test

    DeletionMarkerTests(); // 3 Tests

#if _M_IX86
#else
    JavaMarkerTests();  //9 Tests
#endif

    StandardKeyAccessTests(); // 2 Tests

    HKLMWriteTests(); // 8 Tests

    test_cleanup();
    Sleep(500);
    return result;
}