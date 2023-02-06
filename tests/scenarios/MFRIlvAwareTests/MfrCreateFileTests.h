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

struct MfrCreateFileTest
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    DWORD dwDesiredAccess;
    DWORD dwShareMode;
    LPSECURITY_ATTRIBUTES lpSecurityAttributes;
    DWORD CreationDisposition;
    DWORD FlagAndAttributes;
    bool Expected_Result;  // true if expect a handle, false otherwise
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};


struct MfrCreateFile2Test
{
    std::string TestName;
    bool enabled;
    bool cleanupWritablePackageRoot;
    bool cleanupDocumentsSubfolder;
    std::wstring TestPath;
    DWORD dwDesiredAccess;
    DWORD dwShareMode;
    DWORD CreationDisposition;
    ///LPCREATEFILE2_EXTENDED_PARAMETERS createExParams;
       DWORD Attributes;
       DWORD Flags;
       DWORD dwSecurityQosFlags;
    bool Expected_Result;  // true if expect a handle, false otherwise
    DWORD Expected_LastError;
    bool AllowAlternateError = false;
    DWORD AlternateError;
};



