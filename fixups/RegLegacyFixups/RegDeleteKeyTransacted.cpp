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


auto RegDeleteKeyTransactedImpl = psf::detoured_string_function(&::RegDeleteKeyTransactedA, &::RegDeleteKeyTransactedW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyTransactedFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved,
    HANDLE hTransaction,
    PVOID  pExtendedParameter)
{

    DWORD RegLocalInstance = ++g_RegIntceptInstance;


    auto result = RegDeleteKeyTransactedImpl(key, subKey, viewDesired, Reserved, hTransaction, pExtendedParameter);
    auto functionResult = from_win32(result);
 
    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%d] RegDeleteKeyTransacted:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));
#ifdef _DEBUG
                Log(L"[%d] RegDeleteKeyTransacted: Path=%s", RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#endif
                {
#ifdef _DEBUG
                    Log(L"[%d] RegDeleteKeyTransacted:Fake Success\n", RegLocalInstance);
                    LogCallingModule();
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%d] RegDeleteKeyTransacted logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%d] RegDeleteKeyTransacted:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyTransactedImpl, RegDeleteKeyTransactedFixup);
