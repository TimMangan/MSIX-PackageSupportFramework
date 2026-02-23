//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// Must add psf_logging.cpp from the CommonSrc folder into the project.

#include <windows.h>

// These define the levels of logging detail that may be set, and levels used in logging request.
enum Json_Debug_Levels
{
    LogLevel_None = 0,           // Disable all logging to console
    LogLevel_Exception = 1, // outside of exceptions
    LogLevel_Launching = 2,      // Log only during startup and teardown operations
    LogLevel_DebugBasic = 3,          // Basic debug level logging in intercepts (formerly DEBUG)
    LogLevel_DebugIntermediate = 4,   // Intermediate level logging in intercepts (formerly MOREDEBUG)
    LogLevel_DebugMaximum = 9,         // Maximum level logging in intercepts (formerly MOREDEBUG2 or EVENMOREDEBUG)
    LogLevel_DebugSuperMax = 20         // Beyond, reserved for PSF debugging.
};
extern Json_Debug_Levels g_JsonDebugLevel; // Json_Debug_Levels
extern bool g_psf_NoLogging;  // Acts as a temporary override to disable all logging


void Log(Json_Debug_Levels debugRequestLevel, const char* fmt, ...);


void Log(Json_Debug_Levels debugRequestLevel, const wchar_t* fmt, ...);

void LogString(Json_Debug_Levels debugRequestLevel, const char* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, const char* name, const wchar_t* value);

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* name, const wchar_t* value);

void LogCountedStringW(Json_Debug_Levels debugRequestLevel, const char* name, const wchar_t* value, size_t length);

void Loghexdump(Json_Debug_Levels debugRequestLevel, void* pAddressIn, long  lSize, const wchar_t * ModuleName, DWORD instance = 0);


///// WITH_INST
void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const wchar_t* value);

void LogStringAA(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const char* value);
void LogStringAW(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const wchar_t* value);

void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const wchar_t* value);

void LogStringWA(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const char* value);
void LogStringWW(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const wchar_t* value);
void LogString(Json_Debug_Levels debugRequestLevel, DWORD rememberedInst, DWORD inst, const wchar_t* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, DWORD rememberedInst, DWORD inst, const wchar_t* name, const wchar_t* value);

void LogCountedStringW(Json_Debug_Levels debugRequestLevel, DWORD dllInstance, const char* name, const wchar_t* value, size_t length);



///// WITH_INST_AND_MODULE
void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t * moduleName, DWORD inst, const char* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const char* name, const wchar_t* value);

void LogStringAA(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const char* name, const char* value);
void LogStringAW(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const char* name, const wchar_t* value);

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const wchar_t* value);

void LogStringWA(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const char* value);
void LogStringWW(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const wchar_t* value);
void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD rememberedInst, DWORD inst, const wchar_t* name, const char* value);

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD rememberedInst, DWORD inst, const wchar_t* name, const wchar_t* value);

void LogCountedStringW(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD dllInstance, const char* name, const wchar_t* value, size_t length);

#include <CatchHandler.h>


#define LogCallingModuleInstanceCommon(debugRequestLevel, moduleId, instance) \
    { \
        if (!g_psf_NoLogging) \
        { \
            if (debugRequestLevel <= g_JsonDebugLevel) \
            { \
                HMODULE moduleHandle; \
                if (::GetModuleHandleExW( \
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, \
                    reinterpret_cast<const wchar_t*>(_ReturnAddress()), \
                    &moduleHandle)) \
                { \
                    Log(debugRequestLevel, L"[%s%d]\tCalling Module=%ls\n", moduleId, instance, psf::get_module_path(moduleHandle).c_str()); \
                } \
            } \
        } \
    }