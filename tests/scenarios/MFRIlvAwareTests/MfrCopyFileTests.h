#pragma once
// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <psf_runtime.h>

#include <test_config.h>

///#include "common_paths.h"
#include "file_paths.h"
#include "MFRConsts.h"


struct MfrCopyFileTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathSource;
    std::wstring TestPathDestination;
    bool FailIfExists;
    DWORD Expected_Result;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};


struct MfrCopyFileExTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathSource;
    std::wstring TestPathDestination;
    DWORD dwCopyFlags;
    bool Expected_Result;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};

struct MfrCopyFile2Test
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathSource;
    std::wstring TestPathDestination;
    COPYFILE2_EXTENDED_PARAMETERS* extendedParameters;
    HRESULT Expected_Result;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};
