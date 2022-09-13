//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew

#if _DEBUG
#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"




HANDLE  WRAPPER_CREATEFILE(std::wstring theDestinationFile,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile, 
    DWORD dllInstance, bool debug)
{
    std::wstring LongDestinationFile = MakeLongPath(theDestinationFile);
    HANDLE retfinal = impl::CreateFileW(LongDestinationFile.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
    if (debug)
    {
        Log(L"[%d] CreateFile returns file '%s' and result 0x%x", dllInstance, LongDestinationFile.c_str(), retfinal);
    }
    return retfinal;
}


template <typename CharT>
HANDLE __stdcall CreateFileFixup(_In_ const CharT* pathName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile) noexcept
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
            std::wstring wPathName = widen(pathName);

            if (wPathName.find(L"VFS\\Common AppData\\folder") != std::wstring::npos)
            {
                debug=true;
            }
#if _DEBUG
            LogString(dllInstance, L"CreateFileFixup for path", pathName);
#endif
            std::replace(wPathName.begin(), wPathName.end(), L'/', L'\\');

            // This get is may or may not be a write operation.
            // There may be a need to COW, jand may need to create parent folders in redirection area first.
            Cohorts cohorts;
            DetermineCohorts(wPathName, &cohorts, moredebug, dllInstance, L"CreateFileFixup");

            switch (cohorts.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohorts.map.Valid_mapping &&
                    cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent
                    if (PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;
                }
                if (cohorts.map.Valid_mapping &&
                    (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                { 
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    if (PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (cohorts.UsingNative &&
                        PathExists(cohorts.WsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (cohorts.UsingNative &&
                        PathExists(cohorts.WsNative.c_str()))
                    {
                        if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            ///PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;                   
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                if (cohorts.map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                   if (PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            //PreCreateFolders(testWsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                if (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    cohorts.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    if (PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;
                }
                else if (cohorts.map.Valid_mapping &&
                         (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    if (PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (cohorts.UsingNative &&
                        PathExists(cohorts.WsNative.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohorts.map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    if (PathExists(cohorts.WsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                        return retfinal;
                    }
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsPackage, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (cohorts.UsingNative &&
                        PathExists(cohorts.WsNative.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"CreateFileFixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                        else
                        {
                            retfinal = WRAPPER_CREATEFILE(cohorts.WsNative, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateFileFixup");
                    retfinal = WRAPPER_CREATEFILE(cohorts.WsRedirected, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile, dllInstance, debug);
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
    LOGGED_CATCHHANDLER(dllInstance, L"CreateFileFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateFileFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (pathName != nullptr)
    {
        std::wstring LongDirectory = MakeLongPath(widen(pathName));
        retfinal = impl::CreateFile(LongDirectory.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = INVALID_HANDLE_VALUE; //impl::CreateFile(pathName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
    }
#if _DEBUG
    Log(L"[%d] CreateFileFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateFile, CreateFileFixup);
