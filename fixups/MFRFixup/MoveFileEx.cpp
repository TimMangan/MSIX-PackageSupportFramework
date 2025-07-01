//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexa

#if _DEBUG
/// NOTES:
///     MoveFileEx generally requires the permission to delete the old file, however there is an optional flag to allow it
///     to return success after the copy part.  Other essoteric options (like delayed boot) might not work under MSIX.
///     We could let these fail, but instead we will use a copy technique when the source ends up to be a package file and leave
///     the old file in place.
/// 
///     ***** This file and KernelBase_MoveFileEx must be kept in sync *****
/// 

//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:MoveFileExFixupAnsi_Fixup=impl::_MoveFileExW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:MoveFileExFixupWide_Fixup=impl::_MoveFileExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:MoveFileExFixupAnsi_Fixup=impl::MoveFileExW.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:MoveFileExFixupWide_Fixup=impl::MoveFileExW.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


template <typename CharT>
BOOL __stdcall MoveFileExFixup(
    _In_ const CharT* existingFileName,
    _In_opt_ const CharT* newFileName,
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
            LogString(g_MfrModuleName, dllInstance, L"MoveFileExFixup From", existingFileName);
            LogString(g_MfrModuleName, dllInstance, L"MoveFileExFixup To", newFileName);
            Log(L"[%s%d] MoveFileExFixup with flags 0x%x", g_MfrModuleName, dllInstance, flags);
#endif

            std::wstring wNewFileName = widen(newFileName);
            std::wstring wExistingFileName = widen(existingFileName);
            wNewFileName = AdjustSlashes(wNewFileName, dllInstance);
            wExistingFileName = AdjustSlashes(wExistingFileName, dllInstance);

            wExistingFileName = AdjustBadUNC(wExistingFileName, dllInstance, L"MoveFileExFixup (existing)");
            wNewFileName = AdjustBadUNC(wNewFileName, dllInstance, L"MoveFileExFixup (new)");



            /// Draw.IO problem with getting confused
            std::wstring AAfrom = L"\\AppData\\AppData";
            std::wstring AAto = L"\\AppData\\Roaming";
            std::size_t AApos = wExistingFileName.find(AAfrom);
            if (AApos != std::wstring::npos)
            {
                wExistingFileName.replace(AApos, AAfrom.length(), AAto);
#if _DEBUG
                LogString(g_MfrModuleName, dllInstance, L"MoveFileExFixup existing adjustment", wExistingFileName.c_str());
#endif
            }
            // Might get this somewhere too
            std::wstring ALfrom = L"\\AppData\\Local AppData";
            std::wstring ALto = L"\\AppData\\Local";
            std::size_t ALpos = wExistingFileName.find(ALfrom);
            if (ALpos != std::wstring::npos)
            {
                wExistingFileName.replace(ALpos, ALfrom.length(), ALto);
#if _DEBUG
                LogString(g_MfrModuleName, dllInstance, L"MoveFileExFixup existing adjustment", wExistingFileName.c_str());
#endif
            }

            Cohorts cohortsExisting;
            DetermineCohorts(wExistingFileName, &cohortsExisting, moredebug, dllInstance, L"MoveFileExFixup (existingFileName)");

            Cohorts cohortsNew;
            DetermineCohorts(wNewFileName, &cohortsNew, moredebug, dllInstance, L"MoveFileExFixup (newFileName)");


            // Determine if path of existing file and if in package.
            std::wstring UseExistingFile = cohortsExisting.WsRequested;
            bool         ExistingFileIsPackagePath = false;
            std::wstring UseNewFile = cohortsNew.WsRequested;

            if (!MFRConfiguration.Ilv_Aware)
            {
                switch (cohortsExisting.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
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
                    else if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                        (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        else //if (cohortsExisting.NativeIsValidOptionInScenario && PathExists(cohortsExisting.WsNative) or not since this is what was requested
                        {
                            UseExistingFile = cohortsExisting.WsRequested;
                        }
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
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
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
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
                    else if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                        (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
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
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsRedirected;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            UseExistingFile = cohortsExisting.WsPackage;
                            ExistingFileIsPackagePath = true;
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
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
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                        cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        UseNewFile = cohortsNew.WsRequested;
                        break;
                    }
                    else if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                        (cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        UseNewFile = cohortsNew.WsRedirected;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        UseNewFile = cohortsNew.WsRedirected;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                        cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        UseNewFile = cohortsNew.WsRequested;
                        break;
                    }
                    else if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                        (cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohortsNew.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        UseNewFile = cohortsNew.WsRedirected;
                        break;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
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
                Log(L"[%s%d] MoveFileExFixup: Source      to be is %s", g_MfrModuleName, dllInstance, UseExistingFile.c_str());
                Log(L"[%s%d] MoveFileExFixup: Destination to be is %s", g_MfrModuleName, dllInstance, UseNewFile.c_str());
                if (ExistingFileIsPackagePath)
                {
                    Log(L"[%s%d] MoveFileExFixup: ExistingIsInPackagePath", g_MfrModuleName, dllInstance);
                }
#endif


                if (!ExistingFileIsPackagePath ||
                    (flags & MOVEFILE_COPY_ALLOWED) != 0)
                {
                    // Can try MoveEx  (MOVEFILE_COPY_ALLOWED says it will succeed if the copy occurs without the delete)
                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                    PreCreateFolders(rldUseNewFile, dllInstance, L"MoveFileExFixup");
#if MOREDEBUG
                    Log(L"[%s%d] MoveFileExFixup: from is %s", g_MfrModuleName, dllInstance, rldUseExistingFile.c_str());
                    Log(L"[%s%d] MoveFileExFixup:   to is %s", g_MfrModuleName, dllInstance, rldUseNewFile.c_str());
#endif
                    retfinal = impl::MoveFileEx(rldUseExistingFile.c_str(), rldUseNewFile.c_str(), flags);
#if _DEBUG
                    if (retfinal == 0)
                    {
                        Log(L"[%s%d] MoveFileExFixup returns FAILURE 0x%x", g_MfrModuleName, dllInstance, GetLastError());
                    }
                    else
                    {
                        Log(L"[%s%d] MoveFileExFixup returns SUCCESS 0x%x", g_MfrModuleName, dllInstance, retfinal);
                    }
#endif
                    return retfinal;
                }
                else
                {
                    // Replace move with copy since can't move due to package protections (or file doesn't exist anyway)
                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                    PreCreateFolders(rldUseNewFile, dllInstance, L"MoveFileExFixup");

                    // MoveFile supports directory moves, but CopyFile does not
                    auto atts = impl::GetFileAttributes(rldUseExistingFile.c_str());
                    if (atts != INVALID_FILE_ATTRIBUTES &&
                        (atts & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    {
#if MOREDEBUG
                        Log(L"[%s%d] MoveFileExFixup: Implemeting stdcopy from is %s", g_MfrModuleName, dllInstance, rldUseExistingFile.c_str());
                        Log(L"[%s%d] MoveFileExFixup: Implemeting stdcopy   to is %s", g_MfrModuleName, dllInstance, rldUseNewFile.c_str());
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
                            Log(L"[%s%d] MoveFileExFixup via copy(file) returns FAILURE 0x%x GetLastError 0x%x", g_MfrModuleName, dllInstance, eCode, GetLastError());
                        }
                        else
                        {
                            Log(L"[%s%d] MoveFileExFixup via copy(file) returns SUCCESS 0x%x", g_MfrModuleName, dllInstance, retfinal);
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
                        if (retfinal == 0)
                        {
                            Log(L"[%s%d] MoveFileExFixup via copy(dir) returns FAILURE 0x%x GetLastError 0x%x", g_MfrModuleName, dllInstance, eCode, GetLastError());
                        }
                        else
                        {
                            Log(L"[%s%d] MoveFileExFixup via copy(dir) returns SUCCESS 0x%x", g_MfrModuleName, dllInstance, retfinal);
                        }
                        // TODO: Remove old???
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
                PreCreateLocalFoldersIfNeededForWrite(UseNewFile, cohortsNew.WsPackage, dllInstance, debug, L"MoveFileExFixup");
                // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                CowLocalFoldersIfNeededForWrite(UseNewFile, cohortsNew.WsPackage, dllInstance, debug, L"MoveFileExFixup");
                // In a write to package scenario, folders may be needed.
                PreCreatePackageFoldersIfIlvNeededForWrite(UseNewFile, dllInstance, debug, L"MoveFileExFixup");

#if MOREDEBUG
               Log(L"[%s%d] MoveFileExFixup: IlvAware Source      to be is %s", g_MfrModuleName, dllInstance, UseExistingFile.c_str());
               Log(L"[%s%d] MoveFileExFixup: IlvAware Destination to be is %s", g_MfrModuleName, dllInstance, UseNewFile.c_str());
#endif

               std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
               std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
#if MOREDEBUG
               Log(L"[%s%d] MoveFileExFixup: from is %s", g_MfrModuleName, dllInstance, rldUseExistingFile.c_str());
               Log(L"[%s%d] MoveFileExFixup:   to is %s", g_MfrModuleName, dllInstance, rldUseNewFile.c_str());
#endif
               retfinal = impl::MoveFileEx(rldUseExistingFile.c_str(), rldUseNewFile.c_str(), flags);
#if _DEBUG
               if (retfinal == 0)
               {
                   Log(L"[%s%d] MoveFileExFixup returns FAILURE 0x%x", g_MfrModuleName, dllInstance, GetLastError());
               }
               else
               {
                   Log(L"[%s%d] MoveFileExFixup returns SUCCESS 0x%x", g_MfrModuleName, dllInstance, retfinal);
               }
#endif
               return retfinal;
            }
        }
        else
        {
#if _DEBUG
        LogString(g_MfrModuleName, dllInstance, L"MoveFileExFixup Unguarded From", existingFileName);
        LogString(g_MfrModuleName, dllInstance, L"MoveFileExFixup Unguarded To", newFileName);
#endif
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(g_MfrModuleName, dllInstance, L"MoveFileExFixup")
#else
    catch (...)
    {
        Log(L"[%s%d] MoveFileExFixup Exception=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }
#endif

    if (existingFileName != nullptr && newFileName != nullptr)
    {
        std::wstring LongFile1 = MakeLongPath(widen(existingFileName));
        std::wstring LongFile2 = MakeLongPath(widen(newFileName));
        retfinal = impl::MoveFileEx(LongFile1.c_str(), LongFile2.c_str(), flags);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; // impl::MoveFileEx(existingFileName, newFileName, flags);
    }
#if _DEBUG
    Log(L"[%s%d] MoveFileExFixup returns 0x%x", g_MfrModuleName, dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::MoveFileEx, MoveFileExFixup);

