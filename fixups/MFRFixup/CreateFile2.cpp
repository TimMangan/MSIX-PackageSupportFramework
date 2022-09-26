//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfile2

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"



HANDLE  WRAPPER_CREATEFILE2(std::wstring theDestinationFile,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_ DWORD creationDisposition,
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams,
    DWORD dllInstance, bool debug)
{
    std::wstring LongDestinationFile = MakeLongPath(theDestinationFile);
    HANDLE retfinal = impl::CreateFile2(LongDestinationFile.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
    if (debug)
    {
        Log(L"[%d] CreateFile2 returns handle 0x%x and file '%s'", dllInstance, retfinal, LongDestinationFile.c_str());
    }
    return retfinal;
}


HANDLE __stdcall CreateFile2Fixup(
    _In_ LPCWSTR fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_ DWORD creationDisposition,
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams) noexcept
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
    HANDLE retfinal;


    try
    {
        if (guard)
        {
            std::wstring wPathName = fileName;
            wPathName = AdjustSlashes(wPathName);
#if _DEBUG
            LogString(dllInstance, L"CreateFile2Fixup for ", fileName);
            Log(L"[%d]        DesiredAccess 0x%x  ShareMode 0x%x", dllInstance, desiredAccess, shareMode);
#endif
            bool IsAWriteCase;
            if (createExParams)
            {
                IsAWriteCase = IsCreateForChange(desiredAccess, creationDisposition, createExParams->dwFileAttributes);
            }
            else
            {
                IsAWriteCase = IsCreateForChange(desiredAccess, creationDisposition, 0);
            }
#if MOREDEBUG
            Log(L"[%d] CreateFile2Fixup: Could be a write operation=%d", dllInstance, IsAWriteCase);
#endif

            // This get is may or may not be a write operation.
            // There may be a need to COW, jand may need to create parent folders in redirection area first.
            Cohorts cohorts;
            DetermineCohorts(wPathName, &cohorts, moredebug, dllInstance, L"CreateFile2Fixup");

            switch (cohorts.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohorts.map.Valid_mapping &&
                    cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                    return retfinal;
                }
                else if (cohorts.map.Valid_mapping &&
                         (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (cohorts.UsingNative &&
                        PathExists(cohorts.WsNative.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                ///PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsNative, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsNative, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                if (cohorts.map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                if (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    cohorts.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                    return retfinal;
                }
                else if (cohorts.map.Valid_mapping &&
                         (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (cohorts.UsingNative &&
                             PathExists(cohorts.WsNative.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsNative, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsNative, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohorts.map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsPackage, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (cohorts.UsingNative &&
                        PathExists(cohorts.WsNative.c_str()))
                    {
                        if (IsAWriteCase)
                        {
                            // COW is applicable first.
                            if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFile2Fixup"))
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                            else
                            {
                                retfinal = WRAPPER_CREATEFILE2(cohorts.WsNative, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE2(cohorts.WsNative, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(cohorts.WsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, dllInstance, debug);
                    return retfinal;
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
    LOGGED_CATCHHANDLER(dllInstance, L"CreateFile2Fixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateFile2Fixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (fileName != nullptr)
    {
        std::wstring LongDirectory = MakeLongPath(fileName);
        retfinal = impl::CreateFile2(LongDirectory.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = INVALID_HANDLE_VALUE; //impl::CreateFile2(fileName, desiredAccess, shareMode, creationDisposition, createExParams);
    }
#if _DEBUG
    Log(L"[%d] CreateFile2Fixup returns handle 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_FIXUP(impl::CreateFile2, CreateFile2Fixup);
