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


struct MfrWriteProfileSectionTest
{
    std::string  TestName;
    bool         enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    std::wstring Section;
    DWORD        Expected_Result_Length;
   wchar_t*      Expected_Result_String;
};


struct MfrWriteProfileStringTest
{
    std::string  TestName;
    bool         enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    std::wstring Section;
    std::wstring KeyName;
    std::wstring sDefault;
    DWORD        Expected_Result_Length;
    std::wstring Expected_Result_String;
};

struct MfrWriteProfileStructTest
{
    std::string  TestName;
    bool         enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    std::wstring Section;
    // tbd
    DWORD        Expected_Result;
};
