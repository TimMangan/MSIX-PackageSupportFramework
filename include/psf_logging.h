//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// Must add psf_logging.cpp from the CommonSrc folder into the project.

#include <windows.h>

extern bool g_psf_NoLogging;


void Log(const char* fmt, ...);


void Log(const wchar_t* fmt, ...);

void LogString(const char* name, const char* value);

void LogString(const char* name, const wchar_t* value);

void LogString(const wchar_t* name, const char* value);

void LogString(const wchar_t* name, const wchar_t* value);

void LogCountedStringW(const char* name, const wchar_t* value, size_t length);
void Loghexdump(void* pAddressIn, long  lSize, DWORD instance = 0);


///// WITH_INST
void LogString(DWORD inst, const char* name, const char* value);

void LogString(DWORD inst, const char* name, const wchar_t* value);

void LogStringAA(DWORD inst, const char* name, const char* value);
void LogStringAW(DWORD inst, const char* name, const wchar_t* value);

void LogString(DWORD inst, const wchar_t* name, const char* value);

void LogString(DWORD inst, const wchar_t* name, const wchar_t* value);

void LogStringWA(DWORD inst, const wchar_t* name, const char* value);
void LogStringWW(DWORD inst, const wchar_t* name, const wchar_t* value);
void LogString(DWORD rememberedInst, DWORD inst, const wchar_t* name, const char* value);

void LogString(DWORD rememberedInst, DWORD inst, const wchar_t* name, const wchar_t* value);

void LogCountedStringW(DWORD dllInstance, const char* name, const wchar_t* value, size_t length);



///// WITH_INST_AND_MODULE
void LogString(const wchar_t * moduleName, DWORD inst, const char* name, const char* value);

void LogString(const wchar_t* moduleName, DWORD inst, const char* name, const wchar_t* value);

void LogStringAA(const wchar_t* moduleName, DWORD inst, const char* name, const char* value);
void LogStringAW(const wchar_t* moduleName, DWORD inst, const char* name, const wchar_t* value);

void LogString(const wchar_t* moduleName, DWORD inst, const wchar_t* name, const char* value);

void LogString(const wchar_t* moduleName, DWORD inst, const wchar_t* name, const wchar_t* value);

void LogStringWA(const wchar_t* moduleName, DWORD inst, const wchar_t* name, const char* value);
void LogStringWW(const wchar_t* moduleName, DWORD inst, const wchar_t* name, const wchar_t* value);
void LogString(const wchar_t* moduleName, DWORD rememberedInst, DWORD inst, const wchar_t* name, const char* value);

void LogString(const wchar_t* moduleName, DWORD rememberedInst, DWORD inst, const wchar_t* name, const wchar_t* value);

void LogCountedStringW(const wchar_t* moduleName, DWORD dllInstance, const char* name, const wchar_t* value, size_t length);

#include <CatchHandler.h>


#define LogCallingModuleInstanceCommon(moduleId, instance) \
    { \
        if (!g_psf_NoLogging) \
        { \
            HMODULE moduleHandle; \
            if (::GetModuleHandleExW( \
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, \
                reinterpret_cast<const wchar_t*>(_ReturnAddress()), \
                &moduleHandle)) \
            { \
                Log(L"[%s%d]\tCalling Module=%ls\n", moduleId, instance, psf::get_module_path(moduleHandle).c_str()); \
            } \
        } \
    }