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



auto RegDeleteKeyImpl = psf::detoured_string_function(&::RegDeleteKeyA, &::RegDeleteKeyW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey)
{

    DWORD RegLocalInstance = ++g_RegIntceptInstance;


    auto result = RegDeleteKeyImpl(key, subKey);
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%d] RegDeleteKey:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));

#ifdef _DEBUG
                Log(L"[%d] RegDeleteKey: Path=%s", RegLocalInstance, keypath.c_str());
#endif
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
                {
#ifdef _DEBUG
                    Log(L"[%d] RegDeleteKey:Fake Success\n", RegLocalInstance);
                    LogCallingModule();
#endif
                    result = 0;
                }
            }
            catch (...)
            {
                Log(L"[%d] RegDeleteKey logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%d] RegDeleteKey:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyImpl, RegDeleteKeyFixup);
