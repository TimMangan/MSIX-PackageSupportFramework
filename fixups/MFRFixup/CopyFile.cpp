//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-copyfile



#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:CopyFileFixupAnsi_Fixup=impl::_CopyFileFixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CopyFileFixupWide_Fixup=impl::_CopyFileFixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:WRAPPER_COPYFILE=_WRAPPER_COPYFILE.wide")
#else
#pragma comment(linker, "/EXPORT:CopyFileFixupAnsi_Fixup=impl::CopyFileFixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CopyFileFixupWide_Fixup=impl::CopyFileFixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:WRAPPER_COPYFILE=WRAPPER_COPYFILE.wide")
#endif

#if TRIED_DIDNOT_HELP
BOOL WRAPPER_WORKAROUND(Json_Debug_Levels debugRequestLevel, std::wstring existingFileWs, std::wstring newFileWs, BOOL failIfExists,  DWORD dllInstance)
{
    BOOL retfinal = FALSE;
    std::wstring LongExistingFileWs = MakeLongPath(existingFileWs);
    std::wstring LongNewFileWs = MakeLongPath(newFileWs);
    
    LogString(debugRequestLevel, g_MfrModuleName, dllInstance, L"CopyFileFixup: WrapperWorkaround: Actual From", LongExistingFileWs.c_str());
    LogString(debugRequestLevel, g_MfrModuleName, dllInstance, L"CopyFileFixup: WrapperWorkaround Actual To", LongNewFileWs.c_str());
    
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
                Log(debugRequestLevel, L"[%s%d] CopyFileFixup: WrapperWorkaround: Success", g_MfrModuleName, dllInstance);
            }
            CloseHandle(hOut);
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] CopyFileFixup: WrapperWorkaround: Open output file error 0x%x", g_MfrModuleName, dllInstance, GetLastError());
        }
        CloseHandle(hIn);
    }
    else
    {
        Log(debugRequestLevel, L"[%s%d] CopyFileFixup: WrapperWorkaround Open input file error 0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }
    return retfinal;
}
#endif

BOOL  WRAPPER_COPYFILE(std::wstring existingFileWs, std::wstring newFileWs, BOOL failIfExists, DWORD dllInstance)
{
    BOOL retfinal;
    std::wstring LongExistingFileWs = MakeLongPath(existingFileWs);
    std::wstring LongNewFileWs = MakeLongPath(newFileWs);

    LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFileFixup: WrapperCopyFile: Actual From", LongExistingFileWs.c_str());
    LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFileFixup: WrapperCopyFile: Actual To", LongNewFileWs.c_str());

    retfinal = impl::CopyFile(LongExistingFileWs.c_str(), LongNewFileWs.c_str(), failIfExists);
    if (retfinal == 0)
    {
        // Issue
        DWORD initialError = GetLastError();
        Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: Wrapper initial FAILURE err=0x%x", g_MfrModuleName, dllInstance, initialError);
        if (MFRConfiguration.Ilv_Aware)
        {
            // ILV can cause this error code when things should have worked, attempt to process it ourselves.
            if (initialError == ERROR_CANT_ACCESS_FILE)
            {
#if TRIED_DIDNOT_HELP
                retfinal = WRAPPER_WORKAROUND(LogLevel_DebugIntermediate, LongExistingFileWs, LongNewFileWs, failIfExists, debug, moredebug, dllInstance);
#endif
            }
        }
    }

    if (retfinal)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] CopyFileFixup: return SUCCESS", g_MfrModuleName, dllInstance);
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] CopyFileFixup: return FAILURE err=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }

    return retfinal;
}


template <typename CharT>
BOOL __stdcall CopyFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName, _In_ BOOL failIfExists) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    BOOL retfinal;
    
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyFileFixup from", existingFileName);
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyFileFixup   to", newFileName);
            Log(LogLevel_DebugBasic, L"[%s%d] CopyFileFixup FailIfExists %d", g_MfrModuleName, dllInstance, failIfExists);

            std::wstring wExistingFileName = widen(existingFileName);
            std::wstring wNewFileName = widen(newFileName);
            wExistingFileName = AdjustSlashes(wExistingFileName, dllInstance);
            wNewFileName = AdjustSlashes(wNewFileName, dllInstance);

            wExistingFileName = AdjustBadUNC(wExistingFileName, dllInstance, L"CopyFileFixup (existing)");
            wNewFileName = AdjustBadUNC(wNewFileName, dllInstance, L"CopyFileFixup (new)");
            

            // This get is inherently a write operation in all cases.
            // We will always want the redirected location for the new file name.
            Cohorts cohortsExisting;
            DetermineCohorts(LogLevel_DebugIntermediate, wExistingFileName, &cohortsExisting, dllInstance, L"CopyFileFixup (existing)");

            Cohorts cohortsNew;
            DetermineCohorts(LogLevel_DebugIntermediate, wNewFileName, &cohortsNew, dllInstance, L"CopyFileFixup (new)");
            
            if (!MFRConfiguration.Ilv_Aware)
            {
                std::wstring newFileWsRedirected;
                switch (cohortsNew.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
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
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
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
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
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
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
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
                Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: redirected destination=%s", g_MfrModuleName, dllInstance, newFileWsRedirected.c_str());

#if MOREDEBUG
#if TRIED_DIDNOT_HELP
                DWORD dAtt;
#endif
#endif
                switch (cohortsExisting.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent, or make original call to fail.
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  So the call will fail.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                    }
                    else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled )
                    {
                        // try the redirected path, then package, then native, or let fail using original.
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        //// try the redirected path, then package (COW), then don't need native.
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled && 
                        cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
#if MOREDEBUG
                        Log(L"[%s%d] CopyFileFixup: from VFS with prefer local redirection on source", g_MfrModuleName, dllInstance);
#endif
                        // try the redirection path, then the package (possible previous COW).
                        if (PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            if (MFRConfiguration.Ilv_Aware)
                            {
#if TRIED_DIDNOT_HELP
                                // Under IlV, we sometimes can't see the file (like Personal folder).  But let's try.
                                retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
                                if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                {
                                    std::wstring ilvpath = g_writablePackageRootPath.c_str();
                                    ilvpath.append(cohortsExisting.WsRequested.substr(cohortsExisting.WsRequested.find(L"\\VFS", 0)));

                                    dAtt = ::GetFileAttributes(cohortsExisting.WsRequested.c_str());
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: Test GetFileAttributes Source Requested yields 0x%x on %s", g_MfrModuleName, dllInstance, dAtt, cohortsExisting.WsRequested.c_str());
                                    dAtt = ::GetFileAttributes(cohortsExisting.WsRedirected.c_str());
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: Test GetFileAttributes Source Redirected yields 0x%x on %s", g_MfrModuleName, dllInstance, dAtt, cohortsExisting.WsRedirected.c_str());
                                    dAtt = ::GetFileAttributes(cohortsExisting.WsNative.c_str());
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: Test GetFileAttributes Source Native yields 0x%x on %s", g_MfrModuleName, dllInstance, dAtt, cohortsExisting.WsNative.c_str());
                                    dAtt = ::GetFileAttributes(ilvpath.c_str());
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: Test GetFileAttributes Source Munged yields 0x%x on %s", g_MfrModuleName, dllInstance, dAtt, ilvpath.c_str());

                                    Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: try this as source %s", g_MfrModuleName, dllInstance, ilvpath.c_str());
                                    retfinal = WRAPPER_COPYFILE(ilvpath, newFileWsRedirected, failIfExists, dllInstance);
                                    if (!retfinal)
                                    {
                                        std::wstring ilvpathDest = g_writablePackageRootPath.c_str();
                                        ilvpathDest.append(cohortsExisting.WsRequested.substr(cohortsNew.WsPackage.find(L"\\VFS", 0)));
                                        dAtt = ::GetFileAttributes(ilvpathDest.c_str());
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: Test GetFileAttributes Dest Munged yields 0x%x on %s", g_MfrModuleName, dllInstance, dAtt, ilvpathDest.c_str());
                                        retfinal = WRAPPER_COPYFILE(ilvpath, ilvpathDest, failIfExists, dllInstance);
                                    }
                                    return retfinal;
                                }
#else
                                // There isn't such a file anywhere.  Let the call fails as requested.
                                retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
                                return retfinal;
#endif
                            }
                            else
                            {
                                // There isn't such a file anywhere.  Let the call fails as requested.
                                retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
                                return retfinal;
                            }
                        }
                    }
                    else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        Log(LogLevel_DebugIntermediate, L"[%s%d] CopyFileFixup: from VFS with prefer traditional redirection on source", g_MfrModuleName, dllInstance);
                        // try the redirection path, then the package (COW), then native (possibly COW)
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirected path, then package (COW), then possibly native (Possibly COW).
                        if (cohortsExisting.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded  && 
                            PathExists(cohortsExisting.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (PathExists(cohortsExisting.WsPackage.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else if (cohortsExisting.NativeIsValidOptionInScenario &&
                            PathExists(cohortsExisting.WsNative.c_str()))
                        {
                            PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, dllInstance);
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, dllInstance);
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
                std::wstring usePathNew = DetermineIlvPathForWriteOperations(LogLevel_DebugIntermediate, cohortsNew, dllInstance);
                LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFileFixup ILV UseTo", usePathNew.c_str());

                // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                // if-and-only-if they are present in the package.
                PreCreateLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, usePathNew, cohortsNew.WsPackage, dllInstance, L"CopyFileFixup");
                // In a redirect to local scenario, if the file is not present locally, but is in the package, we are responsible to copy it there first.
                CowLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, usePathNew, cohortsNew.WsPackage, dllInstance, L"CopyFileFixup");
                // In a write to package scenario, folders may be needed.
                PreCreatePackageFoldersIfIlvNeededForWrite(LogLevel_DebugBasic, usePathNew, dllInstance, L"CopyFileFixup");

                std::wstring usePathExisting = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohortsExisting, dllInstance);
                LogString(LogLevel_DebugIntermediate, g_MfrModuleName, dllInstance, L"CopyFileFixup ILV UseFrom", usePathExisting.c_str());

                // In a redirect to local scenario, we are responsible for determining if source is local or in package
                usePathExisting = SelectLocalOrPackageForRead(usePathExisting, cohortsExisting.WsPackage);
                
                retfinal = WRAPPER_COPYFILE(usePathExisting, usePathNew, failIfExists, dllInstance);
                return retfinal;
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyFileFixup")


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
    Log(LogLevel_DebugBasic, L"[%s%d] CopyFile returns 0x%x", g_MfrModuleName, dllInstance, retfinal);
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CopyFile, CopyFileFixup);


