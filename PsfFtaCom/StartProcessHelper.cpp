#include "StartProcessHelper.h"
#include <vector>


HRESULT StartProcess(LPCWSTR applicationName, LPWSTR commandLine, LPCWSTR currentDirectory, int cmdShow, DWORD timeout, bool inheritHandles, DWORD initialCreationFlags, LPPROC_THREAD_ATTRIBUTE_LIST attributeList, bool terminateChidren)
{
    STARTUPINFOEXW startupInfoEx = {};
    startupInfoEx.StartupInfo.cb = sizeof(startupInfoEx);
    startupInfoEx.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfoEx.StartupInfo.wShowWindow = static_cast<WORD>(cmdShow);

    PROCESS_INFORMATION processInfo{};

    startupInfoEx.lpAttributeList = attributeList;
    DWORD CreationFlags = initialCreationFlags;
    if (attributeList != nullptr)
    {
        CreationFlags = EXTENDED_STARTUPINFO_PRESENT;
    }

    std::wstring cmdLineMutable;
    if (commandLine && *commandLine)
    {
        cmdLineMutable.assign(commandLine);
    }
    else if (applicationName)
    {
        cmdLineMutable = QuoteIfNeeded(applicationName);
    }
    LPWSTR cmdLinePtr = cmdLineMutable.empty() ? nullptr : &cmdLineMutable[0];

    BOOL bErr = ::CreateProcessW(
        applicationName,
        cmdLinePtr,
        nullptr, nullptr,
        inheritHandles,
        CreationFlags,
        nullptr,
        currentDirectory,
        (LPSTARTUPINFO)&startupInfoEx,
        &processInfo);
    if (bErr == FALSE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), processInfo.hProcess == INVALID_HANDLE_VALUE);
    DWORD parentPID = processInfo.dwProcessId;
    DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, timeout);
    RETURN_LAST_ERROR_IF_MSG(waitResult != WAIT_OBJECT_0, "Waiting operation failed unexpectedly.");

    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (terminateChidren)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            if (Process32First(hSnapshot, &pe32))
            {
                do
                {
                    if (pe32.th32ParentProcessID == parentPID)
                    {
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                        if (hProcess != NULL)
                        {
                            TerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);
                        }
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
    }

    return S_OK;
}

HRESULT StartProcessAsUser(HANDLE hUserToken, LPCWSTR applicationName, LPWSTR commandLine, LPCWSTR currentDirectory, int cmdShow, DWORD timeout, bool inheritHandles, DWORD initialCreationFlags, LPPROC_THREAD_ATTRIBUTE_LIST attributeList, bool terminateChidren)
{
    STARTUPINFOEXW startupInfoEx = {};
    startupInfoEx.StartupInfo.cb = sizeof(startupInfoEx);
    startupInfoEx.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfoEx.StartupInfo.wShowWindow = static_cast<WORD>(cmdShow);

    PROCESS_INFORMATION processInfo{};
    startupInfoEx.lpAttributeList = attributeList;
    DWORD CreationFlags = initialCreationFlags;
    if (attributeList != nullptr)
    {
        CreationFlags = EXTENDED_STARTUPINFO_PRESENT;
    }

    std::wstring cmdLineMutable;
    if (commandLine && *commandLine)
    {
        cmdLineMutable.assign(commandLine);
    }
    else if (applicationName)
    {
        cmdLineMutable = QuoteIfNeeded(applicationName);
    }
    LPWSTR cmdLinePtr = cmdLineMutable.empty() ? nullptr : &cmdLineMutable[0];

    BOOL bErr = ::CreateProcessAsUserW(
        hUserToken,
        applicationName,
        cmdLinePtr,
        nullptr, nullptr,
        inheritHandles,
        CreationFlags,
        nullptr,
        currentDirectory,
        (LPSTARTUPINFO)&startupInfoEx,
        &processInfo);
    if (bErr == FALSE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), processInfo.hProcess == INVALID_HANDLE_VALUE);
    DWORD parentPID = processInfo.dwProcessId;
    DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, timeout);
    RETURN_LAST_ERROR_IF_MSG(waitResult != WAIT_OBJECT_0, "Waiting operation failed unexpectedly.");

    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (terminateChidren)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            if (Process32First(hSnapshot, &pe32))
            {
                do
                {
                    if (pe32.th32ParentProcessID == parentPID)
                    {
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                        if (hProcess != NULL)
                        {
                            TerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);
                        }
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
    }

    return S_OK;
}

void StartWithShellExecute(LPCWSTR verb, std::filesystem::path packageRoot, std::filesystem::path exeName, std::wstring exeArgString, LPCWSTR dirStr, int cmdShow, DWORD timeout)
{
    auto nonExePath = packageRoot / exeName;

    SHELLEXECUTEINFO shex = {};
    shex.cbSize = sizeof(shex);
    shex.fMask = SEE_MASK_NOCLOSEPROCESS;
    shex.hwnd = nullptr;
    shex.lpVerb = verb;
    shex.lpFile = nonExePath.c_str();
    shex.lpParameters = exeArgString.c_str();
    shex.lpDirectory = dirStr ? (packageRoot / dirStr).c_str() : nullptr;
    shex.nShow = static_cast<int>(cmdShow);

    Log(LogLevel_Launching, "\tUsing Shell launch: %ls %ls", shex.lpFile, shex.lpParameters);
    THROW_LAST_ERROR_IF_MSG(!ShellExecuteEx(&shex), "ERROR: Failed to create detoured shell process");
    THROW_LAST_ERROR_IF(shex.hProcess == INVALID_HANDLE_VALUE);
    DWORD exitCode = ::WaitForSingleObject(shex.hProcess, timeout);
    Log(LogLevel_Launching, "PsfFtaCom: Shell Launch: process returned exit code 0x%x", exitCode);
    CloseHandle(shex.hProcess);
}