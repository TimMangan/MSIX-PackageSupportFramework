//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexa

#if _DEBUG
/// NOTES:
///     MoveFileExA/W, like many Windows API functions exists as an exported function in both Kernel32.dll and KernelBase.dll.
///     Most software builds using kernel32.lib which directs calls to use the Kernel32 implementation.  This is why we normally
///     trap the Kernel32 implementations for the PSF.
///     There are situations where code manages to bypass the Kernel32 implementation, and trapping in KernelBase is necessary.
///     In particular, we have seen an app (Audacity) which appears to be built using the Universal C-Runtime libraries and when
///     _wrename is called, the implementation seems to call the KernelBase version of MoveFileExW.
///     It is therefore necessary to also trap the WinBase version.
/// 
///     At this time we are ignoring the ANSI version, but it should probably be added.  Other APIs needing this additional hook
///     have not (yet) been identified.
/// 
///     ***** This file and MoveFileEx must be kept in sync *****
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

#include "FunctionImplementations_KernelBase.h"


#if FIXUP_FROM_KernelBase

BOOL __stdcall Kb_MoveFileExWFixup(
    _In_ const wchar_t* existingFileName,
    _In_opt_ const wchar_t* newFileName,
    _In_ DWORD flags)
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
            LogString(dllInstance, L"Kb_MoveFileExWFixup From", existingFileName);
            LogString(dllInstance, L"Kb_MoveFileExWFixup To", newFileName);
            Log(L"[%d] Kb_MoveFileExWFixup with flags 0x%x", dllInstance, flags);
#endif            
            //std::wstring wNewFileName = widen(newFileName);
            //std::wstring wExistingFileName = widen(existingFileName);
            std::wstring wNewFileName = AdjustSlashes(newFileName);
            std::wstring wExistingFileName = AdjustSlashes(existingFileName);


            Cohorts cohortsNew;
            DetermineCohorts(wNewFileName, &cohortsNew, moredebug, dllInstance, L"Kb_MoveFileExWFixup (newFileName)");

            Cohorts cohortsExisting;
            DetermineCohorts(wExistingFileName, &cohortsExisting, moredebug, dllInstance, L"Kb_MoveFileExWFixup (existingFileName)");

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
                    if (cohortsNew.map.Valid_mapping)
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
                Log(L"[%d] Kb_MoveFileExWFixup: Source      to be is %s", dllInstance, UseExistingFile.c_str());
                Log(L"[%d] Kb_MoveFileExWFixup: Destination to be is %s", dllInstance, UseNewFile.c_str());
                if (ExistingFileIsPackagePath)
                {
                    Log(L"[%d] Kb_MoveFileExWFixup: ExistingIsInPackagePath", dllInstance);
                }
#endif

                if (!ExistingFileIsPackagePath ||
                    (flags & MOVEFILE_COPY_ALLOWED) != 0)
                {
                    // Can try MoveEx  (MOVEFILE_COPY_ALLOWED says it will succeed if the copy occurs without the delete)
                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                    PreCreateFolders(rldUseNewFile, dllInstance, L"Kb_MoveFileExWFixup");
#if MOREDEBUG
                    Log(L"[%d] Kb_MoveFileExWFixup: from is %s", dllInstance, rldUseExistingFile.c_str());
                    Log(L"[%d] Kb_MoveFileExWFixup:   to is %s", dllInstance, rldUseNewFile.c_str());
#endif
                    retfinal = kernelbaseimpl::MoveFileExWImpl(rldUseExistingFile.c_str(), rldUseNewFile.c_str(), flags);
#if _DEBUG
                    if (retfinal == 0)
                    {
                        Log(L"[%d] Kb_MoveFileExWFixup returns FAILURE 0x%x", dllInstance, GetLastError());
                    }
                    else
                    {
                        Log(L"[%d] Kb_MoveFileExWFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                    }
#endif
                    return retfinal;
                }
                else
                {
                    // Replace move with copy since can't move due to package protections (or file doesn't exist anyway)
                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
                    PreCreateFolders(rldUseNewFile, dllInstance, L"Kb_MoveFileExWFixup");

                    // MoveFile supports directory moves, but CopyFile does not
                    auto atts = impl::GetFileAttributes(rldUseExistingFile.c_str());
                    if (atts != INVALID_FILE_ATTRIBUTES &&
                        (atts & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    {
#if MOREDEBUG
                        Log(L"[%d] Kb_MoveFileExWFixup: Implemeting stdcopy from is %s", dllInstance, rldUseExistingFile.c_str());
                        Log(L"[%d] Kb_MoveFileExWFixup: Implemeting stdcopy   to is %s", dllInstance, rldUseNewFile.c_str());
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
                            Log(L"[%d] Kb_MoveFileExWFixup via copy(file) returns FAILURE 0x%x GetLastError 0x%x", dllInstance, eCode, GetLastError());
                        }
                        else
                        {
                            Log(L"[%d] Kb_MoveFileExWFixup via copy(file) returns SUCCESS 0x%x", dllInstance, retfinal);
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
                            Log(L"[%d] Kb_MoveFileExWFixup via copy(dir) returns FAILURE 0x%x GetLastError 0x%x", dllInstance, eCode, GetLastError());
                        }
                        else
                        {
                            Log(L"[%d] Kb_MoveFileExWFixup via copy(dir) returns SUCCESS 0x%x", dllInstance, retfinal);
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
                PreCreateLocalFoldersIfNeededForWrite(UseNewFile, cohortsNew.WsPackage, dllInstance, debug, L"Kb_MoveFileExWFixup");
                // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                CowLocalFoldersIfNeededForWrite(UseNewFile, cohortsNew.WsPackage, dllInstance, debug, L"Kb_MoveFileExWFixup");
                // In a write to package scenario, folders may be needed.
                PreCreatePackageFoldersIfIlvNeededForWrite(UseNewFile, dllInstance, debug, L"Kb_MoveFileExWFixup");

#if MOREDEBUG
                    Log(L"[%d] Kb_MoveFileExWFixup: IlvAware Source      to be is %s", dllInstance, UseExistingFile.c_str());
                    Log(L"[%d] Kb_MoveFileExWFixup: IlvAware Destination to be is %s", dllInstance, UseNewFile.c_str());
#endif

                    std::wstring rldUseExistingFile = MakeLongPath(UseExistingFile);
                    std::wstring rldUseNewFile = MakeLongPath(UseNewFile);
#if MOREDEBUG
                    Log(L"[%d] Kb_MoveFileExWFixup: from is %s", dllInstance, rldUseExistingFile.c_str());
                    Log(L"[%d] Kb_MoveFileExWFixup:   to is %s", dllInstance, rldUseNewFile.c_str());
#endif
                    retfinal = kernelbaseimpl::MoveFileExWImpl(rldUseExistingFile.c_str(), rldUseNewFile.c_str(), flags);
#if _DEBUG
                    if (retfinal == 0)
                    {
                        Log(L"[%d] Kb_MoveFileExWFixup returns FAILURE 0x%x", dllInstance, GetLastError());
                    }
                    else
                    {
                        Log(L"[%d] Kb_MoveFileExWFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                    }
#endif
                    return retfinal;

            }
        }
        else
        {
#if _DEBUG
            LogString(dllInstance, L"Kb_MoveFileExWFixup Unguarded From", existingFileName);
            LogString(dllInstance, L"Kb_MoveFileExWFixup Unguarded To", newFileName);
#endif
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"Kb_MoveFileExWFixup")
#else
    catch (...)
    {
        Log(L"[%d] Kb_MoveFileExWFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    retfinal = kernelbaseimpl::MoveFileExWImpl(existingFileName, newFileName, flags);
#if _DEBUG
    Log(L"[%d] Kb_MoveFileExWFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;

}
DECLARE_FIXUP(kernelbaseimpl::MoveFileExWImpl, Kb_MoveFileExWFixup);
//DECLARE_STRING_FIXUP(kernelbaseimpl::MoveFileExImpl, Kb_MoveFileExWFixup);
#endif