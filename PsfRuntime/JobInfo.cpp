#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

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
#include <psf_logging.h>
#include <psf_config.h>
#include "JobInfo.h"

template<typename T>
bool QueryJobInfo(JOBOBJECTINFOCLASS cls, T& out, DWORD *pRetlen)
{
    //DWORD retLen = 0;
    if (!QueryInformationJobObject(
        NULL,        // current process's job
        cls,
        &out,
        sizeof(out),
        pRetlen))
    {
        return false;
    }
    return true;
}

bool JobPrintBasicLimitInfo(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    JOBOBJECT_BASIC_LIMIT_INFORMATION info{};
    DWORD retlen;
    if (!QueryJobInfo(JobObjectBasicLimitInformation, info, &retlen)) // 2
    {
        Log(LogLevel, L"\t[%s%d]\tNot in a job", ModuleName, Instance);
        return false;
    }

    Log(LogLevel, L"\t[%s%d]\tProcess is in a job.", ModuleName, Instance);

    Log(LogLevel, L"\t[%s%d]\t\t=== Basic Limit Info ===", ModuleName, Instance);
    Log(LogLevel, L"\t[%s%d]\t\t ActiveProcessLimit: %d", ModuleName, Instance, info.ActiveProcessLimit);
    Log(LogLevel, L"\t[%s%d]\t\t LimitFlags: %d", ModuleName, Instance, info.LimitFlags);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_ACTIVE_PROCESS", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_AFFINITY)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_AFFINITY", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_BREAKAWAY_OK)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_BREAKAWAY_OK", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_JOB_MEMORY", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_JOB_TIME", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_PRIORITY_CLASS", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_PROCESS_MEMORY", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_PROCESS_TIME", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_SCHEDULING_CLASS", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_SUBSET_AFFINITY)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_SUBSET_AFFINITY", ModuleName, Instance);
    if (info.LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET)
        Log(LogLevel, L"\t[%s%d]\t\t\t JOB_OBJECT_LIMIT_WORKINGSET", ModuleName, Instance);

    if (info.LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET)
    {
        Log(LogLevel, L"\t[%s%d]\t\t MinimumWorkingSetSize=%ld", ModuleName, Instance, info.MinimumWorkingSetSize);
        Log(LogLevel, L"\t[%s%d]\t\t MaixmumWorkingSetSize=%ld", ModuleName, Instance, info.MaximumWorkingSetSize);
    }

    if (info.LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
    {
        Log(LogLevel, L"\t[%s%d]\t\t ActiveProcessLimit=%d", ModuleName, Instance, info.ActiveProcessLimit);
    }

    if (info.LimitFlags & JOB_OBJECT_LIMIT_AFFINITY)
    {
        Log(LogLevel, L"\t[%s%d]\t\t ActiveProcessLimit=%lld", ModuleName, Instance, info.Affinity);
    }

    if (info.LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)
    {
        Log(LogLevel, L"\t[%s%d]\t\t PriorityClass=%d", ModuleName, Instance, info.PriorityClass);
    }

    if (info.LimitFlags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS)
    {
        Log(LogLevel, L"\t[%s%d]\t\t SchedulingClass=%d", ModuleName, Instance, info.SchedulingClass);
    }
    return true;
}

void JobPrintBasicAccountingInfo(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info{};
    DWORD retlen;
    if (!QueryJobInfo(JobObjectBasicAccountingInformation, info,&retlen))  // 1
    {
        Log(LogLevel, L"\t[%s%d]\tNot in a job", ModuleName, Instance);
        return;
    }

    //Log(LogLevel, L"\t[%s%d]\tProcess is in a job.", ModuleName, Instance);

    Log(LogLevel, L"\t[%s%d]\t\t=== Basic Accounting Info ===", ModuleName, Instance);
    Log(LogLevel, L"\t[%s%d]\t\t TotalUserTime: %lld", ModuleName, Instance, info.TotalUserTime.QuadPart);
    Log(LogLevel, L"\t[%s%d]\t\t TotalKernelTime: %lld", ModuleName, Instance, info.TotalKernelTime.QuadPart);
    Log(LogLevel, L"\t[%s%d]\t\t TotalPageFaultCount: %d", ModuleName, Instance, info.TotalPageFaultCount);
    Log(LogLevel, L"\t[%s%d]\t\t TotalProcesses: %d", ModuleName, Instance, info.TotalProcesses);
    Log(LogLevel, L"\t[%s%d]\t\t ActiveProcesses: %d", ModuleName, Instance, info.ActiveProcesses);
    Log(LogLevel, L"\t[%s%d]\t\t TotalTerminatedProcesses: %d", ModuleName, Instance, info.TotalTerminatedProcesses);

}

void JobPrintExtendedLimits(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
    DWORD retlen;
    if (!QueryJobInfo(JobObjectExtendedLimitInformation, info, &retlen)) // 9
        return;

    Log(LogLevel, L"\t[%s%d]\t\t=== Extended Limits ===", ModuleName, Instance);
    Log(LogLevel, L"\t[%s%d]\t\t LimitFlags: 0x%x", ModuleName, Instance, info.BasicLimitInformation.LimitFlags);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME)
        Log(LogLevel, L"\t[%s%d]\t\t ProcessTimeLimit: %lld", ModuleName, Instance, info.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME)
        Log(LogLevel, L"\t[%s%d]\t\t JobTimeLimit: %lld", ModuleName, Instance, info.BasicLimitInformation.PerJobUserTimeLimit.QuadPart);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
        Log(LogLevel, L"\t[%s%d]\t\t ActiveProcessLimit: %d", ModuleName, Instance, info.BasicLimitInformation.ActiveProcessLimit);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_AFFINITY)
        Log(LogLevel, L"\t[%s%d]\t\t Affinity: 0x%x", ModuleName, Instance, info.BasicLimitInformation.Affinity);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)
        Log(LogLevel, L"\t[%s%d]\t\t PriorityClass: %d", ModuleName, Instance, info.BasicLimitInformation.PriorityClass);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS)
        Log(LogLevel, L"\t[%s%d]\t\t SchedulingClass: %d", ModuleName, Instance, info.BasicLimitInformation.SchedulingClass);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY)
        Log(LogLevel, L"\t[%s%d]\t\t ProcessMemoryLimit: %d", ModuleName, Instance, info.ProcessMemoryLimit);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY)
        Log(LogLevel, L"\t[%s%d]\t\t JobMemoryLimit: %d", ModuleName, Instance, info.JobMemoryLimit);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION)
        Log(LogLevel, L"\t[%s%d]\t\t DIE_ON_UNHANDLED_EXCEPTION", ModuleName, Instance);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_BREAKAWAY_OK)
        Log(LogLevel, L"\t[%s%d]\t\t BREAKAWAY_OK", ModuleName, Instance);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)
        Log(LogLevel, L"\t[%s%d]\t\t SILENT_BREAKAWAY_OK", ModuleName, Instance);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
        Log(LogLevel, L"\t[%s%d]\t\t KILL_ON_JOB_CLOSE", ModuleName, Instance);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_SUBSET_AFFINITY)
        Log(LogLevel, L"\t[%s%d]\t\t SILENT_SUBSET_AFFINITY", ModuleName, Instance);
    if (info.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY_LOW)
        Log(LogLevel, L"\t[%s%d]\t\t SILENT_JOB_MEMORY_LOW", ModuleName, Instance);
    // there are a few more of these we don't care about in winnt.h
}

void JobPrintUIRestrictions(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS info{};
    DWORD retlen;
    if (!QueryJobInfo(JobObjectBasicUIRestrictions, info, &retlen)) // 4
        return;

    Log(LogLevel, L"\t[%s%d]\t\t=== UI Restrictions ===", ModuleName, Instance);
    Log(LogLevel, L"\t[%s%d]\t\t LimitUIRestrictionClassFlags: 0x%x", ModuleName, Instance, info.UIRestrictionsClass);

    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_DESKTOP)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_DESKTOP", ModuleName, Instance);
    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_DISPLAYSETTINGS)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_DISPLAYSETTINGS", ModuleName, Instance);
    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_EXITWINDOWS)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_EXITWINDOWS", ModuleName, Instance);
    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_GLOBALATOMS)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_GLOBALATOMS", ModuleName, Instance);
    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_HANDLES)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_HANDLES", ModuleName, Instance);
    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_READCLIPBOARD)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_READCLIPBOARD", ModuleName, Instance);
    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS", ModuleName, Instance);
    if (info.UIRestrictionsClass & JOB_OBJECT_UILIMIT_WRITECLIPBOARD)
        Log(LogLevel, L"\t[%s%d]\t\t JOB_OBJECT_UILIMIT_WRITECLIPBOARD", ModuleName, Instance);

}

void JobPrintCpuRate(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info{};
    DWORD retlen;
    if (!QueryJobInfo(JobObjectCpuRateControlInformation, info,&retlen)) //15
        return;

    Log(LogLevel, L"\t[%s%d]\t\t=== CPU Rate Control ===", ModuleName, Instance);
    Log(LogLevel, L"\t[%s%d]\t\t ControlFlags: 0x%x", ModuleName, Instance, info.ControlFlags);

    if (info.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_ENABLE)
    {
        if (info.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP)
            Log(LogLevel, L"\t[%s%d]\t\t HardCap (1/100th %): %d", ModuleName, Instance, info.CpuRate);
        std::cout << "HardCap: " << info.CpuRate << " (1/100th %)\n";

        if (info.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_WEIGHT_BASED)
            Log(LogLevel, L"\t[%s%d]\t\t Weight: %d", ModuleName, Instance, info.Weight);
    }
}

void JobPrintHierarchy(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    JOBOBJECT_JOBSET_INFORMATION info{};
    DWORD retlen;
    if (!QueryJobInfo(JobObjectJobSetInformation, info,&retlen)) // 10 maybe can't get this way, or undocumented?
    {
        Log(LogLevel, L"\t[%s%d]\t\t=== Job Hierarchy ===", ModuleName, Instance);
        Log(LogLevel, L"\t[%s%d]\t\t NONE (needed=%d)", ModuleName, Instance, retlen);
    }
    else
    {
        Log(LogLevel, L"\t[%s%d]\t\t=== Job Hierarchy ===", ModuleName, Instance);
        Log(LogLevel, L"\t[%s%d]\t\t MemberLevel: %d", ModuleName, Instance, info.MemberLevel);
    }
}

void JobPrintChain(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    // Windows 10+ partially supports this
    // Structure is not fully documented but stable enough for diagnostics
    struct JOBOBJECT_JOB_CHAIN_INFORMATION
    {
        DWORD ChainDepth;
        HANDLE JobChainId[1]; // Actually an array of job IDs (kernel object IDs)
    };

    
    DWORD retLen = 0;

    DWORD bufferSize = sizeof(JOBOBJECT_JOB_CHAIN_INFORMATION) + sizeof(HANDLE) * 10;  // more than enough
    JOBOBJECT_JOB_CHAIN_INFORMATION *info = (JOBOBJECT_JOB_CHAIN_INFORMATION*)malloc(bufferSize);
    if (info)
    {
        if (!QueryInformationJobObject(NULL, (JOBOBJECTINFOCLASS)40, info, bufferSize, &retLen)) ////JobObjectJobChainInformation, // Undocumented enum value
        {
            Log(LogLevel, L"\t[%s%d]\t\t=== Job Object Chain Information ===", ModuleName, Instance);
            if (retLen != 0)
                Log(LogLevel, L"\t[%s%d]\t\t NONE (needed=%d)", ModuleName, Instance, retLen);
            else
                Log(LogLevel, L"\t[%s%d]\t\t fail (error=0x%x)", ModuleName, Instance, GetLastError());
            return;
        }


        Log(LogLevel, L"\t[%s%d]\t\t=== Job Object Chain Information ===", ModuleName, Instance);
        Log(LogLevel, L"\t[%s%d]\t\t Chain Depth %d", ModuleName, Instance, info->ChainDepth);

        for (DWORD i = 0; i < info->ChainDepth; i++)
        {
            Log(LogLevel, L"\t[%s%d]\t\t Job Index %d ID %d ", ModuleName, Instance, i, info->JobChainId[i]);
        }
        free(info);
    }
}

void JobPrintLimitViolationInformation2(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    // Believed to be used in App Containers
    JOBOBJECT_LIMIT_VIOLATION_INFORMATION_2 info = {};
    DWORD retLen = 0;
    if (!QueryJobInfo(JobObjectLimitViolationInformation2, info, &retLen))  // 34
    {
        Log(LogLevel, L"\t[%s%d]\t\t=== Job LimitViolationInformation2 ===", ModuleName, Instance);
        if (retLen != 0)
            Log(LogLevel, L"\t[%s%d]\t\t NONE (needed=%d)", ModuleName, Instance, retLen);
        else
            Log(LogLevel, L"\t[%s%d]\t\t fail (error=0x%x)", ModuleName, Instance, GetLastError());
    }
    else
    {
        // Provides details about violated limits.  We don't care about these details
        Log(LogLevel, L"\t[%s%d]\t\t=== Job LimitViolationInformation2 ===", ModuleName, Instance);
        Log(LogLevel, L"\t[%s%d]\t\t Established Limit Flags: %lld", ModuleName, Instance, info.LimitFlags);
        Log(LogLevel, L"\t[%s%d]\t\t Violation   Limit Flags: %lld", ModuleName, Instance, info.ViolationLimitFlags);
    }
}

void JobPrintSiloBasicInformation(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    // Believed to be used in App Containers
    JOBOBJECT_BASIC_INFORMATION info{};  // Cotent not known
    DWORD retLen = 0;
    if (!QueryJobInfo(JobObjectSiloBasicInformation, info, &retLen))
    {
        Log(LogLevel, L"\t[%s%d]\t\t=== Silo Basic Information ===", ModuleName, Instance);
        if (retLen != 0)
            Log(LogLevel, L"\t[%s%d]\t\t NONE (needed=%d)", ModuleName, Instance, retLen);
        else
            Log(LogLevel, L"\t[%s%d]\t\t fail (error=0x%x)", ModuleName, Instance, GetLastError());
    }
    else
    {
        Log(LogLevel, L"\t[%s%d]\t\t=== Silo Basic Information ===", ModuleName, Instance);
        Log(LogLevel, L"\t[%s%d]\t\t Silo ID: %d", ModuleName, Instance, info.SiloIdNumber);
        Log(LogLevel, L"\t[%s%d]\t\t Silo Parent ID: %d", ModuleName, Instance, info.SiloParentIdNumber);
        Log(LogLevel, L"\t[%s%d]\t\t Number of Processes: %d", ModuleName, Instance, info.NumberOfProcesses);
        Log(LogLevel, L"\t[%s%d]\t\t Number of Child Silos: %d", ModuleName, Instance, info.NumberOfChildSilos);
    }
}

#if ALSO_THE_SILO

void JobPrintOtherSiloInformation(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance)
{
    PEPROCESS Process = PsGetCurrentProcess();
    PEJOB Job = PsGetProcessJob(Process);

    if (Job != NULL) {
        PESILO Silo = NULL;
        NTSTATUS status = PsGetJobSilo(Job, &Silo);
    }


}
#endif