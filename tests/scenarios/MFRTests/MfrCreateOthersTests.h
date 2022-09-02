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



struct MfrCreateHardLinkTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathNewHardLinkFrom;
    std::wstring TestPathExistingTo;
    bool Expected_Result;
    DWORD Expected_LastError;
};



struct MfrCreateSymbolicLinkTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPathNewSymbolicLinkFrom;
    std::wstring TestPathExistingTo;
    DWORD flag;
    bool Expected_Result;
    DWORD Expected_LastError;
};