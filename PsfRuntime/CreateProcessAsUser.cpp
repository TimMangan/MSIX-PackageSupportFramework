//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// The PsfRuntime intercepts all CreateProcess[AW] calls so that fixups can be propagated to child processes. The
// general call graph looks something like the following:
//      * CreateProcessFixup gets called by application code; forwards arguments on to:
//      * DetourCreateProcessWithDlls[AW], which calls:
//      * CreateProcessInterceptRunDll32, which identifies attempts to launch rundll32 from System32/SysWOW64,
//        redirecting those attempts to either PsfRunDll32.exe or PsfRunDll64.exe, and then calls:
//      * The actual implementation of CreateProcess[AW]
//
// NOTE: Other CreateProcess variants (e.g. CreateProcessAsUser[AW]) aren't currently detoured as the Detours framework
//       does not make it easy to accomplish that at this time.
//

#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <map>
#include <psf_constants.h>
#include <psf_framework.h>
#include <psf_runtime.h>
#include <psf_logging.h>

#include "Config.h"
#include <StartInfo_helper.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <findStringIC.h>


#include <psf_utils.h>
#include <psf_config.h>
#include "JsonConfig.h"

#define IGNORE_USER 1

#if _DEBUG
//#define MOREDEBUG 1
#endif

using namespace std::literals;


extern wchar_t g_PsfRunTimeModulePath[];

extern DWORD g_CreateProcessIntceptInstance;
extern std::wstring FixDllBitness(std::wstring originalName, USHORT bitness);
extern USHORT ProcessBitness(HANDLE hProcess);
extern void LogCreationFlags(DWORD Instance, DWORD CreationFlags, LPCWSTR InterceptName);
extern BOOL WINAPI CreateProcessWithPsfRunDll(
    [[maybe_unused]] _In_opt_ LPCWSTR applicationName,
    _Inout_opt_ LPWSTR commandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
    _In_ BOOL inheritHandles,
    _In_ DWORD creationFlags,
    _In_opt_ LPVOID environment,
    _In_opt_ LPCWSTR currentDirectory,
    _In_ LPSTARTUPINFOW startupInfo,
    _Out_ LPPROCESS_INFORMATION processInformation);


template <typename CharT>
using startup_info_t = std::conditional_t<std::is_same_v<CharT, char>, STARTUPINFOA, STARTUPINFOW>;

//template <typename CharT>
//extern BOOL WINAPI CreateProcessFixup(
//    _In_opt_ const CharT* applicationName,
//    _Inout_opt_ CharT* commandLine,
//    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
//    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
//    _In_ BOOL inheritHandles,
//    _In_ DWORD creationFlags,
//    _In_opt_ LPVOID environment,
//    _In_opt_ const CharT* currentDirectory,
//    _In_ startup_info_t<CharT>* startupInfo,
//    _Out_ LPPROCESS_INFORMATION processInformation);

auto CreateProcessAsUserImpl = psf::detoured_string_function(&::CreateProcessAsUserA, &::CreateProcessAsUserW);


template <typename CharT, typename LPSTARTUPINFO>
BOOL WINAPI CreateProcessAsUserFixup(
    _In_opt_     HANDLE                hToken,
    _In_opt_     const CharT*          lpApplicationName,
    _Inout_opt_  CharT*                lpCommandLine,
    _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_         BOOL                  bInheritHandles,
    _In_         DWORD                 dwCreationFlags,
    _In_opt_     LPVOID                lpEnvironment,
    _In_opt_     const   CharT*        lpCurrentDirectory,
    _In_         LPSTARTUPINFO         lpStartupInfo,
    _Out_        LPPROCESS_INFORMATION lpProcessInformation
)  noexcept try
{
    DWORD PossiblyModifiedCreationFlags = dwCreationFlags;
    DWORD DllInstance = ++g_CreateProcessIntceptInstance;

    bool skipForce = false;  // exclude out certain processes from forcing to run inside the container, like conhost and maybe cmd and powershell

    MyProcThreadAttributeList* partialList = new MyProcThreadAttributeList(true, true, false);


    STARTUPINFOEXW startupInfoExW =
    {
        {
        sizeof(startupInfoExW)
        , nullptr // lpReserved
        , nullptr // lpDesktop
        , nullptr // lpTitle
        , 0 // dwX
        , 0 // dwY
        , 0 // dwXSize
        , 0 // swYSize
        , 0 // dwXCountChar
        , 0 // dwYCountChar
        , 0 // dwFillAttribute
        , STARTF_USESHOWWINDOW // dwFlags
        , 0
        }
    };
    STARTUPINFOEXA startupInfoExA =
    {
        {
        sizeof(startupInfoExA)
        , nullptr // lpReserved
        , nullptr // lpDesktop
        , nullptr // lpTitle
        , 0 // dwX
        , 0 // dwY
        , 0 // dwXSize
        , 0 // swYSize
        , 0 // dwXCountChar
        , 0 // dwYCountChar
        , 0 // dwFillAttribute
        , STARTF_USESHOWWINDOW // dwFlags
        , 0 // wShowWindow
        }
    };
    STARTUPINFOEX* MyReplacementStartupInfo = reinterpret_cast<STARTUPINFOEX*>(lpStartupInfo);

    LogString(DllInstance, L"CreateProcessAsUserFixup: commandline", lpCommandLine);
#if _DEBUG
    LogCreationFlags(DllInstance, PossiblyModifiedCreationFlags, L"CreateProcessAsUserFixup");
#endif
#ifdef MOREDEBUG
    if (lpProcessAttributes != NULL)
    {
        if (lpProcessAttributes->lpSecurityDescriptor != NULL)
        {
            Log(L" [%d] CreateProcessAsUserFixup: Request has a ProcessAttributes/Security Descriptor.", DllInstance);
        }
        Log(L" [%d] CreateProcessAsUserFixup: Request ProcessAttributes bInheritHandle = 0x%x", DllInstance, lpProcessAttributes->bInheritHandle);
    }
    if (lpThreadAttributes != NULL)
    {
        if (lpThreadAttributes->lpSecurityDescriptor != NULL)
        {
            Log(L" [%d] CreateProcessAsUserFixup: Request has a ThreadAttributes/Security Descriptor.", DllInstance);
        }
        Log(L" [%d] CreateProcessAsUserFixup: Request ThreadAttributes bInheritHandle = 0x%x", DllInstance, lpThreadAttributes->bInheritHandle);
    }
    Log(L" [%d] CreateProcessAsUserFixup: Request base bInheritHandles = 0x%x", DllInstance, bInheritHandles);
    if (lpEnvironment != NULL)
    {
        Log(L" [%d] CreateProcessAsUserFixup: Request has Environment.", DllInstance);
    }
    if (lpCurrentDirectory != NULL)
    {
        Log(L" [%d] CreateProcessAsUserFixup: Request has currentDirectory=%s", DllInstance, lpCurrentDirectory);
    }
#endif
         

    if constexpr (psf::is_ansi<CharT>)
    {
        if (findStringIC(lpCommandLine, "conhost"))
        {
            skipForce = true;
#ifdef _DEBUG
            Log(L"\t[%d] CreateProcessAsUserFixup: skipForce.", DllInstance);
#endif
        }
    }
    else
    {
        if (findStringIC(lpCommandLine, L"conhost"))
        {
            skipForce = true;
#ifdef _DEBUG
            Log(L"\t[%d] CreateProcessAsUserFixup: skipForce.", DllInstance);
#endif
        }
    }

#if CLEANUP_FLAG_CREATE_UNICODE_ENVIRONMENT
    // Remove unneccessary UNICODE directive if not needed
    if (lpEnvironment == NULL &&
        (PossiblyModifiedCreationFlags & CREATE_UNICODE_ENVIRONMENT) != 0)
    {
        PossiblyModifiedCreationFlags &= ~CREATE_UNICODE_ENVIRONMENT;
    }
#endif
#if CLEANUP_FLAG_DETACHED_PROCESS
    // Remove unneccessary DETACHED_PROCESS directive if not needed
    if ( (PossiblyModifiedCreationFlags & DETACHED_PROCESS) != 0)
    {
        PossiblyModifiedCreationFlags &= ~DETACHED_PROCESS;
    }
#endif

    if (!skipForce)
    {

        if ((dwCreationFlags & EXTENDED_STARTUPINFO_PRESENT) != 0)
        {
#if _DEBUG
            Log(L"\t[%d] CreateProcessAsUserFixup: Extended StartupInfo present but want to force running inside container unless app requested otherwise.", DllInstance);
#endif

            // Hopefully it is set to start in the container anyway.
            if constexpr (psf::is_ansi<CharT>)
            {
                STARTUPINFOEXA* si = reinterpret_cast<STARTUPINFOEXA*>(lpStartupInfo);
                if (!si->lpAttributeList)
                {
#ifdef MOREDEBUG
                    Log(L"\t[%d] CreateProcessAsUserFixup no existing attributelist, just add one", DllInstance);
#endif
                    si->lpAttributeList = partialList->get();
                }
                else
                {
#ifdef MOREDEBUG
                    Log(L"\t[%d] CreateProcessAsUserFixup has existing attributelist, fix it up.", DllInstance);
#endif
                    partialList = new MyProcThreadAttributeList(si->lpAttributeList, true, true);
                    si->lpAttributeList = partialList->get();
#if MOREDEBUG
                    DumpStartupAttributes(reinterpret_cast<SIH_PROC_THREAD_ATTRIBUTE_LIST*>(si->lpAttributeList), DllInstance);
#endif
                }
            }
            else
            {
                STARTUPINFOEXW* si = reinterpret_cast<STARTUPINFOEXW*>(lpStartupInfo);
                if (!si->lpAttributeList)
                {
#ifdef MOREDEBUG
                    Log(L"\t[%d] CreateProcessAsUserFixup no existing attributelist, just add one.", DllInstance);
#endif
                    si->lpAttributeList = partialList->get();
                }
                else
                {
#ifdef MOREDEBUG
                    Log(L"\t[%d] CreateProcessAsUserFixup has existing attributelist, fix it up.", DllInstance);
#endif
                    partialList = new MyProcThreadAttributeList(si->lpAttributeList, true, true);
                    si->lpAttributeList = partialList->get();
#if MOREDEBUG
                    DumpStartupAttributes(reinterpret_cast<SIH_PROC_THREAD_ATTRIBUTE_LIST*>(si->lpAttributeList), DllInstance);
#endif
                }
            }
        }
        else
        {
#if _DEBUG
            Log(L"\t[%d] CreateProcessAsUserFixup: Add Extended StartupInfo to force running inside container.", DllInstance);
#endif
            // There are situations where processes jump out of the container and this helps to make them stay within.
            // Both cmd and powershell are such cases.
            PossiblyModifiedCreationFlags |= EXTENDED_STARTUPINFO_PRESENT;
            if constexpr (psf::is_ansi<CharT>)
            {
                STARTUPINFOEXA* si = reinterpret_cast<STARTUPINFOEXA*>(lpStartupInfo);
                startupInfoExA.StartupInfo.cb = sizeof(startupInfoExA);
                startupInfoExA.StartupInfo.cbReserved2 = si->StartupInfo.cbReserved2;
                startupInfoExA.StartupInfo.dwFillAttribute = si->StartupInfo.dwFillAttribute;
                startupInfoExA.StartupInfo.dwFlags = si->StartupInfo.dwFlags;
                startupInfoExA.StartupInfo.dwX = si->StartupInfo.dwX;
                startupInfoExA.StartupInfo.dwXCountChars = si->StartupInfo.dwXCountChars;
                startupInfoExA.StartupInfo.dwXSize = si->StartupInfo.dwXSize;
                startupInfoExA.StartupInfo.dwY = si->StartupInfo.dwY;
                startupInfoExA.StartupInfo.dwYCountChars = si->StartupInfo.dwYCountChars;
                startupInfoExA.StartupInfo.dwYSize = si->StartupInfo.dwYSize;
                startupInfoExA.StartupInfo.hStdError = si->StartupInfo.hStdError;
                startupInfoExA.StartupInfo.hStdInput = si->StartupInfo.hStdInput;
                startupInfoExA.StartupInfo.hStdOutput = si->StartupInfo.hStdOutput;
                startupInfoExA.StartupInfo.lpDesktop = si->StartupInfo.lpDesktop;
                startupInfoExA.StartupInfo.lpReserved = si->StartupInfo.lpReserved;
                startupInfoExA.StartupInfo.lpReserved2 = si->StartupInfo.lpReserved2;
                startupInfoExA.StartupInfo.lpTitle = si->StartupInfo.lpTitle;
                startupInfoExA.StartupInfo.wShowWindow = si->StartupInfo.wShowWindow;
                startupInfoExA.lpAttributeList = partialList->get();
                MyReplacementStartupInfo = reinterpret_cast<STARTUPINFOEX*>(&startupInfoExA);
            }
            else
            {
                STARTUPINFOEXW* si = reinterpret_cast<STARTUPINFOEXW*>(lpStartupInfo);
                startupInfoExW.StartupInfo.cb = sizeof(startupInfoExW);
                startupInfoExW.StartupInfo.cbReserved2 = si->StartupInfo.cbReserved2;
                startupInfoExW.StartupInfo.dwFillAttribute = si->StartupInfo.dwFillAttribute;
                startupInfoExW.StartupInfo.dwFlags = si->StartupInfo.dwFlags;
                startupInfoExW.StartupInfo.dwX = si->StartupInfo.dwX;
                startupInfoExW.StartupInfo.dwXCountChars = si->StartupInfo.dwXCountChars;
                startupInfoExW.StartupInfo.dwXSize = si->StartupInfo.dwXSize;
                startupInfoExW.StartupInfo.dwY = si->StartupInfo.dwY;
                startupInfoExW.StartupInfo.dwYCountChars = si->StartupInfo.dwYCountChars;
                startupInfoExW.StartupInfo.dwYSize = si->StartupInfo.dwYSize;
                startupInfoExW.StartupInfo.hStdError = si->StartupInfo.hStdError;
                startupInfoExW.StartupInfo.hStdInput = si->StartupInfo.hStdInput;
                startupInfoExW.StartupInfo.hStdOutput = si->StartupInfo.hStdOutput;
                startupInfoExW.StartupInfo.lpDesktop = si->StartupInfo.lpDesktop;
                startupInfoExW.StartupInfo.lpReserved = si->StartupInfo.lpReserved;
                startupInfoExW.StartupInfo.lpReserved2 = si->StartupInfo.lpReserved2;
                startupInfoExW.StartupInfo.lpTitle = si->StartupInfo.lpTitle;
                startupInfoExW.StartupInfo.wShowWindow = si->StartupInfo.wShowWindow;
                startupInfoExW.lpAttributeList = partialList->get();
                MyReplacementStartupInfo = reinterpret_cast<STARTUPINFOEX*>(&startupInfoExW);
            }
        }
    }


    // We can't detour child processes whose executables are located outside of the package as they won't have execute
    // access to the fixup dlls. Instead of trying to replicate the executable search logic when determining the location
    // of the target executable, create the process as suspended and let the system tell us where the executable is.
    // The structure below allows us to track the new process, even if the caller didn't specify one.
    PROCESS_INFORMATION pi = { 0 };
    if (!lpProcessInformation)
    {
        lpProcessInformation = &pi;
    }


    // In order to perform injection, we will force the new process to start suspended, then inject PsfRuntime, and then resume the process.
    ///if (!skipForce)
    ///{
    ///    PossiblyModifiedCreationFlags &= ~CREATE_BREAKAWAY_FROM_JOB;  // for debugging only
    ///}
#if _DEBUG
    //////if (!skipForce)
    //////{
        //////PossiblyModifiedCreationFlags &= ~CREATE_NO_WINDOW;  // for debugging only
    //////}
#endif
    PossiblyModifiedCreationFlags |= CREATE_SUSPENDED;

#if MOREDEBUG
    LogCreationFlags(DllInstance, PossiblyModifiedCreationFlags, L"CreateProcessAsUserFixup");
#endif

#if IGNORE_USER
    // So while we can start the new process in the bubble, even using the same user's token, bad things happen.  So let's try to just run it in
    // the current context.
    bool worked;
    if constexpr (psf::is_ansi<CharT>)
    {
        worked = ::CreateProcessA(lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            PossiblyModifiedCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            reinterpret_cast<startup_info_t<CharT>*>(MyReplacementStartupInfo), //lpStartupInfo,
            lpProcessInformation);
    }
    else
    {

        worked = ::CreateProcessW(lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            PossiblyModifiedCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            reinterpret_cast<startup_info_t<CharT>*>(MyReplacementStartupInfo), //lpStartupInfo,
            lpProcessInformation);
    }
    if (worked == TRUE)
    {
        Log(L"\t[%d] CreateProcessAsUserFixup: Returns TRUE", DllInstance);
        return TRUE;
    }
    if (worked == FALSE)
#else
    if (::CreateProcessAsUserImpl(
        hToken,
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        PossiblyModifiedCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        reinterpret_cast<startup_info_t<CharT>*>(MyReplacementStartupInfo), //lpStartupInfo,
        lpProcessInformation) == FALSE)
#endif
    {
        DWORD err = GetLastError();
        if (err == 0x2e4)
        {
            Log(L"\t[%d] CreateProcessAsUserFixup: Creation returned false due to elevation", DllInstance);

            // The app requires elevation, so try it this way.  Not perfect.
            // The better solution is to determine the need during packaging and add
            // an external manifest file to Psflauncher to elevate first.
            {
                // just remove the added startup info
                PossiblyModifiedCreationFlags = dwCreationFlags | CREATE_SUSPENDED;
                if (::CreateProcessAsUserImpl(
                    hToken,
                    lpApplicationName,
                    lpCommandLine,
                    lpProcessAttributes,
                    lpThreadAttributes,
                    bInheritHandles,
                    PossiblyModifiedCreationFlags,
                    lpEnvironment,
                    lpCurrentDirectory,
                    lpStartupInfo,
                    lpProcessInformation) == FALSE)
                {
                    // looks bad, have no idea why or what to do here if here.
                    err = GetLastError();
                    return FALSE;
                }
            }
        }
        else
        {
            Log(L"\t[%d] CreateProcessAsUserFixup: Creation returned false trying to force in container without prot 0x%x retry with prot.", DllInstance, err);
            Log(L"\t[%d] CreateProcessAsUserFixup: pid reported as 0x%x", DllInstance, lpProcessInformation->dwProcessId);
            if (lpProcessInformation->dwProcessId == 0)
            {
                return FALSE;
            }
        }

    }
    iwstring path;
    DWORD size = MAX_PATH;
    path.resize(size_t(size - 1));
    while (true)
    {
        if (::QueryFullProcessImageNameW(lpProcessInformation->hProcess, 0, path.data(), &size))
        {
            path.resize(size_t(size));
            break;
        }
        else if (auto err = ::GetLastError(); err == ERROR_INSUFFICIENT_BUFFER)
        {
            size *= 2;
            path.resize(size_t(size - 1));
        }
        else
        {
            // Unexpected error
            Log(L"\t[%d] CreateProcessAsUserFixup: Unable to retrieve process.", DllInstance);
            ::TerminateProcess(lpProcessInformation->hProcess, ~0u);
            ::CloseHandle(lpProcessInformation->hProcess);
            ::CloseHandle(lpProcessInformation->hThread);

            ::SetLastError(err);
            return FALSE;
        }
    }


    // std::filesystem::path comparison doesn't seem to handle case-insensitivity or root-local device paths...
    iwstring_view packagePath(PackageRootPath().native().c_str(), PackageRootPath().native().length());
    iwstring_view finalPackagePath(FinalPackageRootPath().native().c_str(), FinalPackageRootPath().native().length());
    iwstring_view exePath = path;
    auto fixupPath = [](iwstring_view& p)
    {
        if ((p.length() >= 4) && (p.substr(0, 4) == LR"(\\?\)"_isv))
        {
            p = p.substr(4);
        }
    };
    fixupPath(packagePath);
    fixupPath(finalPackagePath);
    fixupPath(exePath);

#if _DEBUG
    Log(L"\t[%d] CreateProcessAsUserFixup: Possible injection to process %ls %d.\n", DllInstance, exePath.data(), lpProcessInformation->dwProcessId);
#endif
    //if (((exePath.length() >= packagePath.length()) && (exePath.substr(0, packagePath.length()) == packagePath)) ||
    //    ((exePath.length() >= finalPackagePath.length()) && (exePath.substr(0, finalPackagePath.length()) == finalPackagePath)))
    // TRM: 2021-10-21 We do want to inject into exe processes that are outside of the package structure, for example PowerShell for a cmd file,
    // TRM: 2021-11-03 but only if the new process is running inside the Container ...
    bool allowInjection = false;

    try
    {
        if ((PossiblyModifiedCreationFlags & EXTENDED_STARTUPINFO_PRESENT) != 0)
        {

#if MOREDEBUG
            Log(L"\t[%d] CreateProcessAsUserFixup: CreateProcessImpl Attribute: Has extended Attribute.", DllInstance);
#endif
            if constexpr (psf::is_ansi<CharT>)
            {
#if MOREDEBUG
                Log(L"\t[%d] CreateProcessAsUserFixup: CreateProcessImpl Attribute: narrow", DllInstance);
#endif
                STARTUPINFOEXA* si = reinterpret_cast<STARTUPINFOEXA*>(MyReplacementStartupInfo);
                if (si->lpAttributeList != NULL)
                {
#if MOREDEBUG
                    DumpStartupAttributes(reinterpret_cast<SIH_PROC_THREAD_ATTRIBUTE_LIST*>(si->lpAttributeList), DllInstance);
#endif
                    allowInjection = DoesAttributeSpecifyInside(reinterpret_cast<SIH_PROC_THREAD_ATTRIBUTE_LIST*>(si->lpAttributeList));
                }
                else
                {
#if MOREDEBUG
                    Log(L"\t[%d] CreateProcessAsUserFixup: CreateProcessImpl Attribute: attlist is null.", DllInstance);
#endif
                    allowInjection = true;
                }
            }
            else
            {
#if MOREDEBUG
                Log(L"\t[%d] CreateProcessFixup: CreateProcessAsUserImpl Attribute:: wide", DllInstance);
#endif
                STARTUPINFOEXW* si = reinterpret_cast<STARTUPINFOEXW*>(MyReplacementStartupInfo);
                if (si->lpAttributeList != NULL)
                {
#if MOREDEBUG
                    DumpStartupAttributes(reinterpret_cast<SIH_PROC_THREAD_ATTRIBUTE_LIST*>(si->lpAttributeList), DllInstance);
#endif
                    allowInjection = DoesAttributeSpecifyInside(reinterpret_cast<SIH_PROC_THREAD_ATTRIBUTE_LIST*>(si->lpAttributeList));
                }
                else
                {
#if MOREDEBUG
                    Log(L"\t[%d] CreateProcessAsUserFixup: CreateProcessAsUserImpl Attribute: attlist is null.", DllInstance);
#endif
                    allowInjection = true;
                }
            }
        }
        else
        {
#if MOREDEBUG
            Log(L"\t[%d] CreateProcessAsUserFixup: CreateProcessAsUserImpl Attribute: Does not have extended attribute and should be added.", DllInstance);
#endif
            allowInjection = true;
        }
    }
    catch (...)
    {
        Log(L"\t[%d] CreateProcessAsUserFixup: Exception testing for attribute list, assuming none.", DllInstance);
        allowInjection = false;
    }

    if (allowInjection)
    {
        // There are situations where the new process might jump outside of the container to run.
        // In those situations, we can't inject dlls into it.
        try
        {
            BOOL b;
            BOOL res = IsProcessInJob(lpProcessInformation->hProcess, nullptr, &b);
            if (res != 0)
            {
                if (b == false)
                {
                    allowInjection = false;
                    Log(L"\t[%d] CreateProcessAsUserFixup: New process has broken away from container, do not inject.", DllInstance);
                }
                else
                {
                    Log(L"\t[%d] CreateProcessAsUserFixup: New process is in a job, allow injection.", DllInstance);
                    // NOTE: we could maybe try to see if in the same job, but this is probably good enough.
                }
            }
            else
            {
                Log(L"\t[%d] CreateProcessAsUserFixup: Unable to detect job status of new process, ignore for now and try to inject 0x%x 0x%x 0x%x.", DllInstance, res, GetLastError(), b);
            }
        }
        catch (...)
        {
            allowInjection = false;
            Log(L"\t[%d] CreateProcessAsUserFixup: Exception while trying to determine job status of new process. Do not inject.", DllInstance);
        }
    }

    // There are processes listed in the processes of the json that don't have any fixups, so let's not inject the PsfRuntime into those.
    // Some of those are console apps where we have to avoid it.
    std::wstring procname2launch;
    size_t off = exePath.find_last_of(L"\\");
    if (off != std::wstring::npos)
    {
        procname2launch = exePath.substr(off + 1).data();
    }
    else
    {
        procname2launch = exePath.data();
    }
    auto exeConfigJson = PSFQueryExeConfig(procname2launch.c_str());
    if (exeConfigJson != nullptr)
    {
        if (auto fixups = exeConfigJson->try_get("fixups"))
        {
            bool foundany = false;
            if (fixups != nullptr)
            {
                for (auto& fixupConfig : fixups->as_array())
                {
                    auto fixupConfigdll = fixupConfig.as_object().try_get("dll");
                    if (fixupConfigdll != nullptr)
                    {
                        foundany = true;
                    }
                }
            }
            if (!foundany)
            {
                Log(L"\t[%d] CreateProcessAsUserFixup: skip Injections due to json process match without fixup dlls.", DllInstance);
                allowInjection = false;
            }
        }
        else
        {
            Log(L"\t[%d] CreateProcessAsUserFixup: skip Injections due to json process match without fixups.", DllInstance);
            allowInjection = false;
        }
    }
    else
    {
        Log("\t[%d] CreateProcessAsUserFixup: Child process match not found?; allow injections anyway.", DllInstance);
    }


    if (allowInjection)
    {
        // The target executable is in the package, so we _do_ want to fixup it
#if _DEBUG
        Log(L"\t[%d] CreateProcessAsUserFixup: Allowed Injection, so yes", DllInstance);
#endif
        // Fix for issue #167: allow subprocess to be a different bitness than this process.
        USHORT bitness = ProcessBitness(lpProcessInformation->hProcess);
#if _DEBUG
        Log(L"\t[%d] CreateProcessAsUserFixup: Injection for PID=%d Bitness=%d", DllInstance, lpProcessInformation->dwProcessId, bitness);
#endif  
        std::wstring wtargetDllName = FixDllBitness(std::wstring(psf::runtime_dll_name), bitness);
#if _DEBUG
        Log(L"\t[%d] CreateProcessAsUserFixup: Use runtime %ls", DllInstance, wtargetDllName.c_str());
#endif
        ///static const auto pathToPsfRuntime = (PackageRootPath() / wtargetDllName.c_str()).string();
        static std::string pathToPsfRuntime;
        if (g_PsfRunTimeModulePath[0] != 0x0)
        {
            std::filesystem::path RuntimePath = g_PsfRunTimeModulePath;
            pathToPsfRuntime = RuntimePath.string();
        }
        else
        {
            pathToPsfRuntime = (PackageRootPath() / wtargetDllName.c_str()).string();
        }
        const char* targetDllPath = NULL;
#if _DEBUG
        Log("\t[%d] CreateProcessAsUserFixup: Inject %s into PID=%d", DllInstance, pathToPsfRuntime.c_str(), lpProcessInformation->dwProcessId);
#endif

        if (std::filesystem::exists(pathToPsfRuntime))
        {
            targetDllPath = pathToPsfRuntime.c_str();
        }
        else
        {
            // Possibly the dll is in the folder with the exe and not at the package root.
#if _DEBUG
            Log(L"\t[%] CreateProcessAsUserFixup: %ls not found at package root, try target folder.", DllInstance, wtargetDllName.c_str());
#endif

            std::filesystem::path altPathToExeRuntime = exePath.data();
            static const auto altPathToPsfRuntime = (altPathToExeRuntime.parent_path() / pathToPsfRuntime.c_str()).string();
#if _DEBUG
            Log(L"\t[%d] CreateProcessAsUserFixup: alt target filename is now %s", DllInstance, altPathToPsfRuntime.c_str());
#endif
            if (std::filesystem::exists(altPathToPsfRuntime))
            {
                targetDllPath = altPathToPsfRuntime.c_str();
#if _DEBUG
                Log(L"\t[%d] CreateProcessAsUserFixup: alt target exists.", DllInstance);
#endif
            }
            else
            {
#if _DEBUG
                Log(L"\t[%d] CreateProcessAsUserFixup: Not present there either, try elsewhere in package.", DllInstance);
#endif
                // If not in those two locations, must check everywhere in package.
                // The child process might also be in another package folder, so look elsewhere in the package.
                for (auto& dentry : std::filesystem::recursive_directory_iterator(PackageRootPath()))
                {
                    try
                    {
                        if (dentry.path().filename().compare(wtargetDllName) == 0)
                        {
                            static const auto altDirPathToPsfRuntime = narrow(dentry.path().c_str());
#if _DEBUG
                            Log(L"\t[%d] CreateProcessAsUserFixup: Found match as %ls", DllInstance, dentry.path().c_str());
#endif
                            targetDllPath = altDirPathToPsfRuntime.c_str();
                            break;
                        }
                    }
                    catch (...)
                    {
                        Log(L"\t[%d] CreateProcessAsUserFixup: Non-fatal error enumerating directories while looking for PsfRuntime.", DllInstance);
                    }
                }

            }
        }

        if (targetDllPath != NULL)
        {
            Log("\t[%d] CreateProcessAsUserFixup: Attempt injection into %d using %s", DllInstance, lpProcessInformation->dwProcessId, targetDllPath);
            if (!::DetourUpdateProcessWithDll(lpProcessInformation->hProcess, &targetDllPath, 1))
            {
                Log("\t[%d] CreateProcessAsUserFixup: %s unable to inject, err=0x%x.", DllInstance, targetDllPath, ::GetLastError());
                // We failed to detour the created process. Assume that the failure was due to an architecture mis-match
                // and try the launch using PsfRunDl
                if (!::DetourProcessViaHelperDllsW(lpProcessInformation->dwProcessId, 1, &targetDllPath, CreateProcessWithPsfRunDll))
                {
                    auto err = ::GetLastError();
                    Log("\t[%d] CreateProcessAsUserixup: %s unable to inject with RunDll either (Skipping), err=0x%x.", DllInstance, targetDllPath, err);
                    ::TerminateProcess(lpProcessInformation->hProcess, ~0u);
                    ::CloseHandle(lpProcessInformation->hProcess);
                    ::CloseHandle(lpProcessInformation->hThread);

                    ::SetLastError(err);
                    return FALSE;
                }
            }
            else
            {
                Log(L"\t[%d] CreateProcessAsUserFixup: Injected %ls into PID=%d\n", DllInstance, wtargetDllName.c_str(), lpProcessInformation->dwProcessId);
            }
        }
        else
        {
            Log(L"\t[%d] CreateProcessAsUserFixup: %ls not found, skipping.", DllInstance, wtargetDllName.c_str());
        }
    }
    else
    {
        Log(L"\t[%d] CreateProcessAsUserFixup: The new process is not inside the container, so doesn't inject...", DllInstance);
    }
    if ((dwCreationFlags & CREATE_SUSPENDED) != CREATE_SUSPENDED)
    {
        // Caller did not want the process to start suspended
        Log(L"\t[%d] CreateProcesAsUserFixup: Resume PID=%d\n", DllInstance, lpProcessInformation->dwProcessId);
        ::ResumeThread(lpProcessInformation->hThread);
        SetLastError(0);
    }

    if (lpProcessInformation == &pi)
    {
        // If we created this strucure we must close the handles in it
        ::CloseHandle(lpProcessInformation->hProcess);
        ::CloseHandle(lpProcessInformation->hThread);
    }

    Log(L"\t[%d] CreateProcessAsUserFixup: Returns TRUE", DllInstance);
    return TRUE;


#ifdef _DEBUG
    Log(L"\t[%d] CreateProcessAsUserFixup: Process has started as suspended, now consider injections...", DllInstance);
#endif

}
catch (...)
{
    int err = win32_from_caught_exception();
    Log(L"CreateProcessAsUserFixup: exception 0x%x", err);
    ::SetLastError(err);
    return FALSE;
}
DECLARE_STRING_FIXUP(CreateProcessAsUserImpl, CreateProcessAsUserFixup);

