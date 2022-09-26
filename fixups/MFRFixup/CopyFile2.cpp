// ------------------------------------------------------------------------------------------------------ -
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfile2

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"


#define  WRAPPER_COPYFILE2(existingFileWs, newFileWs, extendedParameters, debug, moredebug) \
    { \
        std::wstring LongExistingFileWs = MakeLongPath(existingFileWs); \
        std::wstring LongNewFileWs = MakeLongPath(newFileWs); \
        retfinal = impl::CopyFile2(LongExistingFileWs.c_str(), LongNewFileWs.c_str(), extendedParameters); \
        if (moredebug) \
        { \
            LogString(dllInstance, L"CopyFile2Fixup: Actual From", LongExistingFileWs.c_str()); \
            LogString(dllInstance, L"CopyFile2Fixup: Actual To", LongNewFileWs.c_str()); \
        } \
        if (debug) \
        { \
            if (retfinal == ERROR_SUCCESS) \
            { \
                Log(L"[%d] CopyFile2Fixup: return SUCCESS", dllInstance); \
            } \
            else \
            { \
                Log(L"[%d] CopyFile2Fixup: return FAIL err=0x%x", dllInstance, GetLastError()); \
            } \
        } \
        return (retfinal); \
    }


HRESULT __stdcall CopyFile2Fixup(
    _In_ PCWSTR existingFileName,
    _In_ PCWSTR newFileName,
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
    [[maybe_unused]] bool moredebug = false;
#if _DEBUG
    debug = true;
#if MOREDEBUG
    moredebug = true;
#endif
#endif
    [[maybe_unused]] HRESULT retfinal;

    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(dllInstance, L"CopyFile2Fixup from", existingFileName);
            LogString(dllInstance, L"CopyFile2Fixup to", newFileName);
#endif
            std::wstring wExistingFileName = widen(existingFileName);
            std::wstring wNewFileName = widen(newFileName);
            wExistingFileName = AdjustSlashes(wExistingFileName);
            wNewFileName = AdjustSlashes(wNewFileName);


            // This get is inheirently a write operation in all cases.
            // We will always want the redirected location for the new file name.
            Cohorts cohortsExisting;
            DetermineCohorts(wExistingFileName, &cohortsExisting, moredebug, dllInstance, L"CopyFile2Fixup");

            Cohorts cohortsNew;
            DetermineCohorts(wNewFileName, &cohortsNew, moredebug, dllInstance, L"CopyFile2Fixup");
            std::wstring newFileWsRedirected;



            switch (cohortsNew.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                {
                    newFileWsRedirected = cohortsNew.WsRedirected;
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
                }
                else
                {
                    newFileWsRedirected = cohortsNew.WsRequested;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohortsNew.map.Valid_mapping & !cohortsNew.map.IsAnExclusionToRedirect)
                {
                    newFileWsRedirected = cohortsNew.WsRedirected;
                }
                else
                {
                    newFileWsRedirected = cohortsNew.WsRequested;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                newFileWsRedirected = cohortsNew.WsRequested;
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                newFileWsRedirected = cohortsNew.WsRequested;
                break;
            }
#if MOREDEBUG
            Log(L"[%d] CopyFile2Fixup: redirected destination=%s", dllInstance, newFileWsRedirected.c_str());
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
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  So the call will fail.
                        WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                }
                else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs ) &&
                         cohortsExisting.map.Valid_mapping)
                {
                    // try the redirected path, then package, then native, or let fail using original.
                    if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (cohortsExisting.UsingNative && 
                             PathExists(cohortsExisting.WsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsNative, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                if (cohortsExisting.map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters, debug, moredebug);
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
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                }
                else if ((cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohortsExisting.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs ) &&
                         cohortsExisting.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                   if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (cohortsExisting.UsingNative &&
                             PathExists(cohortsExisting.WsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsNative, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohortsExisting.map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    if (!cohortsExisting.map.IsAnExclusionToRedirect && PathExists(cohortsExisting.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsRedirected, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (PathExists(cohortsExisting.WsPackage.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsPackage, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else if (cohortsExisting.UsingNative &&
                             PathExists(cohortsExisting.WsNative.c_str()))
                    {
                        PreCreateFolders(newFileWsRedirected.c_str(), dllInstance, L"CopyFile2Fixup");
                        WRAPPER_COPYFILE2(cohortsExisting.WsNative, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_COPYFILE2(cohortsExisting.WsRequested, newFileWsRedirected, extendedParameters, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                break;
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_protocol_path:
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
    LOGGED_CATCHHANDLER(dllInstance, L"CopyFile2Fixup")
#else
    catch (...)
    {
        Log(L"[%d] CopyFile2Fixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (existingFileName != nullptr && newFileName != nullptr)
    {
        std::wstring LongFileName1 = MakeLongPath(widen(existingFileName));
        std::wstring LongFileName2 = MakeLongPath(widen(newFileName));
        retfinal = impl::CopyFile2(LongFileName1.c_str(), LongFileName2.c_str(), extendedParameters);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER); //impl::CopyFile2(existingFileName, newFileName, extendedParameters);
    }
#if _DEBUG
    Log(L"[%d] CopyFile2Fixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_FIXUP(impl::CopyFile2, CopyFile2Fixup);
