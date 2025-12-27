//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#define Intercept_NTDLL 1
#if _DEBUG
//#define _ManualDebug 1
#include <thread>
#include <windows.h>
#endif

/////#define PSF_DEFINE_EXPORTS
#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "FunctionImplementations_WindowsStorage.h"
#include "FunctionImplementations_KernelBase.h"
#include "FunctionImplementations_ntdll.h"

#if _DEBUG
#define MOREDEBUG 1
#if DEBUG_NEW_FIXUPS
#define DEBUG_NEW_FIXUPS_MFR 1
#endif
#else
#define MOREDEBUG 1
#define DEBUG_NEW_FIXUPS_MFR 1
#endif

bool trace_function_entry = false;
bool m_inhibitOutput = false;
bool m_shouldLog = true;

void InitializeMFRFixup();
void InitializeConfiguration();


extern "C" {

    void PrintDebugAddrs()
    {
        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
#if DEBUG_NEW_FIXUPS_MFR
            Log(LogLevel_DebugMaximum, "CopyFile Ansi=%p Wide=%p\n", &impl::CopyFileW.ansi, &impl::CopyFileW.wide);
            Log(LogLevel_DebugMaximum, "CopyFile2 neutral=%p\n", &impl::CopyFile2);
            Log(LogLevel_DebugMaximum, "CopyFileEx Ansi=%p Wide=%p\n", &impl::CopyFileExW.ansi, &impl::CopyFileExW.wide);
            Log(LogLevel_DebugMaximum, "CreateDirectory  Ansi=%p Wide=%p\n", &impl::CreateDirectoryW.ansi, &impl::CreateDirectoryW.wide);
            Log(LogLevel_DebugMaximum, "CreateDirectoryEx  Ansi=%p Wide=%p\n", &impl::CreateDirectoryExW.ansi, &impl::CreateDirectoryExW.wide);
            Log(LogLevel_DebugMaximum, "CreateFile  Ansi=%p Wide=%p\n", &impl::CreateFileW.ansi, &impl::CreateFileW.wide);
            Log(LogLevel_DebugMaximum, "CreateFile2  neutral=%p\n", &impl::CreateFile2);
            Log(LogLevel_DebugMaximum, "CreateHardLink  Ansi=%p Wide=%p\n", &impl::CreateHardLinkW.ansi, &impl::CreateHardLinkW.wide);
            Log(LogLevel_DebugMaximum, "CreateSymbolicLink  Ansi=%p Wide=%p\n", &impl::CreateSymbolicLinkW.ansi, &impl::CreateSymbolicLinkW.wide);
            Log(LogLevel_DebugMaximum, "DeleteFile  Ansi=%p Wide=%p\n", &impl::DeleteFileW.ansi, &impl::DeleteFileW.wide);
            Log(LogLevel_DebugMaximum, "FindClose  neutral=%p\n", &impl::FindClose);
            Log(LogLevel_DebugMaximum, "FindFirstFile  Ansi=%p Wide=%p\n", &impl::FindFirstFileW.ansi, &impl::FindFirstFileW.wide);
            Log(LogLevel_DebugMaximum, "FindFirstFileEx  Ansi=%p Wide=%p\n", &impl::FindFirstFileExW.ansi, &impl::FindFirstFileExW.wide);
            Log(LogLevel_DebugMaximum, "FindNextFile  Ansi=%p Wide=%p\n", &impl::FindNextFileW.ansi, &impl::FindNextFileW.wide);

#if FIXUP_FROM_KernelBase
            Log(LogLevel_DebugMaximum, "(KernelBase)MoveFileExW  Wide=%p\n", kernelbaseimpl::MoveFileExWImpl);
#endif

            Log(LogLevel_DebugMaximum, "MoveFile  Ansi=%p Wide=%p\n", &impl::MoveFileW.ansi, &impl::MoveFileW.wide);
            Log(LogLevel_DebugMaximum, "MoveFileEx  Ansi=%p Wide=%p\n", &impl::MoveFileExW.ansi, &impl::MoveFileExW.wide);
            Log(LogLevel_DebugMaximum, "MoveFileWithProgress  Ansi=%p Wide=%p\n", &impl::MoveFileWithProgressW.ansi, &impl::MoveFileWithProgressW.wide);
            Log(LogLevel_DebugMaximum, "RemoveDirectory  Ansi=%p Wide=%p\n", &impl::RemoveDirectoryW.ansi, &impl::RemoveDirectoryW.wide);
            Log(LogLevel_DebugMaximum, "ReplaceFile  Ansi=%p Wide=%p\n", &impl::ReplaceFileW.ansi, &impl::ReplaceFileW.wide);
            Log(LogLevel_DebugMaximum, "SetFileAttributes  Ansi=%p Wide=%p\n", &impl::SetFileAttributesW.ansi, &impl::SetFileAttributesW.wide);

            Log(LogLevel_DebugMaximum, "GetFileAttributes  Ansi=%p Wide=%p\n", &impl::GetFileAttributesW.ansi, &impl::GetFileAttributesW.wide);
            Log(LogLevel_DebugMaximum, "GetFileAttributesEx  Ansi=%p Wide=%p\n", &impl::GetFileAttributesExW.ansi, &impl::GetFileAttributesExW.wide);
            Log(LogLevel_DebugMaximum, "SetFileAttributes  Ansi=%p Wide=%p\n", &impl::SetFileAttributesW.ansi, &impl::SetFileAttributesW.wide);
            Log(LogLevel_DebugMaximum, "GetPrivateProfileInt  Ansi=%p Wide=%p\n", &impl::GetPrivateProfileIntW.ansi, &impl::GetPrivateProfileIntW.wide);
            Log(LogLevel_DebugMaximum, "GetPrivateProfileSection  Ansi=%p Wide=%p\n", &impl::GetPrivateProfileSectionW.ansi, &impl::GetPrivateProfileSectionW.wide);
            Log(LogLevel_DebugMaximum, "GetPrivateProfileSectionNames  Ansi=%p Wide=%p\n", &impl::GetPrivateProfileSectionNamesW.ansi, &impl::GetPrivateProfileSectionNamesW.wide);
            Log(LogLevel_DebugMaximum, "GetPrivateProfileString  Ansi=%p Wide=%p\n", &impl::GetPrivateProfileStringW.ansi, &impl::GetPrivateProfileStringW.wide);
            Log(LogLevel_DebugMaximum, "GetPrivateProfileStruct  Ansi=%p Wide=%p\n", &impl::GetPrivateProfileStructW.ansi, &impl::GetPrivateProfileStructW.wide);

            //Log(LogLevel_DebugMaximum, "GetCurrentDirectory  Ansi=%p Wide=%p\n", &::GetCurrentDirectoryA, &::GetCurrentDirectoryW);
            Log(LogLevel_DebugMaximum, "SetCurrentDirectory  Ansi=%p Wide=%p\n", &impl::SetCurrentDirectoryW.wide, &impl::SetCurrentDirectoryW.wide);

            Log(LogLevel_DebugMaximum, "WritePrivateProfileSection  Ansi=%p Wide=%p\n", &impl::WritePrivateProfileSectionW.wide, &impl::WritePrivateProfileSectionW.wide);
            Log(LogLevel_DebugMaximum, "WritePrivateProfileString  Ansi=%p Wide=%p\n", &impl::WritePrivateProfileStringW.ansi, &impl::WritePrivateProfileStringW.wide);
            Log(LogLevel_DebugMaximum, "WritePrivateProfileStruct  Ansi=%p Wide=%p\n", &impl::WritePrivateProfileStructW.ansi, &impl::WritePrivateProfileStructW.wide);

            //Log(LogLevel_DebugMaximum, "SearchPath  Ansi=%p Wide=%p\n", &::SearchPathA, &::SearchPathW);

#if FIXUP_ORIGINAL_SHELLEXECUTE
            Log(LogLevel_DebugMaximum, "ShellExecute  Ansi=%p Wide=%p\n", &::ShellExecuteA, &::ShellExecuteW);
#endif
#if FIXUP_ORIGINAL_SHELLEXECUTEEX
            Log(LogLevel_DebugMaximum, "SHellExecuteEx  Ansi=%p Wide=%p\n", &::ShellExecuteExA, &::ShellExecuteExW);
#endif

#if Intercept_WindowsStorage
#if DO_WS_Shex_A
            if (windowsstorageimpl::ShellExecuteAImpl == nullptr)
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteA to  Ansi=NULL\n");
            else
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteA to  Ansi=%p\n", windowsstorageimpl::ShellExecuteAImpl);
#endif
#if DO_WS_Shex_W
            if (windowsstorageimpl::ShellExecuteWImpl == nullptr)
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteW to  Wide=NULL\n");
            else
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteW to  Wide=%p\n", windowsstorageimpl::ShellExecuteWImpl);
#endif
#if DO_WS_ShexEx_A
            if (windowsstorageimpl::ShellExecuteExAImpl == nullptr)
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteExA to  Ansi=NULL\n");
            else
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteExA  Ansi=%p\n", &windowsstorageimpl::ShellExecuteExAImpl);
#endif
#if DO_WS_ShexEx_W
            if (windowsstorageimpl::ShellExecuteExWImpl == nullptr)
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteExW to  Wide=NULL\n");
            else
                Log(LogLevel_DebugMaximum, "(windows.storage)ShellExecuteExW  Wide=%p\n", &windowsstorageimpl::ShellExecuteExWImpl);
            Log(LogLevel_DebugMaximum, "WindowsStorage Fixups loaded.\n");
#endif
#endif

#ifdef Intercept_NTDLL
#ifdef DO_Intercept_NtCreateFile
            if (ntdllimpl::NtCreateFileImpl != nullptr)
                Log(LogLevel_DebugMaximum, "(ntdll)NtCreateFile Neutral=%p\n", &ntdllimpl::NtCreateFileImpl);
#endif
#ifdef DO_Intercept_NtOpenFile
            if (ntdllimpl::NtOpenFileImpl != nullptr)
                Log(LogLevel_DebugMaximum, "(ntdll)NtOpenFile Neutral=%p\n", &ntdllimpl::NtOpenFileImpl);
#endif
#ifdef DO_Intercept_NtQueryDirectoryFile
            if (ntdllimpl::NtQueryDirectoryFileImpl != nullptr)
                Log(LogLevel_DebugMaximum, "(ntdll)NtQueryDirectoryFile Neutral=%p\n", &ntdllimpl::NtQueryDirectoryFileImpl);
#endif

#ifdef DO_Intercept_NtQueryDirectoryFileEx
            if (ntdllimpl::NtQueryDirectoryFileExImpl != nullptr)
                Log(LogLevel_DebugMaximum, "(ntdll)NtQueryDirectoryFileEx Neutral=%p\n", &ntdllimpl::NtQueryDirectoryFileExImpl);
#endif

            Log(LogLevel_DebugMaximum, "ntdll Fixups loaded.\n");
#endif

#endif

        }
    }

#if _ManualDebug
    void manual_LogWFD(const wchar_t* msg)
    {
        ::OutputDebugStringW(msg);
    }

    void manual_wait_for_debugger()
    {
        manual_LogWFD(LogLevel_DebugBasic, L"Start WFD");
        // If a debugger is already attached, ignore as they have likely already set all breakpoints, etc. they need
        if (!::IsDebuggerPresent())
        {
            manual_LogWFD(LogLevel_DebugBasic, L"WFD: not yet.");
            while (!::IsDebuggerPresent())
            {
                manual_LogWFD(LogLevel_DebugBasic, L"WFD: still not yet.");
                ::Sleep(1000);
            }
            manual_LogWFD(LogLevel_DebugBasic, L"WFD: Yes.");
            // NOTE: When a debugger attaches (invasively), it will inject a DebugBreak in a new thread. Unfortunately,
            //       that does not synchronize with, and may occur _after_ IsDebuggerPresent returns true, allowing
            //       execution to continue for a short period of time. In order to get around this, we'll insert our own
            //       DebugBreak call here. We also add a short(-ish) sleep so that this is likely to be the second break
            //       seen, so that the injected DebugBreak doesn't preempt us in the middle of debugging. This is of
            //       course best effort
            ::Sleep(5000);
            std::this_thread::yield();
            ::DebugBreak();
        }
        manual_LogWFD(LogLevel_DebugBasic, L"WFD: Done.\n");
    }
#endif

    int __stdcall PSFInitialize() noexcept try
    {
#if _ManualDebug
        manual_wait_for_debugger();
#endif

#if MOREDEBUG
        PrintDebugAddrs();
        int count = psf::attach_count_all_debug();
        Log(LogLevel_DebugIntermediate, L"[0] MFRFixup attaches %d fixups.", count);
#else
        psf::attach_all();
#endif
        return ERROR_SUCCESS;
    }
    catch (...)
    {
        return win32_from_caught_exception();
    }

    int __stdcall PSFUninitialize() noexcept try
    {
        psf::detach_all();
        return ERROR_SUCCESS;
    }
    catch (...)
    {
        return win32_from_caught_exception();
    }

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:PSFInitialize=_PSFInitialize@0")
#pragma comment(linker, "/EXPORT:PSFUninitialize=_PSFUninitialize@0")
#else
#pragma comment(linker, "/EXPORT:PSFInitialize=PSFInitialize")
#pragma comment(linker, "/EXPORT:PSFUninitialize=PSFUninitialize")
#endif

    BOOL APIENTRY DllMain([[maybe_unused]] HMODULE hModule,
        DWORD  ul_reason_for_call,
        [[maybe_unused]] LPVOID lpReserved
    ) noexcept try
    {
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
            InitializeMFRFixup();
            InitializeConfiguration();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        }
        return TRUE;
    }
    catch (...)
    {
        ::SetLastError(win32_from_caught_exception());
        return FALSE;
    }

}

