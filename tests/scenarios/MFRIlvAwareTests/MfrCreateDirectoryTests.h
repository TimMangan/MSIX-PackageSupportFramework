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

struct MfrCreateDirectoryTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring DirectoryPath;
    DWORD Expected_Result;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};


struct MfrCreateDirectoryExTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TemplatePath;
    std::wstring DirectoryPath;
    DWORD Expected_Result;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};
