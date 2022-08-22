//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ManagedPathTypes.h"
#include "ManagedFileMappings.h"
#include "FID.h"
#include "PathUtilities.h"
#include <psf_logging.h>

namespace mfr
{

    std::vector<mfr_folder_mapping> g_MfrFolderMappings;


    void Initialize_MFR_Mappings()
    {
        // This creates an ordered list of folders such that more specific paths are listed prior to less specific.
        // For example, the C:\Windows\System32\Drivers entry is before C:\Windows\System32 and that is before C:\Windows

        FID_Initialize();


        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_System32 / LR"(catroot2)"sv,      L"FOLDERID_System\\Catroot2",       L"AppVSystem32Catroot2",    g_packageVfsRootPath / L"AppVSystem32Catroot2"sv,    true,  g_writablePackageRootPath / L"VFS\\AppVSystem32Catroot2"sv,    mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_System32 / LR"(catroot)"sv,       L"FOLDERID_System\\Catroot",        L"AppVSystem32Catroot",     g_packageVfsRootPath / L"AppVSystem32Catroot"sv,     true,  g_writablePackageRootPath / L"VFS\\AppVSystem32Catroot"sv,     mfr::mfr_redirect_flags::prefer_redirection_containerized });


        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_System32 / LR"(drivers\\etc)"sv,  L"FOLDERID_System\\drivers\\etc",   L"AppVSystem32DriversEtc",  g_packageVfsRootPath / L"AppVSystem32DriversEtc"sv,  true,  g_writablePackageRootPath / L"VFS\\AppVSystem32DriversEtc"sv,  mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_System32 / LR"(driverstore)"sv,   L"FOLDERID_System\\driverstore",    L"AppVSystem32Driverstore", g_packageVfsRootPath / L"AppVSystem32Driverstore"sv, true,  g_writablePackageRootPath / L"VFS\\AppVSystem32Driverstore"sv, mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_System32 / LR"(logfiles)"sv,      L"FOLDERID_System\\logfiles",       L"AppVSystem32Logfiles",    g_packageVfsRootPath / L"AppVSystem32Logfiles"sv,    true,  g_writablePackageRootPath / L"VFS\\AppVSystem32Logfiles"sv,    mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_System32 / LR"(spool)"sv,         L"FOLDERID_System\\spool",          L"AppVSystem32Spool",       g_packageVfsRootPath / L"AppVSystem32Spool"sv,       true,  g_writablePackageRootPath / L"VFS\\AppVSystem32Spool"sv,       mfr::mfr_redirect_flags::prefer_redirection_containerized });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_SystemX86,                        L"FOLDERID_SystemX86",              L"SystemX86",               g_packageVfsRootPath / L"SystemX86"sv,               true,  g_writablePackageRootPath / L"VFS\\SystemX86"sv,               mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_ProgramFilesCommonX86,            L"FOLDERID_ProgramFilesCommonX86",  L"ProgramFilesCommonX86",   g_packageVfsRootPath / L"ProgramFilesCommonX86"sv,   true,  g_writablePackageRootPath / L"VFS\\ProgramFilesCommonX86"sv,   mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_ProgramFilesX86,                  L"FOLDERID_ProgramFilesX86",        L"ProgramFilesX86",         g_packageVfsRootPath / L"ProgramFilesX86"sv,         true,  g_writablePackageRootPath / L"VFS\\ProgramFilesX86"sv,         mfr::mfr_redirect_flags::prefer_redirection_containerized });
#if !_M_IX86
        // FUTURE: We may want to consider the possibility of a 32-bit application trying to reference "%windir%\sysnative\"
        //         in which case we'll have to get smarter about how we resolve paths
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_System32,                         L"SystemX64",                       L"SystemX64",               g_packageVfsRootPath / L"SystemX64"sv,              true,  g_writablePackageRootPath / L"VFS\\SystemX64"sv,                mfr::mfr_redirect_flags::prefer_redirection_containerized });
        // FOLDERID_ProgramFilesX64* not supported for 32-bit applications
        // FUTURE: We may want to consider the possibility of a 32-bit process trying to access this path anyway. E.g. a
        //         32-bit child process of a 64-bit process that set the current directory
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_ProgramFilesCommonX64,            L"ProgramFilesCommonX64",           L"ProgramFilesCommonX64",   g_packageVfsRootPath / L"ProgramFilesCommonX64"sv,  true,  g_writablePackageRootPath / L"VFS\\ProgramFilesCommonX64"sv,    mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_ProgramFilesX64,                  L"ProgramFilesX64",                 L"ProgramFilesX64",         g_packageVfsRootPath / L"ProgramFilesX64"sv,        true,  g_writablePackageRootPath / L"VFS\\ProgramFilesX64"sv,          mfr::mfr_redirect_flags::prefer_redirection_containerized });
#endif
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_Windows / LR"(System)"sv,         L"System",                          L"System",                  g_packageVfsRootPath / L"System"sv,                 false, g_writablePackageRootPath / L"VFS\\System"sv,                   mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_Fonts,                            L"Fonts",                           L"Fonts",                   g_packageVfsRootPath / L"Fonts"sv,                  false, g_writablePackageRootPath / L"VFS\\Fonts"sv,                    mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_Windows,                          L"Windows",                         L"Windows",                 g_packageVfsRootPath / L"Windows"sv,                true,  g_writablePackageRootPath / L"VFS\\Windows"sv,                  mfr::mfr_redirect_flags::prefer_redirection_containerized });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_ProgramData,                      L"Common AppData",                  L"Common AppData",          g_packageVfsRootPath / L"Common AppData"sv,         true,  g_writablePackageRootPath / L"VFS\\Common AppData"sv,           mfr::mfr_redirect_flags::prefer_redirection_containerized });

        // These are additional folders that may appear in MSIX packages and need help
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_LocalAppDataLow,                  L"LocalAppDataLow",                 L"LocalAppDataLow",         g_packageVfsRootPath / L"LocalAppDataLow"sv,        false, g_writablePackageRootPath / L"VFS\\LocalAppDataLow"sv,          mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_LocalAppData,                     L"Local AppData",                   L"Local AppData",           g_packageVfsRootPath / L"Local AppData"sv,          false, g_writablePackageRootPath / L"VFS\\Local AppData"sv,            mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_RoamingAppData,                   L"AppData",                         L"AppData",                 g_packageVfsRootPath / L"AppData"sv,                false, g_writablePackageRootPath / L"VFS\\AppData"sv,                  mfr::mfr_redirect_flags::prefer_redirection_containerized });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_CommonPrograms,                   L"Common Programs",                 L"Common Programs",         g_packageVfsRootPath / L"Common Programs"sv,        false, g_writablePackageRootPath / L"VFS\\Common Programs"sv,          mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_Desktop,                          L"ThisPCDesktopFolder",             L"ThisPCDesktopFolder",     g_packageVfsRootPath / L"ThisPCDesktopFolder"sv,    false, g_writablePackageRootPath / L"VFS\\ThisPCDesktopFolder"sv,      mfr::mfr_redirect_flags::prefer_redirection_local });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_Documents,                        L"Personal",                        L"Personal",                g_packageVfsRootPath / L"Personal"sv,               false, g_writablePackageRootPath / L"VFS\\Personal"sv,                 mfr::mfr_redirect_flags::prefer_redirection_local });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_Profile,                          L"Profile",                         L"Profile",                 g_packageVfsRootPath / L"Profile"sv,                false, g_writablePackageRootPath / L"VFS\\Profile"sv,                  mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_PublicDesktop,                    L"Common Desktop",                  L"Common Desktop",          g_packageVfsRootPath / L"Common Desktop"sv,         false, g_writablePackageRootPath / L"VFS\\Common Desktop"sv,           mfr::mfr_redirect_flags::prefer_redirection_local });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_PublicDocuments,                  L"Common Documents",                L"Common Documents",        g_packageVfsRootPath / L"Common Documents"sv,       false, g_writablePackageRootPath / L"VFS\\Common Documents"sv,         mfr::mfr_redirect_flags::prefer_redirection_local });

        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, FID_RootDrive,                        L"AppVPackageDrive",                L"AppVPackageDrive",        g_packageVfsRootPath / L"AppVPackageDrive"sv,       false, g_writablePackageRootPath / L"VFS\\AppVPackageDrive"sv,         mfr::mfr_redirect_flags::prefer_redirection_containerized });
        g_MfrFolderMappings.push_back(mfr::mfr_folder_mapping{ true, L"PVAD",                              L"PVAD",                            L"PVAD",                    g_packageRootPath,                                  false, g_writablePackageRootPath,                                      mfr::mfr_redirect_flags::prefer_redirection_containerized});
#if _DEBUG
        //Log(L" MFR_Mappings initialized.");
#endif
    } // Initialize_MFR_Mappings()

    mfr_folder_mapping  MakeInvalidMapping()
    {
        mfr_folder_mapping none;
        none.Valid_mapping = false;
        return none;
    }

    mfr_folder_mapping  Find_LocalRedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath)
    {
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_local)
            {
                if (path_isSubsetOf_String(map.NativePathBase, WsPath.c_str()))
                {
                    return map;
                }
            }
        }
        return MakeInvalidMapping();
    }  // Find_LocalRedirMapping_FromNativePath_ForwardSearch() 


    mfr_folder_mapping  Find_LocalRedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath)
    {
        mfr::mfr_folder_mapping packagemap;
        //{ true, FID_ProgramFilesX64, L"ProgramFilesX64", L"ProgramFilesX64", g_packageVfsRootPath / L"ProgramFilesX64"sv, true, g_writablePackageRootPath / L"VFS\\ProgramFilesX64"sv, mfr::mfr_redirect_flags::prefer_redirection_containerized });

        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                map.RedirectionFlags == mfr_redirect_flags::prefer_redirection_local)
            {
                if (path_isSubsetOf_String(map.PackagePathBase, WsPath.c_str()))
                {
                    return map;
                }
            }
        }
        // if still here, this might be PVAD, but PVADs don't map
        return MakeInvalidMapping();
    } // Find_LocalRedirMapping_FromPackagePath_ForwardSearch() 


    mfr_folder_mapping  Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(std::wstring WsPath)
    {
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                map.RedirectionFlags != mfr_redirect_flags::prefer_redirection_local)
            {
                if (path_isSubsetOf_String(map.NativePathBase, WsPath.c_str()))
                {
                    return map;
                }
            }
        }
        return MakeInvalidMapping();
    }  // Find_TraditionalRedirMapping_FromNativePath_ForwardSearch() 

#if DEAD2ME
    mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirPath_BackwardSearch(std::wstring WsPath)
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

   
    mfr_folder_mapping  Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(std::wstring WsPath)
    {
        mfr::mfr_folder_mapping packagemap;
        //{ true, FID_ProgramFilesX64, L"ProgramFilesX64", L"ProgramFilesX64", g_packageVfsRootPath / L"ProgramFilesX64"sv, true, g_writablePackageRootPath / L"VFS\\ProgramFilesX64"sv, mfr::mfr_redirect_flags::prefer_redirection_containerized });

        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                map.RedirectionFlags != mfr_redirect_flags::prefer_redirection_local)
            {
                if (path_isSubsetOf_String(map.PackagePathBase, WsPath.c_str()))
                {
                    return map;
                }
            }
        }
        // if still here, this might be PVAD
        if (path_isSubsetOf_String(g_packageVfsRootPath,WsPath.c_str()))
        {
            packagemap.Valid_mapping = true;
            packagemap.DoesRuntimeMapNativeToVFS = false;
            //packagemap.FolderId = L"";
            packagemap.NativePathBase = FID_RootDrive;
            packagemap.PackagePathBase = L"";
            // TODO why didn't the original work???
        }
        else if (path_isSubsetOf_String(g_packageRootPath, WsPath.c_str()))
        {
            packagemap.Valid_mapping = true;
            packagemap.DoesRuntimeMapNativeToVFS = false;
            //packagemap.FolderId = L"";
            packagemap.NativePathBase = FID_RootDrive;
            packagemap.PackagePathBase = g_packageRootPath;
            //packagemap.VFSFolderName = "";
            packagemap.RedirectedPathBase = g_writablePackageRootPath;
            packagemap.RedirectionFlags = mfr_redirect_flags::prefer_redirection_containerized;
            return packagemap;
        }
        return MakeInvalidMapping();
    } // Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch() 


    mfr_folder_mapping  Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(std::wstring WsPath)
    {
        for (mfr_folder_mapping map : g_MfrFolderMappings)
        {
            if (map.RedirectionFlags != mfr_redirect_flags::disabled &&
                map.RedirectionFlags != mfr_redirect_flags::prefer_redirection_local)
            {
                if (path_isSubsetOf_String(map.RedirectedPathBase, WsPath.c_str()))
                {
                    return map;
                }
            }
        }
        return MakeInvalidMapping();
    }  // Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch()

}