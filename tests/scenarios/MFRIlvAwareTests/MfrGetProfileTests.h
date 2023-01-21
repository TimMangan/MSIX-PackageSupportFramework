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


struct MfrGetProfileSectionNamesTest
{
    std::string  TestName;
    bool         enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    DWORD        Expected_Result_Length;
    DWORD        Expected_Result_NumberStrings;
};

struct MfrGetProfileSectionTest
{
    std::string  TestName;
    bool         enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    std::wstring Section;
    DWORD        Expected_Result_Length;
    std::wstring Expected_Result_String;
};

struct MfrGetProfileIntTest
{
    std::string  TestName;
    bool         enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    std::wstring Section;
    std::wstring KeyName;
    int          nDefault;
    DWORD        Expected_Result;
};


struct MfrGetProfileStringTest
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
