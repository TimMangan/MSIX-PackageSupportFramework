#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>
#include "ManagedFileMappings.h"
#include "ManagedPathTypes.h"
#include "DetermineCohorts.h"

extern std::wstring DetermineIlvPathForReadOperations(Cohorts cohorts, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug);

extern std::wstring DetermineIlvPathForWriteOperations(Cohorts cohorts, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug);

extern bool IsThisALocalPathNow(std::wstring path);

extern std::wstring SelectLocalOrPackageForRead(std::wstring localPath, std::wstring packagePath);

extern void PreCreateLocalFoldersIfNeededForWrite(std::wstring localPath, std::wstring packagePath, DWORD dllInstance,  bool debug,  std::wstring debugString );

extern void CowLocalFoldersIfNeededForWrite(std::wstring localPath, std::wstring packagePath, DWORD dllInstance, bool debug, std::wstring debugString);

extern bool IsThisUnsupportedForInterceptsNow(std::wstring path);