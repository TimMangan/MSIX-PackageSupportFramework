#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include <vector>
#include <psf_framework.h>
#include <psf_logging.h>

#if _DEBUG
extern std::vector<std::wstring> DebugPathTestingList;

extern void DebugPathTesting(DWORD dllInstance);
#endif