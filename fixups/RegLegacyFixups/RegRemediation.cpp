//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//#define MOREDEBUG 1

#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"


/// <summary>
///     ReplaceAppRegistrySyntax takes a string representing a registry path and returns a replacement string in "normal" style if it were in App hive style.
/// </summary>
/// <param name="regPath"></param>
/// <returns></returns>
std::string ReplaceAppRegistrySyntax(std::string regPath)
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

REGSAM RegFixupSam(std::string keypath, REGSAM samDesired, DWORD RegLocalInstance)
{

    REGSAM samModified = samDesired;
    std::string keystring;
    std::string altkeystring;


#ifdef _DEBUG
    Log("[%d] RegFixupSam: path=%s\n", RegLocalInstance, keypath.c_str());
#endif
    for (auto& spec : g_regRemediationSpecs)
    {

        for (auto& specitem : spec.remediationRecords)
        {
            switch (specitem.remeditaionType)
            {
            case Reg_Remediation_Type_ModifyKeyAccess:
#ifdef MOREDEBUG
                Log(L"[%d]   RegFixupSam: rule is Check ModifyKeyAccess...\n", RegLocalInstance);
#endif
                switch (specitem.modifyKeyAccess.hive)
                {
                case Modify_Key_Hive_Type_HKCU:
                    keystring = "HKEY_CURRENT_USER\\";
                    altkeystring = "=\\REGISTRY\\USER\\";
                    if (keypath._Starts_with(keystring) ||
                        keypath._Starts_with(altkeystring))
                    {
#ifdef MOREDEBUG
                        //Log(L"[%d]   RegFixupSam: is HKCU key\n", RegLocalInstance);
#endif
                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            size_t OffsetHkcu = keystring.size();
                            if (keypath._Starts_with(altkeystring))
                            {
                                // Must remove both the pattern and the S-1-5-...\ that follows.
                                OffsetHkcu = keypath.find_first_of('\\', altkeystring.size()) + 1;
                            }
#ifdef MOREDEBUG
                            std::wstring wcheck = widen(keypath.substr(OffsetHkcu));
                            Log(L"[%d]   RegFixupSam: Check %s\n", RegLocalInstance, wcheck.c_str());
                            Log(L"[%d]   RegFixupSam: using %s\n", RegLocalInstance, pattern.c_str());
#endif
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern, std::regex_constants::icase)))
                                {
#ifdef MOREDEBUG
                                    Log(L"[%d]   RegFixupSam: is HKCU pattern match on type=0x%x.\n", RegLocalInstance, specitem.modifyKeyAccess.access);
#endif
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
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: Full2RW\n", RegLocalInstance);
#endif
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
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: Full2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: Full2R\n", RegLocalInstance);
#endif
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: RW2R\n", RegLocalInstance);
#endif
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
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: RW2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                            return samModified;
                                        }
                                        break;
                                    default:
#ifdef _DEBUG
                                        Log(L"[%d]   RegFixupSam: Unknown rule ignored.\n", RegLocalInstance);
#endif 
                                        break;
                                    }
                                }
                            }
                            catch (...)
                            {
                                Log(L"[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
                            }
                        }
                    }
                    else
                    {
#ifdef _DEBUG
                        //Log(L"[%d]   RegFixupSam: is not HKCU key?\n", RegLocalInstance);
#endif
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
#ifdef _DEBUG
                        //Log(L"[%d]   RegFixupSam:  is HKLM key\n", RegLocalInstance);
#endif
                        for (auto& pattern : specitem.modifyKeyAccess.patterns)
                        {
                            try
                            {
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern, std::regex_constants::icase)))
                                {
#ifdef _DEBUG
                                    Log(L"[%d]   RegFixupSam: is HKLM pattern match on type=0x%x.\n", RegLocalInstance, specitem.modifyKeyAccess.access);
#endif
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
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: Full2RW\n", RegLocalInstance);
#endif
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_Full2R:
                                        if ((samDesired & (KEY_ALL_ACCESS)) == (KEY_ALL_ACCESS) ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0 ||
                                            (samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: Full2R\n", RegLocalInstance);
#endif
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
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: Full2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                            return samModified;
                                        }
                                        break;
                                    case Modify_Key_Access_Type_RW2R:
                                        //(samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)) != 0)
                                        if ((samDesired & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY)) != 0 ||
                                            (samDesired & (DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY)) != 0)
                                        {
                                            samModified = samDesired & ~(DELETE | WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE);
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: RW2R\n", RegLocalInstance);
#endif
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
#ifdef _DEBUG
                                            Log(L"[%d]   RegFixupSam: RW2MaxAllowed\n", RegLocalInstance);
#endif                                    
                                            return samModified;
                                        }
                                        break;
                                    default:
#ifdef _DEBUG
                                        Log(L"[%d]   RegFixupSam: Unknown rule ignored.\n", RegLocalInstance);
#endif   
                                        break;
                                    }
                                }
                            }
                            catch (...)
                            {
                                Log(L"[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
                            }
                        }
                    }
                    else
                    {
#ifdef _DEBUG
                        //Log(L"[%d]   RegFixupSam: is not HKLM key?\n", RegLocalInstance);
#endif
                    }
                    break;
                case Modify_Key_Hive_Type_Unknown:
#ifdef _DEBUG
                    Log(L"[%d]   RegFixupSam: is UNKNOWN type key?\n", RegLocalInstance);
#endif
                    break;
                default:
#ifdef _DEBUG
                    Log(L"[%d]   RegFixupSam: is OTHER type key?\n", RegLocalInstance);
#endif                    
                    break;
                }
                break;
            default:
                // other rule type
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
            if (specitem.remeditaionType == Reg_Remdiaton_Type_HKLM_to_HKCU)
            {
                return true;
            }
        }
    }
    return false;
} // HasHKLM2HKCUSpecified()

std::string HKLM2HKCU_Replacement(std::string path)
{
    std::string rPath = "REDIRECTED";
    return rPath + "\\" + path;
}
#endif


// helper for registry deleting
bool RegFixupFakeDelete(std::string keypath, [[maybe_unused]] DWORD RegLocalInstance)
{
#ifdef _DEBUG
    Log("[%d] RegFixupFakeDelete: path=%s\n", RegLocalInstance, keypath.c_str());
#endif
    std::string keystring;
    std::string altkeystring;
    for (auto& spec : g_regRemediationSpecs)
    {

        for (auto& specitem : spec.remediationRecords)
        {
#ifdef MOREDEBUG
            Log(L"[%d] RegFixupFakeDelete: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif      
            if (specitem.remeditaionType == Reg_Remediation_Type_FakeDelete)
            {
#ifdef MOREDEBUG
                Log(L"[%d] RegFixupFakeDelete: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif 
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
#ifdef _DEBUG
                                    Log(L"[%d] RegFixupFakeDelete: match hkcu\n", RegLocalInstance);
#endif                            
                                    return true;
                                }
                            }
                            catch (...)
                            {
#ifdef _DEBUG
                                Log(L"[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
#endif
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
#ifdef _DEBUG
                                    Log(L"[%d] RegFixupFakeDelete: match HKLM\n", RegLocalInstance);
#endif                            
                                    return true;
                                }
                            }
                            catch (...)
                            {
#ifdef _DEBUG
                                Log(L"[%d] Bad Regex pattern ignored in RegLegacyFixups.\n", RegLocalInstance);
#endif
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


// helper for registry deletion marker
// returs ERROR_SUCCESS if the path is not subject to a deletion marker
//        otherwise appropriate error code for the match, either Path or File (aka full string).
LSTATUS RegFixupDeletionMarker(std::string keyPath, std::string Value, [[maybe_unused]] DWORD RegLocalInstance)
{
    try
    {
#ifdef MOREDEBUG
        Log("[%d] RegFixupDeletionMarker: keypath=%s value=%s\n", RegLocalInstance, keyPath.c_str(), Value.c_str());
#endif
        std::wstring wKeyPath = widen(keyPath);
        std::wstring wValue = widen(Value);
        std::wstring wKeyPathValue = wKeyPath + L"\\\\" + wValue;
        std::wstring wRemainingKeyPathValue;
        std::wstring wKeyString;
        std::wstring wAltKeyString;
        for (auto& spec : g_regRemediationSpecs)
        {

            for (auto& specitem : spec.remediationRecords)
            {
                if (specitem.remeditaionType == Reg_Remediation_Type_DeletionMarker)
                {
#ifdef MOREDEBUG
                    Log(L"[%d] RegFixupDeletionMarker: specitem.type=%d\n", RegLocalInstance, specitem.remeditaionType);
#endif 
                    //TODO:  Test this
                    switch (specitem.deletionMarker.hive)
                    {
                    case Modify_Key_Hive_Type_HKCU:
#ifdef MOREDEBUG
                        Log(L"[%d] RegFixupDeletionMarker: checking hive HKCU\n", RegLocalInstance);
#endif 
                        wKeyString = L"HKEY_CURRENT_USER";
                        wAltKeyString = L"=\\REGISTRY\\USER";
                        if (wKeyPathValue._Starts_with(wKeyString) ||
                            wKeyPathValue._Starts_with(wAltKeyString))
                        {
#ifdef MOREDEBUG
                            Log(L"[%d] RegFixupDeletionMarker: request is in hive\n", RegLocalInstance);
#endif                        
                            size_t OffsetHkcu = wKeyString.size() + 2;  // skip next '\' 
                            if (wKeyPathValue._Starts_with(wAltKeyString))
                            {
                                // Must remove both the pattern and the S-1-5-...\ that follows.
                                OffsetHkcu = wKeyPathValue.find_first_of(L'\\', wAltKeyString.size()) + 2;
                            }
                            if (OffsetHkcu < wKeyPathValue.size())
                            {
                                wRemainingKeyPathValue = wKeyPathValue.substr(OffsetHkcu);
                            }
                            else
                            {
                                wRemainingKeyPathValue = L"";
                            }
                            
#ifdef MOREDEBUG
                            Log(L"[%d] RegFixupDeletionMarker: wRemainingKeyPathValue=%Ls\n", RegLocalInstance, wRemainingKeyPathValue.c_str());
                            Log(L"[%d] RegFixupDeletionMarker: regex=%Ls\n", RegLocalInstance, specitem.deletionMarker.key.c_str());
#endif
                            if (std::regex_match(wRemainingKeyPathValue, std::wregex(specitem.deletionMarker.key, std::regex_constants::icase)))
                            {
#ifdef MOREDEBUG
                                Log(L"[%d] RegFixupDeletionMarker: regex match on key\n", RegLocalInstance);
#endif                            
                                if (specitem.deletionMarker.patterns.empty())
                                {
                                    // treat an empty values list as a match on any value
#ifdef _DEBUG
                                    Log("[%d] RegFixupDeletionMarker: no pattern specified return = ERROR_FILE_NOT_FOUND", RegLocalInstance);
#endif
                                    return ERROR_FILE_NOT_FOUND;
                                }
                                else if (wValue.size() == 0)
                                {
#ifdef _DEBUG
                                    Log("[%d] RegFixupDeletionMarker: no value specified return = ERROR_FILE_NOT_FOUND", RegLocalInstance);
#endif
                                    return ERROR_FILE_NOT_FOUND;
                                }
                                else
                                {
                                    for (auto& pattern : specitem.deletionMarker.patterns)
                                    {
                                        std::wstring fullpattern = specitem.deletionMarker.key + L".*" + pattern;
#ifdef MOREDEBUG
                                        Log(L"[%d] RegFixupDeletionMarker: wRemainingKeyPathValue vs regex=%Ls\n", RegLocalInstance, fullpattern.c_str());
#endif
                                        if (std::regex_match(wRemainingKeyPathValue, std::wregex(fullpattern, std::regex_constants::icase)))
                                        {
#ifdef _DEBUG
                                            Log("[%d] RegFixupDeletionMarker: pattern match return = ERROR_PATH_NOT_FOUND", RegLocalInstance);
#endif
                                            return ERROR_PATH_NOT_FOUND;
                                        }
                                    }
                                }
                            }
#ifdef MOREDEBUG
                            Log(L"[%d] RegFixupDeletionMarker: no match found.\n", RegLocalInstance);
#endif 
                        }
                        break;
                    case Modify_Key_Hive_Type_HKLM:
                        wKeyString = L"HKEY_LOCAL_MACHINE";
                        wAltKeyString = L"=\\REGISTRY\\MACHINE";
                        if (wKeyPathValue._Starts_with(wKeyString) ||
                            wKeyPathValue._Starts_with(wAltKeyString))
                        {
                            size_t OffsetHkcu = wKeyString.size() + 2;  // skip next '\' 
                            if (wKeyPathValue._Starts_with(wAltKeyString))
                            {
                                // Must remove both the pattern and the S-1-5-...\ that follows.
                                OffsetHkcu = wKeyPathValue.find_first_of(L'\\', wAltKeyString.size()) + 2;
                            }
                            if (OffsetHkcu < wKeyPathValue.size())
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
#ifdef _DEBUG
                                    Log("[%d] RegFixupDeletionMarker: return = ERROR_FILE_NOT_FOUND", RegLocalInstance);
#endif
                                    return ERROR_FILE_NOT_FOUND;
                                }
                                else if (wValue.size() == 0)
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
#ifdef _DEBUG
                                            Log("[%d] RegFixupDeletionMarker: return = ERROR_PATH_NOT_FOUND", RegLocalInstance);
#endif
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
    catch (...)
    {
        Log(L"[%d] RegFixupDeletionMarker: exception caught\n", RegLocalInstance);
    }
    return ERROR_SUCCESS;
}



// helper for registry java blocker marker
// true = blocked
bool RegFixupJavaBlocker(std::string keyPath, [[maybe_unused]] DWORD RegLocalInstance)
{
    try
    {
#ifdef MOREDEBUG
        Log(L"[%d] RegFixupJavaBlocker: keypath=%S\n", RegLocalInstance, keyPath.c_str());
#endif
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
                if (specitem.remeditaionType == Reg_Remediation_Type_JavaBlocker)
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
#ifdef MOREDEBUG
                                        Log(L"[%d] RegFixupJavaBlocker:  matched\n", RegLocalInstance);
#endif 
                                        return true;
                                    }
                                    if (specitem.javaBlocker.majorVersion == majorVersion)
                                    {
                                        if (specitem.javaBlocker.minorVersion < minorVersion)
                                        {
#ifdef MOREDEBUG
                                            Log(L"[%d] RegFixupJavaBlocker:  matched\n", RegLocalInstance);
#endif 
                                            return true;
                                        }
                                        if (specitem.javaBlocker.minorVersion == minorVersion)
                                        {
                                            if (specitem.javaBlocker.updateVersion < buildVersion)
                                            {
#ifdef MOREDEBUG
                                                Log(L"[%d] RegFixupJavaBlocker:  matched\n", RegLocalInstance);
#endif 
                                                return true;
                                            }
                                        }
                                    }
#ifdef MOREDEBUG
                                    Log(L"[%d] RegFixupJavaBlocker:  allowed\n", RegLocalInstance);
#endif                                return false;
                                }
                                catch (...)
                                {
                                    //update version FFF marker doesn't convert, but we want to block it anyway.
#ifdef MOREDEBUG
                                    Log(L"[%d] RegFixupJavaBlocker:  exception matched\n", RegLocalInstance);
#endif 
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
        Log(L"[%d] RegFixupJavaBlocker: exception caught\n", RegLocalInstance);
    }
#ifdef MOREDEBUG
    Log(L"[%d] RegFixupJavaBlocker: no match\n", RegLocalInstance);
#endif 
    return false;
}
