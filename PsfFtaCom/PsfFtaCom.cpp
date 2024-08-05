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

using namespace std::literals;

// Forward declarations
extern void LogApplicationAndProcessesCollection();
extern bool IsCurrentOSRS2OrGreater();
extern std::wstring ReplaceMisleadingSlashVFS(std::wstring inputString);
extern std::wstring ReplaceVariablesInString(std::wstring inputString, bool ReplaceEnvironmentVars, bool ReplacePseudoVars);

int launcher_main(PCWSTR wargs, int cmdShow) noexcept try
{
    Log(L"PsfFtaCom started.");


    //Log(L"DEBUG TEMP PsfFtaCom waiting for debugger to attach to process...\n");
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
                Log(L"PsfFtaCom waiting for debugger to attach to process...\n");
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
        LogString(L"Input arguments", wargs);

        while (std::getline(wss, temp, L'\"'))
            parts.push_back(temp);
        if (parts.size() >= 3)  // 0="\"" 1=command\" 2 and above are rest of the arguments
        {
            targetFilePath = parts[0] + parts[1];
            for (int inx=2; inx <(int)parts.size(); inx++)
            {
                targetArgs += parts[inx];
            }
            targetFilePath = ReplaceVariablesInString(targetFilePath, true, true);
            targetArgs = ReplaceVariablesInString(targetArgs, true, true);
            LogString(L"TargetFilePath", targetFilePath.c_str());
            LogString(L"TargetArgs", targetArgs.c_str());
        }
        else if (parts.size() == 2)
        {
            targetFilePath = parts[0] + parts[1];
            targetArgs = L"";
            LogString(L"TargetFilePath", targetFilePath.c_str());
            LogString(L"TargetArgs", L"***none***");
        }
        else
        {
            Log(L"Error: Invalid command line arguments passed to PsfFtaCom.");
            Log(L"       Number of parts=%d", parts.size());
            return -1;
        }
    }
    else
    {
        Log(L"Error: No command line arguments passed to PsfFtaCom.");
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

    LogString(L"TargetFilePath", targetFilePath.c_str());
    LogString(L"TargetArgs", targetArgs.c_str());
    std::wstring quotedFullLine = L"\"" + targetFilePath + L"\" " + targetArgs.c_str();
    HRESULT hr = StartProcess(targetFilePath.c_str(), quotedFullLine.data(), currentDirectory.c_str(), cmdShow, INFINITE, true, 0, NULL);
    if (hr != ERROR_SUCCESS)
    {
        Log(L"Error return from launching process second try, try again 0x%x.", GetLastError());
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
                LogString(L"executable", exeStr);
            if (idStr != NULL)
                LogString(L"id", idStr);
            if (hasShellVerbsStr != NULL)
                LogString(L"shellVerbs", hasShellVerbsStr);
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

