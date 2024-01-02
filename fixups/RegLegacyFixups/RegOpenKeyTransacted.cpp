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


    DWORD RegLocalInstance = ++g_RegIntceptInstance;

#if _DEBUG
    Log(L"[%d] RegOpenKeyTransacted:\n", RegLocalInstance);
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

#ifdef _DEBUG
    Log("[%d] RegOpenKeyTransacted result=%d", RegLocalInstance, result);
#endif

#ifdef MOREDEBUG
    auto functionResult = from_win32(result);
    if (auto lock = acquire_output_lock(function_type::registry, functionResult))
    {
        try
        {
            LogKeyPath(key);
            if (subKey) LogString(L"Sub Key", subKey);
            LogRegKeyFlags(options);
            Log(L"\n[%d] SamDesired=%s\n", RegLocalInstance, InterpretRegKeyAccess(samDesired).c_str());
            if (samDesired != samModified)
            {
                Log(L"[%d] ModifiedSam=%s\n", RegLocalInstance, InterpretRegKeyAccess(samModified).c_str());
            }
            LogFunctionResult(functionResult);
            if (function_failed(functionResult))
            {
                LogWin32Error(result);
            }
            LogCallingModule();
        }
        catch (...)
        {
            Log(L"[%d] RegOpenKeyTransacted logging failure.\n", RegLocalInstance);
        }
    }
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegOpenKeyTransactedImpl, RegOpenKeyTransactedFixup);

