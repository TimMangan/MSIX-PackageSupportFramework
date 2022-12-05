//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FID.h"

std::filesystem::path FID_System32;
std::filesystem::path FID_Windows;
std::filesystem::path FID_SystemX86;
std::filesystem::path FID_ProgramFilesCommonX86;
std::filesystem::path FID_ProgramFilesX86;
#if !_M_IX86
std::filesystem::path FID_ProgramFilesCommonX64;
std::filesystem::path FID_ProgramFilesX64;
#endif
std::filesystem::path FID_Fonts;
std::filesystem::path FID_ProgramData;
std::filesystem::path FID_LocalAppDataLow;
std::filesystem::path FID_LocalAppData;
std::filesystem::path FID_RoamingAppData;
std::filesystem::path FID_CommonPrograms;
std::filesystem::path FID_Desktop;
std::filesystem::path FID_Documents;
std::filesystem::path FID_Profile;
std::filesystem::path FID_PublicDesktop;
std::filesystem::path FID_PublicDocuments;
std::filesystem::path FID_RootDrive;

void FID_Initialize()
{
    // The FID variables contain the path on this system for the known mappings that are used by the Microsoft MSIX Packaging tool when packaging an app.
    // This list is undocumented and may need to be enhanced in the future.
    // 
    // This initialization will ensure that the MFRFixup should only need to ask the system once for them.
    FID_System32 =              psf::known_folder(FOLDERID_System);
    FID_Windows =               psf::known_folder(FOLDERID_Windows);
    FID_SystemX86 =             psf::known_folder(FOLDERID_SystemX86);
    FID_ProgramFilesCommonX86 = psf::known_folder(FOLDERID_ProgramFilesCommonX86);
    FID_ProgramFilesX86 =       psf::known_folder(FOLDERID_ProgramFilesX86);
#if !_M_IX86
    FID_ProgramFilesCommonX64 = psf::known_folder(FOLDERID_ProgramFilesCommonX64);
    FID_ProgramFilesX64 =       psf::known_folder(FOLDERID_ProgramFilesX64);
#endif
    FID_Fonts =                 psf::known_folder(FOLDERID_ProgramFilesX86);
    FID_ProgramData =           psf::known_folder(FOLDERID_ProgramData);
    FID_LocalAppDataLow =       psf::known_folder(FOLDERID_LocalAppDataLow);
    FID_LocalAppData =          psf::known_folder(FOLDERID_LocalAppData);
    FID_RoamingAppData =        psf::known_folder(FOLDERID_RoamingAppData);
    FID_CommonPrograms =        psf::known_folder(FOLDERID_CommonPrograms);
    FID_Desktop =               psf::known_folder(FOLDERID_Desktop);
    FID_Documents =             psf::known_folder(FOLDERID_Documents);
    FID_Profile =               psf::known_folder(FOLDERID_Profile);
    FID_PublicDesktop =         psf::known_folder(FOLDERID_PublicDesktop);
    FID_PublicDocuments =       psf::known_folder(FOLDERID_PublicDocuments);
    FID_RootDrive =             FID_Windows.root_name();

}