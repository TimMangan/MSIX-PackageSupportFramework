//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if _DEBUG
//#define _ManualDebug 1
//#define MOREDEBUG 1
#include <thread>
#include <windows.h>
#endif

#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"

#if _DEBUG
#if DEBUG_NEW_FIXUPS 
#define DEBUG_NEW_FIXUPS_REGLEG 1
#endif
#endif


/// <summary>
///     ReplaceAppRegistrySyntaxA/W takes a string representing a registry path and returns a replacement string in "normal" style if it were in App hive style.
/// </summary>
/// <param name="regPath"></param>
/// <returns></returns>
std::string ReplaceAppRegistrySyntaxA(std::string regPath)
{
    std::string returnPath = regPath;

    // string returned is for pattern matching purposes, to keep consistent for the patterns,
    // revert to strings that look like HKEY_....
    if (regPath._Starts_with("=\\REGISTRY\\USER"))
    {
        if (regPath.length() > 15)
        {
            size_t offsetAfterSid = regPath.find('\\', 16);
            if (offsetAfterSid != std::string::npos)
            {
                returnPath = InterpretStringA("HKEY_CURRENT_USER") + regPath.substr(offsetAfterSid);
            }
            else
            {
                returnPath = InterpretStringA("HKEY_CURRENT_USER") + regPath.substr(15);
            }
        }
        else
        {
            returnPath = InterpretStringA("HKEY_CURRENT_USER");
        }
    }
    else if (regPath._Starts_with("=\\REGISTRY\\MACHINE"))
    {
        if (regPath.length() > 18)
        {
            size_t offsetAfterSid = regPath.find('\\', 19);
            if (offsetAfterSid != std::string::npos)
            {
                returnPath = InterpretStringA("HKEY_CURRENT_USER") + regPath.substr(offsetAfterSid);
            }
            else
            {
                returnPath = InterpretStringA("HKEY_LOCAL_MACHINE") + regPath.substr(18);
            }
        }
        else
        {
            returnPath = InterpretStringA("HKEY_LOCAL_MACHINE");
        }
    }
    return returnPath;
}
std::wstring ReplaceAppRegistrySyntaxW(std::wstring regPath)
{
    std::wstring returnPath = regPath;

    // string returned is for pattern matching purposes, to keep consistent for the patterns,
    // revert to strings that look like HKEY_....
    if (regPath._Starts_with(L"=\\REGISTRY\\USER"))
    {
        if (regPath.length() > 15)
        {
            size_t offsetAfterSid = regPath.find(L'\\', 16);
            if (offsetAfterSid != std::string::npos)
            {
                returnPath = InterpretStringW(L"HKEY_CURRENT_USER") + regPath.substr(offsetAfterSid);
            }
            else
            {
                returnPath = InterpretStringW(L"HKEY_CURRENT_USER") + regPath.substr(15);
            }
        }
        else
        {
            returnPath = InterpretStringW(L"HKEY_CURRENT_USER");
        }
    }
    else if (regPath._Starts_with(L"=\\REGISTRY\\MACHINE"))
    {
        if (regPath.length() > 18)
        {
            size_t offsetAfterSid = regPath.find(L'\\', 19);
            if (offsetAfterSid != std::string::npos)
            {
                returnPath = InterpretStringW(L"HKEY_CURRENT_USER") + regPath.substr(offsetAfterSid);
            }
            else
            {
                returnPath = InterpretStringW(L"HKEY_LOCAL_MACHINE") + regPath.substr(18);
            }
        }
        else
        {
            returnPath = InterpretStringW(L"HKEY_LOCAL_MACHINE");
        }
    }
    return returnPath;
}

REGSAM RegFixupSam(Json_Debug_Levels debugRequestLevel, std::string keypath, REGSAM samDesired, DWORD RegLocalInstance)
{

    REGSAM samModified = samDesired;
    std::string keystring;
    std::string altkeystring;


    Log(debugRequestLevel, "[%S%d] RegFixupSamA: path=%s\n", g_RegModuleName, RegLocalInstance, keypath.c_str());

    for (auto& spec : g_regRemediationSpecs)
    {

        for (auto& specitem : spec.remediationRecords)
        {
            switch (specitem.remediationType)
            {
            case Reg_Remediation_Type_ModifyKeyAccess:
                Log(debugRequestLevel, L"[%s%d]   RegFixupSam: rule is Check ModifyKeyAccess...\n", g_RegModuleName, RegLocalInstance);

                switch (specitem.modifyKeyAccess.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    keystring = "HKEY_CURRENT_USER\\";
                    altkeystring = "=\\REGISTRY\\USER\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is HKCU key\n", g_RegModuleName, RegLocalInstance);

                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            size_t OffsetHkcu = keystring.size();
                            if (keypath._Starts_with(altkeystring))
                            {
                                // Must remove both the pattern and the S-1-5-...\ that follows.
                                OffsetHkcu = keypath.find_first_of('\\', altkeystring.size()) + 1;
                            }
                            std::wstring wcheck = widen(keypath.substr(OffsetHkcu));
                            Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Check %s\n", g_RegModuleName, RegLocalInstance, wcheck.c_str());
                            Log(debugRequestLevel, L"[%s%d]   RegFixupSam: using %s\n", g_RegModuleName, RegLocalInstance, pattern.c_str());

                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is HKCU pattern match on type=0x%x.\n", g_RegModuleName, RegLocalInstance, specitem.modifyKeyAccess.access);

                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                            {
                                            //samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
                                            // samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY);
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2RW\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            // MAXIMUM_ALLOWED turns out to not have the maximum permissions allowed for
                                            // running in the container, for example to create a subkey.  So We'll try this.
                                            // samModified = KEY_READ | KEY_WRITE;
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            // MAXIMUM_ALLOWED turns out to not have the maximum permissions allowed for
                                            // running in the container, for example to create a subkey.  So We'll try this.
                                            // samModified = KEY_READ | KEY_WRITE;
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    default:
                                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Unknown rule ignored.\n", g_RegModuleName, RegLocalInstance);
                                        break;
                                    }
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }
                        }
                    }
                    else
                    {
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is not HKCU key?\n", g_RegModuleName, RegLocalInstance);
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = "HKEY_LOCAL_MACHINE\\";
                    altkeystring = "=\\REGISTRY\\MACHINE\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHklm = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHklm = keypath.find_first_of('\\', altkeystring.size()) + 1;
                        }
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam:  is HKLM key\n", g_RegModuleName, RegLocalInstance);

                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is HKLM pattern match on type=0x%x.\n", g_RegModuleName, RegLocalInstance, specitem.modifyKeyAccess.access);
                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            //samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK );
                                            // samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY);
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2RW\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Full2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    default:
                                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Unknown rule ignored.\n", g_RegModuleName, RegLocalInstance);
                                        break;
                                    }
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }
                         }
                    }
                    else
                    {
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is not HKLM key?\n", g_RegModuleName, RegLocalInstance);
                    }
                    break;
                case Modify_Key_Hive_Type_Unknown:
                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is UNKNOWN type key?\n", g_RegModuleName, RegLocalInstance);
                    break;
                default:
                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is OTHER type key?\n", g_RegModuleName, RegLocalInstance);
                    break;
                }
                break;
            default:
                // other rule type
                Log(debugRequestLevel, L"[%s%d]   RegFixupSam: rule is other %d...\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);
                break;
            }
        }
    }
    return samModified;
}

REGSAM RegFixupSam(Json_Debug_Levels debugRequestLevel, std::wstring wKeyPath, REGSAM samDesired, DWORD RegLocalInstance)
{

    REGSAM samModified = samDesired;
    std::wstring wKeyString;
    std::wstring wAltKeyString;


    Log(debugRequestLevel, L"[%S%d] RegFixupSamW: path=%s\n", g_RegModuleName, RegLocalInstance, wKeyPath.c_str());

    for (auto& spec : g_regRemediationSpecs)
    {

        for (auto& specitem : spec.remediationRecords)
        {
            switch (specitem.remediationType)
            {
            case Reg_Remediation_Type_ModifyKeyAccess:
                Log(debugRequestLevel, L"[%s%d]   RegFixupSam: rule is Check ModifyKeyAccess...\n", g_RegModuleName, RegLocalInstance);

                switch (specitem.modifyKeyAccess.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    wKeyString = L"HKEY_CURRENT_USER\\";
                    wAltKeyString = L"=\\REGISTRY\\USER\\";
                    if (wKeyPath._Starts_with(wKeyString) ||
                        wKeyPath._Starts_with(wAltKeyString))
                    {
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is HKCU key\n", g_RegModuleName, RegLocalInstance);

                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            size_t OffsetHkcu = wKeyString.size();
                            if (wKeyPath._Starts_with(wAltKeyString))
                            {
                                // Must remove both the pattern and the S-1-5-...\ that follows.
                                OffsetHkcu = wKeyPath.find_first_of(L'\\', wAltKeyString.size()) + 1;
                            }
                            std::wstring wCheck = wKeyPath.substr(OffsetHkcu);
                            Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Check %s\n", g_RegModuleName, RegLocalInstance, wCheck.c_str());
                            Log(debugRequestLevel, L"[%s%d]   RegFixupSam: using %s\n", g_RegModuleName, RegLocalInstance, pattern.c_str());

                            try
                            {
                                if (std::regex_match(wKeyPath.substr(OffsetHkcu), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is HKCU pattern match on type=0x%x.\n", g_RegModuleName, RegLocalInstance, specitem.modifyKeyAccess.access);

                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            //samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
                                            // samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY);
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2RW\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            // MAXIMUM_ALLOWED turns out to not have the maximum permissions allowed for
                                            // running in the container, for example to create a subkey.  So We'll try this.
                                            // samModified = KEY_READ | KEY_WRITE;
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            // MAXIMUM_ALLOWED turns out to not have the maximum permissions allowed for
                                            // running in the container, for example to create a subkey.  So We'll try this.
                                            // samModified = KEY_READ | KEY_WRITE;
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    default:
                                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Unknown rule ignored.\n", g_RegModuleName, RegLocalInstance);
                                        break;
                                    }
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }
                        }
                    }
                    else
                    {
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is not HKCU key?\n", g_RegModuleName, RegLocalInstance);
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    wKeyString = L"HKEY_LOCAL_MACHINE\\";
                    wAltKeyString = L"=\\REGISTRY\\MACHINE\\";
                    if (wKeyPath._Starts_with(wKeyString) ||
                        wKeyPath._Starts_with(wAltKeyString))
                    {
                        size_t OffsetHklm = wKeyString.length(); // was size()
                        if (wKeyPath._Starts_with(wAltKeyString))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHklm = wKeyPath.find_first_of(L'\\', wAltKeyString.length()) + 1;   // was also size()
                        }
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam:  is HKLM key\n", g_RegModuleName, RegLocalInstance);

                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            try
                            {
                                if (std::regex_match(wKeyPath.substr(OffsetHklm), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is HKLM pattern match on type=0x%x.\n", g_RegModuleName, RegLocalInstance, specitem.modifyKeyAccess.access);
                                    switch (specitem.modifyKeyAccess.access)
                                    {
                                    case Modify_Key_Access_Type_Full2RW:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            //samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK );
                                            // samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY);
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2RW\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Full2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: Full2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2R\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2MaxAllowed:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            // (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = MAXIMUM_ALLOWED;
                                            Log(LogLevel_DebugIntermediate, L"[%s%d]   RegFixupSam: RW2MaxAllowed\n", g_RegModuleName, RegLocalInstance);
                                            return samModified;
                                        }
                                        break;
                                    default:
                                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: Unknown rule ignored.\n", g_RegModuleName, RegLocalInstance);
                                        break;
                                    }
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }
                        }
                    }
                    else
                    {
                        Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is not HKLM key?\n", g_RegModuleName, RegLocalInstance);
                    }
                    break;
                case Modify_Key_Hive_Type_Unknown:
                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is UNKNOWN type key?\n", g_RegModuleName, RegLocalInstance);
                    break;
                default:
                    Log(debugRequestLevel, L"[%s%d]   RegFixupSam: is OTHER type key?\n", g_RegModuleName, RegLocalInstance);
                    break;
                }
                break;
            default:
                // other rule type
                Log(debugRequestLevel, L"[%s%d]   RegFixupSam: rule is other %d...\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);
                break;
            }
        }
    }
    return samModified;
}

#if TRYHKLM2HKCU
bool HasHKLM2HKCUSpecified()
{
    for (auto& spec : g_regRemediationSpecs)
    {
        for (auto& specitem : spec.remediationRecords)
        {
            if (specitem.remediationType == Reg_Remediation_Type_HKLM_to_HKCU)
            {
                return true;
            }
        }
    }
    return false;
} // HasHKLM2HKCUSpecified()


std::string HKLM2HKCU_Replacement(std::string path)
{
    return HKLM2HKCU_RedirNameA + "\\" + path;
}
std::wstring HKLM2HKCU_Replacement(std::wstring path)
{
    return HKLM2HKCU_RedirNameW + L"\\" + path;
}

bool IsKey_HKCUPathRedirectedFromHKLM(HKEY hKey)
{
    if (hKey != NULL)
    {
        std::string keyOnlyPath = InterpretKeyPath(hKey);
        if (keyOnlyPath.find("vHKLM_Redirection") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}
bool IsKeySubKey_HKCUPathRedirectedFromHKLM(HKEY hKey, std::string subKey)
{
    if (hKey != NULL && !subKey.empty())
    {
        //if (IsKey_HKCUPathRedirectedFromHKLM(hKey))
        //{
        //    return true;
        //}
        if (subKey.find("vHKLM_Redirection") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}
bool IsKeySubKey_HKCUPathRedirectedFromHKLM(HKEY hKey, std::wstring subKey)
{
    if (hKey != NULL && !subKey.empty())
    {
        //if (IsKey_HKCUPathRedirectedFromHKLM(hKey))
        //{
        //    return true;
        //}
        if (subKey.find(L"vHKLM_Redirection") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

RegCohorts GenerateRegCohorts(HKEY key, std::wstring subKey, [[maybe_unused]] DWORD RegLocalInstance)
{
    RegCohorts regCohorts;
    regCohorts.RequestedPath = InterpretKeyPathW(key);
    if (!subKey.empty())
        regCohorts.RequestedPath += L"\\" + subKey;
    if (regCohorts.RequestedPath.find(HKLM2HKCU_RedirNameW.c_str()) == std::wstring::npos)
    {
        Log(Json_Debug_Levels::LogLevel_DebugMaximum, L"[%s%d] GenerateRegCohorts: HKLM Reverse Redirection will not be needed.", g_RegModuleName, RegLocalInstance);
        // Requested Path is not redirected, so it is standard
        regCohorts.StandardPath = regCohorts.RequestedPath;
        regCohorts.RequestedIsStandard = true;
        // TODO: derive RedirectedPath from StandardPath correctly
        std::wstring withoutHKLM;
        if (regCohorts.StandardPath._Starts_with(L"HKEY_LOCAL_MACHINE\\"))
        {
            withoutHKLM = regCohorts.StandardPath.substr(19);
            regCohorts.RedirectedPath = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameW + L"\\" + withoutHKLM;
            regCohorts.RedirectionNotPossible = false;
        }
        else if (regCohorts.StandardPath._Starts_with(L"HKEY_LOCAL_MACHINE"))
        {
            Log(Json_Debug_Levels::LogLevel_DebugMaximum,L"[%s%d] GenerateRegCohorts: HKLM Redirection needed.", g_RegModuleName, RegLocalInstance);
            withoutHKLM = regCohorts.StandardPath.substr(18);
            regCohorts.RedirectedPath = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameW + withoutHKLM;
            regCohorts.RedirectionNotPossible = false;
        }
        else if (regCohorts.StandardPath._Starts_with(L"=\\REGISTRY\\MACHINE\\"))
        {
            Log(Json_Debug_Levels::LogLevel_DebugMaximum, L"[%s%d] GenerateRegCohorts: \\Reg\\Mach Redirection needed.", g_RegModuleName, RegLocalInstance);
            withoutHKLM = regCohorts.StandardPath.substr(19);
            regCohorts.RedirectedPath = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameW + L"\\" + withoutHKLM;
            regCohorts.RedirectionNotPossible = false;
        }
        else
        {
            //regCohorts.RedirectedPath = NULL;
            regCohorts.RedirectionNotPossible = true;
        }
    }
    else
    {
        // "HKEY_CURRENT_USER\\"
        //"=\\REGISTRY\\USER\\S-...\\"
        regCohorts.RedirectedPath = regCohorts.RequestedPath;
        regCohorts.RequestedIsStandard = false;
        regCohorts.ReverseRedirectionNotPossible = false;
        regCohorts.RedirectionNotPossible = true;
        // TODO: derive StandardPath from RedirectedPath
        std::wstring hkcuRename = L"HKEY_CURRENT_USER\\" + HKLM2HKCU_RedirNameW;
        if (regCohorts.RedirectedPath._Starts_with(hkcuRename))
        {
            regCohorts.StandardPath = L"HKEY_LOCAL_MACHINE\\" + regCohorts.RedirectedPath.substr(hkcuRename.length());
        }
        else 
        {
            size_t strip = regCohorts.RequestedPath.find(HKLM2HKCU_RedirNameW) + HKLM2HKCU_RedirNameW.length();
            regCohorts.StandardPath = L"HKEY_LOCAL_MACHINE" + regCohorts.RequestedPath.substr(strip);
        }
    }

    LogString(Json_Debug_Levels::LogLevel_DebugMaximum, g_RegModuleName, RegLocalInstance, L"GenerateRegCohorts: RequestedPath ", regCohorts.RequestedPath.c_str());
    LogString(Json_Debug_Levels::LogLevel_DebugMaximum, g_RegModuleName, RegLocalInstance, L"GenerateRegCohorts: StandardPath  ", regCohorts.StandardPath.c_str());
    LogString(Json_Debug_Levels::LogLevel_DebugMaximum, g_RegModuleName, RegLocalInstance, L"GenerateRegCohorts: RedirectedPath", regCohorts.RedirectedPath.c_str());
    return regCohorts;
}

#endif


bool HasFakeDeleteSpecified()
{
    for (auto& spec : g_regRemediationSpecs)
    {
        for (auto& specitem : spec.remediationRecords)
        {
            if (specitem.remediationType == Reg_Remediation_Type_FakeDelete)
            {
                return true;
            }
        }
    }
    return false;
} // HasFakeDeleteSpecified()
// helper for registry deleting
bool RegFixupFakeDelete(Json_Debug_Levels debugRequestLevel, std::string keypath, [[maybe_unused]] DWORD RegLocalInstance)
{
    Log(debugRequestLevel, "[%S%d] RegFixupFakeDelete: path=%s\n", g_RegModuleName, RegLocalInstance, keypath.c_str());
    std::string keystring;
    std::string altkeystring;
    for (auto& spec : g_regRemediationSpecs)
    {

        for (auto& specitem : spec.remediationRecords)
        {
            Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: specitem.type=%d\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);

            if (specitem.remediationType == Reg_Remediation_Type_FakeDelete)
            {
                Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: specitem.type=%d\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);
 
                switch (specitem.fakeDeleteKey.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    keystring = "HKEY_CURRENT_USER\\";
                    altkeystring = "=\\REGISTRY\\USER\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHkcu = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHkcu = keypath.find_first_of('\\', altkeystring.size()) + 1;
                        }
                        for (auto& pattern : specitem.fakeDeleteKey.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: match hkcu\n", g_RegModuleName, RegLocalInstance);
                                    return true;
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }
                        }
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = "HKEY_LOCAL_MACHINE\\";
                    altkeystring = "=\\REGISTRY\\MACHINE\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHklm = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHklm = keypath.find_first_of('\\', altkeystring.size()) + 1;
                        }
                        for (auto& pattern : specitem.fakeDeleteKey.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: match HKLM\n", g_RegModuleName, RegLocalInstance);
                                    return true;
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }

                        }
                    }
                    break;
                }
            }
        }
    }
    return false;
}
bool RegFixupFakeDelete(Json_Debug_Levels debugRequestLevel, std::wstring keypath, [[maybe_unused]] DWORD RegLocalInstance)
{
    Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: path=%s\n", g_RegModuleName, RegLocalInstance, keypath.c_str());
    std::wstring keystring;
    std::wstring altkeystring;
    for (auto& spec : g_regRemediationSpecs)
    {

        for (auto& specitem : spec.remediationRecords)
        {
            Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: specitem.type=%d\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);

            if (specitem.remediationType == Reg_Remediation_Type_FakeDelete)
            {
                Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: specitem.type=%d\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);

                switch (specitem.fakeDeleteKey.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    keystring = L"HKEY_CURRENT_USER\\";
                    altkeystring = L"=\\REGISTRY\\USER\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHkcu = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHkcu = keypath.find_first_of(L'\\', altkeystring.size()) + 1;
                        }
                        for (auto& pattern : specitem.fakeDeleteKey.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: match hkcu\n", g_RegModuleName, RegLocalInstance);
                                    return true;
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }
                        }
                    }
                    break;
                case Modify_Key_Hive_Type_HKLM:
                    keystring = L"HKEY_LOCAL_MACHINE\\";
                    altkeystring = L"=\\REGISTRY\\MACHINE\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
                        size_t OffsetHklm = keystring.size();
                        if (keypath._Starts_with(altkeystring))
                        {
                            // Must remove both the pattern and the S-1-5-...\ that follows.
                            OffsetHklm = keypath.find_first_of(L'\\', altkeystring.size()) + 1;
                        }
                        for (auto& pattern : specitem.fakeDeleteKey.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d] RegFixupFakeDelete: match HKLM\n", g_RegModuleName, RegLocalInstance);
                                    return true;
                                }
                            }
                            catch (...)
                            {
                                Log(LogLevel_Exception, L"[%s%d] Bad Regex pattern ignored in RegLegacyFixups.\n", g_RegModuleName, RegLocalInstance);
                            }

                        }
                    }
                    break;
                }
            }
        }
    }
    return false;
}


bool HasDeletionMarkerSpecified()
{
    for (auto& spec : g_regRemediationSpecs)
    {
        for (auto& specitem : spec.remediationRecords)
        {
            if (specitem.remediationType == Reg_Remediation_Type_DeletionMarker)
            {
                return true;
            }
        }
    }
    return false;
} // HasDeletionMarkerSpecified()

// helper for registry deletion marker (TWO FORMS)
// returns ERROR_SUCCESS if the path is not subject to a deletion marker
//        otherwise appropriate error code for the match, either Path or File (aka full string).
LSTATUS RegFixupDeletionMarker(Json_Debug_Levels debugRequestLevel, std::string keyPath, std::string Value, [[maybe_unused]] DWORD RegLocalInstance)
{
    try
    {
        if (!g_regRemediationSpecs.empty())
        {
            Log(debugRequestLevel, "[%s%d] RegFixupDeletionMarker: keypath=%s value=%s\n", g_RegModuleName, RegLocalInstance, keyPath.c_str(), Value.c_str());
            std::wstring wKeyPath = widen(keyPath);
            std::wstring wValue = widen(Value);
            std::wstring wKeyPathValue = wKeyPath;
            if (wValue.length() > 0)
            {
                wKeyPathValue = wKeyPathValue + L"\\\\" + wValue;
            }
            std::wstring wRemainingKeyPathValue;
            std::wstring wKeyString;
            std::wstring wAltKeyString;
            std::wstring wAltKeyString2;
            for (auto& spec : g_regRemediationSpecs)
            {

                for (auto& specitem : spec.remediationRecords)
                {
                    if (specitem.remediationType == Reg_Remediation_Type_DeletionMarker)
                    {
                        Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: specitem.type=%d\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);
                        //TODO:  Test this
                        switch (specitem.deletionMarker.hive)
                        {
                        case Modify_Key_Hive_Type_HKCU:
                            Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: checking hive HKCU\n", g_RegModuleName, RegLocalInstance);
                            wKeyString = L"HKEY_CURRENT_USER";
                            wAltKeyString = L"=\\REGISTRY\\USER";
                            wAltKeyString2 = L"=\\REGISTRY\\USER\\";
                            if (wKeyPathValue._Starts_with(wKeyString) ||
                                wKeyPathValue._Starts_with(wAltKeyString))
                            {
                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: request is in hive\n", g_RegModuleName, RegLocalInstance);
                                size_t OffsetHkcu = wKeyString.length() + 1;  // skip next '\' 
                                if (wKeyPathValue._Starts_with(wAltKeyString2))
                                {
                                    // Must remove both the pattern and the S-1-5-...\ that follows.
                                    OffsetHkcu = wAltKeyString2.length() + wKeyPathValue.substr(wAltKeyString2.length()).find_first_of(L'\\') + 1;
                                }
                                if (OffsetHkcu < wKeyPathValue.length())
                                {
                                    wRemainingKeyPathValue = wKeyPathValue.substr(OffsetHkcu);
                                }
                                else
                                {
                                    wRemainingKeyPathValue = L"";
                                }

                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: wRemainingKeyPathValue=%Ls\n", g_RegModuleName, RegLocalInstance, wRemainingKeyPathValue.c_str());
                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: regex=%Ls\n", g_RegModuleName, RegLocalInstance, specitem.deletionMarker.key.c_str());

                                if (std::regex_match(wRemainingKeyPathValue, std::wregex(specitem.deletionMarker.key, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: regex match on key\n", g_RegModuleName, RegLocalInstance);
                            
                                    if (specitem.deletionMarker.patterns.empty())
                                    {
                                        // treat an empty values list as a match on any value
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: no pattern specified return = ERROR_FILE_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else if (wValue.length() == 0)
                                    {
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: no value specified return = ERROR_FILE_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else
                                    {
                                        for (auto& pattern : specitem.deletionMarker.patterns)
                                        {
                                            std::wstring fullpattern = specitem.deletionMarker.key + L".*" + pattern;
                                            Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: wRemainingKeyPathValue vs regex=%Ls\n", g_RegModuleName, RegLocalInstance, fullpattern.c_str());
                                            if (std::regex_match(wRemainingKeyPathValue, std::wregex(fullpattern, std::regex_constants::icase)))
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: pattern match return = ERROR_PATH_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                                return ERROR_PATH_NOT_FOUND;
                                            }
                                        }
                                    }
                                }
                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: no match found.\n", g_RegModuleName, RegLocalInstance);
                            }
                            break;
                        case Modify_Key_Hive_Type_HKLM:
                            wKeyString = L"HKEY_LOCAL_MACHINE";
                            wAltKeyString = L"=\\REGISTRY\\MACHINE";
                            if (wKeyPathValue._Starts_with(wKeyString) ||
                                wKeyPathValue._Starts_with(wAltKeyString))
                            {
                                size_t OffsetHkcu = wKeyString.length() + 1;  // skip next '\' 
                                if (wKeyPathValue._Starts_with(wAltKeyString))
                                {
                                    // Must remove both the pattern and the S-1-5-...\ that follows.
                                    OffsetHkcu = wKeyPathValue.find_first_of(L'\\', wAltKeyString.length()) + 1;
                                }
                                if (OffsetHkcu < wKeyPathValue.length())
                                {
                                    wRemainingKeyPathValue = wKeyPathValue.substr(OffsetHkcu);
                                }
                                else
                                {
                                    wRemainingKeyPathValue = L"";
                                }
                                if (std::regex_match(wRemainingKeyPathValue, std::wregex(specitem.deletionMarker.key, std::regex_constants::icase)))
                                {
                                    if (specitem.deletionMarker.patterns.empty())
                                    {
                                        // treat an empty values list as a match on any value
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: return = ERROR_FILE_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else if (wValue.length() == 0)
                                    {
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else
                                    {
                                        for (auto& pattern : specitem.deletionMarker.patterns)
                                        {
                                            std::wstring fullpattern = specitem.deletionMarker.key + L".*" + pattern;
                                            if (std::regex_match(wValue, std::wregex(fullpattern, std::regex_constants::icase)))
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: return = ERROR_PATH_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                                return ERROR_PATH_NOT_FOUND;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] RegFixupDeletionMarker: exception caught\n", g_RegModuleName, RegLocalInstance);
    }
    return ERROR_SUCCESS;
}
LSTATUS RegFixupDeletionMarker(Json_Debug_Levels debugRequestLevel, std::wstring wKeyPath, std::wstring wValue, [[maybe_unused]] DWORD RegLocalInstance)
{
    try
    {
        if (!g_regRemediationSpecs.empty())
        {
            Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: keypath=%s value=%s\n", g_RegModuleName, RegLocalInstance, wKeyPath.c_str(), wValue.c_str());
            std::wstring wKeyPathValue = wKeyPath;
            if (wValue.length() > 0)
            {
                wKeyPathValue = wKeyPathValue + L"\\\\" + wValue;
            }
            std::wstring wRemainingKeyPathValue;
            std::wstring wKeyString;
            std::wstring wAltKeyString;
            std::wstring wAltKeyString2;
            for (auto& spec : g_regRemediationSpecs)
            {

                for (auto& specitem : spec.remediationRecords)
                {
                    if (specitem.remediationType == Reg_Remediation_Type_DeletionMarker)
                    {
                        Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: specitem.type=%d\n", g_RegModuleName, RegLocalInstance, specitem.remediationType);
                        //TODO:  Test this
                        switch (specitem.deletionMarker.hive)
                        {
                        case Modify_Key_Hive_Type_HKCU:
                            Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: checking hive HKCU\n", g_RegModuleName, RegLocalInstance);
                            wKeyString = L"HKEY_CURRENT_USER";
                            wAltKeyString = L"=\\REGISTRY\\USER";
                            wAltKeyString2 = L"=\\REGISTRY\\USER\\";
                            if (wKeyPathValue._Starts_with(wKeyString) ||
                                wKeyPathValue._Starts_with(wAltKeyString))
                            {
                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: request is in hive\n", g_RegModuleName, RegLocalInstance);
                                size_t OffsetHkcu = wKeyString.length() + 1;  // skip next '\' 
                                if (wKeyPathValue._Starts_with(wAltKeyString2))
                                {
                                    // Must remove both the pattern and the S-1-5-...\ that follows.
                                    OffsetHkcu = wAltKeyString2.length() + wKeyPathValue.substr(wAltKeyString2.length()).find_first_of(L'\\') + 1;
                                }
                                if (OffsetHkcu < wKeyPathValue.length())
                                {
                                    wRemainingKeyPathValue = wKeyPathValue.substr(OffsetHkcu);
                                }
                                else
                                {
                                    wRemainingKeyPathValue = L"";
                                }

                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: wRemainingKeyPathValue=%Ls\n", g_RegModuleName, RegLocalInstance, wRemainingKeyPathValue.c_str());
                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: regex=%Ls\n", g_RegModuleName, RegLocalInstance, specitem.deletionMarker.key.c_str());

                                if (std::regex_match(wRemainingKeyPathValue, std::wregex(specitem.deletionMarker.key, std::regex_constants::icase)))
                                {
                                    Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: regex match on key\n", g_RegModuleName, RegLocalInstance);

                                    if (specitem.deletionMarker.patterns.empty())
                                    {
                                        // treat an empty values list as a match on any value
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: no pattern specified return = ERROR_FILE_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else if (wValue.length() == 0)
                                    {
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: no value specified return = ERROR_FILE_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else
                                    {
                                        for (auto& pattern : specitem.deletionMarker.patterns)
                                        {
                                            std::wstring fullPattern = specitem.deletionMarker.key + L".*" + pattern;
                                            Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: wRemainingKeyPathValue vs regex=%Ls\n", g_RegModuleName, RegLocalInstance, fullPattern.c_str());
                                            if (std::regex_match(wRemainingKeyPathValue, std::wregex(fullPattern, std::regex_constants::icase)))
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: pattern match return = ERROR_PATH_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                                return ERROR_PATH_NOT_FOUND;
                                            }
                                        }
                                    }
                                }
                                Log(debugRequestLevel, L"[%s%d] RegFixupDeletionMarker: no match found.\n", g_RegModuleName, RegLocalInstance);
                            }
                            break;
                        case Modify_Key_Hive_Type_HKLM:
                            wKeyString = L"HKEY_LOCAL_MACHINE";
                            wAltKeyString = L"=\\REGISTRY\\MACHINE";
                            if (wKeyPathValue._Starts_with(wKeyString) ||
                                wKeyPathValue._Starts_with(wAltKeyString))
                            {
                                size_t OffsetHkcu = wKeyString.length() + 1;  // skip next '\' 
                                if (wKeyPathValue._Starts_with(wAltKeyString))
                                {
                                    // Must remove both the pattern and the S-1-5-...\ that follows.
                                    OffsetHkcu = wKeyPathValue.find_first_of(L'\\', wAltKeyString.length ()) + 1;
                                }
                                if (OffsetHkcu < wKeyPathValue.length())
                                {
                                    wRemainingKeyPathValue = wKeyPathValue.substr(OffsetHkcu);
                                }
                                else
                                {
                                    wRemainingKeyPathValue = L"";
                                }
                                if (std::regex_match(wRemainingKeyPathValue, std::wregex(specitem.deletionMarker.key, std::regex_constants::icase)))
                                {
                                    if (specitem.deletionMarker.patterns.empty())
                                    {
                                        // treat an empty values list as a match on any value
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: return = ERROR_FILE_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else if (wValue.length() == 0)
                                    {
                                        return ERROR_FILE_NOT_FOUND;
                                    }
                                    else
                                    {
                                        for (auto& pattern : specitem.deletionMarker.patterns)
                                        {
                                            std::wstring fullPattern = specitem.deletionMarker.key + L".*" + pattern;
                                            if (std::regex_match(wValue, std::wregex(fullPattern, std::regex_constants::icase)))
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupDeletionMarker: return = ERROR_PATH_NOT_FOUND", g_RegModuleName, RegLocalInstance);
                                                return ERROR_PATH_NOT_FOUND;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] RegFixupDeletionMarker: exception caught\n", g_RegModuleName, RegLocalInstance);
    }
    return ERROR_SUCCESS;
}




bool HasJavaBlockerSpecified()
{
    for (auto& spec : g_regRemediationSpecs)
    {
        for (auto& specitem : spec.remediationRecords)
        {
            if (specitem.remediationType == Reg_Remediation_Type_JavaBlocker)
            {
                return true;
            }
        }
    }
    return false;
} // HasJavaBlockerSpecified

// helper for registry java blocker marker
// true = blocked
bool RegFixupJavaBlocker(Json_Debug_Levels debugRequestLevel, std::string keyPath, [[maybe_unused]] DWORD RegLocalInstance)
{
    try
    {
        Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker: keypath=%S\n", g_RegModuleName, RegLocalInstance, keyPath.c_str());

        std::wstring wKeyPath = widen(keyPath);
        std::wstring wRemainingKeyPath;
        std::wstring check_strings[6] = { L"HKEY_CURRENT_USER\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"=\\REGISTRY\\USER\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"HKEY_LOCAL_MACHINE\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"=\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432NODE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"=\\REGISTRY\\MACHINE\\SOFTWARE\\WOW6432NODE\\CLASSES\\CLSID\\{CAFEEFAC-"
        };
        //std::wstring wKeyStringU;
        //std::wstring wAltKeyStringU;
        //std::wstring wKeyStringM;
        //std::wstring wAltKeyStringM;
        //std::wstring wKeyStringMw;
        //std::wstring wAltKeyStringMw;
        //std::wstring wMaxAllowedString;
        for (auto& spec : g_regRemediationSpecs)
        {
            for (auto& specitem : spec.remediationRecords)
            {
                if (specitem.remediationType == Reg_Remediation_Type_JavaBlocker)
                {
                    std::wstring wKeyPathUpper = L"";
                    for (size_t i = 0; i < wKeyPath.size(); i++)
                    {
                        wKeyPathUpper += towupper(wKeyPath[i]);
                    }

                    bool isInRange = false;
                    size_t OffsetHkcu = 0;
                    for (size_t index = 0; index < 6; index++)
                    {
                        if (wKeyPathUpper._Starts_with(check_strings[index]))
                        {
                            isInRange = true;
                            OffsetHkcu = check_strings[index].size();
                            break;
                        }
                    }

                    if (isInRange)
                    {
                        if (wKeyPath.size() > OffsetHkcu)
                        {
                            wRemainingKeyPath = wKeyPathUpper.substr(OffsetHkcu);

                            // {CAFEEFAC-0018-0000-0131-ABCDEFFEDCBA}
                            // {CAFEEFAC-0018-0000-0131-ABCDEFFEDCBB}
                            if (wRemainingKeyPath._Starts_with(L"00"))
                            {
                                try
                                {
                                    INT32 majorVersion = _wtoi(wRemainingKeyPath.substr(2, 1).c_str());
                                    INT32 minorVersion = _wtoi(wRemainingKeyPath.substr(3, 1).c_str());
                                    INT32 buildVersion = _wtoi(wRemainingKeyPath.substr(12, 3).c_str());
                                    if (specitem.javaBlocker.majorVersion < majorVersion)
                                    {
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupJavaBlocker:  matched\n", g_RegModuleName, RegLocalInstance);
                                        return true;
                                    }
                                    if (specitem.javaBlocker.majorVersion == majorVersion)
                                    {
                                        if (specitem.javaBlocker.minorVersion < minorVersion)
                                        {
                                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupJavaBlocker:  matched\n", g_RegModuleName, RegLocalInstance);
                                            return true;
                                        }
                                        if (specitem.javaBlocker.minorVersion == minorVersion)
                                        {
                                            if (specitem.javaBlocker.updateVersion < buildVersion)
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupJavaBlocker:  matched\n", g_RegModuleName, RegLocalInstance);
                                                return true;
                                            }
                                        }
                                    }
                                    Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker:  allowed\n", g_RegModuleName, RegLocalInstance);
                                }
                                catch (...)
                                {
                                    //update version FFF marker doesn't convert, but we want to block it anyway.
                                    Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker:  exception matched\n", g_RegModuleName, RegLocalInstance);
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] RegFixupJavaBlocker: exception caught\n", g_RegModuleName, RegLocalInstance);
    }
    Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker: no match\n", g_RegModuleName, RegLocalInstance);
    return false;
}
bool RegFixupJavaBlocker(Json_Debug_Levels debugRequestLevel, std::wstring keyPath, [[maybe_unused]] DWORD RegLocalInstance)
{
    try
    {
        Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker: keypath=%s\n", g_RegModuleName, RegLocalInstance, keyPath.c_str());

        std::wstring wKeyPath = keyPath;
        std::wstring wRemainingKeyPath;
        std::wstring check_strings[6] = { L"HKEY_CURRENT_USER\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"=\\REGISTRY\\USER\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"HKEY_LOCAL_MACHINE\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"=\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432NODE\\CLASSES\\CLSID\\{CAFEEFAC-",
                                         L"=\\REGISTRY\\MACHINE\\SOFTWARE\\WOW6432NODE\\CLASSES\\CLSID\\{CAFEEFAC-"
        };
        //std::wstring wKeyStringU;
        //std::wstring wAltKeyStringU;
        //std::wstring wKeyStringM;
        //std::wstring wAltKeyStringM;
        //std::wstring wKeyStringMw;
        //std::wstring wAltKeyStringMw;
        //std::wstring wMaxAllowedString;
        for (auto& spec : g_regRemediationSpecs)
        {
            for (auto& specitem : spec.remediationRecords)
            {
                if (specitem.remediationType == Reg_Remediation_Type_JavaBlocker)
                {
                    std::wstring wKeyPathUpper = L"";
                    for (size_t i = 0; i < wKeyPath.size(); i++)
                    {
                        wKeyPathUpper += towupper(wKeyPath[i]);
                    }

                    bool isInRange = false;
                    size_t OffsetHkcu = 0;
                    for (size_t index = 0; index < 6; index++)
                    {
                        if (wKeyPathUpper._Starts_with(check_strings[index]))
                        {
                            isInRange = true;
                            OffsetHkcu = check_strings[index].size();
                            break;
                        }
                    }

                    if (isInRange)
                    {
                        if (wKeyPath.size() > OffsetHkcu)
                        {
                            wRemainingKeyPath = wKeyPathUpper.substr(OffsetHkcu);

                            // {CAFEEFAC-0018-0000-0131-ABCDEFFEDCBA}
                            // {CAFEEFAC-0018-0000-0131-ABCDEFFEDCBB}
                            if (wRemainingKeyPath._Starts_with(L"00"))
                            {
                                try
                                {
                                    INT32 majorVersion = _wtoi(wRemainingKeyPath.substr(2, 1).c_str());
                                    INT32 minorVersion = _wtoi(wRemainingKeyPath.substr(3, 1).c_str());
                                    INT32 buildVersion = _wtoi(wRemainingKeyPath.substr(12, 3).c_str());
                                    if (specitem.javaBlocker.majorVersion < majorVersion)
                                    {
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupJavaBlocker:  matched\n", g_RegModuleName, RegLocalInstance);
                                        return true;
                                    }
                                    if (specitem.javaBlocker.majorVersion == majorVersion)
                                    {
                                        if (specitem.javaBlocker.minorVersion < minorVersion)
                                        {
                                            Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupJavaBlocker:  matched\n", g_RegModuleName, RegLocalInstance);
                                            return true;
                                        }
                                        if (specitem.javaBlocker.minorVersion == minorVersion)
                                        {
                                            if (specitem.javaBlocker.updateVersion < buildVersion)
                                            {
                                                Log(LogLevel_DebugIntermediate, L"[%s%d] RegFixupJavaBlocker:  matched\n", g_RegModuleName, RegLocalInstance);
                                                return true;
                                            }
                                        }
                                    }
                                    Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker:  allowed\n", g_RegModuleName, RegLocalInstance);
                                }
                                catch (...)
                                {
                                    //update version FFF marker doesn't convert, but we want to block it anyway.
                                    Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker:  exception matched\n", g_RegModuleName, RegLocalInstance);
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] RegFixupJavaBlocker: exception caught\n", g_RegModuleName, RegLocalInstance);
    }
    Log(debugRequestLevel, L"[%s%d] RegFixupJavaBlocker: no match\n", g_RegModuleName, RegLocalInstance);
    return false;
}

void StoreAndLogRegistryValueA(Json_Debug_Levels debugRequestLevel, DWORD dwType, PVOID lpData, LPDWORD lpcbData, std::wstring functionName, DWORD RegLocalInstance)
{
    try
    {
        switch (dwType)
        {
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
            if (lpData != NULL)
            {
                if (lpcbData != NULL)
                {
                    char* rstring = new char[(*lpcbData) + 1];
                    FillMemory(rstring, (*lpcbData) + 1, 0);
                    memcpy(rstring, lpData, *lpcbData);
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with value=%S", g_RegModuleName, RegLocalInstance, functionName.c_str(), rstring);
                }
            }
            else
            {
                if (lpcbData != NULL)
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with string no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), *lpcbData);
                }
                else
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with string no data", g_RegModuleName, RegLocalInstance, functionName.c_str());
                }
            }
            break;
        case REG_DWORD:
            if (lpData != NULL)
            {
                Log(debugRequestLevel, L"[%s%d] %s:  Returning success with DWORD 0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(),  *((DWORD*)lpData));
            }
            else
            {
                if (lpcbData != NULL)
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with DWORD, len needed=0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), *lpcbData);
                }
                else
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with DWORD no data", g_RegModuleName, RegLocalInstance, functionName.c_str());
            }
            break;
        default:
            if (lpData != NULL)
            {
                Log(debugRequestLevel, L"[%s%d] %s:  Returning success of type 0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), dwType);
            }
            else
            {
                if (lpcbData != NULL)
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success of type 0x%x no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), dwType, *lpcbData);
                }
                else
                {
                    Log(debugRequestLevel, "[%s%d] %s:  Returning success of type 0x%x no data", g_RegModuleName, RegLocalInstance, functionName.c_str(), dwType);
                }
            }
            break;
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] %s:  Exception thrown reading data.", g_RegModuleName, RegLocalInstance, functionName.c_str());
    }
}
void StoreAndLogRegistryValueW(Json_Debug_Levels debugRequestLevel, DWORD dwType, PVOID lpData, LPDWORD lpcbData, std::wstring functionName, DWORD RegLocalInstance)
{
    try
    {
        switch (dwType)
        {
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
            if (lpData != NULL)
            {
                if (lpcbData != NULL)
                {
                    wchar_t* rstring = new wchar_t[(*lpcbData) + 2];
                    FillMemory(rstring, (*lpcbData) + 1, 0);
                    memcpy(rstring, lpData, *lpcbData);
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with value=%s", g_RegModuleName, RegLocalInstance, functionName.c_str(), rstring);
                }
            }
            else
            {
                if (lpcbData != NULL)
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with string no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), *lpcbData);
                }
                else
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with string no data", g_RegModuleName, RegLocalInstance, functionName.c_str());
                }
            }
            break;
        case REG_DWORD:
            if (lpData != NULL)
            {
                Log(debugRequestLevel, L"[%s%d] %s:  Returning success with DWORD 0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), *((DWORD*)lpData));
            }
            else
            {
                if (lpcbData != NULL)
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with DWORD, len needed=0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), *lpcbData);
                }
                else
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success with DWORD no data", g_RegModuleName, RegLocalInstance, functionName.c_str() );
            }
            break;
        default:
            if (lpData != NULL)
            {
                Log(debugRequestLevel, L"[%s%d] %s:  Returning success of type 0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), dwType);
            }
            else
            {
                if (lpcbData != NULL)
                {
                    Log(debugRequestLevel, L"[%s%d] %s:  Returning success of type 0x%x no data, len needed=0x%x", g_RegModuleName, RegLocalInstance, functionName.c_str(), dwType, *lpcbData);
                }
                else
                {
                    Log(debugRequestLevel, "[%s%d] %s:  Returning success of type 0x%x no data", g_RegModuleName, RegLocalInstance, functionName.c_str(), dwType);
                }
            }
            break;
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] %s:  Exception thrown reading data.", g_RegModuleName, RegLocalInstance, functionName.c_str());
    }
}



bool IsValueNameInEnumerationA(std::string& valueName, KeyChildEnumerationsA& enumerations)
{
    return std::find(enumerations.ValueNames.begin(), enumerations.ValueNames.end(), valueName) != enumerations.ValueNames.end();
}

bool IsValueNameInEnumerationW(std::wstring& valueName, KeyChildEnumerationsW& enumerations)
{
    return std::find(enumerations.ValueNames.begin(), enumerations.ValueNames.end(), valueName) != enumerations.ValueNames.end();
}


bool IsSubKeyNameInEnumerationA(std::string& valueName, KeyChildEnumerationsA& enumerations)
{
    return std::find(enumerations.SubKeys.begin(), enumerations.SubKeys.end(), valueName) != enumerations.SubKeys.end();
}

bool IsSubKeyNameInEnumerationW(std::wstring& valueName, KeyChildEnumerationsW& enumerations)
{
    return std::find(enumerations.SubKeys.begin(), enumerations.SubKeys.end(), valueName) != enumerations.SubKeys.end();
}

std::string LStatusToString(LSTATUS status) {
    LPVOID msgBuffer;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        status,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msgBuffer,
        0,
        NULL);

    std::string message((LPSTR)msgBuffer, size);
    LocalFree(msgBuffer);
    return message;
}

std::wstring LStatusToWstring(LSTATUS status) {
    LPVOID msgBuffer;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        status,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&msgBuffer,
        0,
        NULL);

    std::wstring message((LPWSTR)msgBuffer, size);
    LocalFree(msgBuffer);
    return message;
}
