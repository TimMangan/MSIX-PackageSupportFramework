//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#define ADDASNEWREPLACEMENT 1

#include <processthreadsapi.h>
#include "..\PsfLauncher\Globals.h"
#include "psf_logging.h"

/* These Attribute definitions are the form as stored in the dwflags field of the structure*/
#define SIH_PROC_THREAD_ATTRIBUTE_PARENT_PROCESS                    (1 << ProcThreadAttributeParentProcess)
#define SIH_PROC_THREAD_ATTRIBUTE_HANDLE_LIST                       (1 << ProcThreadAttributeHandleList)
#define SIH_PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY                    (1 << ProcThreadAttributeGroupAffinity) 
#define SIH_PROC_THREAD_ATTRIBUTE_PREFERRED_NODE                    (1 << ProcThreadAttributePreferredNode)
#define SIH_PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR                   (1 << ProcThreadAttributeIdealProcessor)
#define SIH_PROC_THREAD_ATTRIBUTE_UMS_THREAD                        (1 << ProcThreadAttributeUmsThread)
#define SIH_PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY                 (1 << ProcThreadAttributeMitigationPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES             (1 << ProcThreadAttributeSecurityCapabilities)
#define SIH_PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL                  (1 << ProcThreadAttributeProtectionLevel)
#define SIH_PROC_THREAD_ATTRIBUTE_JOB_LIST                          (1 << ProcThreadAttributeJobList)
#define SIH_PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY              (1 << ProcThreadAttributeChildProcessPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY   (1 << ProcThreadAttributeAllApplicationPackagesPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_WIN32_KFILTER                     (1 << ProcThreadAttributeWin32kFilter)
#define SIH_PROC_THREAD_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM     (1 << ProcThreadAttributeSafeOpenPromptOriginClaim)
#define SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY                (1 << ProcThreadAttributeDesktopAppPolicy)
#define SIH_PROC_THREAD_ATTRIBUTE_PSEUDO_CONSOLE                    (1 << ProcThreadAttributePseudoConsole)
#define SIH_PROC_THREAD_ATTRIBUTE_MACHINE_TYPE                      (1 << ProcThreadAttributeMachineType)

// Extra ones we are not getting from the system headers
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE ProcThreadAttributeValue (ProcThreadAttributePseudoConsole, FALSE, TRUE, FALSE)
#define PROC_THREAD_ATTRIBUTE_MACHINE_TYPE  ProcThreadAttributeValue (ProcThreadAttributeMachineType, FALSE, TRUE, FALSE)

struct SIH_PROC_THREAD_ATTRIBUTE_ENTRY
{
    DWORD_PTR Attribute;
    size_t cbSize;
    PVOID  lpvalue;
};
struct SIH_PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD dwflags;  // Bit encoded to indicate all of the types of entries included in the array
    ULONG Size;
    ULONG Count;
    ULONG Reserved;
    PULONG Unknown;
    SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry[ANYSIZE_ARRAY];
};

inline void DumpStartupAttributes(Json_Debug_Levels debugRequestLevel, SIH_PROC_THREAD_ATTRIBUTE_LIST* attlist, const wchar_t* moduleName, DWORD instance)
{
    if (debugRequestLevel <= g_JsonDebugLevel)
    {
        if (attlist != NULL)
        {
            Log(debugRequestLevel, L"\t[%s%d]\tAttribute List Dump:", moduleName, instance);
            Log(debugRequestLevel, L"\t\t[%s%d]\t\tdwflags=0x%x Size=0x%x Count=0x%x", moduleName, instance, attlist->dwflags, attlist->Size, attlist->Count);

            for (ULONG inx = 0; inx < attlist->Count; inx++)
            {
                try
                {
                    ULONG_PTR* handleList;
                    size_t handleCount;
                    DWORD attval;
                    SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry = attlist->Entry[inx];
                    Log(debugRequestLevel, L"\t\t[%s%d]\t\t\tIndex %d Attribute 0x%x Size=0x%x", moduleName, instance, inx, Entry.Attribute, Entry.cbSize);
                    Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\tPre item detail vvvvv", moduleName, instance);
                    switch (Entry.Attribute & 0x0FFFF)
                    {
                    case PROC_THREAD_ATTRIBUTE_PARENT_PROCESS & 0x0FFFF: // 0
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Parent_Process", moduleName, instance);
                        break;
                    case 1: // undefined: possibly PROC_THREAD_ATTRIBUTE_REPLACE_VALUE on ProcThreadAttributeParentProcess ?
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is undocumented(1)", moduleName, instance);
                        if (Entry.lpvalue == NULL)
                        {
                            Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is NULL", moduleName, instance);
                        }
                        else
                        {
                            try
                            {
                                if (Entry.cbSize == 4)
                                {
                                    // can't be a pointer
                                    Loghexdump(LogLevel_Launching, &Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                                }
                                else
                                {
                                    Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                                }
                            }
                            catch (...)
                            { 
                                Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tcannot display value", moduleName, instance); 
                            }
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_HANDLE_LIST & 0x0FFFF: // 2
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Handle_List", moduleName, instance);
                        if (Entry.lpvalue != NULL)
                        {
                            handleList = (ULONG_PTR*)(Entry.lpvalue);
                            handleCount = Entry.cbSize / sizeof(ULONG_PTR);
                            for (size_t hInx = 0; hInx < handleCount; hInx++)
                            {
                                Log(LogLevel_Launching, L"\t\t[%s%d]\t\t\t\t\tHandle[%d]=0x%p", moduleName, instance, hInx, handleList[hInx]);
                            }
                        }
                        Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY & 0x0FFFF:  // 3
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Group_Affinity", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_PREFERRED_NODE & 0x0FFFF: // 4
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Preferred_Node", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR & 0x0FFFF: // 5
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Ideal_Processor", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_UMS_THREAD & 0x0FFFF: // 6
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_UMS_Thread", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY & 0x0FFFF: // 7 
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Mitigation_Policy", moduleName, instance);
                        if (Entry.lpvalue == NULL)
                        {
                            Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is NULL", moduleName, instance);
                        }
                        else
                        {
                            Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES & 0x0FFFF: // 9
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Security_Capabilities", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL & 0x0FFFF: // 11
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Protection_Level", moduleName, instance);
                        if (Entry.cbSize == 4)
                        {
                            attval = *((DWORD*)(Entry.lpvalue));
                            switch (attval)
                            {
                            case PROTECTION_LEVEL_SAME:
                                Log(LogLevel_Launching, L"\t\t[%s%d]\t\t\t\tPROTECTION_LEVEL_SAME", moduleName, instance);
                                break;
                            case PROTECTION_LEVEL_NONE:
                                Log(LogLevel_Launching, L"\t\t[%s%d]\t\t\t\tPROTECTION_LEVEL_NONE", moduleName, instance);
                                break;
                                /***
                                case PROTECTION_LEVEL_LIGHT:
                                    Log(LogLevel_Launching, L"\t\t[%s%d]\t\t\t\tPROTECTION_LEVEL_LIGHT", moduleName, instance);
                                    break;
                                case PROTECTION_LEVEL_STANDARD:
                                    Log(LogLevel_Launching, L"\t\t[%s%d]\t\t\t\tPROTECTION_LEVEL_STANDARD", moduleName, instance);
                                    break;
                                case PROTECTION_LEVEL_STRICT:
                                    Log(LogLevel_Launching, L"\t\t[%s%d]\t\t\t\tPROTECTION_LEVEL_STRICT", moduleName, instance);
                                    break;
                                ***/
                            default:
                                Log(LogLevel_Launching, L"\t\t[%s%d]\t\t\t\tUnknown Protection Level: 0x%x", moduleName, instance, attval);
                                break;
                            }
                        }
                        Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_JOB_LIST & 0x0FFFF: //13 = 0xd
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Job_List", moduleName, instance);
                        if (Entry.lpvalue == NULL)
                        {
                            Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is NULL", moduleName, instance);
                        }
                        else
                        {
                            Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY & 0x0FFFF: // 14 = 0xe
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Child_Process_Policy", moduleName, instance);
                        if (Entry.lpvalue == NULL)
                        {
                            Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is NULL", moduleName, instance);
                        }
                        else
                        {
                            Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY & 0x0FFFF: // 15 = 0xf
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_AllApplicationPackagesPolicy", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_WIN32K_FILTER & 0x0FFFF: // 16 = 0x10
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Win32kFilter", moduleName, instance);
                        break;
                    case 17: //PROC_THREAD_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM: // 17 = 0x11
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_SafeOpenPromptOriginClaim", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY & 0x0FFFF: // 18 = 0x12
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Desktop_App_Policy", moduleName, instance);
                        if (Entry.cbSize == 4)
                        {
                            attval = *((DWORD*)(Entry.lpvalue));
                            if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE) != 0)
                            {
                                Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE present.", moduleName, instance);
                            }
                            if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE) != 0)
                            {
                                Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE present.", moduleName, instance);
                            }
                            if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE) != 0)
                            {
                                Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\tPROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE present.", moduleName, instance);
                            }
                        }
                        Loghexdump(debugRequestLevel, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        break;
                    case  22: // 22 = 0x16
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_PseudoConsole as documented (22)", moduleName, instance);
                        if (Entry.lpvalue == NULL)
                        {
                            Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is NULL", moduleName, instance);
                        }
                        else
                        {
                            Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_MITIGATION_AUDIT_POLICY & 0x0FFFF:  // 24 = 0x18
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Mitigation_Audit_Policy", moduleName, instance);
                        if (Entry.lpvalue == NULL)
                        {
                            Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is NULL", moduleName, instance);
                        }
                        else
                        {
                            if (Entry.lpvalue > (void*)3)
                            {
                                Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                            }
                            else
                            {
                                Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is standard handle %p", moduleName, instance, Entry.lpvalue);
                            }
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_MACHINE_TYPE & 0x0FFFF: // 25 = 0x19
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Machine_Type", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_COMPONENT_FILTER & 0x0FFFF: // 26 = 0x1a
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Component_Filter", moduleName, instance);
                        if (Entry.lpvalue == NULL)
                        {
                            Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t\tValue is NULL", moduleName, instance);
                        }
                        else
                        {
                            Loghexdump(LogLevel_Launching, Entry.lpvalue, (long)Entry.cbSize, moduleName, instance);
                        }
                        break;
                    case PROC_THREAD_ATTRIBUTE_ENABLE_OPTIONAL_XSTATE_FEATURES & 0x0FFFF: // 27 = 0x1b
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Enable_Optional_XState_Features", moduleName, instance);
                        break;
                    case PROC_THREAD_ATTRIBUTE_TRUSTED_APP & 0x0FFFF: // 29 = 0x1d
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is Attribute_Trusted_App", moduleName, instance);
                        break;
                    default:
                        Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\t The item is OTHER type", moduleName, instance);
                        // Other attributes can be processed here as needed
                    }
                    Log(debugRequestLevel, L"\t\t[%s%d]\t\t\t\tPost item detail ^^^^^", moduleName, instance);
                }
                catch (...)
                {
                    Log(debugRequestLevel, L"\t\t[%s%d]\t\t\tException processing attribute index %d", moduleName, instance, inx);
                }
            }
            Log(debugRequestLevel, L"\t[%s%d]\t\tEnd of Attribute List Dump", moduleName, instance);
        }

    }
}

inline bool DoesAttributeSpecifyInside(SIH_PROC_THREAD_ATTRIBUTE_LIST* attlist)
{
    bool bRet = true;
    if (attlist != NULL)
    {
        if ((attlist->dwflags & SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY) != 0)
        {
            for (ULONG inx = 0; inx < attlist->Count; inx++)
            {
                SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry = attlist->Entry[inx];
                if (Entry.Attribute == PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY)
                {
                    if (Entry.cbSize == 4)
                    {
                        DWORD attval = *((DWORD*)(Entry.lpvalue));
                        if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE) != 0)
                        {
                            bRet = false;
                        }
                    }
                }
            }
        }
    }
    return bRet;
}
inline bool DoesAttributeDesktopPolicyExistInside(SIH_PROC_THREAD_ATTRIBUTE_LIST* attlist)
{
    if (attlist != NULL)
    {
        if ((attlist->dwflags & SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY) != 0)
        {
            return true;
        }
    }
    return false;
}

struct MyProcThreadAttributeList
{
private:
    // Implementation notes:
    //   Currently (Windows 10 21H1 and some previous releases plus Windows 11 21H2), the default
    //   behavior for most child processes is that they run inside the container.  The two known
    //   exceptions to this are the conhost and cmd.exe processes.  We generally don't care about
    //   the conhost processes.
    // 
    // The documentation on these can be found here:
    //https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute

    // The PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY attribute has some settings that can impact this.
    // 0x04 is equivalent to PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE.  
    //     When used in a process creation call it affects only the new process being created, and 
    //     forces the new process to start inside the container, if possible.
    // 0x01 is equivalent to PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE
    //     When used in a process creation call, it ONLY affects child processes of the process being
    //     created, meaning that grandshildren can/will break away from running inside the container used by
    //     that new process.
    // 0x02 is equivalent to PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE 
    //     When used in a process creation call, it ONLY affects child processes of the process being
    //     created, meaning that grandshildren can/will NOT break away from running inside the container used by
    //     that new process.
    //
    // This PSF code uses the attribute to cause:
    //   1. Create a Powershell process in the same container as PSF.
    //   2. Any process that Powershell stats will also be in the same container as PSF.
    // This means that powershell, and any windows it makes, will have the same restrictions as PSF.

    // NOTE: The documentation for CreateProcess is both sparse and a bit misleading in that it implies that PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE
    //       can be used to force a child process to be inside the container, but it really does the opposite.  It forces the child process to be outside the container.

    DWORD AttributeForCreateInContainerAndPreventBreakaway = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE; //0x02;
    // If this process is inside the container, the child automatically will also be inside the container.
    // This setting sets the default for any child process to also be inside the container, and prevents breakaway from the container (if possible).
    // If this process were outside of the container, it would be ignored as irrelevant.

    DWORD AttributeForCreateInContainerAndAllowBreakaway = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE; //0x01;
    // If this process is inside the container, the child automatically will also be inside the container.
    // This setting sets the default for any child process to be outside the container.  We probably don't need this.
    // If this process were outside of the container, it would be ignored as irrelevant.

    //DWORD AttributeCreateInContainerAttribute = ???; 
    // There is no ability to set this.

    DWORD AttributeForCreateOutsideContainer = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE | PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE; // 0x05;
    // If this process is inside the container, the child process will be outside the container (if possible).  
    // If the process is outside the container, this setting has no effect.
    // In both cases, the enable_process_tree has no effect, but doesn't hurt either.

    // Process Protection Level attribute is completely unrelated to the Desktop App Policy attribute.
    // We are tracking it, but not doing anything with it right now.
    // Processes running inside the container run at a different level (Low) that uncontained processes,
    // and the default behavior is to have the child process run at the same level.  This can be overridden
    // by using PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL.
    //
    // The code here currently does not set this level as we prefer to keep the same level anyway.
    // UPDATE: If we provide an attribute list we must include this for certain processes anyway.
    DWORD attProtLevel = ProcThreadAttributeProtectionLevel;
    DWORD protectionLevel = PROTECTION_LEVEL_SAME;

    // Should it become necessary to change more than one attribute, the number of attributes will need
    // to be modified in both initialization calls in the CTOR.

    std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST> attributeList;
    bool requiresCleanup = false;

public:

    MyProcThreadAttributeList(bool setContainer, bool inside, bool setProtSame)
    {
        DWORD ReservedMustBeZero = 0;
        DWORD countAtt = 0;
        if (setContainer)
        {
            countAtt++;
        }
        if (setProtSame)
        {
            countAtt++;
        }
        // For example of this code with two attributes see: https://github.com/microsoft/terminal/blob/main/src/server/Entrypoints.cpp
        SIZE_T AttributeListSize = 0;
        InitializeProcThreadAttributeList(nullptr, countAtt, ReservedMustBeZero, &AttributeListSize);
        attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(new char[AttributeListSize]));
        //attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(HeapAlloc(GetProcessHeap(), 0, AttributeListSize)));
        requiresCleanup = true;
        InitializeProcThreadAttributeList( attributeList.get(), countAtt, ReservedMustBeZero, &AttributeListSize);

        if (setContainer)
        {
            // 18 stands for
            // PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
            // this is the attribute value we want to add
            if (inside)
            {
                bool b = UpdateProcThreadAttribute( attributeList.get(), ReservedMustBeZero,
                                                    ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),  //ProcThreadAttributeDesktopAppPolicy = 18
                                                    //&createPreventBreakawayAttribute, sizeof(createPreventBreakawayAttribute),
                                                    &AttributeForCreateInContainerAndPreventBreakaway, sizeof(AttributeForCreateInContainerAndPreventBreakaway),
                                                    nullptr, nullptr);
                if (!b)
                {
                    ;
                }
            }
            else
            {
                bool b = UpdateProcThreadAttribute( attributeList.get(), ReservedMustBeZero,
                                                    ProcThreadAttributeValue(18, FALSE, TRUE, FALSE),
                                                    &AttributeForCreateOutsideContainer, sizeof(AttributeForCreateOutsideContainer),
                                                    nullptr, nullptr);
                if (!b)
                {
                    ;
                }
            }
        }

        if (setProtSame)
        {
            // 11 stands for
            // PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL
            // this is the attribute value we want to add
            //
            BOOL additive = FALSE;
            if (countAtt == 2)
                additive = TRUE;
            bool b2 = UpdateProcThreadAttribute(
                attributeList.get(),
                ReservedMustBeZero,
                ProcThreadAttributeValue(attProtLevel, FALSE, TRUE, additive),
                &protectionLevel,
                sizeof(protectionLevel),
                nullptr,
                nullptr);
            if (!b2)
            {
                //	"Could not update Proc thread attribute for PROTECTION_LEVEL.");
                ;
            }
        }
    }

    MyProcThreadAttributeList(SIH_PROC_THREAD_ATTRIBUTE_LIST *AttributeListInput, bool setContainer, bool inside, Json_Debug_Levels debugLevel, const wchar_t* moduleName, DWORD instance)
    {
        DWORD ReservedMustBeZero = 0;
        DWORD countAtt = 0;
        DWORD haveSet_desktop_app_policy = false;
        if (AttributeListInput != NULL)
        {
            countAtt = AttributeListInput->Count;

            if ((AttributeListInput->dwflags & SIH_PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY) != 0)
            {
                // We don't need to add the policy, just fix it.
                for (ULONG inx = 0; inx < AttributeListInput->Count; inx++)
                {
                    SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry = AttributeListInput->Entry[inx];
                    if (Entry.Attribute == PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY)
                    {
                        if (Entry.cbSize == 4)
                        {
                            haveSet_desktop_app_policy = true;
                            DWORD attval = *((DWORD*)(Entry.lpvalue));
                            if ((attval & PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE) == 0)
                            {
                                if (setContainer && inside)
                                {
                                    //attval &= ~PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE;
                                    //attval |= PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE;
                                    //attval &= ~PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE;
                                    attval = AttributeForCreateInContainerAndPreventBreakaway;
                                }
                            }
                            else
                            {
                                if (setContainer && !inside)
                                {
                                    //attval |= PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE;
                                    //attval &= ~PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_DISABLE_PROCESS_TREE;
                                    //attval |= PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_ENABLE_PROCESS_TREE;
                                    attval = AttributeForCreateOutsideContainer;
                                }
                            }
                            *((DWORD*)(Entry.lpvalue)) = attval;  // we just update the value already in place
                        }
                    }
                }
            }
        }
        if (!haveSet_desktop_app_policy)
        {
            countAtt++;
        }


        if (AttributeListInput && !haveSet_desktop_app_policy) // AttributeListInput->Count != countAtt)
        {
#if ADDASNEWREPLACEMENT
            // We need to add an extra attribute, so we'll replace the entire set.
            Log(debugLevel, L"[%s%d] MyProcThreadAttributeList: Rebuilding attribute list to add desktop app policy. Need count=%d in list", moduleName, instance, countAtt);
            SIZE_T AttributeListSize=0;
            BOOL ugh=false;
            InitializeProcThreadAttributeList(nullptr, countAtt, ReservedMustBeZero, &AttributeListSize);
            attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(new char[AttributeListSize]));
            InitializeProcThreadAttributeList( attributeList.get(),  countAtt,
                                                ReservedMustBeZero, &AttributeListSize);
            bool newListOK = true;
            for (ULONG inx = 0; inx < AttributeListInput->Count; inx++)
            {
                SIH_PROC_THREAD_ATTRIBUTE_ENTRY Entry = AttributeListInput->Entry[inx];
                if (Entry.lpvalue > (void*)3)
                {                   
                    ugh = UpdateProcThreadAttribute(attributeList.get(), ReservedMustBeZero,
                        Entry.Attribute,
                        Entry.lpvalue, Entry.cbSize,
                        nullptr, nullptr);
                }
                else
                {
                    // These values are not pointers and must be dealt with differently
                    //HANDLE fake = Entry.lpvalue;
                    ugh = UpdateProcThreadAttribute(attributeList.get(), ReservedMustBeZero,
                        ProcThreadAttributePseudoConsole,
                        //&fake, sizeof(HANDLE),
                        &(Entry.lpvalue), Entry.cbSize,
                        nullptr, nullptr);
                }
                if (!ugh)
                {
                    Log(debugLevel, L"[%s%d] MyProcThreadAttributeList: Could not add original index=0x%d err=0x%x.", moduleName, instance, inx,GetLastError());
                    newListOK = false;
                }
            }
            if (!newListOK)
            {
                Log(debugLevel, L"[%s%d] MyProcThreadAttributeList: Just use original.", moduleName, instance );
                attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(AttributeListInput));
                SetLastError(0);
            }
            else
            {
                if (setContainer && inside)
                {
                    ugh = UpdateProcThreadAttribute(attributeList.get(), ReservedMustBeZero,
                        ProcThreadAttributeValue(18, FALSE, TRUE, FALSE), // PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
                        &AttributeForCreateInContainerAndPreventBreakaway, sizeof(AttributeForCreateInContainerAndPreventBreakaway),
                        nullptr, nullptr);
                }
                else if (setContainer)
                {
                    ugh = UpdateProcThreadAttribute(attributeList.get(), ReservedMustBeZero,
                        ProcThreadAttributeValue(18, FALSE, TRUE, FALSE), // PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY
                        &AttributeForCreateOutsideContainer, sizeof(AttributeForCreateOutsideContainer),
                        nullptr, nullptr);
                }
                else
                {
                    ugh = true;
                }
            }
            if (!ugh)
            {
                Log(debugLevel, L"[%s%d] MyProcThreadAttributeList: Could not add Proc thread attribute for DESKTOP_APP_POLICY err=0x%x.", moduleName, instance,GetLastError());
            }
#else
            // Having trouble with the above.  Microsoft never intended us to query an existing attribute list to be able to copy it.
            // Internal data structures not documented and we can see they do some strange things, or we just don't understand!
            // As the default behavior is to always run child processes inside the container, we'll just use what the caller wanted
            attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(AttributeListInput));
#endif
        }
        else
        {
            // we updated the existing list inline and don't need to replace
            attributeList = std::unique_ptr<_PROC_THREAD_ATTRIBUTE_LIST>(reinterpret_cast<_PROC_THREAD_ATTRIBUTE_LIST*>(AttributeListInput));

        }
    }


    ~MyProcThreadAttributeList()
    {
        if (requiresCleanup)
        {
            DeleteProcThreadAttributeList(attributeList.get());
        }
    }



    LPPROC_THREAD_ATTRIBUTE_LIST get()
    {
        return attributeList.get();
    }

};
