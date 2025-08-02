//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if _DEBUG
//#define _ManualDebug 1
//#define MOREDEBUG 1
#include <thread>
#include <windows.h>
#endif

#include <psf_framework.h>
#include <psf_logging.h>

#include "FunctionImplementations.h"
#include "Framework.h"
#include "Reg_Remediation_Spec.h"
#include "Logging.h"
#include <regex>
#include "RegRemediation.h"

#if _DEBUG
#if DEBUG_NEW_FIXUPS 
#define DEBUG_NEW_FIXUPS_REGLEG 1
#endif
#endif

DWORD g_RegInterceptInstance = 50000;
const wchar_t* g_RegModuleName = L"R";





// Also needed (but we need a Psf detour for apis other than string function pairs):
//  RegOpenClassesRoot
//  RegOpenCurrentUser


