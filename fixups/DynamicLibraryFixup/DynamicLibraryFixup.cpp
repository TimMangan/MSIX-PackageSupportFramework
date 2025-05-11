//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <psf_framework.h>
#include "FunctionImplementations.h"
#include "dll_location_spec.h"
#include <iostream>
#include <algorithm>

#if _DEBUG
//#define MOREDEBUG 1
//#define MOREDEBUG2 2
#endif

extern bool                  g_dynf_forcepackagedlluse;
extern std::vector<dll_location_spec> g_dynf_dllSpecs;

DWORD g_LoadLibraryIntceptInstance = 30000;
const wchar_t* g_LoadLibraryName = L"D";




typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, * PUNICODE_STRING;


typedef DWORD(__stdcall* _LdrLoadDll)(
    wchar_t* PathToFile,
    unsigned long Flags,
    PUNICODE_STRING ModuleFileName,
    PHANDLE* ModuleHandle
    );
_LdrLoadDll LdrLoadDll;

// Utility to perform a case independent comparison with or without the dll in the spec.
int compare_dllname(std::wstring Requested, std::wstring Locationspec)
{
    std::wstring requested = Requested;
    transform(requested.begin(), requested.end(), requested.begin(), towlower);
    std::wstring locationspec = Locationspec;
    std::transform(locationspec.begin(), locationspec.end(), locationspec.begin(), towlower);

    
    if (requested.compare(locationspec) == 0)
    {
        return 0;
    }
    // The caller is allowed to leave off the .dll extension, so we will check for that also.
    return requested.compare(locationspec.append(L".dll"));
}

auto LoadLibraryImpl = psf::detoured_string_function(&::LoadLibraryA, &::LoadLibraryW);
template <typename CharT>
HMODULE __stdcall LoadLibraryFixup(_In_ const CharT* libFileName)
{
    DWORD LoadLibraryInstance = ++g_LoadLibraryIntceptInstance;

#if _DEBUG
    LogString(g_LoadLibraryName, LoadLibraryInstance, L"LoadLibraryFixup called for", libFileName);
#endif
    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    if (guard)
    {
#if MOREDEBUG2
        Log(L" [%s%d] LoadLibraryFixup unguarded.", g_LoadLibraryName, LoadLibraryInstance);
#endif
        // Check against known dlls in package.
        std::wstring libFileNameW = GetFilenameOnly(InterpretStringW(libFileName));

        if (g_dynf_forcepackagedlluse)
        {
#if MOREDEBUG2
            Log(L"[%s%d] LoadLibraryFixup forcepackagedlluse.", g_LoadLibraryName, LoadLibraryInstance);
#endif
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
                try
                {
#if MOREDEBUG2
                    LogString(g_LoadLibraryName, LoadLibraryInstance, L"LoadLibraryFixup: testing against", spec.filename.data());
#endif
                    if (compare_dllname(spec.filename.data(), libFileNameW) == 0)
                    {
                        bool useThis = true;
                        [[maybe_unused]] BOOL procTest = false;
                        switch (spec.architecture)
                        {
                        case x86:
#if defined(_WIN64)
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryFixup:  We are in an x64 build and this match is 32bit.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == TRUE)
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryFixup:   we are in WOW so allow match.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                                    // 32-bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryFixup:   we are NOT in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                                    // 64-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
#if MOREDEBUG
                                Log(L"[%s%d] LoadLibraryFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                                // This call should never fail.
                                useThis = false;
                            }
#else
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryFixup:  We are in a 32-bit build and this match is 32bit.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                            // Only 32-bit is valid if we are built as 32-bit.
                            useThis = true;
#endif
                            break;
                        case x64:
#if defined(_WIN64)
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryFixup:  We are in an x64 build and this match is 64bit.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == FALSE)
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryFixup:   we are not in WOW so allow match.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                                    // 64 bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryFixup:   we are in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                                    // 32-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
#if MOREDEBUG
                                Log(L"[%s%d] LoadLibraryFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                                // This call should never fail.
                                useThis = false;
                            }
#else
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryFixup:  We are in a 32-bit build and this match is 64bit.", g_LoadLibraryName, LoadLibraryInstance);
#endif
                            // Can't use x64 dll if we are a 32-bit process
                            useThis = false;
#endif
                            break;
                        case AnyCPU:
                            useThis = true;
                            break;
                        case NotSpecified:
                        default:
                            useThis = true;
                            break;
                        }

                        if (useThis)
                        {
                            result = LoadLibraryImpl(spec.full_filepath.c_str());
#if TRY_LDRLOADDLL
                            if (result == 0)
                            {
#if _DEBUG
                                DWORD err = GetLastError();
                                Log(L"[%s%d] LoadLibraryFixup: Dll not found(0x%x), try LdrLoadDll", g_LoadLibraryName, LoadLibraryInstance,err);
#endif
                                if (LdrLoadDll == NULL)
                                    LdrLoadDll = (_LdrLoadDll)GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrLoadDll");
                                UNICODE_STRING name; 
                                name.Buffer = (PWCH)spec.filename.data();
                                name.Length = (USHORT)( wcslen(name.Buffer) * sizeof(wchar_t));
                                name.MaximumLength = (USHORT)(name.Length + sizeof(wchar_t));
                                PHANDLE ModuleHandle = NULL;
                                // Maybe flag should be LOAD_WITH_ALTERED_SEARCH_PATH = 0x8?
#if _DEBUG
                                DWORD ns = LdrLoadDll(spec.full_filepath.parent_path().wstring().data(), 0, &name, &ModuleHandle);
                                Log(L"[%s%d] LdrLoadDll: returns 0x%x Handle 0x%x", g_LoadLibraryName, LoadLibraryInstance,ns, ModuleHandle);
#else
                                LdrLoadDll(spec.full_filepath.parent_path().wstring().data(), 0, &name, &ModuleHandle);
#endif
                                result = (HMODULE)ModuleHandle;
                            }
#endif
#if _DEBUG
                            Log(L"[%s%d] LoadLibraryFixup: returns 0x%x using %s", g_LoadLibraryName, LoadLibraryInstance, result, spec.full_filepath.c_str());
#endif
                            return result;
                        }
                    }
                }
                catch (...)
                {
                    Log(L" [%s%d] LoadLibraryFixup: ERROR", g_LoadLibraryName, LoadLibraryInstance);
                }
            }

#if _DEBUG
            Log(L" [%s%d] LoadLibraryFixup: found no match registered.", g_LoadLibraryName, LoadLibraryInstance);
#endif
        }
    }
    result = LoadLibraryImpl(libFileName);
#if _DEBUG
    Log(L" [%s%d] LoadLibraryFixup: fallthrough result=0x%x", g_LoadLibraryName, LoadLibraryInstance, result);
#endif
    ///QueryPerformanceCounter(&TickEnd);
    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryImpl, LoadLibraryFixup);

auto LoadLibraryExImpl = psf::detoured_string_function(&::LoadLibraryExA, &::LoadLibraryExW);
template <typename CharT>
HMODULE __stdcall LoadLibraryExFixup(_In_ const CharT* libFileName, _Reserved_ HANDLE file, _In_ DWORD flags)
{
    DWORD LoadLibraryExInstance = ++g_LoadLibraryIntceptInstance;

#if _DEBUG
    LogString(g_LoadLibraryName, LoadLibraryExInstance, L"LoadLibraryExFixup called on",libFileName);
    if (flags != 0)
    {
        Log(L" [%s%d] LoadLibraryExFixup flags=0x%x", g_LoadLibraryName, LoadLibraryExInstance, flags);
    }
#endif
    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    if (guard)
    {
#if MOREDEBUG2
        Log(L" [%s%d] LoadLibraryExFixup unguarded.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
        // Check against known dlls in package.
        std::wstring libFileNameW = InterpretStringW(libFileName);
        
        if (g_dynf_forcepackagedlluse)
        {
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
                try
                {
#if MOREDEBUG2
                    Log(L" [%s%d] LoadLibraryExFixup testing %ls against %ls", g_LoadLibraryName, LoadLibraryExInstance, libFileNameW.c_str(), spec.full_filepath.native().c_str());
                    LogString(g_LoadLibraryName, LoadLibraryExInstance, L"LoadLibraryExFixup testing against", spec.filename.data());
#endif
                    if (compare_dllname(spec.filename.data(), libFileNameW) == 0)
                    {
                        bool useThis = true;
                        [[maybe_unused]] BOOL procTest = false;
                        switch (spec.architecture)
                        {
                        case x86:
#if defined(_WIN64)
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryExFixup:  We are in an x64 build and this match is 32bit.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == TRUE)
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryExFixup:   we are in WOW so allow match.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                                    // 32-bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryExFixup:   we are NOT in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                                    // 64-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
#if MOREDEBUG
                                Log(L"[%s%d] LoadLibraryExFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                                // This call should never fail.
                                useThis = false;
                            }
#else
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryExFixup:  We are in a 32-bit build and this match is 32bit.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                            // Only 32-bit is valid if we are built as 32-bit.
                            useThis = true;
#endif
                            break;
                        case x64:
#if defined(_WIN64)
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryExFixup:  We are in an x64 build and this match is 64bit.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == FALSE)
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryExFixup:   we are not in WOW so allow match.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                                    // 64 bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
#if MOREDEBUG
                                    Log(L"[%s%d] LoadLibraryExFixup:   we are in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                                    // 32-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
#if MOREDEBUG
                                Log(L"[%s%d] LoadLibraryExFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                                // This call should never fail.
                                useThis = false;
                            }
#else
#if MOREDEBUG
                            Log(L"[%s%d] LoadLibraryExFixup:  We are in a 32-bit build and this match is 64bit.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
                            // Can't use x64 dll if we are a 32-bit process
                            useThis = false;
#endif
                            break;
                        case AnyCPU:
                            useThis = true;
                            break;
                        case NotSpecified:
                        default:
                            useThis = true;
                            break;
                        }

                        if (useThis)
                        {
                            result = LoadLibraryExImpl(spec.full_filepath.c_str(), file, flags);
#if TRY_LDRLOADDLL
                            if (result == 0)
                            {
#if _DEBUG
                                DWORD err = GetLastError();
                                Log(L"[%s%d] LoadLibraryExFixup: Dll not found(0x%x), try LdrLoadDll", g_LoadLibraryName, LoadLibraryExInstance,err);
#endif
                                if (LdrLoadDll == NULL)
                                    LdrLoadDll = (_LdrLoadDll)GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrLoadDll");
                                UNICODE_STRING name;
                                name.Buffer = (PWCH)spec.filename.data();
                                name.Length = (USHORT)(wcslen(name.Buffer) * sizeof(wchar_t));
                                name.MaximumLength = (USHORT)(name.Length + sizeof(wchar_t));
                                PHANDLE ModuleHandle = NULL;
#if _DEBUG
                                DWORD ns = LdrLoadDll(spec.full_filepath.parent_path().wstring().data(), flags, &name, &ModuleHandle);
                                Log(L"[%s%d] LdrLoadDll: returns 0x%x hmodule 0x%x", g_LoadLibraryName, LoadLibraryExInstance, ns, ModuleHandle);
#else
                                LdrLoadDll(spec.full_filepath.parent_path().wstring().data(), flags, &name, &ModuleHandle);     
#endif
                                result = (HMODULE)ModuleHandle;
                            }
#endif
#if _DEBUG
                            Log(L"[%s%d] LoadLibraryExFixup: returns 0x%x using %s", g_LoadLibraryName, LoadLibraryExInstance, result, spec.full_filepath.c_str());
#endif
                            return result;
                        }
                    }
                }
                catch (...)
                {
                    Log(L" [%s%d] LoadLibraryExFixup Error", g_LoadLibraryName, LoadLibraryExInstance);
                }
            }
#if MOREDEBUG
            Log(L" [%s%d] LoadLibraryExFixup: found no match registered.", g_LoadLibraryName, LoadLibraryExInstance);
#endif
        }
    }
    result = LoadLibraryExImpl(libFileName, file, flags);
#if _DEBUG
        Log(L" [%s%d] LoadLibraryExFixup fallthrough result=0x%x", g_LoadLibraryName, LoadLibraryExInstance, result);
#endif
    ///QueryPerformanceCounter(&TickEnd);
    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryExImpl, LoadLibraryExFixup);


// NOTE: The following is a list of functions taken from https://msdn.microsoft.com/en-us/library/windows/desktop/ms682599(v=vs.85).aspx
//       that are _not_ present above. This is just a convenient collection of what's missing; it is not a collection of
//       future work.
//
// AddDllDirectory
// LoadModule
// LoadPackagedLibrary
// RemoveDllDirectory
// SetDefaultDllDirectories
// SetDllDirectory
// 
// DisableThreadLibraryCalls
// DllMain
// FreeLibrary
// FreeLibraryAndExitThread
// GetDllDirectory
// GetModuleFileName
// GetModuleFileNameEx
// GetModuleHandle
// GetModuleHandleEx
// GetProcAddress
// QueryOptionalDelayLoadedAPI
