//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-copyfile

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

BOOL WRAPPER_WORKAROUND(std::wstring existingFileWs, std::wstring newFileWs, BOOL failIfExists, [[maybe_unused]] bool debug, bool moredebug, DWORD dllInstance)
{
    BOOL retfinal = FALSE;
    std::wstring LongExistingFileWs = MakeLongPath(existingFileWs);
    std::wstring LongNewFileWs = MakeLongPath(newFileWs);
    if (moredebug)
    {
        LogString(dllInstance, L"CopyFileFixup: WrapperWorkaround: Actual From", LongExistingFileWs.c_str());
        LogString(dllInstance, L"CopyFileFixup: WrapperWorkaround Actual To", LongNewFileWs.c_str());
    }
    HANDLE hIn = ::CreateFileW(LongExistingFileWs.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hIn != INVALID_HANDLE_VALUE)
    {
        DWORD disp = CREATE_ALWAYS;
        if (failIfExists)
        {
            disp = CREATE_NEW;
        }
        HANDLE hOut = ::CreateFileW(LongNewFileWs.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, disp, NULL);
        if (hOut != INVALID_HANDLE_VALUE)
        {
            DWORD buffsize = 4096;
            BYTE* buff = (BYTE*)malloc(buffsize);
            if (buff != NULL)
            {
                DWORD num2read = buffsize;
                DWORD numRead;
                DWORD numWriten;
                while (ReadFile(hIn, buff, num2read, &numRead, NULL))
                {
                    WriteFile(hOut, buff, numRead, &numWriten, NULL);
                }
                free(buff);
                retfinal = TRUE;
                if (moredebug)
                {
                    Log(L"[%d] CopyFileFixup: WrapperWorkaround: Success", dllInstance);
                }
            }
            CloseHandle(hOut);
        }
        else
        {
            if (moredebug)
            {
                Log(L"[%d] CopyFileFixup: WrapperWorkaround: Open output file error 0x%x", dllInstance, GetLastError());
            }
        }
        CloseHandle(hIn);
    }
    else
    {
        if (moredebug)
        {
            Log(L"[%d] CopyFileFixup: WrapperWorkaround Open input file error 0x%x", dllInstance, GetLastError());
        }
    }
    return retfinal;
}

BOOL  WRAPPER_COPYFILE(std::wstring existingFileWs, std::wstring newFileWs, BOOL failIfExists, bool debug, bool moredebug, DWORD dllInstance)
{
    BOOL retfinal;
    std::wstring LongExistingFileWs = MakeLongPath(existingFileWs);
    std::wstring LongNewFileWs = MakeLongPath(newFileWs);

    if (moredebug)
    {
        LogString(dllInstance, L"CopyFileFixup: WrapperCopyFile: Actual From", LongExistingFileWs.c_str());
        LogString(dllInstance, L"CopyFileFixup: WrapperCopyFile: Actual To", LongNewFileWs.c_str());
    }

    retfinal = impl::CopyFile(LongExistingFileWs.c_str(), LongNewFileWs.c_str(), failIfExists);
    if (retfinal == 0)
    {
        // Issue
        DWORD initialError = GetLastError();
        if (moredebug)
        {
            Log(L"[%d] CopyFileFixup: Wrapper initial FAILURE err=0x%x", dllInstance, initialError);
        }
        if (MFRConfiguration.Ilv_Aware)
        {
            // ILV can cause this error code when things should have worked, attempt to process it ourselves.
            if (initialError == ERROR_CANT_ACCESS_FILE)
            {
#if TRIED_DIDNOT_HELP
                retfinal = WRAPPER_WORKAROUND(LongExistingFileWs, LongNewFileWs, failIfExists, debug, moredebug, dllInstance);
#endif
            }
        }
    }
    if (debug)
    {
        if (retfinal)
        {
            Log(L"[%d] CopyFileFixup: return SUCCESS", dllInstance);
        }
        else
        {
            Log(L"[%d] CopyFileFixup: return FAILURE err=0x%x", dllInstance, GetLastError());
        }
    }
    return retfinal;
}


template <typename CharT>
BOOL __stdcall CopyFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName, _In_ BOOL failIfExists) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    bool debug = false;
    bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    BOOL retfinal;
    
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(dllInstance, L"CopyFileFixup from", existingFileName);
            LogString(dllInstance, L"CopyFileFixup   to", newFileName);
            Log(L"[%d] CopyFileFixup FailIfExists %d", dllInstance, failIfExists);
#endif
            std::wstring wExistingFileName = widen(existingFileName);
            std::wstring wNewFileName = widen(newFileName);
            wExistingFileName = AdjustSlashes(wExistingFileName);
            wNewFileName = AdjustSlashes(wNewFileName);

            // This get is inheirently a write operation in all cases.
            // We will always want the redirected location for the new file name.
            Cohorts cohortsExisting;
            DetermineCohorts(wExistingFileName, &cohortsExisting, moredebug, dllInstance, L"CopyFileFixup (existing)");

            Cohorts cohortsNew;
            DetermineCohorts(wNewFileName, &cohortsNew, moredebug, dllInstance, L"CopyFileFixup (new)");
            
            if (!MFRConfiguration.Ilv_Aware)
            {
                std::wstring newFileWsRedirected;
                switch (cohortsNew.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                        PreCreateFolders(newFileWsRedirected, dllInstance, L"CopyFileFixup");
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                        PreCreateFolders(newFileWsRedirected, dllInstance, L"CopyFileFixup");
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                        PreCreateFolders(newFileWsRedirected, dllInstance, L"CopyFileFixup");
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                    {
                        newFileWsRedirected = cohortsNew.WsRedirected;
                        PreCreateFolders(newFileWsRedirected, dllInstance, L"CopyFileFixup");
                    }
                    else
                    {
                        newFileWsRedirected = cohortsNew.WsRequested;
                        PreCreateFolders(newFileWsRedirected, dllInstance, L"CopyFileFixup");
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    newFileWsRedirected = cohortsNew.WsRequested;
                    break;

                case mfr::mfr_path_types::is_Protocol:
                case mfr::mfr_path_types::is_DosSpecial:
                case mfr::mfr_path_types::is_Shell:
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    newFileWsRedirected = cohortsNew.WsRequested;
                    break;
                }
#if MOREDEBUG
                Log(L"[%d] CopyFileFixup: redirected destination=%s", dllInstance, newFileWsRedirected.c_str());
#endif

#if MOREDEBUG
#if TRIED_DIDNOT_HELP
                DWORD dAtt;
#endif
#endif
                switch (cohortsExisting.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsExisting.map.Valid_mapping)
                    {
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent, or make original call to fail.
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  So the call will fail.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                    }
                    else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsExisting.map.Valid_mapping)
                    {
                        // try the redirected path, then package, then native, or let fail using original.
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (cohortsExisting.UsingNative &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsExisting.map.Valid_mapping)
                    {
                        //// try the redirected path, then package (COW), then don't need native.
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsExisting.map.Valid_mapping &&
                        !cohortsExisting.map.IsAnExclusionToRedirect)
                    {
#if MOREDEBUG
                        Log(L"[%d] CopyFileFixup: from VFS with prefer local redirection on source", dllInstance);
#endif
                        // try the redirection path, then the package (possible previous COW).
                        if (PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            if (MFRConfiguration.Ilv_Aware)
                            {
#if TRIED_DIDNOT_HELP
                                // Under IlV, we sometimes can't see the file (like Personal folder).  But let's try.
                                retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                                if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                {
                                    std::wstring ilvpath = g_writablePackageRootPath.c_str();
                                    ilvpath.append(cohortsExisting.WsRequested.substr(cohortsExisting.WsRequested.find(L"\\VFS", 0)));
#if MOREDEBUG
                                    dAtt = ::GetFileAttributes(cohortsExisting.WsRequested.c_str());
                                    Log(L"[%d] CopyFileFixup: Test GetFileAttributes Source Requested yields 0x%x on %s", dllInstance, dAtt, cohortsExisting.WsRequested.c_str());
                                    dAtt = ::GetFileAttributes(cohortsExisting.WsRedirected.c_str());
                                    Log(L"[%d] CopyFileFixup: Test GetFileAttributes Source Redirected yields 0x%x on %s", dllInstance, dAtt, cohortsExisting.WsRedirected.c_str());
                                    dAtt = ::GetFileAttributes(cohortsExisting.WsNative.c_str());
                                    Log(L"[%d] CopyFileFixup: Test GetFileAttributes Source Native yields 0x%x on %s", dllInstance, dAtt, cohortsExisting.WsNative.c_str());
                                    dAtt = ::GetFileAttributes(ilvpath.c_str());
                                    Log(L"[%d] CopyFileFixup: Test GetFileAttributes Source Munged yields 0x%x on %s", dllInstance, dAtt, ilvpath.c_str());
#endif
#if MOREDEBUG
                                    Log(L"[%d] CopyFileFixup: try this as source %s", dllInstance, ilvpath.c_str());
#endif
                                    retfinal = WRAPPER_COPYFILE(ilvpath, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                                    if (!retfinal)
                                    {
                                        std::wstring ilvpathDest = g_writablePackageRootPath.c_str();
                                        ilvpathDest.append(cohortsExisting.WsRequested.substr(cohortsNew.WsPackage.find(L"\\VFS", 0)));
#if MOREDEBUG
                                        dAtt = ::GetFileAttributes(ilvpathDest.c_str());
                                        Log(L"[%d] CopyFileFixup: Test GetFileAttributes Dest Munged yields 0x%x on %s", dllInstance, dAtt, ilvpathDest.c_str());
#endif
                                        retfinal = WRAPPER_COPYFILE(ilvpath, ilvpathDest, failIfExists, debug, moredebug, dllInstance);
                                    }
                                    return retfinal;
                                }
#else
                                // There isn't such a file anywhere.  Let the call fails as requested.
                                retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                                return retfinal;
#endif
                            }
                            else
                            {
                                // There isn't such a file anywhere.  Let the call fails as requested.
                                retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                                return retfinal;
                            }
                        }
                    }
                    else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsExisting.map.Valid_mapping)
                    {
#if MOREDEBUG
                        Log(L"[%d] CopyFileFixup: from VFS with prefer traditional redirection on source", dllInstance);
#endif
                        // try the redirection path, then the package (COW), then native (possibly COW)
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (cohortsExisting.UsingNative &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsExisting.map.Valid_mapping)
                    {
                        // try the redirected path, then package (COW), then possibly native (Possibly COW).
                        if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else if (cohortsExisting.UsingNative &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    break;
                case mfr::mfr_path_types::is_Protocol:
                case mfr::mfr_path_types::is_DosSpecial:
                case mfr::mfr_path_types::is_Shell:
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    break;
                }
            }
            else
            {
                // ILV
                std::wstring usePathNew = DetermineIlvPathForWriteOperations(cohortsNew, dllInstance, moredebug);
#if MOREDEBUG
                LogString(dllInstance, L"CopyFileFixup ILV UseTo", usePathNew.c_str());
#endif                
                // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                // if-and-only-if they are present in the package.
                PreCreateLocalFoldersIfNeededForWrite(usePathNew, cohortsNew.WsPackage, dllInstance, debug, L"CopyFileFixup");
                // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                CowLocalFoldersIfNeededForWrite(usePathNew, cohortsNew.WsPackage, dllInstance, debug, L"CopyFileFixup");
                // In a write to package scenario, folders may be needed.
                PreCreatePackageFoldersIfIlvNeededForWrite(usePathNew, dllInstance, debug, L"CopyFileFixup");

                std::wstring usePathExisting = DetermineIlvPathForReadOperations(cohortsExisting, dllInstance, moredebug);
#if MOREDEBUG
                LogString(dllInstance, L"CopyFileFixup ILV UseFrom", usePathExisting.c_str());
#endif
                // In a redirect to local scenario, we are responsible for determing if source is local or in package
                usePathExisting = SelectLocalOrPackageForRead(usePathExisting, cohortsExisting.WsPackage);
                
                retfinal = WRAPPER_COPYFILE(usePathExisting, usePathNew, failIfExists, debug, debug, dllInstance);
                return retfinal;
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"CopyFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] CopyFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (existingFileName != nullptr && newFileName != nullptr)
    {
        std::wstring LongFileName1 = MakeLongPath(widen(existingFileName));
        std::wstring LongFileName2 = MakeLongPath(widen(newFileName));
        retfinal =  impl::CopyFile(LongFileName1.c_str(), LongFileName2.c_str(), failIfExists);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; // impl::CopyFile(existingFileName, newFileName, failIfExists);
    }
#if _DEBUG
    Log(L"[%d] CopyFile returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CopyFile, CopyFileFixup);


