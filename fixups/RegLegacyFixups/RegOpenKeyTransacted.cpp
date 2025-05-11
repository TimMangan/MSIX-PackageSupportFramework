//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if _DEBUG
//#define _ManualDebug 1
#define MOREDEBUG 1
#include <thread>
#include <windows.h>
#endif

#include <psf_framework.h>
#include <psf_logging.h>
#include "Logging.h"

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


auto RegOpenKeyTransactedImpl = psf::detoured_string_function(&::RegOpenKeyTransactedA, &::RegOpenKeyTransactedW);
template <typename CharT>
LSTATUS __stdcall RegOpenKeyTransactedFixup(
    _In_ HKEY key,
    _In_opt_ const CharT* subKey,
    _In_opt_ DWORD options,         // reserved
    _In_ REGSAM samDesired,
    _Out_ PHKEY resultKey,
    _In_ HANDLE hTransaction,
    _In_ PVOID  pExtendedParameter)  // reserved
{


    DWORD RegLocalInstance = ++g_RegInterceptInstance;

#if _DEBUG
    Log(L"[%s%d] RegOpenKeyTransacted:\n", g_RegModuleName, RegLocalInstance);
#endif
    std::string keyOnlyath = InterpretStringA(subKey);
    std::string keypath = InterpretKeyPath(key) + "\\" + keyOnlyath;
    REGSAM samModified = RegFixupSam(keypath, samDesired, RegLocalInstance);

    std::string sskey = narrow(subKey);
    LSTATUS result = RegFixupDeletionMarker(keyOnlyath, sskey, RegLocalInstance);
    if (result == ERROR_SUCCESS)
    {
        std::string fullpath = keypath;
        if (subKey != NULL)
        {
            fullpath += "\\" + sskey;
        }
        if (!RegFixupJavaBlocker(fullpath, RegLocalInstance))
        {
            result = RegOpenKeyTransactedImpl(key, subKey, options, samModified, resultKey, hTransaction, pExtendedParameter);
        }
        else
        {
            result = ERROR_PATH_NOT_FOUND;
            resultKey = NULL;
        }
    }
    else
    {
        resultKey = NULL;
    }

#if _DEBUG
    Log(L"[%s%d] RegOpenKeyTransacted result=%d", g_RegModuleName, RegLocalInstance, result);
#endif

#if MOREDEBUG
    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        try
        {
            LogKeyPath(RegLocalInstance, key);
            if (subKey) LogString(g_RegModuleName, RegLocalInstance, L"Sub Key", subKey);
            LogRegKeyFlags(RegLocalInstance, options);
            Log(L"\n[%s%d] SamDesired=%s\n", g_RegModuleName, RegLocalInstance, InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log(L"[%s%d] ModifiedSam=%s\n", g_RegModuleName, RegLocalInstance, InterpretRegKeyAccess(samModified).c_str());
            }
            LogCallingModuleInstanceCommon(g_RegModuleName,RegLocalInstance);
            LogFunctionResultInstance(RegLocalInstance, functionResult);
            if (function_failed(functionResult))
            {
                LogWin32ErrorInstance(RegLocalInstance, result);
            }
        }
        catch (...)
        {
            Log(L"[%s%d] RegOpenKeyTransacted logging failure.\n", g_RegModuleName, RegLocalInstance);
        }
    }
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyTransactedImpl, RegOpenKeyTransactedFixup);
