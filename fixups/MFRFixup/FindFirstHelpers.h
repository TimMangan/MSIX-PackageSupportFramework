#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <errno.h>
#include "FunctionImplementations.h"

#include "ManagedPathTypes.h"
#include "PathUtilities.h"

#include <psf_logging.h>
#include <memory>
#include "FindData3.h"
#include "DetermineCohorts.h"


/////////////////////////////////////////
// Functions to manage the found data.
/////////////////////////////////////////
template <typename CharT>
using win32_find_data_t = std::conditional_t<psf::is_ansi<CharT>, WIN32_FIND_DATAA, WIN32_FIND_DATAW>;

extern DWORD copy_find_data(const WIN32_FIND_DATAW& from, WIN32_FIND_DATAA& to) noexcept;
extern DWORD copy_find_data(const WIN32_FIND_DATAW& from, WIN32_FIND_DATAW& to) noexcept;
extern DWORD copy_find_data(const WIN32_FIND_DATAA& from, WIN32_FIND_DATAA& to) noexcept;
extern DWORD copy_find_data(const WIN32_FIND_DATAA& from, WIN32_FIND_DATAW& to) noexcept;

extern int ChooseIndexForFindResult(const Cohorts cohorts, FindData3A* result);
extern int ChooseIndexForFindResult(const Cohorts cohorts, FindData3W* result);

// 8.3 names: when the chosen entry comes from the redirection area (NTFS) but the same entry also exists in the
// package or native location, return the short name of that location (possibly none, e.g. on a CimFS app attach
// volume). Otherwise an application builds a short path such as <PackageRoot>\VFS\PROGRA~1\... from the redirected
// folder's short name, which does not exist in the package and fails with ERROR_PATH_NOT_FOUND.
template <typename FindData3T, typename FindDataT>
inline void UseAlternateNameOfPackageOrNative(FindData3T* result, int useIndex, FindDataT& out) noexcept
{
    if (useIndex != Result_Redirected)
    {
        return;
    }
    for (int other : { Result_Package, Result_Native })
    {
        if (result->find_handles[other] &&
            CompareStringOrdinal(widen(result->cached_data[other].cFileName).c_str(), -1, widen(result->cached_data[useIndex].cFileName).c_str(), -1, TRUE) == CSTR_EQUAL)
        {
            auto alt = widen(result->cached_data[other].cAlternateFileName);
            if constexpr (std::is_same_v<std::remove_reference_t<decltype(out.cAlternateFileName[0])>, char>)
            {
                strcpy_s(out.cAlternateFileName, narrow(alt).c_str());
            }
            else
            {
                wcscpy_s(out.cAlternateFileName, alt.c_str());
            }
            return;
        }
    }
}
