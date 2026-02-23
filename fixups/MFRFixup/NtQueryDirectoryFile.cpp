//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwquerydirectoryfile


#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include <psf_logging.h>
#include "FunctionImplementations.h"
#include "FunctionImplementations_ntdll.h"
#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

#if Intercept_NTDLL


#ifdef DO_Intercept_NtQueryDirectoryFile

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:Ntdll_NtQueryDirectoryFileFixup_Fixup=_NtDll_NtQueryDirectoryFileFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#else
#pragma comment(linker, "/EXPORT:Ntdll_NtQueryDirectoryFileFixup_Fixup=NtDll_NtQueryDirectoryFileFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#endif


NTSTATUS Test_TripplePlayAlternative(NTSTATUS retfinalIn, std::wstring wThisPathName, std::wstring casetype, DWORD dllInstance,
                                    HANDLE                 Event,
                                    PIO_APC_ROUTINE        ApcRoutine,
                                    PVOID                  ApcContext,
                                    PIO_STATUS_BLOCK       IoStatusBlock,
                                    PVOID                  FileInformation,
                                    ULONG                  Length,
                                    FILE_INFORMATION_CLASS FileInformationClass,
                                    BOOLEAN                ReturnSingleEntry,
                                    PUNICODE_STRING        FileName,
                                    BOOLEAN                RestartScan)
{
    NTSTATUS retfinal = retfinalIn;
    try
    {
        if (!wThisPathName._Starts_with(L"\\\\?\\") &&
            !wThisPathName._Starts_with(L"\\\\.\\"))
        {
            wThisPathName = L"\\\\?\\" + wThisPathName;
        }
        if (!wThisPathName.empty() && wThisPathName.back() != L'\\')
        {
            wThisPathName = wThisPathName + L"\\";
        }
        HANDLE thisHandle = CreateFile(wThisPathName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (thisHandle != INVALID_HANDLE_VALUE)
        {
            retfinal = ntdllimpl::NtQueryDirectoryFileImpl(thisHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
            Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup %s alternative %s for %s return status=0x%x", g_MfrModuleName, dllInstance, casetype.c_str(), wThisPathName.c_str(), FileName->Buffer, retfinal);
            CloseHandle(thisHandle);
        } 
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup %s alternative %s failed to open handle.", g_MfrModuleName, dllInstance, casetype.c_str(), wThisPathName.c_str());
        }
    }
    catch (...)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup %s alternative exception.", g_MfrModuleName, dllInstance, casetype.c_str());
    }
    return retfinal;
}

NTSTATUS TripplePlay_NtQueryDirectoryFileImpl(
    IN             HANDLE                 FileHandle,
    IN OPTIONAL    HANDLE                 Event,
    IN OPTIONAL    PIO_APC_ROUTINE        ApcRoutine,
    IN OPTIONAL    PVOID                  ApcContext,
    OUT            PIO_STATUS_BLOCK       IoStatusBlock,
    OUT            PVOID                  FileInformation,
    IN             ULONG                  Length,
    IN             FILE_INFORMATION_CLASS FileInformationClass,
    IN             BOOLEAN                ReturnSingleEntry,
    IN OPTIONAL    PUNICODE_STRING        FileName,
    IN             BOOLEAN                RestartScan)
{
    NTSTATUS retfinal;
    DWORD dllInstance = g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    std::wstring DirPathUsed;


    // The call into here provides an open handle to a directory to search under.
    // We need to determine the filepath of that directory, so that we can also search it's cohorts.
    wchar_t filePath[1024];
    [[maybe_unused]] DWORD filePathLength = GetFinalPathNameByHandle(FileHandle, filePath, MAX_PATH, FILE_NAME_NORMALIZED);
    DirPathUsed = filePath;
    retfinal = ntdllimpl::NtQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup as requested %s returned status=0x%x", g_MfrModuleName, dllInstance, filePath, retfinal);

    //#define STATUS_BUFFER_OVERFLOW            ((DWORD   )0x80000005L)    // Documented
    //#define STATUS_NO_MORE_FILES              ((DWORD   )0x80000006L)    // Seen
    //#define STATUS_INVALID_INFO_CLASS         ((DWORD   )0xC0000003L)    // Documented
    //#define STATUS_INVALID_PARAMETER          ((DWORD   )0xC000000DL)    // Documented
    //#define STATUS_NO_SUCH_FILE               ((DWORD   )0xC000000FL)    // Seen
    //#define STATUS_BUFFER_TOO_SMALL           ((DWORD   )0xC0000023L)    // Documented
    if (retfinal == 0xC000000FL)
    {
        // We did not find it in the requested area, so we need to try the other areas.  Other results we just return back without looking elsewhere.
        // If we try to handle requests for more than one result that has to return here, we will need to consider no more files scenario also.
        Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup may need alternative forms checked.", g_MfrModuleName, dllInstance);


        // Make adjustments to the found path to more normalize it.  
        // This is probably not needed because it didn't come from the application directly, but we do this in FindFiles and it can't hurt.
        std::wstring wfilePath = AdjustSlashes(filePath, dllInstance);
        wfilePath = AdjustBadUNC(wfilePath, dllInstance, L"NtDll_NtQueryDirectoryFileFixup");

        // Determine possible paths involved
        Cohorts cohorts;
        DetermineCohorts(LogLevel_DebugIntermediate, wfilePath, &cohorts, dllInstance, L"NtDll_NtQueryDirectoryFileFixup");

        // Adjust the cohorts based on the next level when it is a variablized name
        //CohortAdjustment(&cohorts, FileName->Buffer, false, dllInstance, L"NtDll_NtQueryDirectoryFileFixup");
        std::wstring ReplacementRedirectedPath = cohorts.WsRedirected;
        std::wstring ReplaementRedirectedFileName = FileName->Buffer;
        bool bReplacementRedirected = mfr::FindCohortVfsRemapping(cohorts.WsRedirected, FileName->Buffer, ReplacementRedirectedPath, ReplaementRedirectedFileName);

        std::wstring ReplacementPackagePath = cohorts.WsPackage;
        std::wstring ReplaementPackageFileName = FileName->Buffer;
        bool bReplacementPackage = mfr::FindCohortVfsRemapping(cohorts.WsPackage, FileName->Buffer, ReplacementPackagePath, ReplaementPackageFileName);

        if (bReplacementRedirected)
        {
            UNICODE_STRING uReplaementRedirectedFileName = { 0 };
            mfr::ToUnicodeString(ReplaementRedirectedFileName, uReplaementRedirectedFileName);
            retfinal = Test_TripplePlayAlternative(retfinal, ReplacementRedirectedPath, L"redirected", dllInstance,
                Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, &uReplaementRedirectedFileName, RestartScan);
            free(uReplaementRedirectedFileName.Buffer);
        }
        else
        {

            if (cohorts.WsRedirected.size() > 0 &&
                !cohorts.WsRedirected._Equal(cohorts.WsRequested))
            {
                DirPathUsed = cohorts.WsRedirected;
                retfinal = Test_TripplePlayAlternative(retfinal, cohorts.WsRedirected, L"redirected", dllInstance,
                    Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
            }
        }
        if (retfinal == 0xC000000FL)
        {
            if (bReplacementPackage)
            {
                UNICODE_STRING uReplaementPackageFileName = { 0 };
                mfr::ToUnicodeString(ReplaementPackageFileName, uReplaementPackageFileName);
                retfinal = Test_TripplePlayAlternative(retfinal, ReplacementPackagePath, L"package", dllInstance,
                    Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, &uReplaementPackageFileName, RestartScan);
                free(uReplaementPackageFileName.Buffer);
            }
            else
            {
                if (cohorts.WsPackage.size() > 0 &&
                    !cohorts.WsPackage._Equal(cohorts.WsRequested))
                {
                    DirPathUsed = cohorts.WsPackage;
                    retfinal = Test_TripplePlayAlternative(retfinal, cohorts.WsPackage, L"package", dllInstance,
                        Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
                }
            }
        }
        if (retfinal == 0xC000000FL &&
            cohorts.WsNative.size() > 0 &&
            !cohorts.WsNative._Equal(cohorts.WsRequested))
        {
            DirPathUsed = cohorts.WsNative;
            retfinal = Test_TripplePlayAlternative(retfinal, cohorts.WsNative, L"native", dllInstance,
                                                    Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
        }
    }

    // Possible additional logging

    try
    {
        // Show what we got back.  
        // The results don't seem to make sense when asking to return just one result, so maybe the effect is a yes/no based on the return code only.
        // See the NtfileQueryDirectoryFileEx for a list of FileInformationClass types and related structures. 
        auto wideData1 = reinterpret_cast<FILE_DIRECTORY_INFORMATION*>(FileInformation);
        auto wideData2 = reinterpret_cast<PFILE_FULL_DIRECTORY_INFORMATION>(FileInformation);
        auto wideData3 = reinterpret_cast<PFILE_BOTH_DIRECTORY_INFORMATION>(FileInformation);
        auto wideData12 = reinterpret_cast<PFILE_NAMES_INFORMATION>(FileInformation);
        auto wideData37 = reinterpret_cast<PFILE_ID_BOTH_DIR_INFORMATION>(FileInformation);
        auto wideData38 = reinterpret_cast<PFILE_ID_BOTH_DIR_INFORMATION>(FileInformation);
        int classAsInt = (int)FileInformationClass;
        switch (classAsInt)
        {
        case 1: //FILE_DIRECTORY_INFORMATION:
            while (wideData1 != NULL)
            {
                if (wideData1->FileNameLength > 0)
                {
                    Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", g_MfrModuleName, dllInstance, IoStatusBlock->Status, wideData1->FileAttributes, DirPathUsed.c_str(), wideData1->FileNameLength, (const wchar_t*)wideData1->FileName);
                }
                wideData1 = wideData1->NextEntryOffset == 0 ? NULL : reinterpret_cast<FILE_DIRECTORY_INFORMATION*>(reinterpret_cast<BYTE*>(wideData1) + wideData1->NextEntryOffset);
            }
            break;
        case 2: //FILE_FULL_DIRECTORY_INFORMATION:
            if (wideData2->FileNameLength > 0)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", g_MfrModuleName, dllInstance, IoStatusBlock->Status, wideData2->FileAttributes, DirPathUsed.c_str(), wideData2->FileNameLength, (const wchar_t*)wideData2->FileName);
            }
            wideData2 = wideData2->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_FULL_DIRECTORY_INFORMATION>(reinterpret_cast<BYTE*>(wideData2) + wideData2->NextEntryOffset);
            break;
        case 3: //FILE_BOTH_DIRECTORY_INFORMATION:
            if (wideData3->FileNameLength > 0)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", g_MfrModuleName, dllInstance, IoStatusBlock->Status, wideData3->FileAttributes, DirPathUsed.c_str(), wideData3->FileNameLength, (const wchar_t*)wideData3->FileName);
            }
            wideData3 = wideData3->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_BOTH_DIRECTORY_INFORMATION>(reinterpret_cast<BYTE*>(wideData3) + wideData3->NextEntryOffset);
            break;
        case 12: // FILE_NAMES_INFORMATION
            if (wideData12->FileNameLength > 0)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x FromDir=%s FileLen %d File %s", g_MfrModuleName, dllInstance, IoStatusBlock->Status, DirPathUsed.c_str(), wideData12->FileNameLength, (const wchar_t*)wideData12->FileName);
            }
            wideData12 = wideData12->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_NAMES_INFORMATION>(reinterpret_cast<BYTE*>(wideData12) + wideData12->NextEntryOffset);
            break;
        case 29: // FILE_OBJECT_ID:
            Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x", g_MfrModuleName, dllInstance, IoStatusBlock->Status);
            break;
        case 32: // FILE_QUOTA_INFORMATION:
            Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x", g_MfrModuleName, dllInstance, IoStatusBlock->Status);
            break;
        case 33: // FILE_REPARSE_POINT_INFORMATION:
            Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x", g_MfrModuleName, dllInstance, IoStatusBlock->Status);
            break;
        case 37: //FILE_ID_BOTH_DIR_INFORMATION:
            if (wideData37->FileNameLength > 0)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", g_MfrModuleName, dllInstance, IoStatusBlock->Status, wideData37->FileAttributes, DirPathUsed.c_str(), wideData37->FileNameLength, (const wchar_t*)wideData37->FileName);
            }
            wideData37 = wideData37->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_ID_BOTH_DIR_INFORMATION>(reinterpret_cast<BYTE*>(wideData37) + wideData37->NextEntryOffset);
            break; 
        case 38: //FILE_ID_DIR_INFORMATION:
            if (wideData38->FileNameLength > 0)
            {
                Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", g_MfrModuleName, dllInstance, IoStatusBlock->Status, wideData38->FileAttributes, DirPathUsed.c_str(), wideData38->FileNameLength, (const wchar_t*)wideData38->FileName);
            }
            wideData38 = wideData38->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_ID_BOTH_DIR_INFORMATION>(reinterpret_cast<BYTE*>(wideData38) + wideData38->NextEntryOffset);
            break;
        case 50: // FILE_ID_GLOBAL_TX_DIR_INFORMATION
            Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x", g_MfrModuleName, dllInstance, IoStatusBlock->Status);
            break;
        case 60: //  FILE_ID_EXTD_DIR_INFORMATION 
            Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x", g_MfrModuleName, dllInstance, IoStatusBlock->Status);
            break;
        case 63: //  FILE_ID_EXTD_BOTH_DIR_INFORMATION
            Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x", g_MfrModuleName, dllInstance, IoStatusBlock->Status);
            break;
        default:
            Log(LogLevel_DebugIntermediate, L"[%s%d] NtDll_NtQueryDirectoryFileFixup IoStatusBlock Status 0x%x", g_MfrModuleName, dllInstance, IoStatusBlock->Status);
            break;
        }
    }
    catch (...) {}


    return retfinal;
}

NTSTATUS __stdcall
NtDll_NtQueryDirectoryFileFixup(
    IN             HANDLE                 FileHandle,
    IN OPTIONAL    HANDLE                 Event,
    IN OPTIONAL    PIO_APC_ROUTINE        ApcRoutine,
    IN OPTIONAL    PVOID                  ApcContext,
    OUT            PIO_STATUS_BLOCK       IoStatusBlock,
    OUT            PVOID                  FileInformation,
    IN             ULONG                  Length,
    IN             FILE_INFORMATION_CLASS FileInformationClass,
    IN             BOOLEAN                ReturnSingleEntry,
    IN OPTIONAL    PUNICODE_STRING        FileName,
    IN             BOOLEAN                RestartScan
)
{
    NTSTATUS retfinal;
    [[maybe_unused]] DWORD dllInstance = g_InterceptInstance; // only bump in unguarded case

    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {

            g_InterceptInstance++;
            dllInstance = g_InterceptInstance;
            if (Event != NULL || ApcRoutine != NULL)
            {
                // We do not have support for async calls at this time.  Log this, even without debug logging enabled, so we are aware.
                

                Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup Async call not redirected", g_MfrModuleName, dllInstance);
                LogCallingModuleInstance(g_MfrModuleName, dllInstance);


                retfinal = ntdllimpl::NtQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
            }
            else if (!RestartScan)
            {
                // We also do not support returning for more info at this time, although maybe we could...
                Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup Next call not redirected", g_MfrModuleName, dllInstance);
                Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup RootDirectory=0x%x ReqClass=0x%x SingleEntry=%d Restart=%d", g_MfrModuleName, dllInstance, FileHandle, FileInformationClass, (DWORD)ReturnSingleEntry);
                LogCallingModuleInstance(g_MfrModuleName, dllInstance);
                retfinal = ntdllimpl::NtQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
            }
            else
            {

                if (FileName != NULL)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] NtDll_NtQueryDirectoryFileFixup RootDirectory=0x%x ReqClass=0x%x SingleEntry=%d Restart=%d FileName=%ls", g_MfrModuleName, dllInstance, FileHandle, FileInformationClass, (DWORD)ReturnSingleEntry, (DWORD)RestartScan, FileName->Buffer);
                }
                //LogCallingModuleInstance(g_MfrModuleName, dllInstance);
                retfinal = TripplePlay_NtQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
            }
        }
        else
        {
            retfinal = ntdllimpl::NtQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
        }
        return retfinal;
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"NtDll_NtQueryDirectoryFileFixup")


    retfinal = ntdllimpl::NtQueryDirectoryFileImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::NtQueryDirectoryFileImpl, NtDll_NtQueryDirectoryFileFixup);


#endif

#endif