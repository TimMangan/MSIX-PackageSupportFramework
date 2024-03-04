//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <algorithm>
#include <sstream>

#include <detour_transaction.h>
#include <psf_framework.h>
#include <psf_runtime.h>
#include <psf_logging.h>

#include "Config.h"

#if _DEBUG
#define MOREDEBUG 1
#endif 

struct loaded_fixup
{
    HMODULE module_handle = nullptr;
    PSFUninitializeProc uninitialize = nullptr;

    loaded_fixup() = default;
    loaded_fixup(const loaded_fixup&) = delete;
    loaded_fixup& operator=(const loaded_fixup&) = delete;

    loaded_fixup(loaded_fixup&& other) noexcept
    {
        swap(other);
    }

    loaded_fixup& operator=(loaded_fixup&& other) noexcept
    {
        swap(other);
    }

    ~loaded_fixup()
    {
        if (module_handle)
        {
            [[maybe_unused]] auto result = ::FreeLibrary(module_handle);
            assert(result);
        }
    }

    void swap(loaded_fixup& other)
    {
        std::swap(module_handle, other.module_handle);
        std::swap(uninitialize, other.uninitialize);
    }
};
std::vector<loaded_fixup> loaded_fixups;

bool usingPsf = false;
wchar_t g_PsfRunTimeModulePath[MAX_PATH];

void load_fixups()
{
    using namespace std::literals;

    if (auto config = PSFQueryCurrentExeConfig())
    {
        if (config != nullptr)
        {
            if (auto fixups = config->try_get("fixups"))
            {
                if (fixups != nullptr)
                {
                    for (auto& fixupConfig : fixups->as_array())
                    {
                        auto& fixup = loaded_fixups.emplace_back();

                        auto path = PackageRootPath() / fixupConfig.as_object().get("dll").as_string().wide();
#if _DEBUG
                        Log("\tfixup to attempt to load as specified: %ls.", path.c_str());
#endif
                        fixup.module_handle = ::LoadLibraryW(path.c_str());
                        if (!fixup.module_handle)
                        {
                            path.replace_extension();
                            path.concat((sizeof(void*) == 4) ? L"32.dll" : L"64.dll");
#if _DEBUG
                            Log("\tfixup to attempt to load as: %ls.", path.c_str());
#endif
                            fixup.module_handle = ::LoadLibraryW(path.c_str());

                            if (!fixup.module_handle)
                            {
#if _DEBUG
                                Log("\tfixup not found as specified,checkroot of package." );
#endif
                                std::filesystem::path pathfromroot = PackageRootPath() / path.filename().c_str();
                                fixup.module_handle = ::LoadLibraryW(pathfromroot.c_str());
                                if (fixup.module_handle)
                                {
#if _DEBUG
                                    Log("\tfixup found at . %ls", pathfromroot.c_str());
#endif                            
                                    path = pathfromroot;
                                }
#ifdef MOREDEBUG
                                else
                                {
                                    DWORD rember = GetLastError();
                                    DWORD att = ::GetFileAttributesW(pathfromroot.c_str());
                                    if (att != INVALID_FILE_ATTRIBUTES)
                                    {
                                        Log("\t???file %ls attrib 0x%x but load error 0x%x", pathfromroot.c_str(), att, rember);
                                    }
                                }
#endif
                            }

                            if (!fixup.module_handle)
                            {
#if _DEBUG
                                Log("\tfixup not found at root of package, look elsewhere.");
#endif
                                // just try to find it elsewhere as it isn't at the root
                                for (auto& dentry : std::filesystem::recursive_directory_iterator(PackageRootPath()))
                                {
                                    if (!fixup.module_handle)
                                    {
                                        try
                                        {
                                            if (dentry.path().filename().compare(path.filename().c_str()) == 0)
                                            {
#if _DEBUG
                                                Log("\tfixup might be found as %ls.", dentry.path().c_str());
#endif
                                                fixup.module_handle = ::LoadLibraryW(dentry.path().c_str());
                                                if (!fixup.module_handle)
                                                {
                                                    auto d2 = dentry.path();
                                                    d2.replace_extension();
                                                    d2.concat((sizeof(void*) == 4) ? L"32.dll" : L"64.dll");
                                                    fixup.module_handle = ::LoadLibraryW(d2.c_str());
                                                }
                                                if (fixup.module_handle)
                                                {
#if _DEBUG
                                                    Log("\tfixup found at . %ls", dentry.path().c_str());
#endif                            
                                                    path = dentry.path();
                                                    break;
                                                }
                                            }
                                        }
                                        catch (...)
                                        {
                                            Log("Non-fatal error enumerating directories while looking for fixup.");
                                        }
                                    }
                                    else
                                        break;
                                }
                            }


                            if (!fixup.module_handle)
                            {
                                if (GetLastError() == ERROR_NO_MORE_FILES)
                                {
                                    Log("\tERROR: fixup not found in package; ignoring.");
                                }
                                else
                                {
                                    auto message = narrow(path.c_str());
                                    throw_last_error(message.c_str());
                                }
                            }
                        }
                        if (fixup.module_handle)
                        {
                            Log("\tInjected into current process: %ls\n", path.c_str());

                            auto initialize = reinterpret_cast<PSFInitializeProc>(::GetProcAddress(fixup.module_handle, "PSFInitialize"));
                            if (!initialize)
                            {
                                auto message = "PSFInitialize export not found in "s + narrow(path.c_str());
                                throw_win32(ERROR_PROC_NOT_FOUND, message.c_str());
                            }
                            auto uninitialize = reinterpret_cast<PSFUninitializeProc>(::GetProcAddress(fixup.module_handle, "PSFUninitialize"));
                            if (!uninitialize)
                            {
                                auto message = "PSFUninitialize export not found in "s + narrow(path.c_str());
                                throw_win32(ERROR_PROC_NOT_FOUND, message.c_str());
                            }

                            auto transaction = detours::transaction();
                            check_win32(::DetourUpdateThread(::GetCurrentThread()));

                            // Only set the uninitialize pointer if the transaction commits successfully since that's our cue to clean
                            // it up, which will attempt to call DetourDetach
                            check_win32(initialize());
                            transaction.commit();
                            fixup.uninitialize = uninitialize;
                        }
                    }
                }
            }
        }
    }
}

void unload_fixups()
{
    while (!loaded_fixups.empty())
    {
        auto transaction = detours::transaction();
        check_win32(::DetourUpdateThread(::GetCurrentThread()));

        if (loaded_fixups.back().uninitialize)
        {
            [[maybe_unused]] auto result = loaded_fixups.back().uninitialize();
            assert(result == ERROR_SUCCESS);
        }

        transaction.commit();
        loaded_fixups.pop_back();
    }
}

using EntryPoint_t = int(__stdcall*)();
EntryPoint_t ApplicationEntryPoint = nullptr;
static int __stdcall FixupEntryPoint() noexcept try
{
#if _DEBUG
    Log("PsfRuntime FixupEntryPoint in App Pid=%d Tid=%d", GetCurrentProcessId(), GetCurrentThreadId());
#endif
    load_fixups();
    return ApplicationEntryPoint();
}
catch (...)
{
    int err = win32_from_caught_exception();
    Log("Exception in PsfRuntime FixupEntryPoint() = 0x%x", err);
    return  err;
}

void attach()
{
    try
    {
#if _DEBUG
        Log("PsfRuntime Attach Pid=%d Tid=%d", GetCurrentProcessId(), GetCurrentThreadId());
#endif
        usingPsf = LoadConfig();
#if _DEBUG
        Log("DEBUG: PsfRuntime after load config 0x%x",usingPsf);
#endif
        if (usingPsf)
        {
            // Restore the contents of the in memory import table that DetourCreateProcessWithDll* modified
            ::DetourRestoreAfterWith();

            auto transaction = detours::transaction();
            check_win32(::DetourUpdateThread(::GetCurrentThread()));

#if _DEBUG
            //Log("Debug: PsfRuntime before attach all");
#endif
    // Call DetourAttach for all APIs that PsfRuntime detours
            psf::attach_all();
#if _DEBUG
            //Log("DEBUG: PsfRuntime after attach all");
#endif
    // We can't call LoadLibrary in DllMain, so hook the application's entry point and do initialization then
            ApplicationEntryPoint = reinterpret_cast<EntryPoint_t>(::DetourGetEntryPoint(nullptr));
            if (!ApplicationEntryPoint)
            {
                throw_last_error();
            }
            check_win32(::DetourAttach(reinterpret_cast<void**>(&ApplicationEntryPoint), FixupEntryPoint));

            transaction.commit();
#if _DEBUG
            //Log("Debug: PsfRuntime is ready.");
#endif
        }
    }
    catch (...)
    {
        Log("App is not running inside the container, but will be allowed to run.");
    }
}

void detach()
{
    try
    {
        if (usingPsf)
        {
            //Log("DEBUG: PsfRuntime Dettach Pid=%d",GetCurrentProcessId());
            // Unload in the reverse order as we initialized
            unload_fixups();

            auto transaction = detours::transaction();
            check_win32(::DetourUpdateThread(::GetCurrentThread()));
            psf::detach_all();
            transaction.commit();
        }
    }
    catch (...)
    {
        Log("App is not running inside the container, but will be allowed to exit without error.");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) noexcept try
{

#if BLOCKDEBUGOUTPUTTEST
    HANDLE Handle = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        GetCurrentProcessId() /* This is the PID, you can find one from windows task manager */
    );
    if (Handle)
    {
        WCHAR Buffer[MAX_PATH];
        if (::GetModuleFileNameEx(Handle, 0, Buffer, MAX_PATH))
        {
            std::wstring xxx = Buffer;
            if (xxx.find(L"rsession.exe") != std::wstring::npos)
            {
                g_psf_NoLogging = true;
                //printf("testing...");
                //psf::wait_for_debugger();
                // DEBUG_EVENT evt;
                // WaitForDebugEventEx(&evt, 0);
            }
        }
        else
        {
            // You better call GetLastError() here
        }
        CloseHandle(Handle);
    }
#endif

#if _DEBUG
    Log("PsfRuntime: In DllMain Pid=%d Tid=%d", GetCurrentProcessId(), GetCurrentThreadId());
#endif
    // Per detours documentation, immediately return true if running in a helper process
    if (::DetourIsHelperProcess())
    {
        Log("PsfRuntime: Is Helper Process");
        return TRUE;
    }

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        try
        {
            // Record the location for ourselves in case we need to inject it into another new process (CreateProcessHook.cpp).
            if (::GetModuleFileName(hModule, g_PsfRunTimeModulePath, MAX_PATH) == 0)
            {
                g_PsfRunTimeModulePath[0] = 0x0;
            }


            attach();
        }
        catch (...)
        {
            Log("PsfRuntime: Exception attaching");
            unload_fixups();
            throw;
        }
        break;

    case DLL_PROCESS_DETACH:
        detach();
        break;
    case DLL_THREAD_ATTACH:
        //Log("PsfRuntime: Reason Thread Attach Tid=0x%x", GetCurrentThreadId());
        break;
    case DLL_THREAD_DETACH:
        //Log("PsfRuntime: Reason Thread Detach  Tid=0x%x", GetCurrentThreadId()); 
        break;
    default:
        Log("PsfRuntime: Reason %d", reason);
        break;
    }

    return TRUE;
}
catch (...)
{
    Log("PsfRuntime: Exception in dllMain");
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    ::SetLastError(win32_from_caught_exception());
    return false;
}
