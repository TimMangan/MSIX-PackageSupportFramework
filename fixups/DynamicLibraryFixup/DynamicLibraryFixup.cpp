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
#endif

extern bool                  g_dynf_forcepackagedlluse;
extern std::vector<dll_location_spec> g_dynf_dllSpecs;

DWORD g_LoadLibraryIntceptInstance = 30000;

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
    return requested.compare(locationspec.append(L".dll"));
}

auto LoadLibraryImpl = psf::detoured_string_function(&::LoadLibraryA, &::LoadLibraryW);
template <typename CharT>
HMODULE __stdcall LoadLibraryFixup(_In_ const CharT* libFileName)
{
    DWORD LoadLibraryInstance = ++g_LoadLibraryIntceptInstance;

#if _DEBUG
    LogString(LoadLibraryInstance, L"LoadLibraryFixup called for", libFileName);
#endif
    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    if (guard)
    {
#if _DEBUG
        Log(L" [%d] LoadLibraryFixup unguarded.", LoadLibraryInstance);
#endif
        // Check against known dlls in package.
        std::wstring libFileNameW = GetFilenameOnly(InterpretStringW(libFileName));

        if (g_dynf_forcepackagedlluse)
        {
#if _DEBUG
            //Log(L"LoadLibraryFixup forcepackagedlluse.");
#endif
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
#if _DEBUG
                //Log(L"LoadLibraryFixup test");
#endif
                try
                {
#if MOREDEBUG
                    LogString(LoadLibraryInstance, L"LoadLibraryFixup testing against", spec.filename.data());
#endif
                    if (compare_dllname(spec.filename.data(), libFileNameW) == 0)
                    {
                        bool useThis = true;
                        BOOL procTest = false;
                        switch (spec.architecture)
                        {
                        case x86:
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == FALSE)
                                {
                                    // 32 bit process on an x64 OS
                                    useThis = true;
                                }
                            }
                            else
                            {
                                // 32bit OS
                                useThis = true;
                            }
                            break;
                        case x64:
                            if (IsWow64Process(GetCurrentProcess(), &procTest))
                            {
                                if (procTest == TRUE)
                                {
                                    // 64 bit process on an x64 OS
                                    useThis = true;
                                }
                            }
                            break;
                        case AnyCPU:
                            useThis = true;
                            break;
                        case NotSpecified:
                        default:
                            break;
                        }

                        if (useThis)
                        {
#if _DEBUG
                            LogString(LoadLibraryInstance, L"LoadLibraryFixup using", spec.full_filepath.c_str());
#endif
                            result = LoadLibraryImpl(spec.full_filepath.c_str());
                            return result;
                        }
                    }
                }
                catch (...)
                {
                    Log(L" [%d] LoadLibraryFixup ERROR", LoadLibraryInstance);
                }
            }

#if _DEBUG
            Log(L" [%d] LoadLibraryFixup found no match registered.", LoadLibraryInstance);
#endif
        }
    }
    result = LoadLibraryImpl(libFileName);
#if _DEBUG
    Log(L" [%d] LoadLibraryFixup fallthrough result=0x%x", LoadLibraryInstance, result);
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
    LogString(LoadLibraryExInstance, L"LoadLibraryExFixup called on",libFileName);
#endif
    auto guard = g_reentrancyGuard.enter();
    HMODULE result;

    if (guard)
    {
#if _DEBUG
        //Log(L" [%d] LoadLibraryExFixup unguarded.", LoadLibraryExInstance);
#endif
        // Check against known dlls in package.
        std::wstring libFileNameW = InterpretStringW(libFileName);
        
        if (g_dynf_forcepackagedlluse)
        {
            for (dll_location_spec spec : g_dynf_dllSpecs)
            {
                try
                {
#if _DEBUG
                    //Log(L" [%d] LoadLibraryExFixup testing %ls against %ls", LoadLibraryExInstance, libFileNameW.c_str(), spec.full_filepath.native().c_str());
                    //LogString(LoadLibraryExInstance, L"LoadLibraryExFixup testing against", spec.filename.data());
#endif
                    if (compare_dllname(spec.filename.data(), libFileNameW) == 0)
                    {
#if _DEBUG
                        LogString(LoadLibraryExInstance, L"LoadLibraryExFixup using", spec.full_filepath.c_str());
#endif
                        result = LoadLibraryExImpl(spec.full_filepath.c_str(), file, flags);
                        return result;
                    }
                }
                catch (...)
                {
                    Log(L" [%d] LoadLibraryExFixup Error", LoadLibraryExInstance);
                }
            }
        }
    }
    result = LoadLibraryExImpl(libFileName, file, flags);
#if _DEBUG
        Log(L" [%d] LoadLibraryExFixup fallthrough result=0x%x", LoadLibraryExInstance, result);
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
