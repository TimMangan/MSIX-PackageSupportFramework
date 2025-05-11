//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// PsfFtaCom.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>

#include <windows.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <ppltasks.h>
#include <ShObjIdl.h>
#include "StartProcessHelper.h"
#include "PsfPowershellScriptRunner.h"
#include "Globals.h"

#include <psf_constants.h>
#include <psf_runtime.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <debug.h>
#include <shlwapi.h>
#include <WinUser.h>
#include <proc_helper.h>
#include <psf_logging.h>

#include <TlHelp32.h>

using namespace std::literals;

const wchar_t* PsfFtaComName = L"c";

// Forward declarations
extern void LogApplicationAndProcessesCollection();
extern bool IsCurrentOSRS2OrGreater();
extern std::wstring ReplaceMisleadingSlashVFS(std::wstring inputString);
extern std::wstring ReplaceVariablesInString(std::wstring inputString, bool ReplaceEnvironmentVars, bool ReplacePseudoVars);
extern bool IsProcessRunningForThisUser(const std::filesystem::path path);

int launcher_main(PCWSTR wargs, int cmdShow) noexcept try
{
    Log(L"[%s%d] PsfFtaCom started.", PsfFtaComName, 0);


    //Log(L"[%s%d]DEBUG TEMP PsfFtaCom waiting for debugger to attach to process...\n", PsfFtaComName, 0);
    //psf::wait_for_debugger();

    auto appConfig = PSFQueryCurrentAppLaunchConfig(true);
    THROW_HR_IF_MSG(ERROR_NOT_FOUND, !appConfig, "Error: could not find matching appid in config.json and appx manifest");


#ifdef _DEBUG 
    if (appConfig)
    {
        auto waitSignalPtr = appConfig->try_get("waitForDebugger");
        if (waitSignalPtr)
        {
            bool waitSignal = waitSignalPtr->as_boolean().get();
            if (waitSignal)
            {
                Log(L"[%s%d] PsfFtaCom waiting for debugger to attach to process...\n", PsfFtaComName, 0);
                psf::wait_for_debugger();
            }
        }
    }
#endif

    LogApplicationAndProcessesCollection();

    // Determine process and command line arguments from what was passed in.
    std::wstring targetFilePath;
    std::wstring targetArgs;
    if (wargs != NULL)
    {
        std::wstring temp;
        std::vector<std::wstring> parts;
        std::wstringstream wss(wargs);
        LogString(PsfFtaComName, 0, L"Input arguments", wargs);

        while (std::getline(wss, temp, L'\"'))
            parts.push_back(temp);
        if (parts.size() >= 3)  // 0="\"" 1=command\" 2 and above are rest of the arguments
        {
            targetFilePath = parts[0] + parts[1];
            for (int inx=2; inx <(int)parts.size(); inx++)
            {
                if (inx == 2)
                    targetArgs += parts[inx];
                else
                    targetArgs += L" \"" + parts[inx] + L"\"";  // restore these quotes as they might have been around a file path that needs them
            }
            targetFilePath = ReplaceVariablesInString(targetFilePath, true, true);
            targetArgs = ReplaceVariablesInString(targetArgs, true, true);
            LogString(PsfFtaComName, 0, L"TargetFilePath", targetFilePath.c_str());
            LogString(PsfFtaComName, 0, L"TargetArgs", targetArgs.c_str());
        }
        else if (parts.size() == 2)
        {
            targetFilePath = parts[0] + parts[1];
            targetArgs = L"";
            LogString(PsfFtaComName, 0, L"TargetFilePath", targetFilePath.c_str());
            LogString(PsfFtaComName, 0, L"TargetArgs", L"***none***");
        }
        else
        {
            Log(L"[%s%d] Error: Invalid command line arguments passed to PsfFtaCom.", PsfFtaComName, 0);
            Log(L"[%s%d]\tNumber of parts=%d", PsfFtaComName, 0, parts.size());
            return -1;
        }
    }
    else
    {
        Log(L"[%s%d] Error: No command line arguments passed to PsfFtaCom.", PsfFtaComName, 0);
        return -1;
    }

    // Determine currentdirectory for the new process
    const wchar_t* dirStr = L"";
    auto dirPtr = appConfig->try_get("workingDirectory");
    if (dirPtr != NULL)
        dirStr = dirPtr->as_string().wide();
    else
        dirStr =  L"";

    // At least for now, configured launch paths are relative to the package root
    std::filesystem::path packageRoot = PSFQueryPackageRootPath();
    std::wstring dirWstr = dirStr;
    dirWstr = ReplaceMisleadingSlashVFS(dirWstr);
    dirWstr = ReplaceVariablesInString(dirWstr, true, true);
    std::filesystem::path currentDirectory;

    if (dirWstr.size() < 2 || dirWstr[1] != L':')
    {
        if (dirWstr.size() == 0)
        {

            currentDirectory = packageRoot / targetFilePath.substr(0,targetFilePath.find_last_of('\\'));
        }
        else
        {
            currentDirectory = (packageRoot / dirWstr);
        }
    }
    else
    {
        currentDirectory = dirWstr;
    }

    if (targetFilePath._Starts_with(L"VFS\\"))
    {
        targetFilePath = packageRoot / targetFilePath;
    }

    bool preventMultiple = false;
    auto preventMultipleObject = appConfig->try_get("preventMultipleInstances");
    if (preventMultipleObject)
    {
        preventMultiple = preventMultipleObject->as_boolean().get();
    }

    if (preventMultiple)
    {
        Log(L"[%s%d] Checking for existing instances of %ls", PsfFtaComName, 0, targetFilePath.c_str());
        if (IsProcessRunningForThisUser(targetFilePath.c_str()))
        {
            Log(L"[%s%d] Existing instance found, prompting user and exiting.", PsfFtaComName, 0);
            MessageBox(NULL, L"An instance of this application is already running.", L"Multiple Instances Not Allowed", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        Log(L"[%s%d] No existing instance found, continuing.", PsfFtaComName, 0);
    }



    LogString(PsfFtaComName, 0, L"TargetFilePath", targetFilePath.c_str());
    LogString(PsfFtaComName, 0, L"TargetArgs", targetArgs.c_str());
    std::wstring quotedFullLine = L"\"" + targetFilePath + L"\" " + targetArgs.c_str();
    HRESULT hr = StartProcess(targetFilePath.c_str(), quotedFullLine.data(), currentDirectory.c_str(), cmdShow, INFINITE, true, 0, NULL);
    if (hr != ERROR_SUCCESS)
    {
        Log(L"[%s%d] Error return from launching process second try, try again 0x%x.", PsfFtaComName, 0, GetLastError());
    }


    return 0;
}
catch (...)
{
    ::PSFReportError(widen(message_from_caught_exception()).c_str());
    return win32_from_caught_exception();
}  // launcher_main()


int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR args, _In_ int cmdShow)
{
    int ret = launcher_main(args, cmdShow);

    return ret;
}  // wWinMain()



/// ///////////////////////////////////////////////////////
/// ///// REGION: UTIITIES
/// ///////////////////////////////////////////////////////


// Determine if the named process is already running for the current user.
bool IsProcessRunningForThisUser(const std::filesystem::path path)
{
    bool isRunning = false;
    std::wstring procName = path;
    procName = procName.substr(procName.find_last_of(L"\\") + 1);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    size_t num = 0;
    wchar_t* thisUserName;
    errno_t result = _wdupenv_s(&thisUserName, &num, L"USERNAME");
    if (result == ENOMEM)
        return false; // should never happen


    if (Process32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, procName.c_str()) == 0) {
                bool sameUser = false;

                // TODO: Use the entry.th32ProcessID to do this somehow.
                HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
                if (!processHandle) {
                    // We can't open other user handles (unless we are elevated), so assume it is another user.
                    continue;
                }
                else
                {
                    HANDLE tokenHandle;
                    if (OpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle))
                    {
                        DWORD tokenUserSize = 0;
                        GetTokenInformation(tokenHandle, TokenUser, NULL, 0, &tokenUserSize);
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            std::vector<BYTE> tokenUserBuffer(tokenUserSize);
                            if (GetTokenInformation(tokenHandle, TokenUser, tokenUserBuffer.data(), tokenUserSize, &tokenUserSize))
                            {
                                TOKEN_USER* tokenUser = reinterpret_cast<TOKEN_USER*>(tokenUserBuffer.data());
                                DWORD userNameSize = 0;
                                DWORD domainNameSize = 0;
                                SID_NAME_USE sidNameUse;
                                LookupAccountSid(NULL, tokenUser->User.Sid, NULL, &userNameSize, NULL, &domainNameSize, &sidNameUse);
                                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                                {
                                    std::vector<wchar_t> userNameBuffer(userNameSize);
                                    std::vector<wchar_t> domainNameBuffer(domainNameSize);
                                    if (LookupAccountSid(NULL, tokenUser->User.Sid, userNameBuffer.data(), &userNameSize, domainNameBuffer.data(), &domainNameSize, &sidNameUse))
                                    {
                                        std::wstring userName = userNameBuffer.data();
                                        //std::wstring domainName = domainNameBuffer.data();  // Let's not worry about the domain.                                        
                                        if (userName._Equal(thisUserName))
                                        {
                                            sameUser = true;
                                        }
                                    }
                                }
                            }
                        }
                        CloseHandle(tokenHandle);
                    }
                    CloseHandle(processHandle);
                }

                if (sameUser)
                {
                    CloseHandle(snapshot);
                    return true;
                }
            }
        } while (Process32Next(snapshot, &entry));
    }

    free(thisUserName);
    CloseHandle(snapshot);

    return isRunning;
} // IsProcessRunningForThisUser()



void LogApplicationAndProcessesCollection()
{
    auto configRoot = PSFQueryConfigRoot();
    const wchar_t* exeStr = NULL;
    const wchar_t* idStr = NULL;
    const wchar_t* hasShellVerbsStr = NULL;
    if (auto applications = configRoot->as_object().try_get("applications"))
    {
        for (auto& applicationsConfig : applications->as_array())
        {
            try { 
                auto exeObj = applicationsConfig.as_object().try_get("executable"); 
                if (exeObj != NULL)
                    exeStr = exeObj->as_string().wide();
            }
            catch (...) {; }  // These are now optional, and you can't widen a NULL.
            try 
            {
                auto idObj = applicationsConfig.as_object().try_get("id");
                if (idObj != NULL)
                    idStr = idObj->as_string().wide();
            }
            catch (...) {}
            try 
            {
                auto hasShellVerbsObj = applicationsConfig.as_object().try_get("shellVerbs");
                if (hasShellVerbsObj != NULL)
                    hasShellVerbsStr = hasShellVerbsObj->as_string().wide();
            }
            catch (...) {}


            if (exeStr != NULL)
                LogString(PsfFtaComName, 0, L"executable", exeStr);
            if (idStr != NULL)
                LogString(PsfFtaComName, 0, L"id", idStr);
            if (hasShellVerbsStr != NULL)
                LogString(PsfFtaComName, 0, L"shellVerbs", hasShellVerbsStr);
        }
    }

#if _DEBUG
    if (auto processes = configRoot->as_object().try_get("processes"))
    {
        for (auto& processConfig : processes->as_array())
        {
            exeStr = processConfig.as_object().get("executable").as_string().wide();

            if (auto fixups = processConfig.as_object().try_get("fixups"))
            {
                for (auto& fixupConfig : fixups->as_array())
                {
                    [[maybe_unused]] auto dllStr = fixupConfig.as_object().try_get("dll")->as_string().wide();
                }
            }
        }
    }
#endif

}  // LogApplicationAndProcessesCollection()

bool IsCurrentOSRS2OrGreater()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    DWORDLONG const dwlConditionMask = VerSetConditionMask(0, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    osvi.dwBuildNumber = 15063;

    return VerifyVersionInfoW(&osvi, VER_BUILDNUMBER, dwlConditionMask);
} // IsCurrentOSRS2OrGreater()


// Drop the mistaken first slash in \VFS.
// Sometimes people assume they need to reference the relative path starting with a forward slash,
// If they do it with a VFS folder, the mistake is obvious and we can adjust it for them.
std::wstring ReplaceMisleadingSlashVFS(std::wstring inputString)
{
    if (inputString._Starts_with(L"\\VFS"))
    {
        return inputString.substr(1);
    }
    return inputString;
}

// Replace all occurrences of requested environment and/or pseudo-environment variables in a string.
std::wstring ReplaceVariablesInString(std::wstring inputString, bool ReplaceEnvironmentVars, bool ReplacePseudoVars)
{
    std::wstring outputString = inputString;
    if (ReplacePseudoVars)
    {
        std::wstring::size_type pos = 0u;
        std::wstring var2rep = L"%MsixPackageRoot%";
        std::wstring repargs = PSFQueryPackageRootPath();
        while ((pos = outputString.find(var2rep, pos)) != std::string::npos) {
            outputString.replace(pos, var2rep.length(), repargs);
            pos += repargs.length();
        }

        pos = 0u;
        var2rep = L"%MsixWritablePackageRoot%";
        std::filesystem::path writablePackageRootPath = psf::known_folder(FOLDERID_LocalAppData) / std::filesystem::path(L"Packages") / psf::current_package_family_name() / LR"(LocalCache\Local\Microsoft\WritablePackageRoot)";
        repargs = writablePackageRootPath.c_str();
        while ((pos = outputString.find(var2rep, pos)) != std::string::npos) {
            outputString.replace(pos, var2rep.length(), repargs);
            pos += repargs.length();
        }
    }
    if (ReplaceEnvironmentVars)
    {
        // Potentially an environment variable that needs replacing. For Example: "%HomeDir%\\Documents"
        DWORD nSizeBuff = 256;
        LPWSTR buff = new wchar_t[nSizeBuff];
        DWORD nSizeRet = ExpandEnvironmentStrings(outputString.c_str(), buff, nSizeBuff);
        if (nSizeRet > 0)
        {
            outputString = std::wstring(buff);
        }

    }
    return outputString;
}


static inline bool check_suffix_if(iwstring_view str, iwstring_view suffix) noexcept
{
    return ((str.length() >= suffix.length()) && (str.substr(str.length() - suffix.length()) == suffix));
}

