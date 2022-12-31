#pragma once

//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <known_folders.h>


using namespace std::literals;


extern std::filesystem::path FID_System32;
extern std::filesystem::path FID_Windows;
extern std::filesystem::path FID_SystemX86;
extern std::filesystem::path FID_ProgramFilesCommonX86;
extern std::filesystem::path FID_ProgramFilesX86;
#if !_M_IX86
extern std::filesystem::path FID_ProgramFilesCommonX64;
extern std::filesystem::path FID_ProgramFilesX64;
#endif
extern std::filesystem::path FID_UserProgramFiles;
extern std::filesystem::path FID_Fonts;
extern std::filesystem::path FID_ProgramData;
extern std::filesystem::path FID_LocalAppDataLow;
extern std::filesystem::path FID_LocalAppData;
extern std::filesystem::path FID_RoamingAppData;
extern std::filesystem::path FID_CommonPrograms;
extern std::filesystem::path FID_Desktop;
extern std::filesystem::path FID_Documents;
extern std::filesystem::path FID_Profile;
extern std::filesystem::path FID_PublicDesktop;
extern std::filesystem::path FID_PublicDocuments;
extern std::filesystem::path FID_RootDrive;

extern void FID_Initialize();