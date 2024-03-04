//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefilewithprogressa

/// NOTES:
///     MoveFileWithProgress generally requires the permission to delete the old file, however there is an optional flag to allow it
///     to return success after the copy part.  Other essoteric options (like delayed boot) might not work under MSIX.
///     We could let these fail, but instead we will use a copy technique when the source ends up to be a package file and leave
///     the old file in place.

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




template <typename CharT>
BOOL __stdcall MoveFileWithProgressFixup(
    _In_ const CharT* existingFileName,
    _In_opt_ const CharT* newFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID             lpData,
    _In_ DWORD flags) noexcept
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
#if _DEBUG
            LogString(dllInstance, L"MoveFileWithProgressFixup From", existingFileName);
            LogString(dllInstance, L"MoveFileWithProgressFixup To", newFileName);
            Log(L"[%d] MoveFileWithProgressFixup with flags 0x%x", dllInstance, flags);
#endif

            std::wstring wNewFileName = widen(newFileName);
            std::wstring wExistingFileName = widen(existingFileName);
            wNewFileName = AdjustSlashes(wNewFileName);
            wExistingFileName = AdjustSlashes(wExistingFileName);

            if (wExistingFileName._Starts_with(L"\\\\?\\UNC"))
            {
                wExistingFileName = L"\\" + wExistingFileName.substr(7);
#if _DEBUG
                LogString(dllInstance, L"MoveFileWithProgressFixup adjustment to existingFileName", wExistingFileName.c_str());
#endif
            }
            if (wNewFileName._Starts_with(L"\\\\?\\UNC"))
            {
                wNewFileName = L"\\" + wNewFileName.substr(7);
#if _DEBUG
                LogString(dllInstance, L"MoveFileWithProgressFixup adjustment to newFileName", wNewFileName.c_str());
#endif
            }


            Cohorts cohortsNew;
            DetermineCohorts(wNewFileName, &cohortsNew, moredebug, dllInstance, L"MoveFileWithProgressFixup (newFile)");

            Cohorts cohortsExisting;
            DetermineCohorts(wExistingFileName, &cohortsExisting, moredebug, dllInstance, L"MoveFileWithProgressFixup (existingFile)");

            // Determine if path of existing file and if in package.
            std::wstring UseExistingFile = cohortsExisting.WsRequested;
            bool         ExistingFileIsPackagePath = false;
            std::wstring UseNewFile = cohortsNew.WsRequested;

            if (!MFRConfiguration.Ilv_Aware)
            {
                switch (cohortsExisting.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsExisting.map.Valid_mapping &&
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        else
                        {
                            UseExistingFile = cohortsExisting.WsRequested;
                        }
                        break;
                    }
                    else if (cohortsExisting.map.Valid_mapping &&
                        (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        else //if (cohortsExisting.UsingNative && PathExists(cohortsExisting.WsNative) or not since this is what was requested
                        {
                            UseExistingFile = cohortsExisting.WsRequested;
                        }
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsExisting.map.Valid_mapping)
                    {
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsExisting.map.Valid_mapping &&
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        break;
                    }
                    else if (cohortsExisting.map.Valid_mapping &&
                        (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        else if (cohortsExisting.UsingNative &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsNative;
                        }
                        else
                        {
                            UseExistingFile = cohortsExisting.WsRequested;
                            ExistingFileIsPackagePath = true;
                        }
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsExisting.map.Valid_mapping)
                    {
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        else if (cohortsExisting.UsingNative &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsNative;
                        }
                        else
                        {
                            UseExistingFile = cohortsExisting.WsRequested;
                        }
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    UseExistingFile = wExistingFileName;
                    break;
                case mfr::mfr_path_types::is_Protocol:
                case mfr::mfr_path_types::is_DosSpecial:
                case mfr::mfr_path_types::is_Shell:
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    UseExistingFile = wExistingFileName;
                    break;
                }

                // Determing the new destination
                switch (cohortsNew.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect &&
                        cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        UseNewFile = cohortsNew.WsRequested;
                        break;
                    }
                    else if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect &&
                        (cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        UseNewFile = cohortsNew.WsRedirected;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                    {
                        UseNewFile = cohortsNew.WsRedirected;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect &&
                        cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        UseNewFile = cohortsNew.WsRequested;
                        break;
                    }
                    else if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect &&
                        (cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        UseNewFile = cohortsNew.WsRedirected;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                    {
                        UseNewFile = cohortsNew.WsRequested;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    UseNewFile = wNewFileName;
                    break;
                case mfr::mfr_path_types::is_Protocol:
                case mfr::mfr_path_types::is_DosSpecial:
                case mfr::mfr_path_types::is_Shell:
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    UseNewFile = wNewFileName;
                    break;
                }

#if MOREDEBUG
                Log(L"[%d] MoveFileWithProgressFixup: Source      to be is %s", dllInstance, UseExistingFile.c_str());
                Log(L"[%d] MoveFileWithProgressFixup: Destination to be is %s", dllInstance, UseNewFile.c_str());
                if (ExistingFileIsPackagePath)
                {
                    Log(L"[%d] MoveFileWithProgressFixup: ExistingIsInPackagePath", dllInstance);
                }
#endif


                if (!ExistingFileIsPackagePath ||
                    (flags & MOVEFILE_COPY_ALLOWED) != 0)
                {
                    // Can try MoveEx  (MOVEFILE_COPY_ALLOWED says it will succeed if the copy occurs without the delete)
                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                    PreCreateFolders(rldUseNewFile, dllInstance, L"MoveFileWithProgressFixup");
#if MOREDEBUG
                    Log(L"[%d] MoveFileWithProgressFixup: from is %s", dllInstance, rldUseExistingFile.c_str());
                    Log(L"[%d] MoveFileWithProgressFixup:   to is %s", dllInstance, rldUseNewFile.c_str());
#endif
                    retfinal = impl::MoveFileWithProgress(rldUseExistingFile.c_str(), rldUseNewFile.c_str(), lpProgressRoutine, lpData, flags);
#if _DEBUG
                    if (retfinal == 0)
                    {
                        Log(L"[%d] MoveFileWithProgressFixup returns FAILURE 0x%x", dllInstance, GetLastError());
                    }
                    else
                    {
                        Log(L"[%d] MoveFileWithProgressFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                    }
#endif
                    return retfinal;
                }
                else
                {

                    Log(L"[%d] MoveFileWithProgressFixup Warning: called with possibily unsupportable parameters. Remediation might not work.", dllInstance);

                    // Replace move with copy since can't move due to package protections (or file doesn't exist anyway)
                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                    PreCreateFolders(rldUseNewFile, dllInstance, L"MoveFileWithProgressFixup");

                    // MoveFile supports directory moves, but CopyFile does not
                    auto atts = impl::GetFileAttributes(rldUseExistingFile.c_str());
                    if (atts != INVALID_FILE_ATTRIBUTES &&
                        (atts & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    {
#if MOREDEBUG
                        Log(L"[%d] MoveFileWithProgressFixup: Implemeting stdcopy from is %s", dllInstance, rldUseExistingFile.c_str());
                        Log(L"[%d] MoveFileWithProgressFixup: Implemeting stdcopy   to is %s", dllInstance, rldUseNewFile.c_str());
#endif
                        // std::filesystem::copy has some edge cases that might throw us for a loop requiring detection of edge
                        // cases that need to be handled differently.  
                        // Limiting use of this as a substitution to the directory scenario *should* keep that from happening.
                        const std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive;
                        std::error_code eCode;
                        std::filesystem::copy(UseExistingFile.c_str(),   // Not sure if std supports long path syntax
                            UseNewFile.c_str(),
                            copyOptions, eCode);
                        if (eCode)
                        {
                            retfinal = 0; // error
                        }
                        else
                        {
                            retfinal = 1; // success
                        }
#if _DEBUG
                        if (retfinal == 0)
                        {
                            Log(L"[%d] MoveFileWithProgressFixup via copy(file) returns FAILURE 0x%x", dllInstance, GetLastError());
                        }
                        else
                        {
                            Log(L"[%d] MoveFileFixup via copy(file) returns SUCCESS 0x%x", dllInstance, retfinal);
                        }
                        // TODO: remove old???
#endif
                        return retfinal;
                    }
                    else
                    {
                        // Directory Copy
                        // std::filesystem::copy has some edge cases that might throw us for a loop requiring detection of edge
                        // cases that need to be handled differently.  
                        // Limiting use of this as a substitution to the directory scenario *should* keep that from happening.
                        const std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive;
                        std::error_code eCode;
                        std::filesystem::copy(UseExistingFile.c_str(),   // Not sure if std supports long path syntax
                            UseNewFile.c_str(),
                            copyOptions, eCode);
                        if (eCode)
                        {
                            retfinal = 0; // error
                        }
                        else
                        {
                            retfinal = 1; // success
                        }
#if _DEBUG
                        Log(L"[%d] MoveFileWithProgressFixup returns %d", dllInstance, retfinal);
                        if (retfinal == 0)
                        {
                            Log(L"[%d] MoveFileWithProgressFixup via copy(dir) returns FAILURE 0x%x", dllInstance, GetLastError());
                        }
                        else
                        {
                            Log(L"[%d] MoveFileFixup via copy(dir) returns SUCCESS 0x%x", dllInstance, retfinal);
                        }
                        // TODO: remove old???
#endif
                        return retfinal;
                    }
                }
            }
            else
            {
                // ILV Aware.  This is a write (or delete) operation to both the destination and source.  Generally, we prefer to use the package path when traditional redirection is in play since ILV works better there.

                // Determine appropriate source
                UseExistingFile = DetermineIlvPathForReadOperations(cohortsExisting, dllInstance, moredebug);
                // In a redirect to local scenario, we are responsible for determing if source is local or in package
                UseExistingFile = SelectLocalOrPackageForRead(UseExistingFile, cohortsExisting.WsPackage);

                // Determing the new destination
                UseNewFile = DetermineIlvPathForWriteOperations(cohortsNew, dllInstance, moredebug);
                // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                // if-and-only-if they are present in the package.
                PreCreateLocalFoldersIfNeededForWrite(UseNewFile, cohortsNew.WsPackage, dllInstance, debug, L"MoveFileWithProgressFixup");
                // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                CowLocalFoldersIfNeededForWrite(UseNewFile, cohortsNew.WsPackage, dllInstance, debug, L"MoveFileWithProgressFixup");
                // In a write to package scenario, folders may be needed.
                PreCreatePackageFoldersIfIlvNeededForWrite(UseNewFile, dllInstance, debug, L"MoveFileWithProgressFixup");

#if MOREDEBUG
                    Log(L"[%d] MoveFileWithProgressFixup: IlvAware Source      to be is %s", dllInstance, UseExistingFile.c_str());
                    Log(L"[%d] MoveFileWithProgressFixup: IlvAware Destination to be is %s", dllInstance, UseNewFile.c_str());
#endif

                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
#if MOREDEBUG
                    Log(L"[%d] MoveFileWithProgressFixup: from is %s", dllInstance, rldUseExistingFile.c_str());
                    Log(L"[%d] MoveFileWithProgressFixup:   to is %s", dllInstance, rldUseNewFile.c_str());
#endif
                    retfinal = impl::MoveFileWithProgress(rldUseExistingFile.c_str(), rldUseNewFile.c_str(), lpProgressRoutine, lpData, flags);
#if _DEBUG
                    if (retfinal == 0)
                    {
                        Log(L"[%d] MoveFileWithProgressFixup returns FAILURE 0x%x", dllInstance, GetLastError());
                    }
                    else
                    {
                        Log(L"[%d] MoveFileWithProgressFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                    }
#endif
                    return retfinal;
            }
        }
        else
        {
#if _DEBUG
            LogString(dllInstance, L"MoveFileWithProgressFixup Unguarded From", existingFileName);
            LogString(dllInstance, L"MoveFileWithProgressFixup Unguarded To", newFileName);
#endif
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"MoveFileWithProgressFixup")
#else
    catch (...)
    {
        Log(L"[%d] MoveFileWithProgressFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    if (existingFileName != nullptr && newFileName != nullptr)
    {
        std::wstring LongFile1 = MakeLongPath(widen(existingFileName));
        std::wstring LongFile2 = MakeLongPath(widen(newFileName));
        retfinal = impl::MoveFileWithProgress(LongFile1.c_str(), LongFile2.c_str(), lpProgressRoutine, lpData, flags);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; // impl::MoveFileWithProgress(existingFileName, newFileName, lpProgressRoutine, lpData, flags);
    }
#if _DEBUG
    Log(L"[%d] MoveFileWithProgressFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::MoveFileWithProgress, MoveFileWithProgressFixup);

