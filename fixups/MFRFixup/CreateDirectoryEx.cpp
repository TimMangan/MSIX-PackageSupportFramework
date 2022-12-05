//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createdirectoryexw


#if _DEBUG
#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"



#define WRAPPER_CREATEDIRECTORYEX(theTemplateDirectory, theDestinationDirectory, securityAttributes, debug, moredebug) \
    { \
        std::wstring LongTemplateDirectory = MakeLongPath(theTemplateDirectory); \
        std::wstring LongDestinationDirectory = MakeLongPath(theDestinationDirectory); \
        retfinal = impl::CreateDirectoryExW(LongTemplateDirectory.c_str(), LongDestinationDirectory.c_str(), securityAttributes); \
        if (moredebug) \
        { \
            Log(L"[%d] CreateDirectoryEx uses template '%s'", dllInstance, LongTemplateDirectory.c_str()); \
        } \
        if (debug) \
        { \
            if (retfinal == 0) \
            { \
                Log(L"[%d] CreateDirectoryEx returns FAILURE 0x%x and directory '%s'", dllInstance, retfinal, LongDestinationDirectory.c_str()); \
            } \
            else \
            { \
                Log(L"[%d] CreateDirectoryEx returns SUCCESS 0x%x and directory '%s'", dllInstance, retfinal, LongDestinationDirectory.c_str()); \
            } \
        } \
        return retfinal; \
    }



template <typename CharT>
BOOL __stdcall CreateDirectoryExFixup(
    _In_ const CharT* templateDirectory,
    _In_ const CharT* newDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // This function is very much like CopyFile, except that we have a folder instead.
            dllInstance = ++g_InterceptInstance;
#if _DEBUG
            LogString(dllInstance, L"CreateDirectoryExFixup using template", templateDirectory);
            LogString(dllInstance, L"CreateDirectoryExFixup to", newDirectory);
#endif
            std::wstring WtemplateDirectory = widen(templateDirectory);
            std::wstring WnewDirectory = widen(newDirectory);
            WtemplateDirectory = AdjustSlashes(WtemplateDirectory);
            WnewDirectory = AdjustSlashes(WnewDirectory);

            // This get is inheirently a write operation in all cases.
            // We will always want the redirected location for the new directory.
            Cohorts cohortsTemplate;
            DetermineCohorts(WtemplateDirectory, &cohortsTemplate, moredebug, dllInstance, L"CreateDirectoryExFixup (template)");

            Cohorts cohortsNew;
            DetermineCohorts(WnewDirectory, &cohortsNew, moredebug, dllInstance, L"CreateDirectoryExFixup (directory)");
            std::wstring newDirectoryWsRedirected;


            switch (cohortsNew.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                {
                    newDirectoryWsRedirected = cohortsNew.WsRedirected;
                }
                else
                {
                    newDirectoryWsRedirected = cohortsNew.WsRequested;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                {
                    newDirectoryWsRedirected = cohortsNew.WsRedirected;
                }
                else
                {
                    newDirectoryWsRedirected = cohortsNew.WsRequested;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                {
                    newDirectoryWsRedirected = cohortsNew.WsRedirected;
                }
                else
                {
                    newDirectoryWsRedirected = cohortsNew.WsRequested;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohortsNew.map.Valid_mapping && !cohortsNew.map.IsAnExclusionToRedirect)
                {
                    newDirectoryWsRedirected = cohortsNew.WsRedirected;
                }
                else
                {
                    newDirectoryWsRedirected = cohortsNew.WsRequested;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                newDirectoryWsRedirected = cohortsNew.WsRequested;
                break;
            case mfr::mfr_path_types::is_Protocol:
            case mfr::mfr_path_types::is_DosSpecial:
            case mfr::mfr_path_types::is_Shell:
            case mfr::mfr_path_types::in_other_drive_area:
            case mfr::mfr_path_types::is_UNC_path:
            case mfr::mfr_path_types::unsupported_for_intercepts:
            case mfr::mfr_path_types::unknown:
            default:
                newDirectoryWsRedirected = cohortsNew.WsRequested;
                break;
            }
#if MOREDEBUG
            Log(L"[%d] CreateDirectoryExFixup: redirected destination=%s", dllInstance, newDirectoryWsRedirected.c_str());
#endif


            switch (cohortsTemplate.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    cohortsTemplate.map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent, or make original call to fail.
                    if (!cohortsTemplate.map.IsAnExclusionToRedirect && PathExists(cohortsTemplate.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  So the call will fail.
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                else if ((cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                         cohortsTemplate.map.Valid_mapping)
                {
                    // try the redirected path, then package, then native, or let fail using original.
                    if (!cohortsTemplate.map.IsAnExclusionToRedirect && PathExists(cohortsTemplate.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (cohortsTemplate.UsingNative &&
                             PathExists(cohortsTemplate.WsNative.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsNative, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                if (cohortsTemplate.map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    if (!cohortsTemplate.map.IsAnExclusionToRedirect && PathExists(cohortsTemplate.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                if (cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                    cohortsTemplate.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    if (!cohortsTemplate.map.IsAnExclusionToRedirect && PathExists(cohortsTemplate.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                else if ((cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                          cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                         cohortsTemplate.map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    if (!cohortsTemplate.map.IsAnExclusionToRedirect && PathExists(cohortsTemplate.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (cohortsTemplate.UsingNative &&
                             PathExists(cohortsTemplate.WsNative.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsNative, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                if (cohortsTemplate.map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    if (!cohortsTemplate.map.IsAnExclusionToRedirect && PathExists(cohortsTemplate.WsRedirected.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else if (cohortsTemplate.UsingNative &&
                             PathExists(cohortsTemplate.WsNative.c_str()))
                    {
                        PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsNative, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  Let the call fails as requested.
                        WRAPPER_CREATEDIRECTORYEX(cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, debug, moredebug);
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
    LOGGED_CATCHHANDLER(dllInstance, L"CreateDirectoryExFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateDirectoryExFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    if (templateDirectory != nullptr && newDirectory != nullptr)
    {
        std::wstring LongDirectory1 = MakeLongPath(widen(templateDirectory));
        std::wstring LongDirectory2 = MakeLongPath(widen(newDirectory));
        retfinal = impl::CreateDirectoryEx(LongDirectory1.c_str(), LongDirectory2.c_str(), securityAttributes);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; //impl::CreateDirectoryEx(templateDirectory, newDirectory, securityAttributes);
    }
#if _DEBUG
    Log(L"[%d] CreateDirectoryExFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateDirectoryEx, CreateDirectoryExFixup);
