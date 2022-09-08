//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilea

// The ReplaceFile interface is incompatible with movement of files in the package.  This applies to both the file being replaced as well
// as the file being used to replace, which must be removed in the process (see file privleges required in the notes of the documentation).  THerefore it is necessary to make copies of package files into the redirection area.
//
// The removal of a file in the package isn't possible, but to help with appcompat this fixup will get the file copied over and hide any error from the app.
// This has the possibility of causing a problem later on, but if so there isn't anything that is likly to make the app work.

#if _DEBUG
#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"


#include "FunctionImplementations.h"
#include <psf_logging.h>


template <typename CharT>
BOOL __stdcall ReplaceFileFixup(
    _In_ const CharT* replacedFileName,
    _In_ const CharT* replacementFileName,
    _In_opt_ const CharT* backupFileName,
    _In_ DWORD replaceFlags,
    _Reserved_ LPVOID exclude,
    _Reserved_ LPVOID reserved) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            bool DoBackup = false;
            if (backupFileName != nullptr)
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (strlen(backupFileName) > 0)
                    {
                        DoBackup = true;
                    }
                }
                else
                {
                    if (wcslen(backupFileName) > 0)
                    {
                        DoBackup = true;
                    }
                }
            }


#if _DEBUG
            LogString(dllInstance, L"ReplaceFileFixup replacing", replacedFileName);
            LogString(dllInstance, L"ReplaceFileFixup with", replacementFileName);
            if (DoBackup)
            {
                LogString(dllInstance, L"ReplaceFileFixup with backup", backupFileName);
            }
            Log(L"[%d] replaceFlags=0x%x", dllInstance, replaceFlags);
#endif
            std::wstring wReplacedFileName = widen(replacedFileName);
            std::wstring wReplacementFileName = widen(replacementFileName);

            mfr::mfr_path Replaced_mfr = mfr::create_mfr_path(wReplacedFileName);
            mfr::mfr_folder_mapping ReplacedMap;
            std::wstring ReplacedWsRequested = Replaced_mfr.Request_NormalizedPath.c_str();
            std::wstring ReplacedWsNative;
            std::wstring ReplacedWsPackage;
            std::wstring ReplacedWsRedirected;

            mfr::mfr_path Replacement_mfr = mfr::create_mfr_path(wReplacementFileName);
            mfr::mfr_folder_mapping ReplacementMap;
            std::wstring ReplacementWsRequested = Replacement_mfr.Request_NormalizedPath.c_str();
            std::wstring ReplacementWsNative;
            std::wstring ReplacementWsPackage;
            std::wstring ReplacementWsRedirected;


            // Determine if path of file to be replaced and use redirection area.
            std::wstring UseReplacedFile;
            switch (Replaced_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                ReplacedMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(Replaced_mfr.Request_NormalizedPath.c_str());
                if (ReplacedMap.Valid_mapping)
                {
                    ReplacedWsNative = ReplacedWsRequested;
                    ReplacedWsPackage = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.NativePathBase, ReplacedMap.PackagePathBase);
                    ReplacedWsRedirected = ReplacedWsRequested;
                    UseReplacedFile = ReplacedWsRedirected;
                    break;
                }
                ReplacedMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(Replaced_mfr.Request_NormalizedPath.c_str());
                if (ReplacedMap.Valid_mapping)
                {
                    ReplacedWsNative = ReplacedWsRequested;
                    ReplacedWsPackage = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.NativePathBase, ReplacedMap.PackagePathBase);
                    ReplacedWsRedirected = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.NativePathBase, ReplacedMap.RedirectedPathBase);
                    UseReplacedFile = ReplacedWsRedirected;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                ReplacedMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(Replaced_mfr.Request_NormalizedPath.c_str());
                if (ReplacedMap.Valid_mapping)
                {
                    ReplacedWsNative = ReplacedWsRequested;
                    ReplacedWsPackage = ReplacedWsRequested;
                    ReplacedWsRedirected = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.PackagePathBase, ReplacedMap.RedirectedPathBase);
                    UseReplacedFile = ReplacedWsRedirected;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                ReplacedMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(Replaced_mfr.Request_NormalizedPath.c_str());
                if (ReplacedMap.Valid_mapping)
                {
                    ReplacedWsNative = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.PackagePathBase, ReplacedMap.NativePathBase);
                    ReplacedWsPackage = ReplacedWsRequested;
                    ReplacedWsRedirected = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.PackagePathBase, ReplacedMap.RedirectedPathBase);
                    UseReplacedFile = ReplacedWsRedirected;
                    break;
                }
                ReplacedMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(Replaced_mfr.Request_NormalizedPath.c_str());
                if (ReplacedMap.Valid_mapping)
                {
                    ReplacedWsNative = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.PackagePathBase, ReplacedMap.NativePathBase);
                    ReplacedWsPackage = ReplacedWsRequested;
                    ReplacedWsRedirected = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.PackagePathBase, ReplacedMap.RedirectedPathBase);
                    UseReplacedFile = ReplacedWsRedirected;
                    UseReplacedFile = ReplacedWsPackage;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                ReplacedMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(Replaced_mfr.Request_NormalizedPath.c_str());
                if (ReplacedMap.Valid_mapping)
                {
                    ReplacedWsNative = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.RedirectedPathBase, ReplacedMap.NativePathBase);
                    ReplacedWsPackage = ReplacePathPart(ReplacedWsRequested.c_str(), ReplacedMap.RedirectedPathBase, ReplacedMap.PackagePathBase);
                    ReplacedWsRedirected = ReplacedWsRequested;
                    UseReplacedFile = ReplacedWsRedirected;
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                ReplacedMap = mfr::MakeInvalidMapping();
                ReplacedWsNative = ReplacedWsRequested;
                ReplacedWsPackage = ReplacedWsRequested;
                ReplacedWsRedirected = ReplacedWsRequested;
                UseReplacedFile = ReplacedWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                ReplacedMap = mfr::MakeInvalidMapping();
                ReplacedWsNative = ReplacedWsRequested;
                ReplacedWsPackage = ReplacedWsRequested;
                ReplacedWsRedirected = ReplacedWsRequested;
                UseReplacedFile = ReplacedWsRequested;
                break;
            }


            std::wstring UseReplacementFile;
            // Determing the actual source to use
            switch (Replacement_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                ReplacementMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(Replacement_mfr.Request_NormalizedPath.c_str());
                if (ReplacementMap.Valid_mapping)
                {
                    ReplacementWsNative = ReplacementWsRequested;
                    ReplacementWsPackage = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.NativePathBase, ReplacementMap.PackagePathBase);
                    ReplacementWsRedirected = ReplacementWsRequested;
                    if (PathExists(ReplacementWsNative.c_str()))
                    {
                        UseReplacementFile = ReplacementWsNative;
                    }
                    else if (PathExists(ReplacementWsPackage.c_str()))
                    {
                        UseReplacementFile = ReplacementWsPackage;
                    }
                    else
                    {
                        UseReplacementFile = ReplacementWsRequested;
                    }
                    break;
                }
                ReplacementMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(Replacement_mfr.Request_NormalizedPath.c_str());
                if (ReplacementMap.Valid_mapping)
                {
                    ReplacementWsNative = ReplacementWsRequested;
                    ReplacementWsPackage = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.NativePathBase, ReplacementMap.PackagePathBase);
                    ReplacementWsRedirected = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.NativePathBase, ReplacementMap.RedirectedPathBase);
                    if (PathExists(ReplacementWsRedirected.c_str()))
                    {
                        UseReplacementFile = ReplacementWsRedirected;
                    }
                    else if (PathExists(ReplacementWsPackage.c_str()))
                    {
                        UseReplacementFile = ReplacementWsPackage;
                    }
                    else if (PathExists(ReplacementWsNative.c_str()))
                    {
                        UseReplacementFile = ReplacementWsNative;
                    }
                    else
                    {
                        UseReplacementFile = ReplacementWsRequested;
                    }
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                ReplacementMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(Replacement_mfr.Request_NormalizedPath.c_str());
                if (ReplacementMap.Valid_mapping)
                {
                    ReplacementWsNative = ReplacementWsRequested;
                    ReplacementWsPackage = ReplacementWsRequested;
                    ReplacementWsRedirected = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.PackagePathBase, ReplacementMap.RedirectedPathBase);
                    if (PathExists(ReplacementWsRedirected.c_str()))
                    {
                        UseReplacementFile = ReplacementWsRedirected;
                    }
                    else
                    {
                        UseReplacementFile = ReplacementWsRequested;
                    }
                    break;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                ReplacementMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(Replacement_mfr.Request_NormalizedPath.c_str());
                if (ReplacementMap.Valid_mapping)
                {
                    ReplacementWsNative = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.PackagePathBase, ReplacementMap.NativePathBase);
                    ReplacementWsPackage = ReplacementWsRequested;
                    ReplacementWsRedirected = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.PackagePathBase, ReplacementMap.RedirectedPathBase);
                    if (PathExists(ReplacementWsRedirected.c_str()))
                    {
                        UseReplacementFile = ReplacementWsRedirected;
                    }
                    else if (PathExists(ReplacementWsPackage.c_str()))
                    {
                        UseReplacementFile = ReplacementWsPackage;
                    }
                    else
                    {
                        UseReplacementFile = ReplacementWsRequested;
                    }
                    break;
                }
                ReplacementMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(Replacement_mfr.Request_NormalizedPath.c_str());
                if (ReplacementMap.Valid_mapping)
                {
                    ReplacementWsNative = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.PackagePathBase, ReplacementMap.NativePathBase);
                    ReplacementWsPackage = ReplacementWsRequested;
                    ReplacementWsRedirected = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.PackagePathBase, ReplacementMap.RedirectedPathBase);
                    if (PathExists(ReplacementWsRedirected.c_str()))
                    {
                        UseReplacementFile = ReplacementWsRedirected;
                    }
                    else if (PathExists(ReplacementWsPackage.c_str()))
                    {
                        UseReplacementFile = ReplacementWsPackage;
                    }
                    else if (PathExists(ReplacementWsNative.c_str()))
                    {
                        UseReplacementFile = ReplacementWsNative;
                    }
                    else
                    {
                        UseReplacementFile = ReplacementWsRequested;
                    }
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                ReplacementMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(Replacement_mfr.Request_NormalizedPath.c_str());
                if (ReplacementMap.Valid_mapping)
                {
                    ReplacementWsNative = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.RedirectedPathBase, ReplacementMap.NativePathBase);
                    ReplacementWsPackage = ReplacePathPart(ReplacementWsRequested.c_str(), ReplacementMap.RedirectedPathBase, ReplacementMap.PackagePathBase);
                    ReplacementWsRedirected = ReplacementWsRequested;
                    if (PathExists(ReplacementWsRedirected.c_str()))
                    {
                        UseReplacementFile = ReplacementWsRedirected;
                    }
                    else if (PathExists(ReplacementWsPackage.c_str()))
                    {
                        UseReplacementFile = ReplacementWsPackage;
                    }
                    else if (PathExists(ReplacementWsNative.c_str()))
                    {
                        UseReplacementFile = ReplacementWsNative;
                    }
                    else
                    {
                        UseReplacementFile = ReplacementWsRequested;
                    }
                    break;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                ReplacementMap = mfr::MakeInvalidMapping();
                UseReplacementFile = ReplacementWsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                ReplacementMap = mfr::MakeInvalidMapping();
                UseReplacementFile = ReplacementWsRequested;
                break;
            }


            // Work on the backup first
            if (DoBackup)
            {
                std::wstring wBackupFileName;
                wBackupFileName = widen(backupFileName);
                mfr::mfr_path Backup_mfr = mfr::create_mfr_path(wBackupFileName);
                mfr::mfr_folder_mapping BackupMap;
                std::wstring BackupWsRequested = Backup_mfr.Request_NormalizedPath.c_str();
                std::wstring BackupWsNative;
                std::wstring BackupWsPackage;
                std::wstring BackupWsRedirected;

                std::wstring UseBackupFile;
                // Determing the backup destination in redirection area
                switch (Backup_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    BackupMap = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(Backup_mfr.Request_NormalizedPath.c_str());
                    if (BackupMap.Valid_mapping)
                    {
                        UseBackupFile = BackupWsRequested;
                        break;
                    }
                    BackupMap = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(Backup_mfr.Request_NormalizedPath.c_str());
                    if (BackupMap.Valid_mapping)
                    {
                        UseBackupFile = ReplacePathPart(BackupWsRequested.c_str(), BackupMap.NativePathBase, BackupMap.RedirectedPathBase);
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    BackupMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(Backup_mfr.Request_NormalizedPath.c_str());
                    if (BackupMap.Valid_mapping)
                    {
                        UseBackupFile = ReplacePathPart(BackupWsRequested.c_str(), BackupMap.PackagePathBase, BackupMap.RedirectedPathBase);
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    BackupMap = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(Backup_mfr.Request_NormalizedPath.c_str());
                    if (BackupMap.Valid_mapping)
                    {
                        UseBackupFile = ReplacePathPart(BackupWsRequested.c_str(), BackupMap.PackagePathBase, BackupMap.RedirectedPathBase);;
                        break;
                    }
                    BackupMap = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(Backup_mfr.Request_NormalizedPath.c_str());
                    if (BackupMap.Valid_mapping)
                    {
                        UseBackupFile = ReplacePathPart(BackupWsRequested.c_str(), BackupMap.PackagePathBase, BackupMap.RedirectedPathBase);
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    BackupMap = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(Backup_mfr.Request_NormalizedPath.c_str());
                    if (BackupMap.Valid_mapping)
                    {
                        UseBackupFile = BackupWsRequested;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    BackupMap = mfr::MakeInvalidMapping();
                    UseBackupFile = BackupWsRequested;
                    break;
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_protocol_path:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    BackupMap = mfr::MakeInvalidMapping();
                    UseBackupFile = BackupWsRequested;
                    break;
                }
#if MOREDEBUG
                LogString(dllInstance, L"ReplaceFileFixup: Backup to", UseBackupFile.c_str());
#endif
                std::wstring rldUseReplacedFile;
                if (PathExists(ReplacedWsRedirected.c_str()))
                {
                    rldUseReplacedFile = MakeLongPath(ReplacedWsRedirected);
                }
                else if (PathExists(ReplacedWsPackage.c_str()))
                {
                    rldUseReplacedFile = MakeLongPath(ReplacedWsPackage);
                }
                else if (PathExists(ReplacedWsNative.c_str()))
                {
                    rldUseReplacedFile = MakeLongPath(ReplacedWsNative);
                }
                else
                {
                    rldUseReplacedFile = MakeLongPath(ReplacedWsRequested);
                }

                std::wstring rldUseBackupFile = MakeLongPath(UseBackupFile);
                PreCreateFolders(rldUseBackupFile, dllInstance, L"ReplaceFileFixup");
#if MOREDEBUG
                Log(L"[%d] ReplaceFileFixup: backup from is %s", dllInstance, rldUseReplacedFile.c_str());
                Log(L"[%d] ReplaceFileFixup: backup   to is %s", dllInstance, rldUseBackupFile.c_str());
#endif               
                retfinal = impl::CopyFile(rldUseReplacedFile.c_str(), rldUseBackupFile.c_str(), false);
#if MOREDEBUG
                Log(L"[%d] ReplaceFileFixup backup copy return is %d", dllInstance, retfinal);
#endif
            }

            // Make redirected copies as needed
#if MOREDEBUG
            LogString(dllInstance, L"ReplaceFileFixup: replaced file in redirected area.",  ReplacedWsRedirected.c_str());
            LogString(dllInstance, L"ReplaceFileFixup: replaced file in package area.",  ReplacedWsPackage.c_str());
            LogString(dllInstance, L"ReplaceFileFixup: replaced file in native area.",  ReplacedWsNative.c_str());
#endif
            if (!PathExists(ReplacedWsRedirected.c_str()))
            {
                // Cannot use ReplaceFile when there isn't a file in the redirected area to replace.
                // While std::filesystem::copy is a possible substitution, we can try to avoid side effects
                // by doing a Cow and then replacing.
                if (PathExists(ReplacedWsPackage.c_str()))
                {
                    Cow(ReplacedWsPackage, ReplacedWsRedirected, dllInstance, L"ReplaceFileFixup");
                    UseReplacedFile = ReplacedWsRedirected;
                }
                else if (PathExists(ReplacedWsNative.c_str()))
                {
                    Cow(ReplacedWsNative, ReplacedWsRedirected, dllInstance, L"ReplaceFileFixup");
                    UseReplacedFile = ReplacedWsRedirected;
                }
                else
                {
#if _DEBUG
                    Log(L"[%d] ReplaceFileFixup: Return 0 as Replaced file not found.", dllInstance);
#endif
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    return 0;
                }
            }

#if MOREDEBUG
            LogString(dllInstance, L"ReplaceFileFixup: replacement file in redirected area.", ReplacementWsRedirected.c_str());
            LogString(dllInstance, L"ReplaceFileFixup: replacement file in package area.", ReplacementWsPackage.c_str());
            LogString(dllInstance, L"ReplaceFileFixup: replacement file in native area.", ReplacementWsNative.c_str());
#endif

            if (!PathExists(ReplacementWsRedirected.c_str()))
            {
                if (PathExists(ReplacementWsPackage.c_str()))
                {
                    // Replace needs delete access so we can't use the source file copy inside the package.
                    Cow(ReplacementWsPackage, ReplacementWsRedirected, dllInstance, L"ReplaceFileFixup");
                    UseReplacementFile = ReplacementWsRedirected;
                }
                else if (PathExists(ReplacementWsNative.c_str()))
                {
                    // The function might fail with the source in the native area, but if so it never would have worked natively, 
                    // so skip the copy.
                    UseReplacementFile = ReplacementWsNative;
                }
                else
                {
#if _DEBUG
                    Log(L"[%d] ReplaceFileFixup: Return 0 as Replace mentfile not found.", dllInstance);
#endif
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    return 0;
                }
            }

#if MOREDEBUG
            Log(L"[%d] ReplaceFileFixup: Source      to be is %s", dllInstance, UseReplacementFile.c_str());
            Log(L"[%d] ReplaceFileFixup: Destination to be is %s", dllInstance, UseReplacedFile.c_str());
#endif

            if (true)
            {
                // Can try Replace
                std::wstring rldUseReplacedFile = MakeLongPath(UseReplacedFile);
                std::wstring rldUseReplacementFile = MakeLongPath(UseReplacementFile);
                PreCreateFolders(rldUseReplacedFile, dllInstance, L"ReplaceFileFixup");
#if MOREDEBUG
                Log(L"[%d] ReplaceFileFixup: from is %s", dllInstance, rldUseReplacementFile.c_str());
                Log(L"[%d] ReplaceFileFixup:   to is %s", dllInstance, rldUseReplacedFile.c_str());
#endif
                DWORD Replace_replaceFlags = replaceFlags;
                if (replaceFlags == 0)
                {
                    Replace_replaceFlags = REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS;  // needed as destination might not exist yet due to redirection.
                }
                retfinal = impl::ReplaceFile(rldUseReplacedFile.c_str(), rldUseReplacementFile.c_str(), nullptr, Replace_replaceFlags, exclude, reserved);
#if _DEBUG
                Log(L"[%d] ReplaceFileFixup returns %d", dllInstance, retfinal);
#endif
                return retfinal;
            }
            else
            {
                // Replace move with copy since can't move due to package protections (or file doesn't exist anyway)
                std::wstring rldUseReplacedFile = MakeLongPath(UseReplacedFile);
                std::wstring rldUseReplacementFile = MakeLongPath(UseReplacementFile);
                PreCreateFolders(rldUseReplacementFile, dllInstance, L"ReplaceFileFixup");


#if MOREDEBUG
                Log(L"[%d] ReplaceFileFixup: Implemeting stdcopy from is %s", dllInstance, UseReplacementFile.c_str());
                Log(L"[%d] ReplaceFileFixup: Implemeting stdcopy   to is %s", dllInstance, UseReplacedFile.c_str());
#endif
                // std::filesystem::copy has some edge cases that might throw us for a loop requiring detection of edge
                // cases that need to be handled differently.  
                // Limiting use of this as a substitution to only when necessary.
                // This also ignores the flag options, but since the new file is being wriiten to the redirection area,
                // it should be OK.
                const std::filesystem::copy_options copyOptions = std::filesystem::copy_options::overwrite_existing;
                std::error_code eCode;
                std::filesystem::copy(UseReplacementFile.c_str(),   // Not sure if std supports long path syntax
                    UseReplacedFile.c_str(),
                    copyOptions, eCode);
                if (eCode.value() != 0)
                {
                    retfinal = 0; // error
                }
                else
                {
                    retfinal = 1; // success
                }
                SetLastError(eCode.value()); // Make this match since copy may not set it.
#if _DEBUG
                Log(L"[%d] ReplaceFileFixup returns %d", dllInstance, retfinal);
#endif
                return retfinal;
            }



        }
        else
        {
            LogString(dllInstance, L"ReplaceFileFixup unguarded replacing", replacedFileName);
            LogString(dllInstance, L"ReplaceFileFixup with", replacementFileName);
            if (backupFileName != nullptr)
            {
                LogString(dllInstance, L"ReplaceFileFixup with backup", backupFileName);
            }
            Log(L"[%d] replaceFlags=0x%x", dllInstance, replaceFlags);
        }
    }
#if _DEBUG
        // Fall back to assuming no redirection is necessary if exception
        LOGGED_CATCHHANDLER(dllInstance, L"MoveFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] MoveFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif


    retfinal = impl::ReplaceFile(replacedFileName, replacementFileName, backupFileName, replaceFlags, exclude, reserved);
#if _DEBUG
    Log(L"[%d] ReplaceFile returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::ReplaceFile, ReplaceFileFixup);

