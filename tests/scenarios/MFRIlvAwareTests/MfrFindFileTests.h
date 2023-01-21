#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <psf_runtime.h>

#include <test_config.h>

///#include "common_paths.h"
#include "file_paths.h"
#include "MFRConsts.h"

struct MfrFindFileTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    bool Expected_Result;  // true if expect a handle, false otherwise
    DWORD Expected_FilesFound;
    DWORD Expected_LastError;
};


struct MfrFindFileExTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    FINDEX_INFO_LEVELS info_levels;
    FINDEX_SEARCH_OPS  search_ops;
    DWORD additional_flags;
    bool Expected_Result;  // true if expect a handle, false otherwise
    DWORD Expected_FilesFound;
    DWORD Expected_LastError;
};


