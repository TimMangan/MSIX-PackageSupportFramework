//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "pch.h"

#include <regex>
#include <vector>

#include <known_folders.h>
#include <objbase.h>

#include <psf_framework.h>
#include <psf_logging.h>
#include <utilities.h>

#include <filesystem>
using namespace std::literals;

#include "FunctionImplementations.h"
#include "Reg_Remediation_spec.h"


std::vector<Reg_Remediation_Spec>  g_regRemediationSpecs;




void InitializeFixups()
{
#if _DEBUG
    Log(L"Initializing RegLegacyFixups\n");
#endif
}


void InitializeConfiguration()
{
#if _DEBUG
    Log(L"RegLegacyFixups Start InitializeConfiguration()\n");
#endif
    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        if (rootConfig != NULL)
        {
#if _DEBUG
            Log(L"RegLegacyFixups process config\n");
#endif
            const psf::json_array& rootConfigArray = rootConfig->as_array();
            for (auto& spec : rootConfigArray)
            {
#if _DEBUG
                Log(L"RegLegacyFixups: process spec\n");
#endif
                Reg_Remediation_Spec specItem;
                auto& specObject = spec.as_object();
                if (auto regItems = specObject.try_get("remediation"))
                {
#if _DEBUG
                    Log(L"RegLegacyFixups:  remediation array:\n");
#endif
                    const psf::json_array& remediationArray = regItems->as_array();
                    for (auto& regItem : remediationArray)
                    {
#if _DEBUG
                        Log(L"RegLegacyFixups:    remediation entry:\n");
#endif
                        auto& regItemObject = regItem.as_object();
                        Reg_Remediation_Record recordItem;
                        auto type = regItemObject.get("type").as_string().wstring();
#if _DEBUG
                        Log(L"RegLegacyFixups:      Type: %Ls\n", type.data());
#endif
                        //Reg_Remediation_Spec specItem;
                        if (type.compare(L"ModifyKeyAccess") == 0)
                        {
#if _DEBUG
                            Log(L"RegLegacyFixups:      is ModifyKeyAccess\n");
#endif
                            recordItem.remeditaionType = Reg_Remediation_Type_ModifyKeyAccess;
                            
                            try
                            {
                                std::wstring hiveType = regItemObject.try_get("hive")->as_string().wstring().data();
                                std::transform(
                                    hiveType.begin(), hiveType.end(),
                                    hiveType.begin(),
                                    [](wchar_t wc) { return (wchar_t)std::toupper(wc); });
#if _DEBUG
                                Log(L"RegLegacyFixups:      Hive: %Ls\n", hiveType.data());
#endif
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
#if _DEBUG
                                Log(L"RegLegacyFixups:      hive: %Ls\n", hiveType.data());
#endif
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading ModifyKeyAccess hive from config.json.");
                            }
                            try
                            {
                                for (auto& pattern : regItemObject.get("patterns").as_array())
                                {
                                    auto patternString = pattern.as_string().wstring();
#if _DEBUG
                                    Log(L"RegLegacyFixups:      Pattern: %Ls\n", patternString.data());
#endif
                                    recordItem.modifyKeyAccess.patterns.push_back(patternString.data());

                                }
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading ModifyKeyAccess patterns from config.json.");
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
#if _DEBUG
                                Log(L"RegLegacyFixups:      access: %Ls\n", accessType.data());
#endif
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading ModifyKeyAccess access from config.json.");
                            }
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else if (type.compare(L"FakeDelete") == 0)
                        {
#if _DEBUG
                            Log(L"RegLegacyFixups:      is FakeDelete\n");
#endif
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
#if _DEBUG
                                Log(L"RegLegacyFixups:      hive: %Ls\n", hiveType.data());
#endif
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading FakeDelete hive from config.json.");
                            }
                            try
                            {
                                for (auto& pattern : regItemObject.get("patterns").as_array())
                                {
                                    auto patternString = pattern.as_string().wstring();
#if _DEBUG
                                    Log(L"RegLegacyFixups:      Pattern: %Ls\n", patternString.data());
#endif
                                    recordItem.fakeDeleteKey.patterns.push_back(patternString.data());
                                }
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading FakeDelete patterns from config.json.");
                            }

                            specItem.remediationRecords.push_back(recordItem);
                        }
#if TRYHKLM2HKCU
                        else if (type.compare(L"HKLM2HKCU") == 0)
                        {
#if _DEBUG
                            Log(L"RegLegacyFixups:      is HKLM2HKCU\n");
#endif
                            recordItem.remeditaionType = Reg_Remediation_Type_HKLM_to_HKCU;
                            specItem.remediationRecords.push_back(recordItem);
                        }
#endif
                        else if (type.compare(L"DeletionMarker") == 0)
                        {
#if _DEBUG
                            Log(L"RegLegacyFixups:      is DeletionMarker\n");
#endif
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
#if _DEBUG
                                Log(L"RegLegacyFixups:      Hive: %Ls\n", hiveType.data());
#endif
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading DeletionMarker hive from config.json.");
                            }

                            try
                            {
                                recordItem.deletionMarker.key = regItemObject.try_get("key")->as_string().wstring();
#if _DEBUG
                                Log(L"RegLegacyFixups:      Key: %Ls\n", recordItem.deletionMarker.key.data());
#endif
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading DeletionMarker hive from config.json.");
                            }

                            try
                            {
                                for (auto& pattern : regItemObject.get("patterns").as_array())
                                {
                                    auto patternString = pattern.as_string().wstring();
#if _DEBUG
                                    Log(L"RegLegacyFixups:      Pattern: %Ls\n", patternString.data());
#endif
                                    recordItem.deletionMarker.patterns.push_back(patternString.data());
                                }  
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading DeletionMarker patterns from config.json.");
                            }
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else if (type.compare(L"JavaBlocker") == 0)
                        {
#if _DEBUG
                            Log(L"RegLegacyFixups:      is JavaBlocker\n");
#endif
                            recordItem.remeditaionType = Reg_Remediation_Type_JavaBlocker;
                            try
                            {
                                std::wstring s_majorVersion = (std::wstring)regItemObject.try_get("majorVersion")->as_string().wstring();
                                recordItem.javaBlocker.majorVersion = _wtoi(s_majorVersion.c_str());
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading JavaBlocker majorVersion from config.json.");
                            }
                            try
                            {
                                std::wstring s_minorVersion = (std::wstring)regItemObject.try_get("minorVersion")->as_string().wstring();
                                recordItem.javaBlocker.minorVersion = _wtoi(s_minorVersion.c_str());
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading JavaBlocker minorVersion from config.json.");
                            }
                            try
                            {
                                std::wstring s_updateVersion = (std::wstring)regItemObject.try_get("updateVersion")->as_string().wstring();
                                recordItem.javaBlocker.updateVersion = _wtoi(s_updateVersion.c_str());
                            }
                            catch (...)
                            {
                                Log(L"RegLegacyFixups:      EXCEPTION: reading JavaBlocker updateVersion from config.json.");
                            }
#if _DEBUG
                            Log(L"RegLegacyFixups:      MaxVersion Allowed: %d.%dU%d\n", recordItem.javaBlocker.majorVersion, recordItem.javaBlocker.minorVersion, recordItem.javaBlocker.updateVersion );
#endif
                            specItem.remediationRecords.push_back(recordItem);
                        }
                        else
                        {
                            LogString(L"RegLegacyFixups:      Have unknown type from config.json",type.data());
                        }
                        g_regRemediationSpecs.push_back(specItem);
                    }
                }
            }
        }
        else
        {
#if _DEBUG
            Log(L"RegLegacyFixups: Fixup not found in json config.\n");
#endif
        }
#if _DEBUG
        Log(L"RegLegacyFixups End InitializeConfiguration()\n");
#endif
    }
}
