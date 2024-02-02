//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// Collection of function LoadLibrary will (presumably) call kernelbase!LoadLibraryA/W, even though we detour that call. Useful to
// reduce the risk of us fixing ourselves, which can easily lead to infinite recursion. Note that this isn't perfect.
// For example, CreateFileFixup could call kernelbase!CopyFileW, which could in turn call (the fixed) CreateFile again
#pragma once

#include <reentrancy_guard.h>
#include <psf_framework.h>

#include <cassert>
//#include <ntstatus.h>
#include <windows.h>
#include <winternl.h>

// A much bigger hammer to avoid reentrancy. Still, the impl::* functions are good to have around to prevent the
// unnecessary invocation of the fixup
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace winternl
{
    typedef enum _KEY_INFORMATION_CLASS
    {
        KeyBasicInformation,
        KeyNodeInformation,
        KeyFullInformation,
        KeyNameInformation,
        KeyCachedInformation,
        KeyFlagsInformation,
        KeyVirtualizationInformation,
        KeyHandleTagsInformation,
        KeyTrustInformation,
        KeyLayerInformation,
        MaxKeyInfoClass
    } KEY_INFORMATION_CLASS;

    typedef enum _KEY_VALUE_INFORMATION_CLASS
    {
        KeyValueBasicInformation,
        KeyValueFullInformation,
        KeyValuePartialInformation,
        KeyValueFullInformationAlign64,
        KeyValuePartialInformationAlign64,
        KeyValueLayerInformation,
        MaxKeyValueInfoClass
    } KEY_VALUE_INFORMATION_CLASS;

    typedef struct _KEY_BASIC_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         NameLength;
        WCHAR         Name[1];
    } KEY_BASIC_INFORMATION, * PKEY_BASIC_INFORMATION;

    typedef struct _KEY_NODE_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         ClassOffset;
        ULONG         ClassLength;
        ULONG         NameLength;
        WCHAR         Name[1];
    } KEY_NODE_INFORMATION, * PKEY_NODE_INFORMATION;

    typedef struct _KEY_FULL_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         ClassOffset;
        ULONG         ClassLength;
        ULONG         SubKeys;
        ULONG         MaxNameLen;
        ULONG         MaxClassLen;
        ULONG         Values;
        ULONG         MaxValueNameLen;
        ULONG         MaxValueDataLen;
        WCHAR         Class[1];
    } KEY_FULL_INFORMATION, * PKEY_FULL_INFORMATION;

    typedef struct _KEY_NAME_INFORMATION
    {
        ULONG NameLength;
        WCHAR Name[1];
    } KEY_NAME_INFORMATION, * PKEY_NAME_INFORMATION;

    typedef struct _KEY_CACHED_INFORMATION
    {
        LARGE_INTEGER LastWriteTime;
        ULONG         TitleIndex;
        ULONG         SubKeys;
        ULONG         MaxNameLen;
        ULONG         Values;
        ULONG         MaxValueNameLen;
        ULONG         MaxValueDataLen;
        ULONG         NameLength;
    } KEY_CACHED_INFORMATION, * PKEY_CACHED_INFORMATION;

    typedef struct _KEY_VIRTUALIZATION_INFORMATION
    {
        ULONG VirtualizationCandidate;
        ULONG VirtualizationEnabled;
        ULONG VirtualTarget;
        ULONG VirtualStore;
        ULONG VirtualSource;
        ULONG Reserved;
    } KEY_VIRTUALIZATION_INFORMATION, * PKEY_VIRTUALIZATION_INFORMATION;

    typedef struct _KEY_VALUE_BASIC_INFORMATION
    {
        ULONG TitleIndex;
        ULONG Type;
        ULONG NameLength;
        WCHAR Name[1];
    } KEY_VALUE_BASIC_INFORMATION, PKEY_VALUE_BASIC_INFORMATION;

    typedef struct _KEY_VALUE_FULL_INFORMATION
    {
        ULONG TitleIndex;
        ULONG Type;
        ULONG DataOffset;
        ULONG DataLength;
        ULONG NameLength;
        WCHAR Name[1];
    } KEY_VALUE_FULL_INFORMATION, PKEY_VALUE_FULL_INFORMATION;

    typedef struct _KEY_VALUE_PARTIAL_INFORMATION
    {
        ULONG TitleIndex;
        ULONG Type;
        ULONG DataLength;
        UCHAR Data[1];
    } KEY_VALUE_PARTIAL_INFORMATION, PKEY_VALUE_PARTIAL_INFORMATION;


    // Function declarations not present in winternl.h
    NTSTATUS __stdcall NtQueryInformationFile(
        HANDLE FileHandle,
        PIO_STATUS_BLOCK IoStatusBlock,
        PVOID FileInformation,
        ULONG Length,
        FILE_INFORMATION_CLASS FileInformationClass);

    NTSTATUS __stdcall NtQueryKey(
        HANDLE KeyHandle,
        KEY_INFORMATION_CLASS KeyInformationClass,
        PVOID KeyInformation,
        ULONG Length,
        PULONG ResultLength);

    NTSTATUS __stdcall NtQueryValueKey(
        HANDLE KeyHandle,
        PUNICODE_STRING ValueName,
        KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        PVOID KeyValueInformation,
        ULONG Length,
        PULONG ResultLength);

    NTSTATUS __stdcall RegDeleteKey(
        HANDLE hKey,
        PUNICODE_STRING lpSubKey
    );
#ifndef INTERCEPT_KERNELBASE
    NTSTATUS __stdcall RegDeleteKeyEx(
        HANDLE hKey,
        PUNICODE_STRING lpSubKey,
        DWORD viewDesired, // 32/64
        DWORD Reserved
    );
#endif

    NTSTATUS __stdcall RegDeleteKeyTransacted(
        HANDLE hKey,
        PUNICODE_STRING lpSubKey,
        DWORD viewDesired, // 32/64
        DWORD Reserved,
        HANDLE hTransaction,
        PVOID  pExtendedParameter
    );
#ifndef INTERCEPT_KERNELBASE
    NTSTATUS __stdcall RegDeleteValue(
        HANDLE hKey,
        PUNICODE_STRING lpValueName
    );
#endif


#ifdef INTERCEPT_NTLL

#endif

#ifdef INTERCEPT_KERNELBASE
    // There are system dlls that call directly into kernelbase.dll, bypassing our detours which hook into Kernel32. We need to fix those too.
    // An example is the Winsock2 library ws2_32.dll, which calls RegOpenKeyExW directly in kernelbase.dll.  This dll is used by Bloomberg Terminal, for example.
    
    LSTATUS __stdcall RegCreateKeyExA(
        _In_ HKEY key,
        _In_ const char* subKey,
        _Reserved_ DWORD reserved,
        _In_opt_ char* classType,
        _In_ DWORD options,
        _In_ REGSAM samDesired,
        _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
        _Out_ PHKEY resultKey,
        _Out_opt_ LPDWORD disposition);

    LSTATUS __stdcall RegCreateKeyExW(
        _In_ HKEY key,
        _In_ const wchar_t* subKey,
        _Reserved_ DWORD reserved,
        _In_opt_ wchar_t* classType,
        _In_ DWORD options,
        _In_ REGSAM samDesired,
        _In_opt_ CONST LPSECURITY_ATTRIBUTES securityAttributes,
        _Out_ PHKEY resultKey,
        _Out_opt_ LPDWORD disposition);

    
    NTSTATUS __stdcall RegOpenKeyExA(
        _In_ HKEY key,
        _In_ const char* subKey,
        _In_ DWORD options,
        _In_ REGSAM samDesired,
        _Out_ PHKEY resultKey
    );
    NTSTATUS __stdcall RegOpenKeyExW(
        _In_ HKEY key,
        _In_ const wchar_t* subKey,
        _In_ DWORD options,
        _In_ REGSAM samDesired,
        _Out_ PHKEY resultKey
    );

    LSTATUS __stdcall RegDeleteKeyExA(
        _In_ HKEY key,
        _In_ const char* subKey,
        _In_ REGSAM samDesired,
        _Reserved_ DWORD reserved
    );
    LSTATUS __stdcall RegDeleteKeyExW(
        _In_ HKEY key,
        _In_ const wchar_t* subKey,
        _In_ REGSAM samDesired,
        _Reserved_ DWORD reserved
    );

    LSTATUS __stdcall RegDeleteKeyValueA(
        _In_ HKEY key,
        _In_ const char* subKey,
        _In_ const char* subValueName);
    LSTATUS __stdcall RegDeleteKeyValueW(
        _In_ HKEY key,
        _In_ const wchar_t* subKey, 
        _In_ const wchar_t* subValueName);

    LSTATUS __stdcall RegDeleteValueA(
        _In_ HKEY key,
        _In_ const char* subValueName);
    LSTATUS __stdcall RegDeleteValueW(
        _In_ HKEY key,
        _In_ const wchar_t* subValueName);

    LSTATUS __stdcall RegEnumKeyExA(
        _In_ HKEY key,
        _In_ DWORD dwIndex,
        _Out_ LPSTR lpName,
        _In_ _Out_ LPDWORD lpcchName,
        _Reserved_ LPDWORD lpReserved,
        _In_ _Out_ LPSTR lpClass,
        _In_opt_ _Out_opt_  LPDWORD lpcchClass,
        _Out_opt_ PFILETIME lpftLastWriteTime);
    LSTATUS __stdcall RegEnumKeyExW(
        _In_ HKEY key,
        _In_ DWORD dwIndex,
        _Out_ LPWSTR lpName,
        _In_ _Out_ LPDWORD lpcchName,
        _Reserved_ LPDWORD lpReserved,
        _In_ _Out_ LPWSTR lpClass,
        _In_opt_ _Out_opt_  LPDWORD lpcchClass,
        _Out_opt_ PFILETIME lpftLastWriteTime);

    LSTATUS __stdcall RegEnumValueA(
        _In_ HKEY key,
        _In_ DWORD dwIndex,
        _Out_ LPSTR lpName,
        _In_ _Out_ LPDWORD lpcchName,
        _Reserved_ LPDWORD lpReserved,
        _Out_opt_ LPDWORD lpType,
        _Out_opt_ LPBYTE   lpData,
        _In_opt_ _Out_opt_ LPDWORD lpcbData);
    LSTATUS __stdcall RegEnumValueW(
        _In_ HKEY key,
        _In_ DWORD dwIndex,
        _Out_ LPWSTR lpName,
        _In_ _Out_ LPDWORD lpcchName,
        _Reserved_ LPDWORD lpReserved,
        _Out_opt_ LPDWORD lpType,
        _Out_opt_ LPBYTE lpData,
        _In_opt_ _Out_opt_ LPDWORD lpcbData);
#ifdef INTERCEPT_KERNELBASE_PlusRegGetValue
    LSTATUS __stdcall RegGetValueA(
        _In_ HKEY key,
        _In_opt_ LPCSTR lpSubKey,
        _In_opt_ LPCSTR lpValue,
        _In_opt_ DWORD dwFlags,
        _Out_opt_ LPDWORD lpDwType,
        _Out_opt_ PVOID lpData,
        _In_opt_ _Out_opt_ LPDWORD lpcbData);
    LSTATUS __stdcall RegGetValueW(
        _In_ HKEY key,
        _In_opt_ LPCWSTR lpSubKey,
        _In_opt_ LPCWSTR lpValue,
        _In_opt_ DWORD dwFlags,
        _Out_opt_ LPDWORD lpDwType,
        _Out_opt_ PVOID lpData,
        _In_opt_ _Out_opt_ LPDWORD lpcbData);
#endif

    LSTATUS __stdcall RegQueryValueExA(
        _In_ HKEY key,
        _In_opt_ LPCSTR lpValueName,
                 LPDWORD lpReservered,
        _Out_opt_ LPDWORD lpDwType,
        _Out_opt_ PVOID lpData,
        _In_opt_ _Out_opt_ LPDWORD lpcbData);
    LSTATUS __stdcall RegQueryValueExW(
        _In_ HKEY key,
        _In_opt_ LPCWSTR lpValueName,
        LPDWORD lpReservered,
        _Out_opt_ LPDWORD lpDwType,
        _Out_opt_ PVOID lpData,
        _In_opt_ _Out_opt_ LPDWORD lpcbData);

#endif

}



namespace winternl
{
    // NOTE: The functions in winternl.h are not included in any import lib and therefore must be manually loaded in
    template <typename Func>
    inline Func GetNtDllInternalFunction(const char* functionName)
    {
        static auto mod = ::LoadLibraryW(L"ntdll.dll");
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
        //Log(L">>>NtDll Fixup loaded name=%S 0x%x", functionName, result);
        assert(result);
        return result;
    }

#ifdef  INTERCEPT_KERNELBASE
    template <typename Func>
    inline Func GetKernelBaseInternalFunction(const char* functionName)
    {
        static auto mod = ::LoadLibraryW(L"kernelbase.dll");
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
        //Log(L">>>KernelBase Fixup loaded name=%S 0x%x", functionName, result);
        assert(result);
        return result;
    }
#endif
}



#define WINTERNL_FUNCTION(Name) (winternl::GetNtDllInternalFunction<decltype(&Name)>(#Name));
#define KERNELBASEINTERNL_FUNCTION(Name) (winternl::GetKernelBaseInternalFunction<decltype(&Name)>(#Name));

namespace impl
{

    inline auto NtQueryKey = WINTERNL_FUNCTION(winternl::NtQueryKey);

#ifdef INTERCEPT_KERNELBASE
    inline auto KernelBaseRegCreateKeyExA = KERNELBASEINTERNL_FUNCTION(winternl::RegCreateKeyExA);
    inline auto KernelBaseRegCreateKeyExW = KERNELBASEINTERNL_FUNCTION(winternl::RegCreateKeyExW);

    inline auto KernelBaseRegOpenKeyExA = KERNELBASEINTERNL_FUNCTION(winternl::RegOpenKeyExA);
    inline auto KernelBaseRegOpenKeyExW = KERNELBASEINTERNL_FUNCTION(winternl::RegOpenKeyExW);

    inline auto KernelBaseRegDeleteKeyExA = KERNELBASEINTERNL_FUNCTION(winternl::RegDeleteKeyExA);
    inline auto KernelBaseRegDeleteKeyExW = KERNELBASEINTERNL_FUNCTION(winternl::RegDeleteKeyExW);

    inline auto KernelBaseRegDeleteKeyValueA = KERNELBASEINTERNL_FUNCTION(winternl::RegDeleteKeyValueA);
    inline auto KernelBaseRegDeleteKeyValueW = KERNELBASEINTERNL_FUNCTION(winternl::RegDeleteKeyValueW);

    inline auto KernelBaseRegDeleteValueA = KERNELBASEINTERNL_FUNCTION(winternl::RegDeleteValueA);
    inline auto KernelBaseRegDeleteValueW = KERNELBASEINTERNL_FUNCTION(winternl::RegDeleteValueW);

    inline auto KernelBaseRegEnumKeyExA = KERNELBASEINTERNL_FUNCTION(winternl::RegEnumKeyExA);
    inline auto KernelBaseRegEnumKeyExW = KERNELBASEINTERNL_FUNCTION(winternl::RegEnumKeyExW);

    inline auto KernelBaseRegEnumValueA = KERNELBASEINTERNL_FUNCTION(winternl::RegEnumValueA);
    inline auto KernelBaseRegEnumValueW = KERNELBASEINTERNL_FUNCTION(winternl::RegEnumValueW);

#ifdef INTERCEPT_KERNELBASE_PlusRegGetValue
    inline auto KernelBaseRegGetValueA = KERNELBASEINTERNL_FUNCTION(winternl::RegGetValueA);
    inline auto KernelBaseRegGetValueW = KERNELBASEINTERNL_FUNCTION(winternl::RegGetValueW);
#endif

    inline auto KernelBaseRegQueryValueExA = KERNELBASEINTERNL_FUNCTION(winternl::RegQueryValueExA);
    inline auto KernelBaseRegQueryValueExW = KERNELBASEINTERNL_FUNCTION(winternl::RegQueryValueExW);
#endif

}





