//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ManagedPathTypes.h"
#include "ManagedFileMappings.h"
#include "FID.h"
#include "PathUtilities.h"
#include <psf_logging.h>


#if _DEBUG
#define MOREDEBUG 1
#endif

namespace mfr
{

    std::vector<mfr_folder_mapping> g_MfrFolderMappings;
    std::vector<mfr_vfs_remapping> g_MfrVfsRemappings;

    

    void Initialize_MFR_Mappings()
    {
        // This creates an ordered list of folders such that more specific paths are listed prior to less specific.
        // For example, the C:\Windows\System32\Drivers entry is before C:\Windows\System32 and that is before C:\Windows

        // NOTE: Mappings that would have a different string if requested as a short name need to be in the list below.
        //       Those that never get to more than 8 characters are not.  The User name can be more than 8.
        //       The package has long names for all files, and we sill redirect using long names as well.  We just need
        //       to recognize it to match up.

        FID_Initialize(); 

#if MOREDEBUG
        Log(L"\t\t\t\tMFRFixup FID_Initialize: FID_UserProfiles = %s", FID_UserProfiles.wstring().c_str());
        Log(L"\t\t\t\tMFRFixup FID_Initialize: FID_UserFolder =   %s", FID_UserFolder.wstring().c_str());
        Log(L"\t\t\t\tMFRFixup FID_Initialize: FID_Profile =      %s", FID_Profile.wstring().c_str());

        Log("\t\t\tMFRFixup Initialize_MFR_Mappings: post FID");
#endif  
        // This first set are a set of exact-match special exceptions that we need to be handled as native paths.  The system might layer into the package/redirected paths.
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                        mfr_exactmatchonly_types::exactmatchonly,
                                                        mfr_exclusion_types::not_excluded,
                                                        mfr::mfr_redirect_flags::prefer_redirection_local,
                                                        FID_RootDrive,  // without trailing backslash
                                                        L"AppVPackageDrive",
                                                        L"AppVPackageDrive",
                                                        g_packageVfsRootPath / L"AppVPackageDrive"sv,
                                                        false,
                                                        //g_writablePackageRootPath / L"VFS"sv / L"AppVPackageDrive"sv  
                                                        FID_RootDrive 
                                                        });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                        mfr_exactmatchonly_types::exactmatchonly,
                                                        mfr_exclusion_types::not_excluded,
                                                        mfr::mfr_redirect_flags::prefer_redirection_local,
                                                        FID_RootDrive / L""sv, // and with trailing backslash
                                                        L"AppVPackageDrive",
                                                        L"AppVPackageDrive",
                                                        g_packageVfsRootPath / L"AppVPackageDrive\\"sv,
                                                        false,
                                                        //g_writablePackageRootPath / L"VFS"sv / L"AppVPackageDrive\\"sv    
                                                        FID_RootDrive / L""sv
                                                        });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                        mfr_exactmatchonly_types::exactmatchonly,
                                                        mfr_exclusion_types::not_excluded,
                                                        mfr::mfr_redirect_flags::prefer_redirection_local,
                                                        FID_UserProfiles,
                                                        L"UserProfiles",
                                                        L"UserProfiles",
                                                        g_packageVfsRootPath / L"UserProfiles"sv,
                                                        false,
                                                        //g_writablePackageRootPath / L"VFS"sv / L"UserProfiles"sv  
                                                        FID_UserProfiles
                                                        });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                        mfr_exactmatchonly_types::exactmatchonly,
                                                        mfr_exclusion_types::not_excluded,
                                                        mfr::mfr_redirect_flags::prefer_redirection_local,
                                                        FID_UserProfiles / L""sv,
                                                        L"UserProfiles",
                                                        L"UserProfiles",
                                                        g_packageVfsRootPath / L"UserProfiles\\"sv,
                                                        false,
                                                        //g_writablePackageRootPath / L"VFS"sv / L"UserProfiles\\"sv   
                                                        FID_UserProfiles / L""sv
                                                        });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_UserFolder,                           // This path is the users\username folder
                                                                L"Profile",  
                                                                L"Profile",
                                                                g_packageVfsRootPath / L"Profile"sv,
                                                                false,
                                                                //g_writablePackageRootPath / L"VFS"sv / L"Profile"sv   ExactMarchRedirLocal
                                                                FID_UserFolder
                                                              });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_UserFolder / L""sv,                           // This path is the users\username folder
                                                                L"Profile",
                                                                L"Profile",
                                                                g_packageVfsRootPath / L"Profile"sv,
                                                                false,
                                                                //g_writablePackageRootPath / L"VFS"sv / L"Profile"sv   ExactMarchRedirLocal
                                                                FID_UserFolder / L""sv
                                                            });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_Profile,
                                                                L"Profile\\AppData",
                                                                L"Profile\\AppData",
                                                                g_packageVfsRootPath / L"Profile"sv / L"AppData"sv ,
                                                                false,
                                                                //g_writablePackageRootPath / L"VFS"sv / L"Profile"sv / L"AppData"sv     ExactMarchRedirLocal
                                                                FID_Profile
                                                            });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_Profile / L""sv,
                                                                L"Profile\\AppData",
                                                                L"Profile\\AppData",
                                                                g_packageVfsRootPath / L"Profile"sv / L"AppData"sv ,
                                                                false,
                                                                //g_writablePackageRootPath / L"VFS"sv / L"Profile"sv / L"AppData"sv     ExactMarchRedirLocal
                                                                FID_Profile / L""sv
                                                            });


        // This is the normal ordered list of folders that we will redirect.
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping { /*Valid_mapping =*/ mfr_enabled_types::enabled, 
                                                                /*IsExactMatchOnly */ mfr_exactmatchonly_types::not_exactmatchonly,
                                                                /*IsAnExclusionToRedirect =*/ mfr_exclusion_types::not_excluded,
                                                                /*RedirectionFlags =*/ mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                /*NativePathBase =*/ FID_System32 / LR"(catroot2)"sv,
                                                                /*FolderId =*/ L"FOLDERID_System\\Catroot2",
                                                                /*VFSFolderName =*/ L"AppVSystem32Catroot2",
                                                                /*PackagePathBase =*/ g_packageVfsRootPath / L"AppVSystem32Catroot2"sv,
                                                                /*DoesRuntimeMapNativeToVFS =*/ true,
                                                                /*RedirectedPathBase =*/g_writablePackageRootPath / L"VFS"sv / L"AppVSystem32Catroot2"sv });


        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_System32 / LR"(catroot)"sv,       
                                                                L"FOLDERID_System\\Catroot",        
                                                                L"AppVSystem32Catroot",     
                                                                g_packageVfsRootPath / L"AppVSystem32Catroot"sv,     
                                                                true,  
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppVSystem32Catroot"sv });


        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_System32 / LR"(drivers)"sv / LR"(etc)"sv,
                                                                L"FOLDERID_System\\drivers\\etc",
                                                                L"AppVSystem32DriversEtc",
                                                                g_packageVfsRootPath / L"AppVSystem32DriversEtc"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppVSystem32DriversEtc"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_System32 / LR"(driverstore)"sv,
                                                                L"FOLDERID_System\\driverstore",
                                                                L"AppVSystem32Driverstore", g_packageVfsRootPath / L"AppVSystem32Driverstore"sv, 
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppVSystem32Driverstore"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_System32 / LR"(logfiles)"sv,
                                                                L"FOLDERID_System\\logfiles",
                                                                L"AppVSystem32Logfiles",
                                                                g_packageVfsRootPath / L"AppVSystem32Logfiles"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppVSystem32Logfiles"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_System32 / LR"(spool)"sv,
                                                                L"FOLDERID_System\\spool",
                                                                L"AppVSystem32Spool",
                                                                g_packageVfsRootPath / L"AppVSystem32Spool"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppVSystem32Spool"sv });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_SystemX86,
                                                                L"FOLDERID_SystemX86",
                                                                L"SystemX86",
                                                                g_packageVfsRootPath / L"SystemX86"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"SystemX86"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_ProgramFilesCommonX86,
                                                                L"FOLDERID_ProgramFilesCommonX86",
                                                                L"ProgramFilesCommonX86",
                                                                g_packageVfsRootPath / L"ProgramFilesCommonX86"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesCommonX86"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_ProgramFilesCommonX86),
                                                                L"FOLDERID_ProgramFilesCommonX86",
                                                                L"ProgramFilesCommonX86",
                                                                g_packageVfsRootPath / L"ProgramFilesCommonX86"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesCommonX86"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_ProgramFilesX86,
                                                                L"FOLDERID_ProgramFilesX86",
                                                                L"ProgramFilesX86",
                                                                g_packageVfsRootPath / L"ProgramFilesX86"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesX86"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_ProgramFilesX86),
                                                                L"FOLDERID_ProgramFilesX86",
                                                                L"ProgramFilesX86",
                                                                g_packageVfsRootPath / L"ProgramFilesX86"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesX86"sv });
#if !_M_IX86
        // FUTURE: We may want to consider the possibility of a 32-bit application trying to reference "%windir%\sysnative\"
        //         in which case we'll have to get smarter about how we resolve paths
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_System32,
                                                                L"SystemX64",
                                                                L"SystemX64",
                                                                g_packageVfsRootPath / L"SystemX64"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"SystemX64"sv });
        // FOLDERID_ProgramFilesX64* not supported for 32-bit applications
        // FUTURE: We may want to consider the possibility of a 32-bit process trying to access this path anyway. E.g. a
        //         32-bit child process of a 64-bit process that set the current directory
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_ProgramFilesCommonX64,
                                                                L"ProgramFilesCommonX64",
                                                                L"ProgramFilesCommonX64",
                                                                g_packageVfsRootPath / L"ProgramFilesCommonX64"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesCommonX64"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_ProgramFilesCommonX64),
                                                                L"ProgramFilesCommonX64",
                                                                L"ProgramFilesCommonX64",
                                                                g_packageVfsRootPath / L"ProgramFilesCommonX64"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesCommonX64"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_ProgramFilesX64,
                                                                L"ProgramFilesX64",
                                                                L"ProgramFilesX64",
                                                                g_packageVfsRootPath / L"ProgramFilesX64"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesX64"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_ProgramFilesX64),
                                                                L"ProgramFilesX64",
                                                                L"ProgramFilesX64",
                                                                g_packageVfsRootPath / L"ProgramFilesX64"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"ProgramFilesX64"sv });
#endif
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_Windows / LR"(System)"sv,
                                                                L"System",
                                                                L"System",
                                                                g_packageVfsRootPath / L"System"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"System"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_Fonts,
                                                                L"Fonts",
                                                                L"Fonts",
                                                                g_packageVfsRootPath / L"Fonts"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Fonts"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_Windows / LR"(Microsoft.NET)"sv,
                                                                L"Windows\\Microsoft.NET",
                                                                L"Windows\\Microsoft.NET",
                                                                g_packageVfsRootPath / L"Windows"sv / L"Microsoft.NET"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Windows"sv / L"Microsoft.Net"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_Windows / LR"(SystemApps)"sv,
                                                                L"SystemApps",
                                                                L"SystemApps",
                                                                g_packageVfsRootPath / L"SystemApps"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"SystemApps"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_Windows,
                                                                L"Windows",
                                                                L"Windows",
                                                                g_packageVfsRootPath / L"Windows"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS\\Windows"sv });

        // Exclude the AppRepository from redirection, but add rest of ProgramData.
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_ProgramData / LR"(Microsoft)"sv / LR"(Windows)"sv / LR"(AppRepository)"sv,
                                                                L"Common AppData\\Microsoft\\Windows\\AppRepository",
                                                                L"Common AppData\\Microsoft\\Windows\\AppRepository",
                                                                g_packageVfsRootPath / L"Common AppData"sv / L"Microsoft"sv / L"Windows"sv / L"AppRepository"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS\\Common AppData\\Microsoft\\Windows\\AppRepository"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                ConvertPathToShortPath(FID_ProgramData / LR"(Microsoft)"sv / LR"(Windows)"sv / LR"(AppRepository)"sv),
                                                                L"Common AppData\\Microsoft\\Windows\\AppRepository",
                                                                L"Common AppData\\Microsoft\\Windows\\AppRepository",
                                                                g_packageVfsRootPath / L"Common AppData"sv / L"Microsoft"sv / L"Windows"sv / L"AppRepository"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS\\Common AppData\\Microsoft\\Windows\\AppRepository"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_ProgramData,
                                                                L"Common AppData",
                                                                L"Common AppData",
                                                                g_packageVfsRootPath / L"Common AppData"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Common AppData"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_ProgramData),
                                                                L"Common AppData",
                                                                L"Common AppData",
                                                                g_packageVfsRootPath / L"Common AppData"sv,
                                                                true,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Common AppData"sv });

        // These are additional folders that may appear in MSIX packages and need help
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_LocalAppDataLow,
                                                                L"LocalAppDataLow",
                                                                L"LocalAppDataLow",
                                                                g_packageVfsRootPath / L"LocalAppDataLow"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"LocalAppDataLow"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_LocalAppDataLow),
                                                                L"LocalAppDataLow",
                                                                L"LocalAppDataLow",
                                                                g_packageVfsRootPath / L"LocalAppDataLow"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"LocalAppDataLow"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_LocalAppData / LR"(Temp)"sv,
                                                                L"Local AppData\\Temp",
                                                                L"Local AppData\\Temp",
                                                                g_packageVfsRootPath / L"Local AppData"sv / L"Temp"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Local AppData"sv / L"Temp"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_LocalAppData / LR"(Microsoft)"sv / LR"(Windows)"sv,
                                                                L"Local AppData\\Microsoft\\Windows",
                                                                L"Local AppData\\Microsoft\\Windows",
                                                                g_packageVfsRootPath / L"Local AppData"sv / L"Microsoft"sv / L"Windows"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Local AppData"sv / L"Microsoft"sv / L"Windows"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_LocalAppData / LR"(Microsoft)"sv / LR"(Windows)"sv),
                                                                L"Local AppData\\Microsoft\\Windows",
                                                                L"Local AppData\\Microsoft\\Windows",
                                                                g_packageVfsRootPath / L"Local AppData"sv / L"Microsoft"sv / L"Windows"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Local AppData"sv / L"Microsoft"sv / L"Windows"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_LocalAppData,
                                                                L"Local AppData",
                                                                L"Local AppData",
                                                                g_packageVfsRootPath / L"Local AppData"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Local AppData"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_LocalAppData),
                                                                L"Local AppData",
                                                                L"Local AppData",
                                                                g_packageVfsRootPath / L"Local AppData"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Local AppData"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_RoamingAppData / LR"(Microsoft)"sv / LR"(Windows)"sv / LR"(Recent)"sv,
                                                                L"AppData\\Microsoft\\Windows\\Recent",
                                                                L"AppData\\Microsoft\\Windows\\Recent",
                                                                g_packageVfsRootPath / L"AppData"sv / L"Microsoft"sv / L"Windows"sv / L"Recent"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppData"sv / L"Microsoft"sv / L"Windows"sv / L"Recent"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_RoamingAppData,
                                                                L"AppData",
                                                                L"AppData",
                                                                g_packageVfsRootPath / L"AppData"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppData"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_RoamingAppData),
                                                                L"AppData",
                                                                L"AppData",
                                                                g_packageVfsRootPath / L"AppData"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppData"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_UserProgramFiles,
                                                                L"UserProgramFiles",
                                                                L"UserProgramFiles",
                                                                g_packageVfsRootPath / L"UserProgramFiles"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"UserProgramFiles"sv });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_CommonPrograms,
                                                                L"Common Programs",
                                                                L"Common Programs",
                                                                g_packageVfsRootPath / L"Common Programs"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Common Programs"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_CommonPrograms),
                                                                L"Common Programs",
                                                                L"Common Programs",
                                                                g_packageVfsRootPath / L"Common Programs"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Common Programs"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_Desktop,
                                                                L"ThisPCDesktopFolder",
                                                                L"ThisPCDesktopFolder",
                                                                g_packageVfsRootPath / L"ThisPCDesktopFolder"sv,
                                                                false,
                                                                FID_Desktop });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                ConvertPathToShortPath(FID_Desktop),
                                                                L"ThisPCDesktopFolder",
                                                                L"ThisPCDesktopFolder",
                                                                g_packageVfsRootPath / L"ThisPCDesktopFolder"sv,
                                                                false,
                                                                FID_Desktop });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_Documents,
                                                                L"Personal",
                                                                L"Personal",
                                                                g_packageVfsRootPath / L"Personal"sv,
                                                                false,
                                                                FID_Documents });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                ConvertPathToShortPath(FID_Documents),
                                                                L"Personal",
                                                                L"Personal",
                                                                g_packageVfsRootPath / L"Personal"sv,
                                                                false,
                                                                FID_Documents });        
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_Profile,
                                                                L"Profile\\AppData",
                                                                L"Profile\\AppData",
                                                                g_packageVfsRootPath / L"Profile"sv / L"AppData"sv ,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Profile"sv / L"AppData"sv });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_Profile),
                                                                L"Profile\\AppData",
                                                                L"Profile\\AppData",
                                                                g_packageVfsRootPath / L"Profile"sv / L"AppData"sv ,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Profile"sv / L"AppData"sv });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_UserFolder,
                                                                L"Profile",
                                                                L"Profile",
                                                                g_packageVfsRootPath / L"Profile"sv ,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Profile"sv  });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                ConvertPathToShortPath(FID_UserFolder),
                                                                L"Profile",
                                                                L"Profile",
                                                                g_packageVfsRootPath / L"Profile"sv  ,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"Profile"sv  });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_PublicDesktop,
                                                                L"Common Desktop",
                                                                L"Common Desktop",
                                                                g_packageVfsRootPath / L"Common Desktop"sv,
                                                                false,
                                                                FID_PublicDesktop });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_local,
                                                                FID_PublicDocuments,
                                                                L"Common Documents",
                                                                L"Common Documents",
                                                                g_packageVfsRootPath / L"Common Documents"sv,
                                                                false,
                                                                FID_PublicDocuments });


        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                FID_RootDrive,
                                                                L"AppVPackageDrive",
                                                                L"AppVPackageDrive",
                                                                g_packageVfsRootPath / L"AppVPackageDrive"sv,
                                                                false,
                                                                g_writablePackageRootPath / L"VFS"sv / L"AppVPackageDrive"sv });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ mfr_enabled_types::enabled,
                                                                mfr_exactmatchonly_types::not_exactmatchonly,
                                                                mfr_exclusion_types::not_excluded,
                                                                mfr::mfr_redirect_flags::prefer_redirection_containerized,
                                                                g_packageRootPath, // changed out 2/14/25 from this? L"PVAD",
                                                                L"PVAD",
                                                                L"PVAD",
                                                                g_packageRootPath,
                                                                false,
                                                                g_writablePackageRootPath});


        // Remapping for the Find cases (See Cohorts for QueryDirectoryFile etc)
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\SystemX64",          L"Catroot2",            L"VFS", L"AppVSystem32Catroot2" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\SystemX64",          L"Catroot",             L"VFS", L"AppVSystem32Catroot" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\SystemX64\\drivers", L"etc",                 L"VFS", L"AppVSystem32DriversEtc" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\SystemX64",          L"driverstore",         L"VFS", L"AppVSystem32Driverstore" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\SystemX64",          L"spool",               L"VFS", L"AppVSystem32Spool" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\Windows",            L"SystemApps",          L"VFS", L"SystemApps" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\Profile\\AppData",   L"Local",               L"VFS", L"Local AppData" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\Profile\\AppData",   L"Roaming",             L"VFS", L"AppData" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\AppVPackageDrive",   L"Windows",             L"VFS", L"Windows" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\AppVPackageDrive",   L"ProgramFilesX64",     L"VFS", L"ProgramFilesX64" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\AppVPackageDrive",   L"ProgramFilesX86",     L"VFS", L"ProgramFilesX86" });
        g_MfrVfsRemappings.push_back(mfr_vfs_remapping{ L"VFS\\AppVPackageDrive",   L"ProgramData",         L"VFS", L"ProgramData" });



#if MOREDEBUG
        Log(L"\t\t\tMFR Mappings initialized.");
#endif
    } // Initialize_MFR_Mappings()

    mfr_folder_mapping  MakeInvalidMapping()
    {
        mfr_folder_mapping mapdisabled;
        mapdisabled.Valid_mapping = mfr_enabled_types::disabled;
        return mapdisabled;
    }

    mfr_folder_mapping  Find_RedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath, [[maybe_unused]] DWORD dllInstance)
    {
        int i = 0;
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.Valid_mapping == mfr_enabled_types::enabled)
            {
                if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                    map.RedirectionFlags != mfr_redirect_flags::prefer_redirection_none
                    )
                {
                    switch (map.IsExactMatchOnly)
                    {
                    case mfr_exactmatchonly_types::exactmatchonly:
                        if (path_isExactMatchOf_String(map.NativePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            switch (map.RedirectionFlags)
                            {
                            case mfr_redirect_flags::prefer_redirection_local:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found exact match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_containerized:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found exact match prefer_containerized index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found exact match prefer_if_package index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            default:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found exact match none index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            }
#endif
                            return map;
                        }
                        break;
                    case mfr_exactmatchonly_types::not_exactmatchonly:
                    default:
                        if (path_isSubsetOf_String(map.NativePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            switch (map.RedirectionFlags)
                            {
                            case mfr_redirect_flags::prefer_redirection_local:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found subset match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_containerized:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found subset match prefer_containerized index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found subset match prefer_if_package index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            default:
                                Log(L"[%d]      MFR_Mappings: LocalFromNative Found subset match none index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            }
#endif
                            return map;
                        }
                        break;
                    }
                }
            }
            i++;
        }
#if MOREDEBUG
        Log(L"[%d]      MFR_Mappings: LocalFromNative No mapping found for %s", dllInstance, WsPath.c_str());
#endif
        return MakeInvalidMapping();
    }  // Find_RedirMapping_FromNativePath_ForwardSearch() 


    mfr_folder_mapping  Find_RedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath, [[maybe_unused]] DWORD dllInstance)
    {
        mfr::mfr_folder_mapping packagemap;

        int i = 0;
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.Valid_mapping == mfr_enabled_types::enabled)
            {
                if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                    map.RedirectionFlags != mfr_redirect_flags::prefer_redirection_none
                    )
                {
                    switch (map.IsExactMatchOnly)
                    {
                    case mfr_exactmatchonly_types::exactmatchonly:
                        if (path_isExactMatchOf_String(map.PackagePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            switch (map.RedirectionFlags)
                            {
                            case mfr_redirect_flags::prefer_redirection_local:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found exact match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_containerized:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found exact match prefer_containerized index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found exact match prefer_if_package index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            default:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found exact match none index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            }
#endif
                            return map;
                        }
                        break;
                    case mfr_exactmatchonly_types::not_exactmatchonly:
                    default:
                        if (path_isSubsetOf_String(map.PackagePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            switch (map.RedirectionFlags)
                            {
                            case mfr_redirect_flags::prefer_redirection_local:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found subset match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_containerized:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found subset match prefer_containerized index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            case mfr_redirect_flags::prefer_redirection_if_package_vfs:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found subset match prefer_if_package index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            default:
                                Log(L"[%d]      MFR_Mappings: LocalFromPackage Found subset match none index=%d for %s", dllInstance, i, WsPath.c_str());
                                break;
                            }
#endif
                            return map;
                        }
                        break;
                    }
                }
            }
            i++;
        }
        // if still here, this might be PVAD, but PVADs don't map
#if MOREDEBUG
        Log(L"[%x]      MFR_Mappings: LocalFromPackage No mapping found for %s", dllInstance, WsPath.c_str());
#endif
        return MakeInvalidMapping();
    } // Find_RedirMapping_FromPackagePath_ForwardSearch() 


    mfr_folder_mapping  Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath, [[maybe_unused]] DWORD dllInstance)
    {
        int i = 0;
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.Valid_mapping == mfr_enabled_types::enabled)
            {
                if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                    (map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_containerized ||
                        map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    switch (map.IsExactMatchOnly)
                    {
                    case mfr_exactmatchonly_types::exactmatchonly:
                        if (path_isExactMatchOf_String(map.NativePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d]      MFR_Mappings: TraditionalFromNative Found exact match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
#endif
                            return map;
                        }
                        break;
                    case mfr_exactmatchonly_types::not_exactmatchonly:
                    default:
                        if (path_isSubsetOf_String(map.NativePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d]      MFR_Mappings: TraditionalFromNative Found subset match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
#endif
                            return map;
                        }
                        break;
                    }
                }
            }
            i++;
        }
#if MOREDEBUG
        Log(L"[%d]      MFR_Mappings: TraditionalFromNative No mapping found for %s", dllInstance, WsPath.c_str());
#endif
        return MakeInvalidMapping();
    }  // Find_TraditionalRedirMapping_FromNativePath_ForwardSearch() 

#if DEAD2ME
    mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirPath_BackwardSearch(std::wstring WsPath, [[maybe_unused]] DWORD dllInstance)
    {
        // Reverse lookup still needs to leave bottom wildcard entry until last.
        for (auto map = g_MfrFolderMappings.rbegin(); map != g_MfrFolderMappings.rend(); map++)
        {
            if (map != g_MfrFolderMappings.rbegin())
            {
                if (map->RedirectionFlags != mfr_redirect_flags::disabled &&
                    map->RedirectionFlags != mfr_redirect_flags::prefer_redirection_local)
                {
                    if (path_isSubsetOf_String(map->NativePathBase, WsPath.c_str()))
                    {
                        return *map;
                    }
                }
            }
        }
        auto maplast = g_MfrFolderMappings.rbegin();
        if (maplast->RedirectionFlags != mfr_redirect_flags::disabled &&
            maplast->RedirectionFlags != mfr_redirect_flags::prefer_redirection_local)
        {
            if (path_isSubsetOf_String(maplast->NativePathBase, WsPath.c_str()))
            {
                return *maplast;
            }
        }
        return MakeInvalidMapping();
    }  // Find_TraditionalRedirMapping_FromRedirPath_BackwardSearch() 
#endif


    mfr_folder_mapping Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath, [[maybe_unused]] DWORD dllInstance)
    {
        mfr::mfr_folder_mapping packagemap;
        //{ true, FID_ProgramFilesX64, L"ProgramFilesX64", L"ProgramFilesX64", g_packageVfsRootPath / L"ProgramFilesX64"sv, true, g_writablePackageRootPath / L"VFS\\ProgramFilesX64"sv, mfr::mfr_redirect_flags::prefer_redirection_containerized });

        int i = 0;
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.Valid_mapping == mfr_enabled_types::enabled)
            {
                if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                    (map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_containerized ||
                     map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    switch (map.IsExactMatchOnly)
                    {
                    case mfr_exactmatchonly_types::exactmatchonly:
                        if (path_isExactMatchOf_String(map.PackagePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d]      MFR_Mappings: TraditionalFromPackage Found exact match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
#endif
                            return map;
                        }
                        break;
                    case mfr_exactmatchonly_types::not_exactmatchonly:
                    default:
                        if (path_isSubsetOf_String(map.PackagePathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d]      MFR_Mappings: TraditionalFromPackage Found subset match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
#endif
                            return map;
                        }
                        break;
                    }
                }
            }
            i++;
        }
        // if still here, this might be PVAD
        if (path_isSubsetOf_String(g_packageVfsRootPath,WsPath.c_str()))
        {
            packagemap.Valid_mapping = mfr_enabled_types::enabled;
            packagemap.DoesRuntimeMapNativeToVFS = false;
            //packagemap.FolderId = L"";
            packagemap.NativePathBase = FID_RootDrive;
            packagemap.PackagePathBase = L"";
            // TODO why didn't the original work???
        }
        else if (path_isSubsetOf_String(g_packageRootPath, WsPath.c_str()))
        {
            packagemap.Valid_mapping = mfr_enabled_types::enabled;
            packagemap.DoesRuntimeMapNativeToVFS = false;
            //packagemap.FolderId = L"";
            packagemap.NativePathBase = FID_RootDrive;
            packagemap.PackagePathBase = g_packageRootPath;
            //packagemap.VFSFolderName = "";
            packagemap.RedirectedPathBase = g_writablePackageRootPath;
            packagemap.RedirectionFlags = mfr_redirect_flags::prefer_redirection_containerized;
#if MOREDEBUG
            Log(L"[%d]      MFR_Mappings: TraditionalFromNative PVAD mapping found for %s", dllInstance, WsPath.c_str());
#endif
            return packagemap;
        }
#if MOREDEBUG
        Log(L"[%d]      MFR_Mappings: TraditionalFromPackage No mapping found for %s", dllInstance, WsPath.c_str());
#endif
        return MakeInvalidMapping();
    } // Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch()


    mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(std::wstring WsPath, [[maybe_unused]] DWORD dllInstance)
    {
        int i = 0;
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.Valid_mapping == mfr_enabled_types::enabled)
            {
                if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                    (map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_containerized ||
                        map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    switch (map.IsExactMatchOnly)
                    {
                    case mfr_exactmatchonly_types::exactmatchonly:
                        if (path_isExactMatchOf_String(map.RedirectedPathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d]      MFR_Mappings: TraditionalFromRedirected Found exact match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
#endif
                            return map;
                        }
                        break;
                    case mfr_exactmatchonly_types::not_exactmatchonly:
                    default:
                        if (path_isSubsetOf_String(map.RedirectedPathBase, WsPath.c_str()))
                        {
#if MOREDEBUG
                            Log(L"[%d]      MFR_Mappings: TraditionalFromRedirected Found subset match prefer_local index=%d for %s", dllInstance, i, WsPath.c_str());
#endif
                            return map;
                        }
                        break;
                    }
                }
            }
            i++;
        }
#if MOREDEBUG
        Log(L"[%d]      MFR_Mappings: TraditionalFromRedirected No mapping found for %s", dllInstance, WsPath.c_str());
#endif
        return MakeInvalidMapping();
    }  // Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch()



    mfr_folder_mapping CloneFolderMapping(mfr_folder_mapping inputMap)
    {
        mfr_folder_mapping newMap;
        newMap.Valid_mapping = inputMap.Valid_mapping;
        newMap.IsExactMatchOnly = inputMap.IsExactMatchOnly;
        newMap.IsAnExclusionToRedirect = inputMap.IsAnExclusionToRedirect;
        newMap.DoesRuntimeMapNativeToVFS = inputMap.DoesRuntimeMapNativeToVFS;
        newMap.FolderId = inputMap.FolderId;
        newMap.NativePathBase = inputMap.NativePathBase;
        newMap.PackagePathBase = inputMap.PackagePathBase;
        newMap.RedirectedPathBase = inputMap.RedirectedPathBase;
        newMap.RedirectionFlags = inputMap.RedirectionFlags;
        newMap.VFSFolderName = inputMap.VFSFolderName;
        return newMap;
    }



    bool findStringIC(const std::wstring& strHaystack, const std::wstring& strNeedle)
    {
        auto it = std::search(
            strHaystack.begin(), strHaystack.end(),
            strNeedle.begin(), strNeedle.end(),
            [](wchar_t ch1, wchar_t ch2) { return std::toupper(ch1) == std::toupper(ch2); }
        );
        return (it != strHaystack.end());
    }

    std::wstring wStringToLower(const std::wstring& str) {
        std::wstring lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::towlower);
        return lowerStr;
    }

    std::wstring caseInsensitiveReplace(const std::wstring& str, const std::wstring& from, const std::wstring& to) {
        std::wstring lowerStr = wStringToLower(str);
        std::wstring lowerFrom = wStringToLower(from);

        size_t pos = 0;
        std::wstring result = str;
        while ((pos = lowerStr.find(lowerFrom, pos)) != std::wstring::npos) {
            result.replace(pos, from.length(), to);
            lowerStr.replace(pos, from.length(), wStringToLower(to));
            pos += to.length();
        }

        return result;
    }

    void ToUnicodeString(const std::wstring source, IN OUT UNICODE_STRING dest) 
    {
        dest.Length = static_cast<USHORT>(source.size() * sizeof(wchar_t));
        dest.MaximumLength = dest.Length + sizeof(wchar_t); // Include space for the null-terminator
        dest.Buffer = (PWSTR)malloc(dest.MaximumLength);
        if (dest.Buffer) 
        {
            memcpy(dest.Buffer, source.c_str(), dest.Length);
            dest.Buffer[source.size()] = L'\0'; // Null-terminate the string
        }
    }

    bool FindCohortVfsRemapping(IN std::wstring cohort, IN std::wstring nextLevel, OUT std::wstring returnCohort, OUT std::wstring returnNextLevel)
    {
        returnCohort = cohort;
        returnNextLevel = nextLevel;

        for (std::vector<mfr_vfs_remapping>::iterator iter = g_MfrVfsRemappings.begin(); iter != g_MfrVfsRemappings.end(); ++iter)
        {
            if (findStringIC(cohort, iter->OrigPath))
            {
                if (wStringToLower(nextLevel).compare(wStringToLower(iter->SubPath)) != 0)
                {
                    returnCohort = caseInsensitiveReplace(cohort, iter->OrigPath, iter->RetargetedPath);
                    returnNextLevel = iter->RetargetedSubPath;
                    return true;
                }
            }
        }
        return false;
    }
}