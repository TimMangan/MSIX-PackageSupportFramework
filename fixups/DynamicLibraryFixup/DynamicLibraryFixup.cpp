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
auto LoadLibraryExImpl = psf::detoured_string_function(&::LoadLibraryExA, &::LoadLibraryExW);

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:LoadLibraryFixupAnsi_Fixup=impl::_LoadLibraryA.ansi")  // Exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:LoadLibraryFixupWide_Fixup=impl::_LoadLibraryW.wide")  // Exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:LoadLibraryExFixupAnsi_Fixup=impl::_LoadLibraryExA.ansi")  // Exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:LoadLibraryExFixupWide_Fixup=impl::_LoadLibraryExW.wide")  // Exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:LoadLibraryFixupAnsi_Fixup=impl::LoadLibraryW.ansi")  // Exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:LoadLibraryFixupWide_Fixup=impl::LoadLibraryW.wide")  // Exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:LoadLibraryExFixupAnsi_Fixup=impl::LoadLibraryExW.ansi")  // Exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:LoadLibraryExFixupWide_Fixup=impl::LoadLibraryExW.wide")  // Exporting these names helps ProcessMonitor stack traces.
#endif



template <typename CharT>
HMODULE __stdcall LoadLibraryFixup(_In_ const CharT* libFileName)
{
    DWORD LoadLibraryInstance = ++g_LoadLibraryIntceptInstance;

    LogString(LogLevel_DebugBasic, g_LoadLibraryName, LoadLibraryInstance, L"LoadLibraryFixup called for", libFileName);

    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    SetLastError(0); // Clear the last error before we start.

    if (guard)
    {
        Log(LogLevel_DebugIntermediate, L" [%s%d] LoadLibraryFixup unguarded.", g_LoadLibraryName, LoadLibraryInstance);
        // Check against known dlls in package.
        std::wstring libFileNameW = GetFilenameOnly(InterpretStringW(libFileName));

        if (g_dynf_forcepackagedlluse)
        {
            Log(LogLevel_DebugIntermediate, L"[%s%d] LoadLibraryFixup forcepackagedlluse.", g_LoadLibraryName, LoadLibraryInstance);
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
                try
                {
                    LogString(LogLevel_DebugSuperMax, g_LoadLibraryName, LoadLibraryInstance, L"LoadLibraryFixup: testing against", spec.filename.data());
                    if (compare_dllname(spec.filename.data(), libFileNameW) == 0)
                    {
                        bool useThis = true;
                        [[maybe_unused]] BOOL procTest = false;
                        switch (spec.architecture)
                        {
                        case x86:
#if defined(_WIN64)
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:  We are in an x64 build and this match is 32bit.", g_LoadLibraryName, LoadLibraryInstance);
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == TRUE)
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:   we are in WOW so allow match.", g_LoadLibraryName, LoadLibraryInstance);
                                    // 32-bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:   we are NOT in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryInstance);
                                    // 64-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryInstance);
                                // This call should never fail.
                                useThis = false;
                            }
#else
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:  We are in a 32-bit build and this match is 32bit.", g_LoadLibraryName, LoadLibraryInstance);
                            // Only 32-bit is valid if we are built as 32-bit.
                            useThis = true;
#endif
                            break;
                        case x64:
#if defined(_WIN64)
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:  We are in an x64 build and this match is 64bit.", g_LoadLibraryName, LoadLibraryInstance);
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == FALSE)
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:   we are not in WOW so allow match.", g_LoadLibraryName, LoadLibraryInstance);
                                    // 64 bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:   we are in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryInstance);
                                    // 32-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryInstance);

                                // This call should never fail.
                                useThis = false;
                            }
#else
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryFixup:  We are in a 32-bit build and this match is 64bit.", g_LoadLibraryName, LoadLibraryInstance);
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
                            SetLastError(0); // Clear the last error before we try.
                            result = LoadLibraryImpl(spec.full_filepath.c_str());
#if TRY_LDRLOADDLL
                            if (result == 0)
                            {
                                DWORD err = GetLastError();
                                Log(LogLevel_DebugBasic, L"[%s%d] LoadLibraryFixup: Dll not found(0x%x), try LdrLoadDll", g_LoadLibraryName, LoadLibraryInstance,err);
                                if (LdrLoadDll == NULL)
                                    LdrLoadDll = (_LdrLoadDll)GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrLoadDll");
                                UNICODE_STRING name; 
                                name.Buffer = (PWCH)spec.filename.data();
                                name.Length = (USHORT)( wcslen(name.Buffer) * sizeof(wchar_t));
                                name.MaximumLength = (USHORT)(name.Length + sizeof(wchar_t));
                                PHANDLE ModuleHandle = NULL;
                                // Maybe flag should be LOAD_WITH_ALTERED_SEARCH_PATH = 0x8?
                                DWORD ns;
#if _DEBUG
                                ns = LdrLoadDll(spec.full_filepath.parent_path().wstring().data(), 0, &name, &ModuleHandle);
#else
                                ns = LdrLoadDll(spec.full_filepath.parent_path().wstring().data(), 0, &name, &ModuleHandle);
#endif
                                Log(LogLevel_DebugBasic, L"[%s%d] LdrLoadDll: returns 0x%x Handle 0x%x", g_LoadLibraryName, LoadLibraryInstance, ns, ModuleHandle);
                                result = (HMODULE)ModuleHandle;
                            }
#endif
                            Log(LogLevel_DebugBasic, L"[%s%d] LoadLibraryFixup: returns 0x%x with LastError=0x%x using %s", g_LoadLibraryName, LoadLibraryInstance, result, GetLastError(), spec.full_filepath.c_str());
                            return result;
                        }
                    }
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L" [%s%d] LoadLibraryFixup: Exception ERROR=0x%x", g_LoadLibraryName, LoadLibraryInstance, GetLastError());
                }
            }

            Log(LogLevel_DebugBasic, L" [%s%d] LoadLibraryFixup: found no match registered.", g_LoadLibraryName, LoadLibraryInstance);
        }
    }
    result = LoadLibraryImpl(libFileName);
    Log(LogLevel_DebugBasic, L" [%s%d] LoadLibraryFixup: fallthrough result=0x%x with LastError=0x%x", g_LoadLibraryName, LoadLibraryInstance, result, GetLastError());
    ///QueryPerformanceCounter(&TickEnd);
    return result;
}
DECLARE_STRING_FIXUP(LoadLibraryImpl, LoadLibraryFixup);

template <typename CharT>
HMODULE __stdcall LoadLibraryExFixup(_In_ const CharT* libFileName, _Reserved_ HANDLE file, _In_ DWORD flags)
{
    DWORD LoadLibraryExInstance = ++g_LoadLibraryIntceptInstance;

    LogString(LogLevel_DebugBasic, g_LoadLibraryName, LoadLibraryExInstance, L"LoadLibraryExFixup called on",libFileName);
    if (flags != 0)
    {
        Log(LogLevel_DebugBasic, L" [%s%d] LoadLibraryExFixup flags=0x%x", g_LoadLibraryName, LoadLibraryExInstance, flags);
    }

    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    SetLastError(0); // Clear the last error before we start.

    if (guard)
    {
        Log(LogLevel_DebugIntermediate, L" [%s%d] LoadLibraryExFixup unguarded.", g_LoadLibraryName, LoadLibraryExInstance);

        // Check against known dlls in package.
        std::wstring libFileNameW = InterpretStringW(libFileName);
        
        if (g_dynf_forcepackagedlluse)
        {
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
                try
                {
                    Log(LogLevel_DebugSuperMax, L" [%s%d] LoadLibraryExFixup testing %ls against entry %ls", g_LoadLibraryName, LoadLibraryExInstance, libFileNameW.c_str(), spec.full_filepath.native().c_str());
                    LogString(LogLevel_DebugSuperMax, g_LoadLibraryName, LoadLibraryExInstance, L"LoadLibraryExFixup testing against just filename", spec.filename.data());

                    bool isAMatch = false;
                    if (compare_dllname(spec.filename.data(), libFileNameW) == 0)
                    {
                        isAMatch = true;
                    }
                    else
                    {
                        // Possibly a full or relative file path was provided.  We should just match up anyway.
                        std::wstring libFileNameOnly = GetFilenameOnly(libFileNameW);
                        if (compare_dllname(spec.filename.data(), libFileNameOnly) == 0)
                        {
                            isAMatch = true;
                        }
                    }
                    if (isAMatch)
                    {
                        bool useThis = true;
                        [[maybe_unused]] BOOL procTest = false;
                        switch (spec.architecture)
                        {
                        case x86:
#if defined(_WIN64)
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:  We are in an x64 build and this match is 32bit.", g_LoadLibraryName, LoadLibraryExInstance);
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == TRUE)
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:   we are in WOW so allow match.", g_LoadLibraryName, LoadLibraryExInstance);

                                    // 32-bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:   we are NOT in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryExInstance);
                                    // 64-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryExInstance);
                                // This call should never fail.
                                useThis = false;
                            }
#else
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:  We are in a 32-bit build and this match is 32bit.", g_LoadLibraryName, LoadLibraryExInstance);
                            // Only 32-bit is valid if we are built as 32-bit.
                            useThis = true;
#endif
                            break;
                        case x64:
#if defined(_WIN64)
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:  We are in an x64 build and this match is 64bit.", g_LoadLibraryName, LoadLibraryExInstance);
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == FALSE)
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:   we are not in WOW so allow match.", g_LoadLibraryName, LoadLibraryExInstance);
                                    // 64 bit process on an x64 OS
                                    useThis = true;
                                }
                                else
                                {
                                    Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:   we are in WOW so dont allow match.", g_LoadLibraryName, LoadLibraryExInstance);
                                    // 32-bit process on 64-bit OS
                                    useThis = false;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:   WOW check failed.", g_LoadLibraryName, LoadLibraryExInstance);
                                // This call should never fail.
                                useThis = false;
                            }
#else
                            Log(LogLevel_DebugSuperMax, L"[%s%d] LoadLibraryExFixup:  We are in a 32-bit build and this match is 64bit.", g_LoadLibraryName, LoadLibraryExInstance);
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
                            /// The flags parameter set by the caller might not make sense when we are trying to force a specific path, 
                            /// and the caller did not supply the path.
                            /// In this code, we can look for cases and adjust as appropriate.  
                            /// It is possible that there are other cases needing adjustment, but we will start with the most obvious ones.
                            DWORD altFlags = flags;
                            Log(LogLevel_DebugMaximum, L"[%s%d] LoadLibraryExFixup: Original flags=0x%x", g_LoadLibraryName, LoadLibraryExInstance, flags);
                            if ((altFlags & LOAD_WITH_ALTERED_SEARCH_PATH) != 0 &&
                                libFileNameW.find(L'\\') == std::wstring::npos)
                            {
                                // Can't be combined with other options.  As we are supplying a full path, the use of this flag would tell the call
                                // to ignore our path and use the search path instead.  Can't have that!
                                altFlags = 0;
                            }
                            if (altFlags != flags)
                            {
                                Log(LogLevel_DebugMaximum, L"[%s%d] LoadLibraryExFixup: Adjusted flags from 0x%x to 0x%x", g_LoadLibraryName, LoadLibraryExInstance, flags, altFlags);
                            }


                            // Now make the call!
                            result = LoadLibraryExImpl(spec.full_filepath.c_str(), file, altFlags);
#if TRY_LDRLOADDLL
                            if (result == 0)
                            {
                                DWORD err = GetLastError();
                                Log(LogLevel_DebugIntermediate, L"[%s%d] LoadLibraryExFixup: Dll not found(0x%x), try LdrLoadDll", g_LoadLibraryName, LoadLibraryExInstance,err);
                                if (LdrLoadDll == NULL)
                                    LdrLoadDll = (_LdrLoadDll)GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrLoadDll");
                                UNICODE_STRING name;
                                name.Buffer = (PWCH)spec.filename.data();
                                name.Length = (USHORT)(wcslen(name.Buffer) * sizeof(wchar_t));
                                name.MaximumLength = (USHORT)(name.Length + sizeof(wchar_t));
                                PHANDLE ModuleHandle = NULL;;
                                DWORD ns = LdrLoadDll(spec.full_filepath.parent_path().wstring().data(), flags, &name, &ModuleHandle);
                                Log(LogLevel_DebugBasic, L"[%s%d] LdrLoadDll: returns 0x%x hmodule 0x%x", g_LoadLibraryName, LoadLibraryExInstance, ns, ModuleHandle);
                                result = (HMODULE)ModuleHandle;
                            }
#endif
                            if (result != 0)
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] LoadLibraryExFixup: returns 0x%x using %s", g_LoadLibraryName, LoadLibraryExInstance, result, spec.full_filepath.c_str());
                            }
                            else
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] LoadLibraryExFixup: returns 0x%x and LastError=0x%x using %s", g_LoadLibraryName, LoadLibraryExInstance, result, GetLastError(), spec.full_filepath.c_str());
                            }
                            return result;
                        }
                    }
                }
                catch (...)
                {
                    Log(LogLevel_Exception, L" [%s%d] LoadLibraryExFixup Exception Error=x%x", g_LoadLibraryName, LoadLibraryExInstance, GetLastError());
                }
            }
 
            Log(LogLevel_DebugBasic, L" [%s%d] LoadLibraryExFixup: found no match registered.", g_LoadLibraryName, LoadLibraryExInstance);
        }
    }
    result = LoadLibraryExImpl(libFileName, file, flags);
    Log(LogLevel_DebugBasic, L" [%s%d] LoadLibraryExFixup fallthrough result=0x%x with LastError=0x%x", g_LoadLibraryName, LoadLibraryExInstance, result, GetLastError());
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
