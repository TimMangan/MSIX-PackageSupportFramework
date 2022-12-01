//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-deletefilea

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"


BOOL  WRAPPER_DELETEFILE(std::wstring theDeletingFile, DWORD dllInstance, bool debug)
{
    std::wstring LongDeletingFile = MakeLongPath(theDeletingFile);
    BOOL retfinal = impl::DeleteFileW(LongDeletingFile.c_str());
    if (debug)
    {
        if (retfinal == 0)
        {
            Log(L"[%d] DeleteFile returns result FAILURE 0x%x on file '%s'", dllInstance, GetLastError(), LongDeletingFile.c_str());
        }
        else
        {
            Log(L"[%d] DeleteFile returns result SUCCESS 0x%x on file '%s'", dllInstance, retfinal, LongDeletingFile.c_str());
        }
    }
    return retfinal;
}


template <typename CharT>
BOOL __stdcall DeleteFileFixup(_In_ const CharT* pathName) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;
    try
    {
        if (guard)
        {
            std::wstring wPathName = widen(pathName);
            wPathName = AdjustSlashes(wPathName);

#if _DEBUG
            LogString(dllInstance, L"DeleteFileFixup for pathName", wPathName.c_str());
#endif

            Cohorts cohorts;
            DetermineCohorts(wPathName, &cohorts, moredebug, dllInstance, L"DeleteFileFixup");

            // This get is inheirently a write operation in all cases.
            // There is no need to COW, just create the redirected folder, but may need to create parent folders first.

            switch (cohorts.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohorts.map.Valid_mapping &&
                    cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        // Still do this to set attributes
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsRedirected, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                            Log("[%d] DeleteFileFixup: Resetting return code to ERROR_ACCESS_DENIED.", dllInstance);
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.", dllInstance);
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere. 
                        retfinal = FALSE;
                        SetLastError(ERROR_FILE_NOT_FOUND);  // not important if PATH or FILE not found.
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                else if (cohorts.map.Valid_mapping &&
                         (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsRedirected, dllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.", dllInstance);
#endif
                        return retfinal;
                    }
                    else if (cohorts.UsingNative &&
                             PathExists(cohorts.WsNative.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsNative, dllInstance, debug);
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.
                        return WRAPPER_DELETEFILE(cohorts.WsRequested, dllInstance, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                if (cohorts.map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsRedirected, dllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Ssetting return code to ERROR_ACCESS_DENIED.", dllInstance);
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.
                        retfinal = false;
                        SetLastError(ERROR_FILE_NOT_FOUND); // doesn't matter if path or file not found.
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Ssetting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                if (cohorts.map.Valid_mapping &&
                    cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                {
                    // try the redirection path, then the package (COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsRedirected, dllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = false;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.", dllInstance);
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.  
                        retfinal = false;
                        SetLastError(ERROR_FILE_NOT_FOUND); // not important if file or path
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.");
#endif
                        return retfinal;
                    }
                }
                else if (cohorts.map.Valid_mapping &&
                         (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsRedirected, dllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.", dllInstance);
#endif
                        return retfinal;
                    }
                    else if (cohorts.UsingNative &&
                             PathExists(cohorts.WsNative.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsNative, dllInstance, debug);
                        return retfinal;
                    }
                    else
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_FILE_NOT_FOUND);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.", dllInstance);
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohorts.map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsRedirected, dllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = FALSE;
                        SetLastError(ERROR_ACCESS_DENIED);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_ACCESS_DENIED.", dllInstance);
#endif
                        return retfinal;
                    }
                    else if (cohorts.UsingNative &&
                             PathExists(cohorts.WsNative.c_str()))
                    {
                        retfinal = WRAPPER_DELETEFILE(cohorts.WsNative, dllInstance, debug);
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.
                        retfinal = FALSE;
                        SetLastError(ERROR_FILE_NOT_FOUND);
#if _DEBUG
                        Log("[%d] DeleteFileFixup: Setting return code to ERROR_FILE_NOT_FOUND.", dllInstance);
#endif
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
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"DeleteFile")
#else
    catch (...)
    {
        Log(L"[%d] DeleteFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (pathName != nullptr)
    {
        std::wstring LongDeletingFile = MakeLongPath(widen(pathName));
        retfinal =  impl::DeleteFile(LongDeletingFile.c_str());
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; //  impl::DeleteFile(pathName);
    }
#if _DEBUG
    Log(L"[%d] DeleteFileFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::DeleteFile, DeleteFileFixup);
