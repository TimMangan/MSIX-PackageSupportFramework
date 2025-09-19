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

extern std::wstring DetermineIlvPathForReadOperations(Json_Debug_Levels debugRequestLevel, Cohorts cohorts, DWORD dllInstance);

extern std::wstring DetermineIlvPathForWriteOperations(Json_Debug_Levels debugRequestLevel, Cohorts cohorts, DWORD dllInstance);

extern bool IsThisALocalPathNow(std::wstring path);
extern bool IsThisAPackagePathNow(std::wstring path);

extern std::wstring SelectLocalOrPackageForRead(std::wstring localPath, std::wstring packagePath);

extern void PreCreateLocalFoldersIfNeededForWrite(Json_Debug_Levels debugRequestLevel, std::wstring localPath, std::wstring packagePath, DWORD dllInstance, std::wstring debugString );

extern void PreCreatePackageFoldersIfIlvNeededForWrite(Json_Debug_Levels debugRequestLevel, std::wstring localPath, DWORD dllInstance, std::wstring debugString);

extern void CowLocalFoldersIfNeededForWrite(Json_Debug_Levels debugRequestLevel, std::wstring localPath, std::wstring packagePath, DWORD dllInstance, std::wstring debugString);

extern bool IsThisUnsupportedForInterceptsNow(std::wstring path);