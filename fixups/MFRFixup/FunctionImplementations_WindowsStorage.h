//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Collection of function pointers that will always point to (what we think are) the actual function implementations in kernelbase.dll.
// 
// Normally programs link to other API functions that we intercept. But there are utilities that bypass those functions and call equivalent kinds of things we need to trap as well.
// Windows.Storage.dll is a large utility with a ton of exports.  It is known to be used by certain file picker dialogs and we want the dialogs to display a merged layer view.
// This happens for some folders but not others, in part due to the underlying system mapping only certain folders to VFS in the package (and then to WriteRedirection if ILV is in use).
// 
// We must be careful to avoid a recursion of any of these methods accidentally calling back to other intercepted counterparts 
#pragma once
#define Intercept_WindowsStorage 1
//#define DO_WS_Shex_A 1
#define DO_WS_Shex_W 1
//#define DO_WS_ShexEx_A 1
#define DO_WS_ShexEx_W 1

#if Intercept_WindowsStorage

#include <reentrancy_guard.h>
#include <psf_framework.h>
#include <shellapi.h>



template <typename Func>
inline Func GetWindowsStorageDllInternalFunction(const char* functionName)
{
    // There are two copies of this dll, 32 and 64 bit, but asking by name should yield the one matching the process we are running in.
    static auto mod = ::LoadLibraryW(L"windows.storage.dll");
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
    if (functionName != NULL)
    {
        Log(L">>>WindowsStorage Fixup loaded name=%S from 0x%x", functionName, result);
    }
    else
    {
        Log(L">>>WindowsStorage Fixup mistaken loaded name=??? 0x%x",  result);
    }
   
    assert(result);
    return result;
}
#define WINDOWSSTORAGE_FUNCTION(Name) (GetWindowsStorageDllInternalFunction<decltype(&Name)>(#Name));


namespace windowsstorageimpl
{
#ifdef DO_WS_Shex_A
    inline auto ShellExecuteAImpl = WINDOWSSTORAGE_FUNCTION(ShellExecuteA);
#endif
#ifdef DO_WS_Shex_W
    inline auto ShellExecuteWImpl = WINDOWSSTORAGE_FUNCTION(ShellExecuteW);
    //inline auto ShellExecuteImpl = psf::detoured_string_function(windowsstorageimpl::ShellExecuteAImpl, windowsstorageimpl::ShellExecuteWImpl);
#endif

#ifdef DO_WS_ShexEx_A
    inline auto ShellExecuteExAImpl = WINDOWSSTORAGE_FUNCTION(ShellExecuteExA);
#endif
#ifdef DO_WS_ShexEx_W
    inline auto ShellExecuteExWImpl = WINDOWSSTORAGE_FUNCTION(ShellExecuteExW);
#endif
}

#endif
