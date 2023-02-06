//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Collection of function pointers that will always point to (what we think are) the actual function implementations in kernelbase.dll.
// 
// Normally programs link to Kernel32.dll for API functions.
// While we typically intercept in kernel32.dll API functions, and they then normally call the same named function in KernelBase.
// Occaisionally we find a path that calls directly into kernelbase.dll API and we need to also intercept that.
// This collection covers the known cases.
// 
// We must be careful to avoid a recursion of any of these methods accidentally calling back to the kernel32 counterpart! 
#pragma once

#include <reentrancy_guard.h>
#include <psf_framework.h>
#include <shellapi.h>


#define FIXUP_FROM_KernelBase 1

#if FIXUP_FROM_KernelBase

// NOTE: Most of the Windows API functions present in Kernel32.dll are also in KernelBase.dll. With most library imports, it is the kernel32.dll that is targeted.
// There are possibly situations where kernelbase can be targeted.  This function gives us a chance to do so.
template <typename Func>
inline Func GetKernelBaseDllInternalFunction(const char* functionName)
{
    static auto mod = ::LoadLibraryW(L"KernelBase.dll");
    assert(mod);

    // Ignore namespaces
    for (auto ptr = functionName; *ptr; ++ptr)
    {
        if (*ptr == ':')
        {
            functionName = ptr + 1;
        }
    }

    auto result = reinterpret_cast<Func>(::GetProcAddress(mod, functionName));
    assert(result);
    return result;
}
#define KERNELBASEL_FUNCTION(Name) (GetKernelBaseDllInternalFunction<decltype(&Name)>(#Name));


namespace kernelbaseimpl
{
    inline auto MoveFileExWImpl = KERNELBASEL_FUNCTION(MoveFileExW);
}

#endif

