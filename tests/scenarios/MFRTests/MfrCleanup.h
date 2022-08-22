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

#define MFRTESTDOCS L"MFRTestDocs"

extern DWORD MfrCleanupWritablePackageRoot();
extern DWORD MfrCleanupLocalDocuments(std::wstring subfoldername);