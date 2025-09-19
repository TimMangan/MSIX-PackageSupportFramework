//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#if _DEBUG
//#define _ManualDebug 1
#define MOREDEBUG 1
#include <thread>
#include <windows.h>
#endif

#include "pch.h"

#include <regex>
#include <vector>

#include <known_folders.h>
#include <objbase.h>

#include <psf_framework.h>
#include "Logging.h"
#include <psf_logging.h>
#include <utilities.h>

#if _DEBUG
#if DEBUG_NEW_FIXUPS 
#define DEBUG_NEW_FIXUPS_REGLEG 1
#endif
#endif


#include <filesystem>
using namespace std::literals;

#include "FunctionImplementations.h"
#include "Reg_Remediation_spec.h"


std::vector<Reg_Remediation_Spec>  g_regRemediationSpecs;




void InitializeFixups()
{
    g_JsonDebugLevel = (Json_Debug_Levels)::PSFGetDebugLevelFromJson();
    Json_Debug_Levels tempLog = g_JsonDebugLevel;
    g_JsonDebugLevel = LogLevel_DebugMaximum; // force this to at least basic for the init logging
    Log(LogLevel_DebugBasic, "[%s%d]\tRegLegacyFixups InitializeFixups: start Debug Level=%d", g_RegModuleName, 0, tempLog);
    g_JsonDebugLevel = tempLog;


}


void InitializeConfiguration()
{
    Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups Start InitializeConfiguration()\n");

    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        if (rootConfig != NULL)
        {
            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups process config\n");

            const psf::json_array& rootConfigArray = rootConfig->as_array();
            for (auto& spec : rootConfigArray)
            {
                Log(LogLevel_DebugIntermediate, L"[R0] RegLegacyFixups: process spec\n");

                Reg_Remediation_Spec specItem;
                auto& specObject = spec.as_object();
                if (auto regItems = specObject.try_get("remediation"))
                {
                    Log(LogLevel_DebugIntermediate, L"[R0] RegLegacyFixups:  remediation array:\n");

                    const psf::json_array& remediationArray = regItems->as_array();
                    for (auto& regItem : remediationArray)
                    {
                        Log(LogLevel_DebugIntermediate, L"[R0] RegLegacyFixups:    remediation entry:\n");

                        auto& regItemObject = regItem.as_object();
                        Reg_Remediation_Record recordItem;
                        auto type = regItemObject.get("type").as_string().wstring();
                        Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      Type: %Ls\n", type.data());

                        //Reg_Remediation_Spec specItem;
                        if (type.compare(L"ModifyKeyAccess") == 0)
                        {
                            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      is ModifyKeyAccess\n");

                            recordItem.remeditaionType = Reg_Remediation_Type_ModifyKeyAccess;
                            
                            try
                            {
                                std::wstring hiveType = regItemObject.try_get("hive")->as_string().wstring().data();
                                std::transform(
                                    hiveType.begin(), hiveType.end(),
                                    hiveType.begin(),
                                    [](wchar_t wc) { return (wchar_t)std::toupper(wc); });
                                Log(LogLevel_DebugIntermediate, L"[R0] RegLegacyFixups:      Hive: %Ls\n", hiveType.data());

                                if (hiveType.compare(L"HKCU") == 0 )
                                {
                                    recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_HKCU;
                                }
                                else if (hiveType.compare(L"HKLM") == 0)
                                {
                                    recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_HKLM;
                                }
                                else
                                {
                                    recordItem.modifyKeyAccess.hive = Modify_Key_Hive_Type_Unknown;
                                }
                                Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      hive: %Ls\n", hiveType.data());

                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading ModifyKeyAccess hive from config.json.");
                            }
                            try
                            {
                                for (auto& pattern : regItemObject.get("patterns").as_array())
                                {
                                    auto patternString = pattern.as_string().wstring();
                                    Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      Pattern: %Ls\n", patternString.data());
                                    recordItem.modifyKeyAccess.patterns.push_back(patternString.data());

                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading ModifyKeyAccess patterns from config.json.");
                            }
                            try
                            {
                                auto accessType = regItemObject.try_get("access")->as_string().wstring();
                                if (accessType.compare(L"Full2RW") == 0)
                                {
                                    recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2RW;
                                }
                                else if (accessType.compare(L"Full2MaxAllowed") == 0)
                                {
                                    recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2MaxAllowed;
                                }
                                else if (accessType.compare(L"Full2R") == 0)
                                {
                                    recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Full2R;
                                }
                                else if (accessType.compare(L"RW2R") == 0)
                                {
                                    recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_RW2R;
                                }
                                else if (accessType.compare(L"RW2MaxAllowed") == 0)
                                {
                                    recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_RW2MaxAllowed;
                                }
                                else
                                {
                                    recordItem.modifyKeyAccess.access = Modify_Key_Access_Type_Unknown;
                                }
                                Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      access: %Ls\n", accessType.data());
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading ModifyKeyAccess access from config.json.");
                            }
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else if (type.compare(L"FakeDelete") == 0)
                        {
                            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      is FakeDelete\n");

                            recordItem.remeditaionType = Reg_Remediation_Type_FakeDelete;
                            try
                            {
                                std::wstring hiveType = regItemObject.try_get("hive")->as_string().wstring().data();
                                std::transform(
                                    hiveType.begin(), hiveType.end(),
                                    hiveType.begin(),
                                    [](wchar_t wc) { return (wchar_t)std::toupper(wc); });
                                if (hiveType.compare(L"HKCU") == 0)
                                {
                                    recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_HKCU;
                                }
                                else if (hiveType.compare(L"HKLM") == 0)
                                {
                                    recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_HKLM;
                                }
                                else
                                {
                                    recordItem.fakeDeleteKey.hive = Modify_Key_Hive_Type_Unknown;
                                }
                                Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      hive: %Ls\n", hiveType.data());

                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading FakeDelete hive from config.json.");
                            }
                            try
                            {
                                for (auto& pattern : regItemObject.get("patterns").as_array())
                                {
                                    auto patternString = pattern.as_string().wstring();
                                    Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      Pattern: %Ls\n", patternString.data());

                                    recordItem.fakeDeleteKey.patterns.push_back(patternString.data());
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading FakeDelete patterns from config.json.");
                            }

                            specItem.remediationRecords.push_back(recordItem);
                        }
#if TRYHKLM2HKCU
                        else if (type.compare(L"HKLM2HKCU") == 0)
                        {
                            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      is HKLM2HKCU\n");

                            recordItem.remeditaionType = Reg_Remediation_Type_HKLM_to_HKCU;
                            specItem.remediationRecords.push_back(recordItem);
                        }
#endif
                        else if (type.compare(L"DeletionMarker") == 0)
                        {
                            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      is DeletionMarker\n");

                            recordItem.remeditaionType = Reg_Remediation_Type_DeletionMarker;
                            try
                            {
                                std::wstring hiveType = regItemObject.try_get("hive")->as_string().wstring().data();
                                std::transform(
                                    hiveType.begin(), hiveType.end(),
                                    hiveType.begin(),
                                    [](wchar_t wc) { return (wchar_t)std::toupper(wc); });
                                if (hiveType.compare(L"HKCU") == 0)
                                {
                                    recordItem.deletionMarker.hive = Modify_Key_Hive_Type_HKCU;
                                }
                                else if (hiveType.compare(L"HKLM") == 0)
                                {
                                    recordItem.deletionMarker.hive = Modify_Key_Hive_Type_HKLM;
                                }
                                else
                                {
                                    recordItem.deletionMarker.hive = Modify_Key_Hive_Type_Unknown;
                                } 
                                Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      Hive: %Ls\n", hiveType.data());
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading DeletionMarker hive from config.json.");
                            }

                            try
                            {
                                recordItem.deletionMarker.key = regItemObject.try_get("key")->as_string().wstring();
                                Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      Key: %Ls\n", recordItem.deletionMarker.key.data());
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading DeletionMarker hive from config.json.");
                            }

                            try
                            {
                                for (auto& pattern : regItemObject.get("patterns").as_array())
                                {
                                    auto patternString = pattern.as_string().wstring();
                                    Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      Pattern: %Ls\n", patternString.data());
                                    recordItem.deletionMarker.patterns.push_back(patternString.data());
                                }  
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading DeletionMarker patterns from config.json.");
                            }
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else if (type.compare(L"JavaBlocker") == 0)
                        {
                            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      is JavaBlocker\n");

                            recordItem.remeditaionType = Reg_Remediation_Type_JavaBlocker;
                            try
                            {
                                std::wstring s_majorVersion = (std::wstring)regItemObject.try_get("majorVersion")->as_string().wstring();
                                recordItem.javaBlocker.majorVersion = _wtoi(s_majorVersion.c_str());
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading JavaBlocker majorVersion from config.json.");
                            }
                            try
                            {
                                std::wstring s_minorVersion = (std::wstring)regItemObject.try_get("minorVersion")->as_string().wstring();
                                recordItem.javaBlocker.minorVersion = _wtoi(s_minorVersion.c_str());
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading JavaBlocker minorVersion from config.json.");
                            }
                            try
                            {
                                std::wstring s_updateVersion = (std::wstring)regItemObject.try_get("updateVersion")->as_string().wstring();
                                recordItem.javaBlocker.updateVersion = _wtoi(s_updateVersion.c_str());
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[R0] RegLegacyFixups:      EXCEPTION: reading JavaBlocker updateVersion from config.json.");
                            }
                            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups:      MaxVersion Allowed: %d.%dU%d\n", recordItem.javaBlocker.majorVersion, recordItem.javaBlocker.minorVersion, recordItem.javaBlocker.updateVersion );
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else
                        {
                            LogString(LogLevel_DebugBasic, L"R",0, L"RegLegacyFixups:      Have unknown type from config.json", type.data());
                        }
                        g_regRemediationSpecs.push_back(specItem);
                    }
                }
            }
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups: Fixup not found in json config.\n");
        }
        Log(LogLevel_DebugBasic, L"[R0] RegLegacyFixups End InitializeConfiguration()\n");
    }
}
