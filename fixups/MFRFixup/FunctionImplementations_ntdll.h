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

#define Intercept_NTDLL 1
//#define DO_Intercept_ZwCreateFile 1
//#define DO_Intercept_ZwOpenFile 1
#define DO_Intercept_ZwQueryDirectoryFile 1
#define DO_Intercept_ZwQueryDirectoryFileEx 1
#if Intercept_NTDLL


#include <reentrancy_guard.h>
#include <psf_framework.h>
#include <shellapi.h>


#include "CKernelIf_FileInformation.h"


typedef
VOID
(NTAPI* PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );



#ifdef __cplusplus
extern "C" {
#endif
// Most of the functions in NTDll that appear file/directory based take handles as input, so we don't need to worry about intercepting those.
// These are the ones that seem most interesting.

#ifdef DO_Intercept_ZwCreateFile
NTSTATUS __stdcall ZwCreateFile(
    _Out_          PHANDLE            FileHandle,
    _In_           ACCESS_MASK        DesiredAccess,
    _In_           POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_          PIO_STATUS_BLOCK   IoStatusBlock,
    _In_opt_       PLARGE_INTEGER     AllocationSize,
    _In_           ULONG              FileAttributes,
    _In_           ULONG              ShareAccess,
    _In_           ULONG              CreateDisposition,
    _In_           ULONG              CreateOptions,
    _In_opt_       PVOID              EaBuffer,
    _In_           ULONG              EaLength
    );
#endif

#if DO_Intercept_ZwOpenFile
NTSTATUS __stdcall ZwOpenFile(
    _Out_          PHANDLE            FileHandle,
    _In_           ACCESS_MASK        DesiredAccess,
    _In_           POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_          PIO_STATUS_BLOCK   IoStatusBlock,
    _In_           ULONG              ShareAccess,
    _In_           ULONG              OpenOptions
    );
#endif

#ifdef DO_Intercept_ZwQueryDirectoryFile
// This one may be needed anyway, in which case we'd have to determine the location of the file associated with the handle and try other places similar to FindFirstFile???
NTSTATUS __stdcall ZwQueryDirectoryFile(
    _In_           HANDLE                 FileHandle,
    _In_opt_       HANDLE                 Event,
    _In_opt_       PIO_APC_ROUTINE        ApcRoutine,
    _In_opt_       PVOID                  ApcContext,
    _Out_          PIO_STATUS_BLOCK       IoStatusBlock,
    _Out_          PVOID                  FileInformation,
    _In_           ULONG                  Length,
    _In_           FILE_INFORMATION_CLASS FileInformationClass,
    _In_           BOOLEAN                ReturnSingleEntry,
    _In_opt_       PUNICODE_STRING        FileName,
    _In_           BOOLEAN                RestartScan
);
#endif

 #ifdef DO_Intercept_NtQueryDirectoryFile
// This call ends up calling Zw.  But we find we need to trap at Zw because, well Microsoft sometimes calls the Zw version directly.
// So this is really here for documentation in case we find a future need to trap at Nt.
NTSTATUS __stdcall   NtQueryDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_           BOOLEAN                ReturnSingleEntry,
    _In_opt_       PUNICODE_STRING        FileName,
    _In_           BOOLEAN                RestartScan
);
#endif


#ifdef DO_Intercept_ZwQueryDirectoryFileEx
NTSTATUS __stdcall ZwQueryDirectoryFileEx(
        _In_           HANDLE                 FileHandle,
        _In_opt_       HANDLE                 Event,
        _In_opt_       PIO_APC_ROUTINE        ApcRoutine,
        _In_opt_       PVOID                  ApcContext,
        _Out_          PIO_STATUS_BLOCK       IoStatusBlock,
        _Out_          PVOID                  FileInformation,
        _In_           ULONG                  Length,
        _In_           FILE_INFORMATION_CLASS FileInformationClass,
        _In_           ULONG                  QueryFlags,
        _In_opt_       PUNICODE_STRING        FileName
    );
#endif

#ifdef DO_Intercept_NtQueryDirectoryFileEx
// This call ends up calling Zw.  But we find we need to trap at Zw because, well Microsoft sometimes calls the Zw version directly.
// So this is really here for documentation in case we find a future need to trap at Nt.
NTSTATUS __stdcall  NtQueryDirectoryFileEx(
        _In_        HANDLE                  FileHandle,
        _In_opt_    HANDLE                  Event,
        _In_opt_    PIO_APC_ROUTINE         ApcRoutine,
        _In_opt_    PVOID                   ApcContext,
        _Out_       PIO_STATUS_BLOCK        IoStatusBlock,
        _Out_writes_bytes_(Length) PVOID    FileInformation,
        _In_        ULONG                   Length,
        _In_        FILE_INFORMATION_CLASS  FileInformationClass,
        _In_        ULONG                   QueryFlags,
        _In_opt_    PUNICODE_STRING         FileName
    );
#endif

#ifdef __cplusplus
}
#endif


template <typename Func>
inline Func GetNtDllInternalFunction(const char* functionName)
{
    // There are two copies of this dll, 32 and 64 bit, but asking by name should yield the one matching the process we are running in.
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
    if (functionName != NULL)
    {
        Log(L">>>NtDll Fixup loaded name=%S from 0x%x", functionName, result);
    }
    else
    {
        Log(L">>>NtDll Fixup mistaken loaded name=??? 0x%x", result);
    }

    assert(result);
    return result;
}
#define NTDLL_FUNCTION(Name) (GetNtDllInternalFunction<decltype(&Name)>(#Name));

namespace ntdllimpl
{
#if DO_Intercept_ZwCreateFile
    inline auto ZwCreateFileImpl = NTDLL_FUNCTION(ZwCreateFile);
#endif

#if DO_Intercept_ZwOpenFile
    inline auto ZwOpenFileImpl = NTDLL_FUNCTION(ZwOpenFile);
#endif


#ifdef DO_Intercept_ZwQueryDirectoryFile
    inline auto ZwQueryDirectoryFileImpl = NTDLL_FUNCTION(ZwQueryDirectoryFile);
#endif

#ifdef DO_Intercept_ZwQueryDirectoryFileEx
    inline auto ZwQueryDirectoryFileExImpl = NTDLL_FUNCTION(ZwQueryDirectoryFileEx);
#endif

}
#endif

