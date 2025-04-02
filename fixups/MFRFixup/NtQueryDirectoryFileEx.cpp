//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwquerydirectoryfileex


#if _DEBUG
//define MOREDEBUG 1
#endif

#include <errno.h>
#include <psf_logging.h>
#include "FunctionImplementations.h"
#include "FunctionImplementations_ntdll.h"
#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

#if Intercept_NTDLL

#ifdef DO_Intercept_NtQueryDirectoryFileEx

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:Ntdll_NtQueryDirectoryFileExFixup_Fixup=_NtDll_NtQueryDirectoryFileExFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#else
#pragma comment(linker, "/EXPORT:Ntdll_NtQueryDirectoryFileExFixup_Fixup=NtDll_NtQueryDirectoryFileExFixup_Fixup_v")  // A test to see if exporting these names helps ProcessMonitor stack traces.    
#endif

NTSTATUS Test_TripplePlayExAlternative(NTSTATUS retfinalIn, std::wstring wThisPathName, std::wstring casetype, DWORD dllInstance,
                                    HANDLE                 Event,
                                    PIO_APC_ROUTINE        ApcRoutine,
                                    PVOID                  ApcContext,
                                    PIO_STATUS_BLOCK       IoStatusBlock,
                                    PVOID                  FileInformation,
                                    ULONG                  Length,
                                    FILE_INFORMATION_CLASS FileInformationClass,
                                    ULONG                  QueryFlags,
                                    PUNICODE_STRING        FileName)
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
            retfinal = ntdllimpl::NtQueryDirectoryFileExImpl(thisHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup %s alternative %s return status=0x%x", dllInstance, casetype.c_str(), wThisPathName.c_str(), retfinal);
            CloseHandle(thisHandle);
        }
        else
        {
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup %s alternative %s failed to open handle.", dllInstance, casetype.c_str(), wThisPathName.c_str());
        }
    }
    catch (...)
    {
        Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup %s alternative exception.", dllInstance, casetype.c_str());
    }
    return retfinal;
}

NTSTATUS TripplePlay_NtQueryDirectoryFileExImpl(
    IN             HANDLE                 FileHandle,
    IN OPTIONAL    HANDLE                 Event,
    IN OPTIONAL    PIO_APC_ROUTINE        ApcRoutine,
    IN OPTIONAL    PVOID                  ApcContext,
    OUT            PIO_STATUS_BLOCK       IoStatusBlock,
    OUT            PVOID                  FileInformation,
    IN             ULONG                  Length,
    IN             FILE_INFORMATION_CLASS FileInformationClass,
    _In_           ULONG                  QueryFlags,
    _In_opt_       PUNICODE_STRING        FileName
)
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
    retfinal = ntdllimpl::NtQueryDirectoryFileExImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);

    Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup return status=0x%x", dllInstance, retfinal);
    
    //#define STATUS_BUFFER_OVERFLOW            ((DWORD   )0x80000005L)    // Documented
    //#define STATUS_NO_MORE_FILES              ((DWORD   )0x80000006L)    // Seen
    //#define STATUS_INVALID_INFO_CLASS         ((DWORD   )0xC0000003L)    // Documented
    //#define STATUS_INVALID_PARAMETER          ((DWORD   )0xC000000DL)    // Documented
    //#define STATUS_NO_SUCH_FILE               ((DWORD   )0xC000000FL)    // Seen
    //#define STATUS_BUFFER_TOO_SMALL           ((DWORD   )0xC0000023L)    // Documented

    if (retfinal == 0xC000000FL)
    {
        // We did not it in the requested area, so we need to try the other areas.  Other results we just return back without looking elsewhere.
        // If we try to handle requests for more than one result that has to return here, we will need to consider no more files scenario also.


        // Make adjustments to the found path to more normalize it.  
        // This is probably not needed because it didn't come from the application directly, but we do this in FindFiles and it can't hurt.
        std::wstring wfilePath = AdjustSlashes(filePath);
        wfilePath = AdjustBadUNC(wfilePath, dllInstance, L"NtDll_NtQueryDirectoryFileExFixup");

        // Determine possible paths involved
        Cohorts cohorts;
        DetermineCohorts(wfilePath, &cohorts, false, dllInstance, L"NtDll_NtQueryDirectoryFileExFixup");


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
            DirPathUsed = ReplacementRedirectedPath;
            retfinal = Test_TripplePlayExAlternative(retfinal, ReplaementRedirectedFileName, L"redirected", dllInstance,
                Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
        }
        else
        {
            if (cohorts.WsRedirected.size() > 0 &&
                !cohorts.WsRedirected._Equal(cohorts.WsRequested))
            {
                DirPathUsed = cohorts.WsRedirected;
                retfinal = Test_TripplePlayExAlternative(retfinal, cohorts.WsRedirected, L"redirected", dllInstance,
                    Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
            }
        }
        if (retfinal == 0xC000000FL)
        {
            if (bReplacementPackage)
            {
                DirPathUsed = ReplacementPackagePath;
                retfinal = Test_TripplePlayExAlternative(retfinal, ReplaementPackageFileName, L"package", dllInstance,
                    Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
            }
            else if (cohorts.WsPackage.size() > 0 &&
                !cohorts.WsPackage._Equal(cohorts.WsRequested))
            {
                DirPathUsed = cohorts.WsPackage;
                retfinal = Test_TripplePlayExAlternative(retfinal, cohorts.WsPackage, L"package", dllInstance,
                    Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
            }
        }
        if (retfinal == 0xC000000FL)
        {
            if (cohorts.WsNative.size() > 0 &&
                !cohorts.WsNative._Equal(cohorts.WsRequested))
            {
                DirPathUsed = cohorts.WsNative;
                retfinal = Test_TripplePlayExAlternative(retfinal, cohorts.WsNative, L"native", dllInstance,
                    Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
            }
        }
    }
         

    // Possible addtional logging, even in release build
    bool temp = g_psf_NoLogging;
#if MOREDEBUG
    g_psf_NoLogging = false;
#endif

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
                    Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", dllInstance, IoStatusBlock->Status, wideData1->FileAttributes, DirPathUsed.c_str(), wideData1->FileNameLength, (const wchar_t*)wideData1->FileName);
                }
                wideData1 = wideData1->NextEntryOffset == 0 ? NULL : reinterpret_cast<FILE_DIRECTORY_INFORMATION*>(reinterpret_cast<BYTE*>(wideData1) + wideData1->NextEntryOffset);
            }
            break;
        case 2: //FILE_FULL_DIRECTORY_INFORMATION:
            if (wideData2->FileNameLength > 0)
            {
                Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", dllInstance, IoStatusBlock->Status, wideData2->FileAttributes, DirPathUsed.c_str(), wideData2->FileNameLength, (const wchar_t*)wideData2->FileName);
            }
            wideData2 = wideData2->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_FULL_DIRECTORY_INFORMATION>(reinterpret_cast<BYTE*>(wideData2) + wideData2->NextEntryOffset);
            break;
        case 3: //FILE_BOTH_DIRECTORY_INFORMATION:
            if (wideData3->FileNameLength > 0)
            {
                Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", dllInstance, IoStatusBlock->Status, wideData3->FileAttributes, DirPathUsed.c_str(), wideData3->FileNameLength, (const wchar_t*)&wideData3->FileName[0]);
            }
            wideData3 = wideData3->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_BOTH_DIRECTORY_INFORMATION>(reinterpret_cast<BYTE*>(wideData3) + wideData3->NextEntryOffset);
            break;
        case 12: // FILE_NAMES_INFORMATION
            if (wideData12->FileNameLength > 0)
            {
                Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x FromDir=%s FileLen %d File %s", dllInstance, IoStatusBlock->Status, DirPathUsed.c_str(), wideData12->FileNameLength, (const wchar_t*)wideData12->FileName);
            }
            wideData12 = wideData12->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_NAMES_INFORMATION>(reinterpret_cast<BYTE*>(wideData12) + wideData12->NextEntryOffset);
            break;
        case 29: // FILE_OBJECT_ID:
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x", dllInstance, IoStatusBlock->Status);
            break;
        case 32: // FILE_QUOTA_INFORMATION:
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x", dllInstance, IoStatusBlock->Status);
            break;
        case 33: // FILE_REPARSE_POINT_INFORMATION:
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x", dllInstance, IoStatusBlock->Status);
            break;
        case 37: //FILE_ID_BOTH_DIR_INFORMATION:
            if (wideData37->FileNameLength > 0)
            {
                Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", dllInstance, IoStatusBlock->Status, wideData37->FileAttributes, DirPathUsed.c_str(), wideData37->FileNameLength, (const wchar_t*)wideData37->FileName);
            }
            wideData37 = wideData37->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_ID_BOTH_DIR_INFORMATION>(reinterpret_cast<BYTE*>(wideData37) + wideData37->NextEntryOffset);
            break;
        case 38: //FILE_ID_DIR_INFORMATION:
            if (wideData38->FileNameLength > 0)
            {
                Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x Attributes=0x%x FromDir=%s FileLen %d File %s", dllInstance, IoStatusBlock->Status, wideData38->FileAttributes, DirPathUsed.c_str(), wideData38->FileNameLength, (const wchar_t*)wideData38->FileName);
            }
            wideData38 = wideData38->NextEntryOffset == 0 ? NULL : reinterpret_cast<PFILE_ID_BOTH_DIR_INFORMATION>(reinterpret_cast<BYTE*>(wideData38) + wideData38->NextEntryOffset);
            break;
        case 50: // FILE_ID_GLOBAL_TX_DIR_INFORMATION
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x", dllInstance, IoStatusBlock->Status);
            break;
        case 60: //  FILE_ID_EXTD_DIR_INFORMATION 
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x", dllInstance, IoStatusBlock->Status);
            break;
        case 63: //  FILE_ID_EXTD_BOTH_DIR_INFORMATION
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x", dllInstance, IoStatusBlock->Status);
            break;
        default:
            Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup IoStatusBlock Status 0x%x", dllInstance, IoStatusBlock->Status);
            break;
        }
    }
    catch (...) {}

    g_psf_NoLogging = temp;
    return retfinal;
}

NTSTATUS __stdcall NtDll_NtQueryDirectoryFileExFixup(
    IN             HANDLE                 FileHandle,
    IN OPTIONAL    HANDLE                 Event,
    IN OPTIONAL    PIO_APC_ROUTINE        ApcRoutine,
    IN OPTIONAL    PVOID                  ApcContext,
    OUT            PIO_STATUS_BLOCK       IoStatusBlock,
    OUT            PVOID                  FileInformation,
    IN             ULONG                  Length,
    IN             FILE_INFORMATION_CLASS FileInformationClass,
    _In_           ULONG                  QueryFlags,
    _In_opt_       PUNICODE_STRING        FileName
)
{
    NTSTATUS retfinal;
    DWORD dllInstance = g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if MOREDEBUG
            bool tempLogging = g_psf_NoLogging;
#endif
            g_InterceptInstance++;
            dllInstance = g_InterceptInstance;
            if (Event != NULL || ApcRoutine != NULL)
            {
                // We do not have support for async calls at this time.  Log this, even without debug logging enabled, so we are aware.
#if MOREDEBUG
                g_psf_NoLogging = false;
#endif
                Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup isAsync not redirected", dllInstance);
                LogCallingModuleInstance(dllInstance); 
#if MOREDEBUG
                g_psf_NoLogging = tempLogging;
#endif
                retfinal = ntdllimpl::NtQueryDirectoryFileExImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
            }
            else
            {
                //bool remember = g_psf_NoLogging;
#if MOREDEBUG
                g_psf_NoLogging = false;
#endif
                if (FileName != NULL)
                {
                    Log(L"[%d] NtDll_NtQueryDirectoryExFileFixup RootDirectory=0x%x ReqClass=0x%x QueryFlags=%d FileName=%ls", dllInstance, FileHandle, FileInformationClass, QueryFlags, FileName->Buffer);
                }
                //LogCallingModuleInstance(dllInstance);
#if MOREDEBUG
                g_psf_NoLogging = tempLogging;
#endif
                retfinal = TripplePlay_NtQueryDirectoryFileExImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
            }
#if MOREDEBUG
            g_psf_NoLogging = tempLogging;
#endif
        }
        else
        {
            retfinal = ntdllimpl::NtQueryDirectoryFileExImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
        }


        return retfinal;
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"NtDll_NtQueryDirectoryFileExFixup")
#else
    catch (...)
    {
        Log(L"[%d] NtDll_NtQueryDirectoryFileExFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    retfinal = ntdllimpl::NtQueryDirectoryFileExImpl(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, QueryFlags, FileName);
    return retfinal;
}
DECLARE_FIXUP(ntdllimpl::NtQueryDirectoryFileExImpl, NtDll_NtQueryDirectoryFileExFixup);
#endif

#endif