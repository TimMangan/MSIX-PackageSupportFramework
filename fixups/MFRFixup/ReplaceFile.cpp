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
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"


std::wstring DetermineNonIlvPathForReplaced(Cohorts cohortsReplaced, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug)
{
    std::wstring UseReplacedFile = cohortsReplaced.WsRequested;
    switch (cohortsReplaced.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
        if (cohortsReplaced.map.Valid_mapping)
        {
            UseReplacedFile = cohortsReplaced.WsRedirected;
        }
        else
        {
            UseReplacedFile = cohortsReplaced.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        if (cohortsReplaced.map.Valid_mapping)
        {
            UseReplacedFile = cohortsReplaced.WsRedirected;
        }
        else
        {
            UseReplacedFile = cohortsReplaced.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        if (cohortsReplaced.map.Valid_mapping)
        {
            UseReplacedFile = cohortsReplaced.WsRedirected;
        }
        else
        {
            UseReplacedFile = cohortsReplaced.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        if (cohortsReplaced.map.Valid_mapping)
        {
            UseReplacedFile = cohortsReplaced.WsRequested;
        }
        else
        {
            UseReplacedFile = cohortsReplaced.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        UseReplacedFile = cohortsReplaced.WsRequested;
        break;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        UseReplacedFile = cohortsReplaced.WsRequested;
        break;
    }
    return UseReplacedFile;
}

std::wstring DetermineNonIlvPathForReplacement(Cohorts cohortsReplacement, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug)
{
    std::wstring UseReplacementFile = cohortsReplacement.WsRequested;
    switch (cohortsReplacement.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
        if (cohortsReplacement.map.Valid_mapping)
        {
            switch (cohortsReplacement.map.RedirectionFlags)
            {
            case mfr::mfr_redirect_flags::prefer_redirection_local:
                if (!cohortsReplacement.map.IsAnExclusionToRedirect && PathExists(cohortsReplacement.WsNative.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsNative;
                }
                else if (PathExists(cohortsReplacement.WsPackage.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsPackage;
                }
                else
                {
                    UseReplacementFile = cohortsReplacement.WsRequested;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                if (!cohortsReplacement.map.IsAnExclusionToRedirect && PathExists(cohortsReplacement.WsRedirected.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsRedirected;
                }
                else if (PathExists(cohortsReplacement.WsPackage.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsPackage;
                }
                else if (cohortsReplacement.UsingNative &&
                    PathExists(cohortsReplacement.WsNative.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsNative;
                }
                else
                {
                    UseReplacementFile = cohortsReplacement.WsRequested;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_none:
            case mfr::mfr_redirect_flags::disabled:
            default:
                // just fall through to unguarded code
                break;
            }
        }
        else
        {
            UseReplacementFile = cohortsReplacement.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        if (cohortsReplacement.map.Valid_mapping)
        {
            switch (cohortsReplacement.map.RedirectionFlags)
            {
            case mfr::mfr_redirect_flags::prefer_redirection_local:
                // not possible, fall through
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                if (!cohortsReplacement.map.IsAnExclusionToRedirect && PathExists(cohortsReplacement.WsRedirected.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsRedirected;
                }
                else
                {
                    UseReplacementFile = cohortsReplacement.WsRequested;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_none:
            case mfr::mfr_redirect_flags::disabled:
            default:
                // just fall through to unguarded code
                break;
            }
        }
        else
        {
            UseReplacementFile = cohortsReplacement.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        if (cohortsReplacement.map.Valid_mapping)
        {
            switch (cohortsReplacement.map.RedirectionFlags)
            {
            case mfr::mfr_redirect_flags::prefer_redirection_local:
                if (!cohortsReplacement.map.IsAnExclusionToRedirect && PathExists(cohortsReplacement.WsRedirected.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsRedirected;
                }
                else if (PathExists(cohortsReplacement.WsPackage.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsPackage;
                }
                else
                {
                    UseReplacementFile = cohortsReplacement.WsRequested;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                if (!cohortsReplacement.map.IsAnExclusionToRedirect && PathExists(cohortsReplacement.WsRedirected.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsRedirected;
                }
                else if (PathExists(cohortsReplacement.WsPackage.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsPackage;
                }
                else if (cohortsReplacement.UsingNative &&
                    PathExists(cohortsReplacement.WsNative.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsNative;
                }
                else
                {
                    UseReplacementFile = cohortsReplacement.WsRequested;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_none:
            case mfr::mfr_redirect_flags::disabled:
            default:
                // just fall through to unguarded code
                break;
            }
        }
        else
        {
            UseReplacementFile = cohortsReplacement.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        if (cohortsReplacement.map.Valid_mapping)
        {
            switch (cohortsReplacement.map.RedirectionFlags)
            {
            case mfr::mfr_redirect_flags::prefer_redirection_local:
                // not possible
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                if (!cohortsReplacement.map.IsAnExclusionToRedirect && PathExists(cohortsReplacement.WsRedirected.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsRedirected;
                }
                else if (PathExists(cohortsReplacement.WsPackage.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsPackage;
                }
                else if (cohortsReplacement.UsingNative &&
                    PathExists(cohortsReplacement.WsNative.c_str()))
                {
                    UseReplacementFile = cohortsReplacement.WsNative;
                }
                else
                {
                    UseReplacementFile = cohortsReplacement.WsRequested;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_none:
            case mfr::mfr_redirect_flags::disabled:
            default:
                // just fall through to unguarded code
                break;
            }
        }
        else
        {
            UseReplacementFile = cohortsReplacement.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        UseReplacementFile = cohortsReplacement.WsRequested;
        break;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        UseReplacementFile = cohortsReplacement.WsRequested;
        break;
    }

    return UseReplacementFile;
}

std::wstring DetermineNonIlvPathForBackup(Cohorts cohortsBackup, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] bool moredebug)
{
    std::wstring UseBackupFile = cohortsBackup.WsRequested;
    switch (cohortsBackup.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_native_area:
        if (cohortsBackup.map.Valid_mapping)
        {
            switch (cohortsBackup.map.RedirectionFlags)
            {
            case mfr::mfr_redirect_flags::prefer_redirection_local:
                UseBackupFile = cohortsBackup.WsRequested;
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                if (!cohortsBackup.map.IsAnExclusionToRedirect)
                {
                    UseBackupFile = cohortsBackup.WsRedirected;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_none:
            case mfr::mfr_redirect_flags::disabled:
            default:
                UseBackupFile = cohortsBackup.WsRequested;
                break;
            }
        }
        else
        {
            UseBackupFile = cohortsBackup.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_pvad_area:
        if (cohortsBackup.map.Valid_mapping && !cohortsBackup.map.IsAnExclusionToRedirect)
        {
            UseBackupFile = cohortsBackup.WsRedirected;
            break;
        }
        else
        {
            UseBackupFile = cohortsBackup.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_package_vfs_area:
        if (cohortsBackup.map.Valid_mapping)
        {
            switch (cohortsBackup.map.RedirectionFlags)
            {
            case mfr::mfr_redirect_flags::prefer_redirection_local:
                if (!cohortsBackup.map.IsAnExclusionToRedirect)
                {
                    UseBackupFile = cohortsBackup.WsRequested;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_containerized:
            case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                if (!cohortsBackup.map.IsAnExclusionToRedirect)
                {
                    UseBackupFile = cohortsBackup.WsRedirected;
                }
                break;
            case mfr::mfr_redirect_flags::prefer_redirection_none:
            case mfr::mfr_redirect_flags::disabled:
            default:
                UseBackupFile = cohortsBackup.WsRequested;
                break;
            }
        }
        else
        {
            UseBackupFile = cohortsBackup.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
        if (cohortsBackup.map.Valid_mapping && !cohortsBackup.map.IsAnExclusionToRedirect)
        {
            UseBackupFile = cohortsBackup.WsRedirected;
            break;
        }
        else
        {
            UseBackupFile = cohortsBackup.WsRequested;
        }
        break;
    case mfr::mfr_path_types::in_redirection_area_other:
        UseBackupFile = cohortsBackup.WsRequested;
        break;
    case mfr::mfr_path_types::is_Protocol:
    case mfr::mfr_path_types::is_DosSpecial:
    case mfr::mfr_path_types::is_Shell:
    case mfr::mfr_path_types::in_other_drive_area:
    case mfr::mfr_path_types::is_UNC_path:
    case mfr::mfr_path_types::unsupported_for_intercepts:
    case mfr::mfr_path_types::unknown:
    default:
        UseBackupFile = cohortsBackup.WsRequested;
        break;
    }
    return UseBackupFile;
}

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
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif
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
            wReplacedFileName = AdjustSlashes(wReplacedFileName);
            wReplacementFileName = AdjustSlashes(wReplacementFileName);

            Cohorts cohortsReplaced;
            DetermineCohorts(wReplacedFileName, &cohortsReplaced, moredebug, dllInstance, L"ReplaceFileFixup (replacedFile)");

            Cohorts cohortsReplacement;
            DetermineCohorts(wReplacementFileName, &cohortsReplacement, moredebug, dllInstance, L"ReplaceFileFixup (replacementFile)");


            std::wstring UseReplacedFile;
            std::wstring UseReplacementFile;
            if (MFRConfiguration.Ilv_Aware)
            {
#if MOREDEBUG
                LogString(dllInstance, L"ReplaceFileFixup replacing", replacedFileName);
#endif
                // Determine if path of file to be replaced and use redirection area.
                UseReplacedFile = DetermineIlvPathForWriteOperations(cohortsReplaced, dllInstance, moredebug);
                // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                // if-and-only-if they are present in the package.
                PreCreateLocalFoldersIfNeededForWrite(UseReplacedFile, cohortsReplaced.WsPackage, dllInstance, debug, L"ReplaceFileFixup");
                // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                CowLocalFoldersIfNeededForWrite(UseReplacedFile, cohortsReplaced.WsPackage, dllInstance, debug, L"ReplaceFileFixup");
                // In a write to package scenario, folders may be needed.
                PreCreatePackageFoldersIfIlvNeededForWrite(UseReplacedFile, dllInstance, debug, L"ReplaceFileFixup");

                // Determine the actual source to use
                UseReplacementFile = DetermineIlvPathForReadOperations(cohortsReplacement, dllInstance, moredebug);
                // In a redirect to local scenario, we are responsible for determing if source is local or in package
                UseReplacementFile = SelectLocalOrPackageForRead(UseReplacementFile, cohortsReplacement.WsPackage);
#if MOREDEBUG
                LogString(dllInstance, L"ReplaceFileFixup IlvAware replacing", UseReplacedFile.c_str());
                LogString(dllInstance, L"ReplaceFileFixup IlvAware with", UseReplacementFile.c_str());
#endif
            }
            else
            {
                // Determine if path of file to be replaced and use redirection area.
                UseReplacedFile = DetermineNonIlvPathForReplaced(cohortsReplaced, dllInstance, moredebug);
                // Determine the actual source to use
                UseReplacementFile = DetermineNonIlvPathForReplacement(cohortsReplacement, dllInstance, moredebug);
            }

            // Work on the making the backup first, if requested (as the backup must end up in the redirected area too
            if (DoBackup)
            {
                std::wstring wBackupFileName;
                wBackupFileName = widen(backupFileName);
                Cohorts cohortsBackup;
                DetermineCohorts(wBackupFileName, &cohortsBackup, moredebug, dllInstance, L"ReplaceFileFixup (backup)");

                std::wstring UseBackupFile;
                if (MFRConfiguration.Ilv_Aware)
                {
                    // Determing the backup destination in redirection area
                    UseBackupFile = DetermineIlvPathForWriteOperations(cohortsBackup, dllInstance, moredebug);
                    // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                    // if-and-only-if they are present in the package.
                    PreCreateLocalFoldersIfNeededForWrite(UseBackupFile, cohortsBackup.WsPackage, dllInstance, debug, L"ReplaceFileFixup (backup)");
                    // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                    CowLocalFoldersIfNeededForWrite(UseBackupFile, cohortsBackup.WsPackage, dllInstance, debug, L"ReplaceFileFixup (backup)");
                    // In a write to package scenario, folders may be needed.
                    PreCreatePackageFoldersIfIlvNeededForWrite(UseBackupFile, dllInstance, debug, L"ReplaceFileFixup (backup)");
                }
                else
                {
                    // Determing the backup destination in redirection area
                    UseBackupFile = DetermineNonIlvPathForBackup(cohortsBackup, dllInstance, moredebug);
                }
#if MOREDEBUG
                LogString(dllInstance, L"ReplaceFileFixup: Backup to", UseBackupFile.c_str());
#endif

                std::wstring rldUseReplacedFile;
                if (PathExists(cohortsReplaced.WsRedirected.c_str()))
                {
                    rldUseReplacedFile = MakeLongPath(cohortsReplaced.WsRedirected);
                }
                else if (PathExists(cohortsReplaced.WsPackage.c_str()))
                {
                    rldUseReplacedFile = MakeLongPath(cohortsReplaced.WsPackage);
                }
                else if (cohortsReplaced.UsingNative &&
                    PathExists(cohortsReplaced.WsNative.c_str()))
                {
                    rldUseReplacedFile = MakeLongPath(cohortsReplaced.WsNative);
                }
                else
                {
                    rldUseReplacedFile = MakeLongPath(cohortsReplaced.WsRequested);
                }

                std::wstring rldUseBackupFile = MakeLongPath(UseBackupFile);
                if (!MFRConfiguration.Ilv_Aware)
                {
                    PreCreateFolders(rldUseBackupFile, dllInstance, L"ReplaceFileFixup");

#if MOREDEBUG
                    Log(L"[%d] ReplaceFileFixup: backup from is %s", dllInstance, rldUseReplacedFile.c_str());
                    Log(L"[%d] ReplaceFileFixup: backup   to is %s", dllInstance, rldUseBackupFile.c_str());
#endif               
                    retfinal = impl::CopyFile(rldUseReplacedFile.c_str(), rldUseBackupFile.c_str(), false);
#if MOREDEBUG
                    if (retfinal != 0)
                    {
                        Log(L"[%d] ReplaceFileFixup backup copy return is FAILURE 0x%x", dllInstance, GetLastError());
                    }
                    else
                    {
                        Log(L"[%d] ReplaceFileFixup backup copy return is SUCCESS 0x%x", dllInstance, retfinal);
                    }
#endif
                }
            }

            if (MFRConfiguration.Ilv_Aware)
            {
                std::wstring rldUseReplacedFile = MakeLongPath(UseReplacedFile);
                std::wstring rldUseReplacementFile = MakeLongPath(UseReplacementFile);
                //PreCreateFolders(rldUseReplacedFile, dllInstance, L"ReplaceFileFixup");
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
                if (retfinal == 0)
                {
                    Log(L"[%d] ReplaceFileFixup returnsFAILURE 0x%x", dllInstance, GetLastError());
                }
                else
                {
                    Log(L"[%d] ReplaceFileFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                }
#endif
            }
            else
            {
                // Make redirected copies as needed
                if (!PathExists(cohortsReplaced.WsRedirected.c_str()))
                {
                    // Cannot use ReplaceFile when there isn't a file in the redirected area to replace.
                    // While std::filesystem::copy is a possible substitution, we can try to avoid side effects
                    // by doing a Cow and then replacing.
                    if (PathExists(cohortsReplaced.WsPackage.c_str()))
                    {
                        if (Cow(cohortsReplaced.WsPackage, cohortsReplaced.WsRedirected, dllInstance, L"ReplaceFileFixup"))
                        {
                            UseReplacedFile = cohortsReplaced.WsRedirected;
                        }
                        else
                        {
                            UseReplacedFile = cohortsReplaced.WsPackage;
                        }
                    }
                    else if (cohortsReplaced.UsingNative &&
                        PathExists(cohortsReplaced.WsNative.c_str()))
                    {
                        if (Cow(cohortsReplaced.WsNative, cohortsReplaced.WsRedirected, dllInstance, L"ReplaceFileFixup"))
                        {
                            UseReplacedFile = cohortsReplaced.WsRedirected;
                        }
                        else
                        {
                            UseReplacedFile = cohortsReplaced.WsNative;
                        }
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] ReplaceFileFixup: Return FAILURE 0 as Replaced file not found.", dllInstance);
#endif
                        SetLastError(ERROR_FILE_NOT_FOUND);
                        return 0;
                    }
                }

                if (!PathExists(cohortsReplacement.WsRedirected.c_str()))
                {
                    if (PathExists(cohortsReplacement.WsPackage.c_str()))
                    {
                        // Replace needs delete access so we can't use the source file copy inside the package.
                        if (Cow(cohortsReplacement.WsPackage, cohortsReplacement.WsRedirected, dllInstance, L"ReplaceFileFixup"))
                        {
                            UseReplacementFile = cohortsReplacement.WsRedirected;
                        }
                        else
                        {
                            UseReplacementFile = cohortsReplacement.WsPackage;
                        }
                    }
                    else if (cohortsReplacement.UsingNative &&
                        PathExists(cohortsReplacement.WsNative.c_str()))
                    {
                        // The function might fail with the source in the native area, but if so it never would have worked natively, 
                        // so skip the copy.
                        UseReplacementFile = cohortsReplacement.WsNative;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] ReplaceFileFixup: Return FAILURE as Replacementfile not found.", dllInstance);
#endif
                        SetLastError(ERROR_FILE_NOT_FOUND);
                        return 0;
                    }
                }

#if MOREDEBUG
                Log(L"[%d] ReplaceFileFixup: Source      to be is %s", dllInstance, UseReplacementFile.c_str());
                Log(L"[%d] ReplaceFileFixup: Destination to be is %s", dllInstance, UseReplacedFile.c_str());
#endif

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
                if (retfinal == 0)
                {
                    Log(L"[%d] ReplaceFileFixup returnsFAILURE 0x%x", dllInstance, GetLastError());
                }
                else
                {
                    Log(L"[%d] ReplaceFileFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                }
#endif
            }
            return retfinal;

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
    Log(L"[%d] ReplaceFileFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::ReplaceFile, ReplaceFileFixup);

