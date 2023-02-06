#pragma once

//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>




///
// getLocalPipeName
// Given an input string, prefixs the pipe path with a UWP accepted pipe path
//  - Converts path: \\.\pipe\<token>
//               to: \\.\pipe\LOCAL\<token>
extern std::wstring AdjustLocalPipeName(std::wstring pipeName);
extern std::string AdjustLocalPipeName(std::string  pipeName);
