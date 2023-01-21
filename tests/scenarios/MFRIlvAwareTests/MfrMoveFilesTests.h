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


struct MfrMoveFileTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathSource;
    std::wstring TestPathDestination;
    bool Expected_Success;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};


struct MfrMoveFileExTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathSource;
    std::wstring TestPathDestination;
    DWORD flags;
    bool Expected_Success;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};


struct MfrMoveFileWithProgressTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathSource;
    std::wstring TestPathDestination;
    DWORD flags;
    bool Expected_Success;
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};
