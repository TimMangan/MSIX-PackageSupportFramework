#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>
#include "ManagedFileMappings.h"
#include "ManagedPathTypes.h"
#include <psf_logging.h>

struct Cohorts
{
    std::wstring WsRequested;

    mfr::mfr_path file_mfr;
    mfr::mfr_folder_mapping map;

    std::wstring WsRedirected;
    std::wstring WsPackage;
    std::wstring WsNative;
    bool NativeIsValidOptionInScenario = true;        // Indicates if WsNative is an option for this scenario (not is)
};

extern void DetermineCohorts(Json_Debug_Levels debugRequestLevel, std::wstring requestedPath, Cohorts *cohorts, DWORD dllInstance, const wchar_t* FixupName);