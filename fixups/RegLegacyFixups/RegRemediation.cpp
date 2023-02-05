//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

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
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern)))
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
                                        }
                                        break;
                                    default:
#ifdef _DEBUG
                                        Log(L"[%d]   RegFixupSam: No fixup needed.\n", RegLocalInstance);
#endif 
                                        break;
                                    }
                                    return samModified;
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
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern)))
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
                                        }
                                        break;
                                    default:
#ifdef _DEBUG
                                        Log(L"[%d]   RegFixupSam: No fixup needed.\n", RegLocalInstance);
#endif   
                                        break;
                                    }
                                    return samModified;
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
                                if (std::regex_match(widen(keypath.substr(OffsetHkcu)), std::wregex(pattern)))
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
                                if (std::regex_match(widen(keypath.substr(OffsetHklm)), std::wregex(pattern)))
                                {
#ifdef _DEBUG
                                    Log(L"[%d] RegFixupFakeDelete: match hklm\n", RegLocalInstance);
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
