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

struct MfrAttributeTest
{
    std::string TestName;
    bool enabled;
    std::wstring TestPath;
    DWORD Expected_Result;
    DWORD Expected_LastError;
};


struct MfrAttributeExTest
{
    std::string TestName;
    bool enabled;
    std::wstring TestPath;
    bool Expect_Success;
    DWORD Expected_LastError;
};

struct MfrSetAttributeTest
{
    std::string TestName;
    bool enabled;
    std::wstring TestPath;
    DWORD fileAttributes;
    bool Expect_Success;
    DWORD Expected_AttributeWhenTested;
    DWORD Expected_LastError;
};