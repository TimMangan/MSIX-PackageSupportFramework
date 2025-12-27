#pragma once
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

#if ALSO_THE_SILO
#include <ntddk.h>
#include <ntifs.h>
#endif

template<typename T>
bool QueryJobInfo(JOBOBJECTINFOCLASS cls, T& out);

bool JobPrintBasicLimitInfo(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintBasicAccountingInfo(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintExtendedLimits(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintUIRestrictions(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintCpuRate(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintHierarchy(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintChain(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintLimitViolationInformation2(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);
void JobPrintSiloBasicInformation(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);

// Not officially documented and has changed in the past, last updated in Windows 10.0
#if defined(_X86_)
typedef struct _JOBOBJECT_BASIC_INFORMATION {
    HANDLE SiloIdNumber;
    HANDLE SiloParentIdNumber;
    DWORD  NumberOfProcesses;
    DWORD  NumberOfChildSilos;
    BOOLEAN IsInServerSilo;
    BYTE    Reserved[3];
} JOBOBJECT_BASIC_INFORMATION, * PJOBOBJECT_BASIC_INFORMATION;
#else
typedef struct _JOBOBJECT_BASIC_INFORMATION {
    HANDLE  SiloIdNumber;
    HANDLE  SiloParentIdNumber;
    DWORD   NumberOfProcesses;
    DWORD   NumberOfChildSilos;
    BOOLEAN IsInServerSilo;
    BYTE    Reserved[3];
} JOBOBJECT_BASIC_INFORMATION, * PJOBOBJECT_BASIC_INFORMATION;
#endif
#if ALSO_THE_SILO

typedef struct _ESILO ESILO;
typedef ESILO* PESILO;

EXTERN_C
NTSTATUS PsGetJobSilo(
    _In_  PEJOB Job,
    _Out_ PESILO* Silo
);

EXTERN_C
NTSTATUS
PsGetJobSilo(
    _In_  PEJOB Job,
    _Out_ PESILO* Silo
);

void JobPrintOtherSiloInformation(Json_Debug_Levels LogLevel, CONST wchar_t* ModuleName, DWORD Instance);

#endif
