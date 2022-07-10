//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
///#include "Reg_Remediation_Spec.h"

#include "Logging.h"


static trace_level configured_trace_level(function_type)
{
    return trace_level::always;
}

static trace_level configured_break_level(function_type)
{
    return trace_level::ignore;
}

result_configuration configured_result(function_type type, function_result result)
{
    auto impl = [&](trace_level level)
    {
        switch (level)
        {
        case trace_level::always:
            return result >= function_result::success;
            break;

        case trace_level::ignore_success:
            return result >= function_result::indeterminate;
            break;

        case trace_level::all_failures:
            return result >= function_result::expected_failure;
            break;

        case trace_level::unexpected_failures:
            return result >= function_result::failure;
            break;

        case trace_level::ignore:
        default:
            return false;
        }
    };

    return { impl(configured_trace_level(type)), impl(configured_break_level(type)) };
}

// Logging functions for enums, flags, and other defines
template <typename T, typename U>
constexpr bool IsFlagSet(T value, U flag)
{
    return static_cast<U>(value & flag) == flag;
}


void LogCountedString(const char* name, const wchar_t* value, std::size_t length)
{
    if (value != NULL)
    {
        Log("\t%s=%.*ls\n", name, length, value);
    }
    else
    {
        Log("\t%s=NULL", name);
    }
}
void LogCountedString(const wchar_t* name, const wchar_t* value, std::size_t length)
{
    if (value != NULL)
    {
        Log(L"\t%s=%.*ls\n", name, length, value);
    }
    else
    {
        Log(L"\t%s=NULL", name);
    }
}
std::string InterpretStringA(const char* value)
{
    if (value != NULL)
    {
        return value;
    }
    return "";
}

std::string InterpretStringA(const wchar_t* value)
{
    if (value != NULL)
    {
        return narrow(value);
    }
    return "";
}



std::wstring InterpretStringW(const char* value)
{
    return widen(value);
}
std::wstring InterpretStringW(const wchar_t* value)
{
    return value;
}


std::string InterpretCountedString(const char* name, const wchar_t* value, std::size_t length)
{
    std::ostringstream sout;
    if (value != NULL)
    {
        sout << name << "=" << std::setw(length) << narrow(value).c_str();
    }
    else
    {
        sout << name << "=NULL";
    }
    return sout.str();
}



std::string InterpretAsHex(const char* name, DWORD value)
{
    std::ostringstream sout;
    if (strlen(name) > 0)
    {
        sout << name << "=";
    }
    sout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << value;
    return sout.str();
}


// Error logging

std::string InterpretFrom_win32(DWORD code)
{
    switch (code)
    {
    case ERROR_SUCCESS:
        return "Success";
    case ERROR_FILE_NOT_FOUND:
        return "File not found";
    case ERROR_PATH_NOT_FOUND:
        return "Path not found";
    case ERROR_INVALID_NAME:
        return "Invalid Name";
    case ERROR_ALREADY_EXISTS:
        return "Already exists";
    case ERROR_FILE_EXISTS:
        return "File exists";
    case ERROR_INSUFFICIENT_BUFFER:
        return "Buffer overflow";
    case ERROR_MORE_DATA:
        return "More data";
    case ERROR_NO_MORE_ITEMS:
        return "No more items";
    case ERROR_NO_MORE_FILES:
        return "No more files";
    case ERROR_MOD_NOT_FOUND:
        return "Module not found";
    default:
        return "Unknown failure";
    }
}

std::string win32_error_description(DWORD error)
{
    // std::system_category represents Win32 error values, so leverage it for textual descriptions. Unfortunately, it
    // perpetuates the issue where the last character is always a new line character, so remove it
    auto str = std::system_category().message(error);
    auto remove_last_if = [&](char ch) { if (!str.empty() && (str.back() == ch)) str.pop_back(); };
    remove_last_if('\n');
    remove_last_if('\r');
    remove_last_if('.');

    return str;
}

void LogWin32Error(DWORD error, const wchar_t* msg )
{
    auto str = win32_error_description(error);
    Log(L"\t%s=%d (%s)\n", msg, error, widen(str).c_str());
}

std::string InterpretWin32Error(DWORD error, const char* msg )
{
    return InterpretAsHex(msg, error);
}

void LogLastError(const char* msg )
{
    LogWin32Error(::GetLastError(), widen(msg).c_str());
}

std::string InterpretLastError(const char* msg )
{
    DWORD err = ::GetLastError();
    return InterpretFrom_win32(err) + "\n" + InterpretWin32Error(err, msg);
}

void LogKeyPath(HKEY key, const wchar_t* msg )
{
    ULONG size;
    if (auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);
        (status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))
    {
        try
        {
            auto buffer = std::make_unique<std::uint8_t[]>(size + 2);
            if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))
            {
                buffer[size] = 0x0;
                buffer[size + 1] = 0x0;  // Add string termination character
                auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());
                LogCountedString(msg, info->Name, info->NameLength / 2);
            }
        }
        catch (...)
        {
            Log(L"%s Unable to log Key Path", msg);
        }
    }
    else if (status == STATUS_INVALID_HANDLE)
    {
        if (key == HKEY_CURRENT_USER)
        {
            Log(L"%s HKEY_CURRENT_USER", msg);
        }
        else if (key == HKEY_LOCAL_MACHINE)
        {
            Log(L"%s HKEY_LOCAL_MACHINE", msg);
        }
        else if (key == HKEY_CLASSES_ROOT)
        {
            Log(L"%s HKEY_CLASSES_ROOT", msg);
        }
        else
        {
            Log(L"%s Unable to log Key Path: Invalid handle", msg);
        }
    }
    else
    {
        Log(L"%s Unable to log Key Path 0x%x", msg, status);
    }
}


std::string InterpretKeyPath(HKEY key, const char* msg)
{
    std::string sret = "";
    ULONG size;
    try
    {
        auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);
        if ((status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))
        {
            auto buffer = std::make_unique<std::uint8_t[]>(size + 2);
            if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))
            {
                buffer[size] = 0x0;
                buffer[size + 1] = 0x0;  // Add string termination character
                auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());
                sret = InterpretCountedString(msg, info->Name, info->NameLength / 2);
            }
            else
                sret = "InterpretKeyPath failure2";
        }
        else if (status == STATUS_INVALID_HANDLE)
        {
            if (key == HKEY_LOCAL_MACHINE)
                sret += msg + InterpretStringA("HKEY_LOCAL_MACHINE");
            else if (key == HKEY_CURRENT_USER)
                sret = msg + InterpretStringA("HKEY_CURRENT_USER");
            else if (key == HKEY_CLASSES_ROOT)
                sret = msg + InterpretStringA("HKEY_CLASSES_ROOT");
        }
        else
            sret = "InterpretKeyPath failure1" + InterpretAsHex("status", (DWORD)status);
    }
    catch (...)
    {
        Log(L"InterpretKeyPath failure.");
    }
    return sret;
}

std::string InterpretKeyPath(HKEY key)
{
    std::string sret = "";
    ULONG size;
    try
    {
        auto status = impl::NtQueryKey(key, winternl::KeyNameInformation, nullptr, 0, &size);
        if ((status == STATUS_BUFFER_TOO_SMALL) || (status == STATUS_BUFFER_OVERFLOW))
        {
            auto buffer = std::make_unique<std::uint8_t[]>(size + 2);
            if (NT_SUCCESS(impl::NtQueryKey(key, winternl::KeyNameInformation, buffer.get(), size, &size)))
            {
                buffer[size] = 0x0;
                buffer[size + 1] = 0x0;  // Add string termination character
                auto info = reinterpret_cast<winternl::PKEY_NAME_INFORMATION>(buffer.get());
                sret = InterpretCountedString("", info->Name, info->NameLength / 2);
            }
            else
                sret = "InterpretKeyPath failure2";
        }
        else if (status == STATUS_INVALID_HANDLE)
        {
            if (key == HKEY_LOCAL_MACHINE)
                sret = InterpretStringA("HKEY_LOCAL_MACHINE");
            else if (key == HKEY_CURRENT_USER)
                sret = InterpretStringA("HKEY_CURRENT_USER");
            else if (key == HKEY_CLASSES_ROOT)
                sret = InterpretStringA("HKEY_CLASSES_ROOT");
            else
                Log(L"InterpretKeyPath failure2.");
        }
        else
        {
            sret = "InterpretKeyPath failure1" + InterpretAsHex("status", (DWORD)status);
            Log(L"InterpretKeyPath failure1.");
        }
    }
    catch (...)
    {
        Log(L"InterpretKeyPath failure.");
    }
    return sret;
}


void LogRegKeyFlags(DWORD flags, const wchar_t* msg )
{
    Log(L"\t%s=%08X", msg, flags);
    if (flags)
    {
        const char* prefix = "";
        Log(L" (");
        LogIfFlagSet(flags, REG_OPTION_VOLATILE);           // 0x0001
        LogIfFlagSet(flags, REG_OPTION_CREATE_LINK);        // 0x0002
        LogIfFlagSet(flags, REG_OPTION_BACKUP_RESTORE);     // 0x0004
        LogIfFlagSet(flags, REG_OPTION_OPEN_LINK);          // 0x0008
        LogIfFlagSet(flags, REG_OPTION_DONT_VIRTUALIZE);    // 0x0010
        Log(")");
    }
    else
    {
        Log(L" (REG_OPTION_NON_VOLATILE)"); // 0x0000
    }

    //Log(L"\n");
}


void LogRegKeyDisposition(DWORD disposition, const char* msg )
{
    Log(L"\t%s=%d (", msg, disposition);
    LogIfEqual(disposition, REG_CREATED_NEW_KEY)
    else LogIfEqual(disposition, REG_OPENED_EXISTING_KEY)
    else Log(L"UNKNOWN");
    Log(L")\n");
}


void LogCommonAccess(ACCESS_MASK access, const char*& prefix)
{
    // Standard Rights (bits 16-23)
    LogIfFlagSet(access, DELETE);
    LogIfFlagSet(access, READ_CONTROL);
    LogIfFlagSet(access, WRITE_DAC);
    LogIfFlagSet(access, WRITE_OWNER);
    LogIfFlagSet(access, SYNCHRONIZE);

    // Access System Security (bit 24)
    LogIfFlagSet(access, ACCESS_SYSTEM_SECURITY);

    // Maximum Allowed (bit 25)
    LogIfFlagSet(access, MAXIMUM_ALLOWED);

    // NOTE: Bits 26-27 are reserved

    // Generic Rights (bits 28-31)
    LogIfFlagSet(access, GENERIC_ALL);
    LogIfFlagSet(access, GENERIC_EXECUTE);
    LogIfFlagSet(access, GENERIC_READ);
    LogIfFlagSet(access, GENERIC_WRITE);
}


std::string InterpretCommonAccess(ACCESS_MASK access, const char*& prefix)
{
    std::ostringstream sout;
    // Standard Rights (bits 16-23)
    if (IsFlagSet(access, DELETE))
    {
        sout << prefix << "DELETE";
        prefix = " | ";
    }
    if (IsFlagSet(access, READ_CONTROL))
    {
        sout << prefix << "READ_CONTROL";
        prefix = " | ";
    }
    if (IsFlagSet(access, WRITE_DAC))
    {
        sout << prefix << "WRITE_DAC";
        prefix = " | ";
    }
    if (IsFlagSet(access, WRITE_OWNER))
    {
        sout << prefix << "WRITE_OWNER";
        prefix = " | ";
    }
    if (IsFlagSet(access, SYNCHRONIZE))
    {
        sout << prefix << "SYNCHRONIZE";
        prefix = " | ";
    }

    // Access System Security (bit 24)
    if (IsFlagSet(access, ACCESS_SYSTEM_SECURITY))
    {
        sout << prefix << "ACCESS_SYSTEM_SECURITY";
        prefix = " | ";
    }

    // Maximum Allowed (bit 25)
    if (IsFlagSet(access, MAXIMUM_ALLOWED))
    {
        sout << prefix << "MAXIMUM_ALLOWED";
        prefix = " | ";
    }

    // NOTE: Bits 26-27 are reserved

    // Generic Rights (bits 28-31)
    if (IsFlagSet(access, GENERIC_ALL))
    {
        sout << prefix << "GENERIC_ALL";
        prefix = " | ";
    }
    if (IsFlagSet(access, GENERIC_EXECUTE))
    {
        sout << prefix << "GENERIC_EXECUTE";
        prefix = " | ";
    }
    if (IsFlagSet(access, GENERIC_READ))
    {
        sout << prefix << "GENERIC_READ";
        prefix = " | ";
    }
    if (IsFlagSet(access, GENERIC_WRITE))
    {
        sout << prefix << "GENERIC_WRITE";
        prefix = " | ";
    }
    return sout.str();
}

std::string InterpretRegKeyAccess(DWORD access, const char* msg )
{
    std::ostringstream sout;
    sout << InterpretAsHex(msg, access);

    if (access)
    {
        sout << " (";
        const char* prefix = "";

        // Specific Rights (bits 0-15)
        if (IsFlagSet(access, KEY_QUERY_VALUE))
        {
            sout << prefix << "KEY_QUERY_VALUE";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_SET_VALUE))
        {
            sout << prefix << "KEY_SET_VALUE";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_CREATE_SUB_KEY))
        {
            sout << prefix << "KEY_CREATE_SUB_KEY";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_ENUMERATE_SUB_KEYS))
        {
            sout << prefix << "KEY_ENUMERATE_SUB_KEYS";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_NOTIFY))
        {
            sout << prefix << "KEY_NOTIFY";
            prefix = " | ";
        }
        if (IsFlagSet(access, KEY_CREATE_LINK))
        {
            sout << prefix << "KEY_CREATE_LINK";
            prefix = " | ";
        }

        sout << InterpretCommonAccess(access, prefix).c_str();

        sout << ")";
    }

    return sout.str();
}


const char* InterperetFunctionResult(function_result result)
{
    const char* resultMsg = "Unknown";
    switch (result)
    {
    case function_result::success:
        resultMsg = "Success";
        break;

    case function_result::indeterminate:
        resultMsg = "Indeterminate";
        break;

    case function_result::expected_failure:
        resultMsg = "Expected Failure";
        break;

    case function_result::failure:
        resultMsg = "Failure";
        break;
    default:
        break;
    }
    return resultMsg;
}

void LogFunctionResult(function_result result, const wchar_t* msg )
{
    const char* interp = InterperetFunctionResult(result);
    std::wstring winterp = widen(interp);

    Log(L"\t%s=%s\n", msg, winterp.c_str());
}

void LogRegKeyAccess(DWORD access, const char* msg )
{
    Log(L"\t%s=%08X", msg, access);
    if (access)
    {
        const char* prefix = "";
        Log(" (");

        // Specific Rights (bits 0-15)
        LogIfFlagSet(access, KEY_QUERY_VALUE);          // 0x0001
        LogIfFlagSet(access, KEY_SET_VALUE);            // 0x0002
        LogIfFlagSet(access, KEY_CREATE_SUB_KEY);       // 0x0004
        LogIfFlagSet(access, KEY_ENUMERATE_SUB_KEYS);   // 0x0008
        LogIfFlagSet(access, KEY_NOTIFY);               // 0x0010
        LogIfFlagSet(access, KEY_CREATE_LINK);          // 0x0020

        LogCommonAccess(access, prefix);

        Log(L")");
    }

    Log(L"\n");
}

void LogSecurityAttributes(LPSECURITY_ATTRIBUTES securityAttributes, DWORD instance)
{
    try
    {
        if (securityAttributes != NULL)
        {
            ULONG len = 2048;
            wchar_t* xvert;
            bool xverted = ConvertSecurityDescriptorToStringSecurityDescriptor(
                securityAttributes->lpSecurityDescriptor,
                SDDL_REVISION_1,
                ATTRIBUTE_SECURITY_INFORMATION | BACKUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
                &xvert,
                &len
            );
            if (xverted)
            {
                Log(L"[%d] SecurityAccess %d %d %Ls\n", instance, securityAttributes->nLength, securityAttributes->bInheritHandle, xvert);
                LocalFree(xvert);
            }
            else
            {
                Log(L"[%d] error to query security descriptor.\n", instance);
            }
        }
        else
        {
            Log(L"[%d] No security descriptor provided.\n", instance);
        }
    }
    catch (...)
    {
        Log(L"[%x] exception to query security descriptor.\n", instance);
    }
}


function_result from_win32(DWORD code)
{
    switch (code)
    {
    case ERROR_SUCCESS:
        return function_result::success;

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS:
    case ERROR_INSUFFICIENT_BUFFER:
    case ERROR_MORE_DATA:
    case ERROR_NO_MORE_ITEMS:
    case ERROR_NO_MORE_FILES:
    case ERROR_MOD_NOT_FOUND:
        return function_result::expected_failure;

    default:
        return function_result::failure;
    }
}

bool function_succeeded(function_result result)
{
    // NOTE: Only true for explicit success
    return result == function_result::success;
}

bool function_failed(function_result result)
{
    return result >= function_result::expected_failure;
}

output_lock acquire_output_lock(function_type type, function_result result)
{
    return output_lock(type, result);
}