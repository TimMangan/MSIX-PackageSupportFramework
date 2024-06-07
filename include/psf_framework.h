//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if _DEBUG
#define MOREDEBUG 1
#else
//#define MOREDEBUG 1
#endif

#define DEBUG_NEW_FIXUPS 1

#include <algorithm>
#include <type_traits>

#include <windows.h>

#include "psf_runtime.h"
#include "win32_error.h"

// Sections where the detour mappings are stored so that DllMain can enumerate through them
// NOTE: Sections with the same name before the '$' get merged into a single section. The order of data inside the
// section gets sorted alphabetically w.r.t. the name after the '$'. Other than that, the only other guarantee is that
// the "sub-sections" are pointer aligned and any extra data is initialized with zero. Therefore, we use sub-sections
// 'a' and 'z' to indicate the start/end and sub-section 'm' to store the actual data. Internally, we store pointers to
// the actual data to (1) ensure alignment and avoid splicing, and (2) leverage nullptr checks to ignore padded data
#pragma section("psf$a", read)
#pragma section("psf$m", read)
#pragma section("psf$z", read)

namespace psf
{
    // Representation of the mapping from target function -> detoured function. This is used by DllMain when calling
    // DetourAttach/DetourDetach, updating the `Target` pointer as appropriate. In general, `DECLARE_PSF` should be
    // favored over constructing `fixup` objects directly
    template <typename Func>
    struct detour_pair
    {
        static_assert(std::is_pointer_v<Func> && std::is_function_v<std::remove_pointer_t<Func>>,
            "Must use pointer-to-function type. Did you forget an '&'?");

        Func& Target;
        Func Detour;
        bool Registered = false;
    };

    namespace details
    {
        // fixup<T> ensures that function signatures match, but enumeration requires pointer aliasing as the function
        // pointer types being fixed will differ
        struct detour_function_pair
        {
            void*& Target;
            void* Detour;
            bool Registered;
        };
        static_assert(sizeof(detour_pair<void(*)()>) == sizeof(detour_function_pair));

        inline __declspec(allocate("psf$a")) detour_function_pair* const fixups_begin_v = nullptr;
        inline __declspec(allocate("psf$z")) detour_function_pair* const fixups_end_v = nullptr;

        // NOTE: Bug in MSVC compiler emits incorrect pointer arithmetic for constexpr expressions; const as workaround
        inline const auto fixups_begin = &fixups_begin_v + 1;
        inline const auto fixups_end = &fixups_end_v;
    }

    inline void attach_all()
    {
        std::for_each(details::fixups_begin, details::fixups_end, [](details::detour_function_pair* target)
        {
            if (target && !target->Registered)
            {
                if (target->Target != nullptr)
                {
                    check_win32(::PSFRegister(&target->Target, target->Detour));
                   target->Registered = true;
                }
                else
                {
                    ;
                }
            }
        });
    }

//#if _DEBUG
#if MOREDEBUG
    inline int attach_count_all()
    {
        int count = 0;
        std::for_each(details::fixups_begin, details::fixups_end, [&](details::detour_function_pair* target)
        {
            if (target && !target->Registered)
            {
                check_win32(::PSFRegister(&target->Target, target->Detour));
                target->Registered = true;
                count++;
            }
        });
        return count;
    }

    inline void pvtLog(const wchar_t* fmt, ...)
    {
        try
        {
            va_list args;
            va_start(args, fmt);

            std::wstring wstr;
            wstr.resize(256);
            std::size_t count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args);
            va_end(args);

            if (count > wstr.size())
            {
                count = 1024;       // vswprintf actually returns a negative number, let's just go with something big enough for our long strings; it is resized shortly.
                wstr.resize(count);
                va_list args2;
                va_start(args2, fmt);
                count = std::vswprintf(wstr.data(), wstr.size() + 1, fmt, args2);
                va_end(args2);
            }
            wstr.resize(count);
            ::OutputDebugStringW(wstr.c_str());
        }
        catch (...)
        {
            ::OutputDebugStringA("Exception in wide Log()");
            ::OutputDebugStringW(fmt);
        }
       
    }

    inline void attach_count_all_debug()
    {
        
        std::for_each(details::fixups_begin, details::fixups_end, [](details::detour_function_pair* target)
        {
            if (target && !target->Registered)
            {
                if (target->Target != nullptr)
                {
#if MOREDEBUG
#if DEBUG_NEW_FIXUPS
                    pvtLog(L">>>>>Register FIXUP from 0x0%x to 0x0%x", target->Target, target->Detour);
                    if (target->Target == (void*)0x743d77a0)   //ws_ShellExecuteExW
                    {
                        pvtLog(L">>>>>Registering (WS)ShellExecuteExW");
                    }
                    if (target->Target == (void*)0x743d78e0)   //ws_ShellExecuteW
                    {
                        pvtLog(L">>>>>Registering (WS)ShellExecuteW");
                    }
                    if (target->Target == (void*)0x75cac880)    // KernelBase MoveFileExW
                    {
                        pvtLog(L">>>>>Registering (KernelBase)MoveFileExW");
                    }
                    if (target->Target == (void*)0x77306a40)    // ntdll ZwQueryDirectoryFile
                    {
                        pvtLog(L">>>>>Registering (ntdll)ZwQueryDirectoryFile");
                    }
#endif
#endif
                    check_win32(::PSFRegister(&target->Target, target->Detour));
                    target->Registered = true;
#if MOREDEBUG
#if DEBUG_NEW_FIXUPS
                    pvtLog(L"<<<<<Registered FIXUP Complete.\n");
#endif
#endif
                }
                else
                {
#if MOREDEBUG
#if DEBUG_NEW_FIXUPS
                    pvtLog(L">>>>>Register FIXUP was null target, skipped.\n");
#endif
#endif
                }
            }
        });
        return;
    }
#endif

    inline void detach_all()
    {
        std::for_each(details::fixups_begin, details::fixups_end, [](details::detour_function_pair* target)
        {
            if (target && target->Registered)
            {
                // Best effort; ignore failures since there's not much we can do
                ::PSFUnregister(&target->Target, target->Detour);
                target->Registered = false;
            }
        });
    }

    // Useful helper for determining if a function is ANSI, e.g. for simpler std::conditional_t arguments
    template <typename CharT>
    constexpr bool is_ansi = std::is_same_v<CharT, char>;

    // Make writing character encoding-agnostic code slightly easier
    template <typename CharT>
    inline constexpr auto select_string([[maybe_unused]] const char* ansi, [[maybe_unused]] const wchar_t* wide)
    {
        if constexpr (is_ansi<CharT>)
        {
            return ansi;
        }
        else
        {
            return wide;
        }
    }

    // Used as a helper when two variants of a function exist - "ansi" and "wide" - but the implementation should be
    // practically identical, minus a few character-specific function calls. An example of its use:
    //
    //  auto OutputDebugStringImpl = fixups::detoured_string_function(&::OutputDebugStringA, &::OutputDebugStringW);
    //
    //  template <typename CharT>
    //  void OutputDebugStringFixup(const CharT* output)
    //  {
    //      // Perhaps do something with 'output'...
    //      OutputDebugStringImpl(output);
    //  }
    //  DECLARE_STRING_FIXUP(OutputDebugStringImpl, OutputDebugStringFixup);
    template <typename AnsiFunc, typename WideFunc>
    struct detoured_string_function_t
    {
        AnsiFunc ansi;
        WideFunc wide;

        template <typename... Args>
        inline auto operator()(Args&&... args)
        {
            constexpr bool is_ansi_call = std::is_invocable_v<AnsiFunc, Args...>;
            constexpr bool is_wide_call = std::is_invocable_v<WideFunc, Args...>;

            static_assert(is_ansi_call || is_wide_call, "Could not deduce target function. Neither accept the provided arguments");
            static_assert(!is_ansi_call || !is_wide_call, "Could not deduce target function. Both are viable options");

            using char_type = std::conditional_t<is_ansi_call, char, wchar_t>;
            return invoke<char_type>(std::forward<Args>(args)...);
        }

        template <
            typename CharT,
            typename... Args,
            std::enable_if_t<is_ansi<CharT> ?
                std::is_invocable_v<AnsiFunc, Args...> :
                std::is_invocable_v<WideFunc, Args...>,
            int> = 0>
        inline auto invoke(Args&&... args)
        {
            if constexpr (std::is_same_v<CharT, char>)
            {
                return ansi(std::forward<Args>(args)...);
            }
            else
            {
                return wide(std::forward<Args>(args)...);
            }
        }
    };

    template <typename AnsiFunc, typename WideFunc>
    inline auto detoured_string_function(AnsiFunc ansi, WideFunc wide)
    {
#if MOREDEBUG
#if DEBUG_NEW_FIXUPS
        if (ansi != nullptr && wide != nullptr)
        {
            pvtLog(L">>>>>Define FIXUP pair Ansi=0x0%p Wide=0x0%p\n", ansi, wide);
        }
        else
        {
            pvtLog(L">>>>>Define FIXUP null pair\n");
        }
#endif
#endif
        return detoured_string_function_t<AnsiFunc, WideFunc>{ ansi, wide };
    }
}

#define SELECT_STRING(CharT, String) psf::select_string<CharT>(String, L##String)

// Force a reference to symbols so that the linker does not optimize away due to /OPT:REF
#if defined(_M_IX86)
#define PSF_LINKER_INCLUDE(Name) __pragma(comment(linker, "/include:_" #Name))
#else
#define PSF_LINKER_INCLUDE(Name) __pragma(comment(linker, "/include:" #Name))
#endif

#define DECLARE_FIXUP(TargetFunc, DetouredFunc) \
    static psf::detour_pair<decltype(TargetFunc)> DetouredFunc##_Fixup{ TargetFunc, DetouredFunc }; \
    extern "C" __declspec(allocate("psf$m")) auto DetouredFunc##_Fixup_v = &DetouredFunc##_Fixup; \
    PSF_LINKER_INCLUDE(DetouredFunc##_Fixup_v)

#define DECLARE_STRING_FIXUP(StringFunctions, DetouredFunc) \
    static psf::detour_pair<decltype(StringFunctions.ansi)> DetouredFunc##Ansi_Fixup{ StringFunctions.ansi, DetouredFunc<char> }; \
    static psf::detour_pair<decltype(StringFunctions.wide)> DetouredFunc##Wide_Fixup{ StringFunctions.wide, DetouredFunc<wchar_t> }; \
    extern "C" __declspec(allocate("psf$m")) auto DetouredFunc##Ansi_Fixup_v = &DetouredFunc##Ansi_Fixup; \
    extern "C" __declspec(allocate("psf$m")) auto DetouredFunc##Wide_Fixup_v = &DetouredFunc##Wide_Fixup; \
    PSF_LINKER_INCLUDE(DetouredFunc##Ansi_Fixup_v) \
    PSF_LINKER_INCLUDE(DetouredFunc##Wide_Fixup_v)

#ifdef PSF_DEFINE_EXPORTS
extern "C" {

int __stdcall PSFInitialize() noexcept try
{
    psf::attach_all();
    return ERROR_SUCCESS;
}
catch (...)
{
    return win32_from_caught_exception();
}

int __stdcall PSFUninitialize() noexcept try
{
    psf::detach_all();
    return ERROR_SUCCESS;
}
catch (...)
{
    return win32_from_caught_exception();
}

#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:PSFInitialize=_PSFInitialize@0")
#pragma comment(linker, "/EXPORT:PSFUninitialize=_PSFUninitialize@0")
#else
#pragma comment(linker, "/EXPORT:PSFInitialize=PSFInitialize")
#pragma comment(linker, "/EXPORT:PSFUninitialize=PSFUninitialize")
#endif

}
#endif
