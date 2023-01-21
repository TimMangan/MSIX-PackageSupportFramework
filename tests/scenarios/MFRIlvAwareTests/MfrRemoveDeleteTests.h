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

struct MfrRemoveDirectoryTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring OptionalPathPreCreate;
    std::wstring TestPathRemove;
    DWORD Expected_Result_FromRemove;
    DWORD Expected_LastError_FromRemove;
    bool AllowAlternateErrorFromRemove = false;
    DWORD AlternateErrorFromRemove;
    DWORD Expected_Result_FromVerify;
    DWORD Expected_LastError_FromVerify;
};


struct MfrDeleteFileTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring OptionalPathPreCreate;
    std::wstring TestPathRemove;
    DWORD Expected_Result_FromRemove;
    DWORD Expected_LastError_FromRemove;
    bool AllowAlternateErrorFromRemove = false;
    DWORD AlternateErrorFromRemove;
    DWORD Expected_Result_FromVerify;
    DWORD Expected_LastError_FromVerify;
};
