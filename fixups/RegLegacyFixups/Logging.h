//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once


#include <cassert>
#include <cstdio>
#include <cwchar>
#include <mutex>
#include <string>

#include <intrin.h>
#include <windows.h>
#include <lzexpand.h>
#include <winternl.h>


#include <psf_utils.h>
#include <psf_logging.h>
#include <sddl.h>

//#include <ntstatus.h>
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif
#ifndef REG_OPTION_DONT_VIRTUALIZE
#define REG_OPTION_DONT_VIRTUALIZE  0x00000010L
#endif



extern bool trace_function_entry; 
extern bool m_inhibitOutput;
extern bool m_shouldLog;
inline std::recursive_mutex g_outputMutex;

enum class function_type
{
    filesystem,
    registry,
    process_and_thread,
    dynamic_link_library,
};


// NOTE: Function entry tracing unaffected by these settings
enum class trace_level
{
    // Always traces function invocation, even if the function succeeded
    always,

    // Ignores success
    ignore_success,

    // Trace all failures, even expected ones
    all_failures,

    // Only traces failures that aren't considered "common" (file not found, etc.)
    unexpected_failures,

    // Don't trace any output (except function entry, if enabled)
    ignore,
};

enum class function_result
{
    // The function succeeded
    success,

    // Function does not report success/failure (e.g. void-returning function)
    indeterminate,

    // The function failed, but the failure is a common expected failure (e.g. file not found, insufficient buffer, etc.)
    expected_failure,

    // The function failed
    failure,
};

struct result_configuration
{
    bool should_log;
    bool should_break;
};


extern DWORD g_RegIntceptInstance;

std::wstring InterpretStringW(const char* value);
std::wstring InterpretStringW(const wchar_t* value);


static trace_level configured_trace_level(function_type);

static trace_level configured_break_level(function_type);

result_configuration configured_result(function_type type, function_result result);


// RAII type to acquire/release the output lock that also tracks/exposes whether or not the function result should be logged
struct output_lock
{
    // Don't let function calls made while processing output cause more output. This is effectively a "were we the first
    // to acquire the lock" check
    static inline bool processing_output = false;

    output_lock(function_type type, function_result result)
    {
        g_outputMutex.lock();
        m_inhibitOutput = std::exchange(processing_output, true);

        auto [shouldLog, shouldBreak] = configured_result(type, result);
        m_shouldLog = !m_inhibitOutput && shouldLog;
        if (shouldBreak)
        {
            ::DebugBreak();
        }
    }

    ~output_lock()
    {
        processing_output = m_inhibitOutput;
        g_outputMutex.unlock();
    }

    explicit operator bool() const noexcept
    {
        return m_shouldLog;
    }

};

// RAII helper for handling the 'traceFunctionEntry' configuration
struct function_entry_tracker
{
    // Used for printing function entry/exit separators
    static inline thread_local std::size_t function_call_depth = 0;

    function_entry_tracker(const char* functionName)
    {
        if (trace_function_entry)
        {
            std::lock_guard<std::recursive_mutex> lock(g_outputMutex);
            if (!output_lock::processing_output)
            {
                if (++function_call_depth == 1)
                {
                    Log(L"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
                }

                // Most functions are named "SomeFunctionFixup" where the target API is "SomeFunction". Logging the API
                // is much more helpful than the Fixup function name, so remove the "Fixup" suffix, if present
                using namespace std::literals;
                constexpr auto fixupSuffix = "Fixup"sv;
                std::string name = functionName;
                if ((name.length() >= fixupSuffix.length()) && (name.substr(name.length() - fixupSuffix.length()) == fixupSuffix))
                {
                    name.resize(name.length() - fixupSuffix.length());
                }

                Log(L"Function Entry: %s\n", name.c_str());
            }
        }
    }

    ~function_entry_tracker()
    {
        if (trace_function_entry)
        {
            std::lock_guard<std::recursive_mutex> lock(g_outputMutex);
            if (!output_lock::processing_output)
            {
                if (--function_call_depth == 0)
                {
                    Log(L"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
                }
            }
        }
    }
};
#define LogFunctionEntry() function_entry_tracker{ __FUNCTION__ }

// Logging functions for enums, flags, and other defines
template <typename T, typename U>
constexpr bool IsFlagSet(T value, U flag);


#define LogIfFlagSetMsg(value, flag, msg) \
    if (IsFlagSet(value, flag)) \
    { \
        Log(L"%s%s", prefix, msg); \
        prefix = " | "; \
    }


#define LogIfFlagSet(value, flag) \
    LogIfFlagSetMsg(value, flag, #flag)

#define LogIfEqual(value, expected) \
    if (value == expected) \
    { \
        Log(#expected); \
    }

void LogCountedString(const char* name, const wchar_t* value, std::size_t length);
void LogCountedString(const wchar_t* name, const wchar_t* value, std::size_t length);
std::string InterpretStringA(const char* value);

std::string InterpretStringA(const wchar_t* value);

std::string InterpretCountedString(const char* name, const wchar_t* value, std::size_t length);



std::string InterpretAsHex(const char* name, DWORD value);


// Error logging

std::string InterpretFrom_win32(DWORD code);

std::string win32_error_description(DWORD error);

void LogWin32Error(DWORD error, const wchar_t* msg = L"Error");

std::string InterpretWin32Error(DWORD error, const char* msg = "Error");

void LogLastError(const char* msg = "Last Error");

std::string InterpretLastError(const char* msg = "Last Error");

void LogKeyPath(HKEY key, const wchar_t* msg = L"Key");


std::string InterpretKeyPath(HKEY key, const char* msg);



std::string InterpretKeyPath(HKEY key);

void LogRegKeyFlags(DWORD flags, const wchar_t* msg = L"Options");


void LogRegKeyDisposition(DWORD disposition, const char* msg = "Disposition");


void LogCommonAccess(ACCESS_MASK access, const char*& prefix);


std::string InterpretCommonAccess(ACCESS_MASK access, const char*& prefix);

std::string InterpretRegKeyAccess(DWORD access, const char* msg = "Access");


const char* InterperetFunctionResult(function_result result);

void LogFunctionResult(function_result result, const wchar_t* msg = L"Result");

void LogRegKeyAccess(DWORD access, const char* msg = "Access");

void LogSecurityAttributes(LPSECURITY_ATTRIBUTES securityAttributes, DWORD instance);


#define LogCallingModule() \
    { \
        HMODULE moduleHandle; \
        if (::GetModuleHandleExW( \
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, \
            reinterpret_cast<const wchar_t*>(_ReturnAddress()), \
            &moduleHandle)) \
        { \
            Log(L"\tCalling Module=%ls\n", psf::get_module_path(moduleHandle).c_str()); \
        } \
    }


function_result from_win32(DWORD code);

bool function_succeeded(function_result result);

bool function_failed(function_result result);

output_lock acquire_output_lock(function_type type, function_result result);