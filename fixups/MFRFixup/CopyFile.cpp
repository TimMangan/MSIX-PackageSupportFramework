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


#define  WRAPPER_COPYFILE(existingFileWs, newFileWs, failIfExists, debug, moredebug) \
    { \
        std::wstring LongExistingFileWs = MakeLongPath(existingFileWs); \
        std::wstring LongNewFileWs = MakeLongPath(newFileWs); \
        retfinal = impl::CopyFile(LongExistingFileWs.c_str(), LongNewFileWs.c_str(), failIfExists); \
        if (moredebug) \
        { \
            LogString(dllInstance, L"CopyFileFixup: Actual From", LongExistingFileWs.c_str()); \
            LogString(dllInstance, L"CopyFileFixup: Actual To", LongNewFileWs.c_str()); \
        } \
        if (debug) \
        { \
            if (retfinal) \
            { \
                Log(L"[%d] CopyFileFixup: return SUCCESS", dllInstance); \
            } \
            else \
            { \
                Log(L"[%d] CopyFileFixup: return FAILURE err=0x%x", dllInstance, GetLastError()); \
            } \
        } \
        return retfinal; \
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
                        WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  So the call will fail.
                        WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                }
                else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs ) &&
                         cohortsExisting.map.Valid_mapping)
                {
                    // try the redirected path, then package, then native, or let fail using original.
                    if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (cohortsExisting.UsingNative &&
                             PathExists(cohortsExisting.WsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug);
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
                        WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                if (cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    cohortsExisting.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                }
                else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized  ||
                          cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                         cohortsExisting.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (cohortsExisting.UsingNative &&
                             PathExists(cohortsExisting.WsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug);
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
                        WRAPPER_COPYFILE(cohortsExisting.WsRedirected, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsPackage, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else if (cohortsExisting.UsingNative &&
                             PathExists(cohortsExisting.WsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFileFixup");
                        WRAPPER_COPYFILE(cohortsExisting.WsNative, newFileWsRedirected, failIfExists, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE(cohortsExisting.WsRequested, newFileWsRedirected, failIfExists, debug, moredebug);
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


