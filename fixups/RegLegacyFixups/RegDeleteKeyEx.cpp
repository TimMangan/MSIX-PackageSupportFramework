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


auto RegDeleteKeyExImpl = psf::detoured_string_function(&::RegDeleteKeyExA, &::RegDeleteKeyExW);
template <typename CharT>
LSTATUS __stdcall RegDeleteKeyExFixup(
    _In_ HKEY key,
    _In_ const CharT* subKey,
    DWORD viewDesired, // 32/64
    DWORD Reserved)
{

    DWORD RegLocalInstance = ++g_RegIntceptInstance;


    auto result = RegDeleteKeyExImpl(key, subKey, viewDesired, Reserved);
    auto functionResult = from_win32(result);

    if (functionResult != from_win32(0))
    {
        if (auto lock = acquire_output_lock(function_type::registry, functionResult))
        {
            try
            {
#if _DEBUG
                Log(L"[%d] RegDeleteKeyEx:\n", RegLocalInstance);
#endif
                std::string keypath = ReplaceAppRegistrySyntax(InterpretKeyPath(key) + "\\" + InterpretStringA(subKey));
#ifdef MOREDEBUG
                Log(L"[%d] RegDeleteKeyEx: Path=%s", RegLocalInstance, keypath.c_str());
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#else
                if (RegFixupFakeDelete(keypath, RegLocalInstance) == true)
#endif
                {
#ifdef MOREDEBUG
                    Log(L"[%d] RegDeleteKeyEx:Fake Success\n", RegLocalInstance);
#endif
                    result = 0;
                    LogCallingModule();
                }
            }
            catch (...)
            {
                Log(L"[%d] RegDeleteKeyEx logging failure.\n", RegLocalInstance);
            }
        }
    }
#if _DEBUG
    Log(L"[%d] RegDeleteKeyEx:Fake returns %d\n", RegLocalInstance, result);
#endif
    return result;
}
DECLARE_STRING_FIXUP(RegDeleteKeyExImpl, RegDeleteKeyExFixup);
