//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
 

#pragma once


#if _DEBUG
//#define MOREDEBUG
#endif

std::string ReplaceAppRegistrySyntaxA(std::string regPath);
std::wstring ReplaceAppRegistrySyntaxW(std::wstring regPath);

REGSAM RegFixupSam(Json_Debug_Levels debugRequestLevel, std::string keypath, REGSAM samDesired, DWORD RegLocalInstance);
REGSAM RegFixupSam(Json_Debug_Levels debugRequestLevel, std::wstring keypath, REGSAM samDesired, DWORD RegLocalInstance);

bool HasFakeDeleteSpecified();
bool RegFixupFakeDelete(Json_Debug_Levels debugRequestLevel, std::string keypath, [[maybe_unused]] DWORD RegLocalInstance);
bool RegFixupFakeDelete(Json_Debug_Levels debugRequestLevel, std::wstring keypath, [[maybe_unused]] DWORD RegLocalInstance);

bool HasDeletionMarkerSpecified();
LSTATUS RegFixupDeletionMarker(Json_Debug_Levels debugRequestLevel, std::string keyPath,std::string Value, [[maybe_unused]] DWORD RegLocalInstance);
LSTATUS RegFixupDeletionMarker(Json_Debug_Levels debugRequestLevel, std::wstring keyPath, std::wstring Value, [[maybe_unused]] DWORD RegLocalInstance);

bool HasJavaBlockerSpecified();
bool RegFixupJavaBlocker(Json_Debug_Levels debugRequestLevel, std::string keypath, [[maybe_unused]] DWORD RegLocalInstance);
bool RegFixupJavaBlocker(Json_Debug_Levels debugRequestLevel, std::wstring keypath, [[maybe_unused]] DWORD RegLocalInstance);

#if TRYHKLM2HKCU
bool HasHKLM2HKCUSpecified();

static std::string  HKLM2HKCU_RedirNameA =  "vHKLM_Redirection";
static std::wstring HKLM2HKCU_RedirNameW = L"vHKLM_Redirection";
static std::string  HKCU_RedirNameA =  "HKEY_CURRENT_USER\\vHKLM_Redirection";
static std::wstring HKCU_RedirNameW = L"HKEY_CURRENT_USER\\vHKLM_Redirection";

std::string HKLM2HKCU_Replacement(std::string path);
std::wstring HKLM2HKCU_Replacement(std::wstring path);

void StoreAndLogRegistryValueA(Json_Debug_Levels debugRequestLevel, DWORD dwType, PVOID lpData, LPDWORD lpcbData, std::wstring functionName, DWORD RegLocalInstance);
void StoreAndLogRegistryValueW(Json_Debug_Levels debugRequestLevel, DWORD dwType, PVOID lpData, LPDWORD lpcbData, std::wstring functionName, DWORD RegLocalInstance);

#endif

RegCohorts GenerateRegCohorts(HKEY key, std::wstring subKey, [[maybe_unused]] DWORD RegLocalInstance);

struct KeyChildEnumerationsA
{
    bool ValidKey = false;
    HKEY Key = NULL;
    DWORD SubKeyCount = 0;
    DWORD ValueCount = 0;
    DWORD MaxSubKeyNameLen = 0;
    DWORD MaxValueNameLen = 0;
    std::vector<std::string> SubKeys;
    std::vector<std::string> ValueNames;
};
struct KeyChildEnumerationsW
{
    bool ValidKey = false;
    HKEY Key = NULL;
    DWORD SubKeyCount = 0;
    DWORD ValueCount = 0;
    DWORD MaxSubKeyNameLen = 0;
    DWORD MaxValueNameLen = 0;
    std::vector<std::wstring> SubKeys;
    std::vector<std::wstring> ValueNames;
};

bool IsValueNameInEnumerationA(std::string& valueName, KeyChildEnumerationsA& enumerations);
bool IsValueNameInEnumerationW(std::wstring& valueName, KeyChildEnumerationsW& enumerations);

bool IsSubKeyNameInEnumerationA(std::string& valueName, KeyChildEnumerationsA& enumerations);
bool IsSubKeyNameInEnumerationW(std::wstring& valueName, KeyChildEnumerationsW& enumerations);


std::string LStatusToString(LSTATUS status);
std::wstring LStatusToWstring(LSTATUS status);