//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "MfrCleanup.h"
#include "MfrConsts.h"
#include <Windows.h>
#include <stdio.h>
#include <wil\resource.h>
#include <wil\result_macros.h>

// Define _PROC_THREAD_ATTRIBUTE_LIST as an empty struct because it's internal-only and variable-sized
struct _PROC_THREAD_ATTRIBUTE_LIST {};

/// <summary>
/// Utility to clean up the redirection area(s) used by these tests.
/// It is really nice to do this without adding to the debug logging, so it will be an external process.
/// </summary>

DWORD RunThisCommandOutsideContainer(std::wstring commandArguments);

DWORD MfrCleanupWritablePackageRoot()
{
    // Doing this upsets the package when ILV is in use.
    //std::wstring directoryPath = g_writablePackageRootPath.c_str();
    //
    //std::wstring arguments = L"/C rmdir /S /Q \"";
    //arguments.append(directoryPath.c_str());
    //arguments.append(L"\" >nul  2>&1"); // hide not errors like not found
    //return RunThisCommandOutsideContainer(arguments);

    // Can't do this, as package is in use!
    //std::wstring pscmd = L"reset-appxpackage -package " + g_PackageFamilyName;
    //std::wstring cmd = L"/C powershell.exe  -Command \"" + pscmd + L"\"";
    //DWORD ret =  RunThisCommandOutsideContainer(cmd);
    //if (ret != 0)
    //{
    //    trace_message(L"Issue resetting the package.", error_color);
    //}
    //return ret;

    // So we'll just skip this step and adjust the testing to not require cleanups.
    return ERROR_SUCCESS;
}

DWORD MfrCleanupLocalDocuments(std::wstring subfoldername)
{
    std::wstring directoryPath = psf::known_folder(FOLDERID_Documents);
    directoryPath.append(L"\\");
    directoryPath.append(subfoldername.c_str());

    std::wstring arguments = L"/C rmdir /S /Q \"";
    arguments.append(directoryPath.c_str());
    arguments.append(L"\" >nul  2>&1");  // hide not errors like not found
    return RunThisCommandOutsideContainer(arguments);
}


DWORD RunThisCommandOutsideContainer(std::wstring commandArguments)
{
    std::wstring cmd = L"C:\\Windows\\System32\\cmd.exe";

    std::wstring commandLine = cmd.c_str();
    commandLine.append(L" ");
    commandLine.append(commandArguments.c_str());

    DWORD bytes = (DWORD)(512 * sizeof(wchar_t));
    wchar_t* CL = (wchar_t*)malloc(bytes);
    if (CL)
    {
        ZeroMemory(CL, bytes);
        memcpy_s(CL, bytes, commandLine.c_str(), (DWORD)(commandLine.length()*sizeof(wchar_t)));

        std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST> attributeList;
        
        SIZE_T AttributeListSize; //{};
	    InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);
        attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(new char[AttributeListSize]));
        THROW_LAST_ERROR_IF_MSG(
                !InitializeProcThreadAttributeList(
                    attributeList.get(),
                    1,
                    0,
                    &AttributeListSize),
                "Could not initialize the proc thread attribute list.");
        // 18 stands for
        // PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
        // this is the attribute value we want to add
        DWORD createOutsideContainerAttribute = 0x04;
        THROW_LAST_ERROR_IF_MSG(
					!UpdateProcThreadAttribute(
						attributeList.get(),
						0,
						ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),
						&createOutsideContainerAttribute,
						sizeof(createOutsideContainerAttribute),
						nullptr,
						nullptr),
					"Could not update Proc thread attribute for DESKTOP_APP_POLICY (outside).");
        
        STARTUPINFOEXW startupInfoEx =
        {
            {
            sizeof(startupInfoEx)
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
            , static_cast<WORD>(0) // wShowWindow
            }
        };

        PROCESS_INFORMATION processInfo{};

        
        startupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST) attributeList.get();
        DWORD CreationFlags = 0;
        if (attributeList != nullptr)
        {
            CreationFlags = EXTENDED_STARTUPINFO_PRESENT;
        }

        BOOL bErr = ::CreateProcessW(
            cmd.c_str(),
            CL,
            nullptr, nullptr, // Process/ThreadAttributes
            false, // InheritHandles
            CreationFlags,
            nullptr, // Environment
            nullptr, //currentDirectory,
            (LPSTARTUPINFO)&startupInfoEx,
            &processInfo);
        if (bErr == FALSE)
        {
            DeleteProcThreadAttributeList(attributeList.get());
            return GetLastError();
        }

        RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE), processInfo.hProcess == INVALID_HANDLE_VALUE);
        DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, INFINITE);
        RETURN_LAST_ERROR_IF_MSG(waitResult != WAIT_OBJECT_0, "Waiting operation failed unexpectedly.");

        DWORD exitCode;
        GetExitCodeProcess(processInfo.hProcess, &exitCode);

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        free(CL);
        DeleteProcThreadAttributeList(attributeList.get());
        
        return ERROR_SUCCESS;  // Some apps return codes even when happy.
    }
    return 0xFFFFFFFF;
}