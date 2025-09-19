//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createdirectoryexw

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// See DESIGN NOTE in CreateDirectory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define IMPROVE_RETURN_ACCURACY 1

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"

BOOL WRAPPER_CREATEDIRECTORYEX(Json_Debug_Levels debugRequestLevel, std::wstring theTemplateDirectory, std::wstring theDestinationDirectory, LPSECURITY_ATTRIBUTES securityAttributes, DWORD dllInstance)
{
    std::wstring LongTemplateDirectory = MakeLongPath(theTemplateDirectory);
    std::wstring LongDestinationDirectory = MakeLongPath(theDestinationDirectory);
    BOOL retfinal = impl::CreateDirectoryExW(LongTemplateDirectory.c_str(), LongDestinationDirectory.c_str(), securityAttributes);
    Log(debugRequestLevel, L"[%s%d] CreateDirectoryEx uses template '%s'", g_MfrModuleName, dllInstance, LongTemplateDirectory.c_str());

    if (retfinal == 0)
    {
        DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS)
        {
            Log(debugRequestLevel, L"[%s%d] CreateDirectoryEx returns FAILURE 0x%x Error=ALREADY_EXISTS(0x%x) and directory '%s' not found", g_MfrModuleName, dllInstance, retfinal, error, LongDestinationDirectory.c_str());
        }
        else if (error == ERROR_FILE_NOT_FOUND)
        {
            Log(debugRequestLevel, L"[%s%d] CreateDirectoryEx returns FAILURE 0x%x Error=FILE_NOT_FOUND(0x%x) and directory '%s' not found", g_MfrModuleName, dllInstance, retfinal, error, LongDestinationDirectory.c_str());
        }
        else if (error == ERROR_PATH_NOT_FOUND)
        {
            Log(debugRequestLevel, L"[%s%d] CreateDirectoryEx returns FAILURE 0x%x Error=PATH_NOT_FOUND(0x%x) and directory '%s' not found", g_MfrModuleName, dllInstance, retfinal, error, LongDestinationDirectory.c_str());
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] CreateDirectoryEx returns FAILURE 0x%x Error=0x%x and directory '%s'", g_MfrModuleName, dllInstance, retfinal, error, LongDestinationDirectory.c_str());
        }
    }
    else
    {
        Log(debugRequestLevel, L"[%s%d] CreateDirectoryEx returns SUCCESS 0x%x and directory '%s'", g_MfrModuleName, dllInstance, retfinal, LongDestinationDirectory.c_str());
    }
    return retfinal;
}



template <typename CharT>
BOOL __stdcall CreateDirectoryExFixup(
    _In_ const CharT* templateDirectory,
    _In_ const CharT* newDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            // This function is very much like CopyFile, except that we have a folder instead.
            dllInstance = ++g_InterceptInstance;
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateDirectoryExFixup using template", templateDirectory);
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateDirectoryExFixup to", newDirectory);

            std::wstring WtemplateDirectory = widen(templateDirectory);
            std::wstring WnewDirectory = widen(newDirectory);
            WtemplateDirectory = AdjustSlashes(WtemplateDirectory, dllInstance);
            WnewDirectory = AdjustSlashes(WnewDirectory, dllInstance);

            WtemplateDirectory = AdjustBadUNC(WtemplateDirectory, dllInstance, L"CreateDirectoryExFixup (template)");
            WnewDirectory = AdjustBadUNC(WnewDirectory, dllInstance, L"CreateDirectoryExFixup (new)");


            // This get is inherently a write operation in all cases.
            // We will always want the redirected location for the new directory.
            Cohorts cohortsTemplate;
            DetermineCohorts(LogLevel_DebugIntermediate, WtemplateDirectory, &cohortsTemplate, dllInstance, L"CreateDirectoryExFixup (template)");

            Cohorts cohortsNew;
            DetermineCohorts(LogLevel_DebugIntermediate, WnewDirectory, &cohortsNew, dllInstance, L"CreateDirectoryExFixup (directory)");
            std::wstring newDirectoryWsRedirected;

            if (!MFRConfiguration.Ilv_Aware)
            {
                switch (cohortsNew.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        newDirectoryWsRedirected = cohortsNew.WsRedirected;
                    }
                    else
                    {
                        newDirectoryWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        newDirectoryWsRedirected = cohortsNew.WsRedirected;
                    }
                    else
                    {
                        newDirectoryWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                    {
                        newDirectoryWsRedirected = cohortsNew.WsRedirected;
                    }
                    else
                    {
                        newDirectoryWsRedirected = cohortsNew.WsRequested;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsNew.map.Valid_mapping == mfr::mfr_enabled_types::enabled && cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
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
                Log(LogLevel_DebugIntermediate, L"[%s%d] CreateDirectoryExFixup: redirected destination=%s", g_MfrModuleName, dllInstance, newDirectoryWsRedirected.c_str());

                switch (cohortsTemplate.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsTemplate.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent, or make original call to fail.
                        if (cohortsTemplate.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                            PathExists(cohortsTemplate.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()))
                                {
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif                  
                            return retfinal;
                        }
                        else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()))
                                {
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif                  
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  So the call will fail.
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, dllInstance);
                            return retfinal;
                        }
                    }
                    else if ((cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsTemplate.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirected path, then package, then native, or let fail using original.
                        if (cohortsTemplate.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                            PathExists(cohortsTemplate.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (cohortsTemplate.NativeIsValidOptionInScenario &&
                            PathExists(cohortsTemplate.WsNative.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsNative, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
#endif
                                return retfinal;
                            }
                            else
                            {
                                // There isn't such a file anywhere.  Let the call fails as requested.
                                retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, dllInstance);
                                return retfinal;
                            }
                        }
                        break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohortsTemplate.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        //// try the redirected path, then package (COW), then don't need native.
                        if (cohortsTemplate.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                            PathExists(cohortsTemplate.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local &&
                        cohortsTemplate.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirection path, then the package (COW).
                        if (cohortsTemplate.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                            PathExists(cohortsTemplate.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, dllInstance);
                            return retfinal;
                        }
                    }
                    else if ((cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                        cohortsTemplate.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs) &&
                        cohortsTemplate.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirection path, then the package (COW), then native (possibly COW)
                        if (cohortsTemplate.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                            PathExists(cohortsTemplate.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, dllInstance);
                            return retfinal;
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                        }
                        else if (cohortsTemplate.NativeIsValidOptionInScenario &&
                            PathExists(cohortsTemplate.WsNative.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsNative, newDirectoryWsRedirected, securityAttributes, dllInstance);
                            return retfinal;
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, dllInstance);
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohortsTemplate.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        // try the redirected path, then package (COW), then possibly native (Possibly COW).
                        if (cohortsTemplate.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded &&
                            PathExists(cohortsTemplate.WsRedirected.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRedirected, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohortsTemplate.WsPackage.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsPackage, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (cohortsTemplate.NativeIsValidOptionInScenario &&
                            PathExists(cohortsTemplate.WsNative.c_str()))
                        {
                            PreCreateFolders(newDirectoryWsRedirected.c_str(), dllInstance, L"CreateDirectoryExFixup");
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsNative, newDirectoryWsRedirected, securityAttributes, dllInstance);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohortsNew.WsPackage.c_str()) ||
                                    (cohortsNew.NativeIsValidOptionInScenario && PathExists(cohortsNew.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
                                    Log(LogLevel_DebugBasic, "[%s%d] CreateDirectoryExFixup: Resetting return code to ERROR_ALREADY_EXISTS.", g_MfrModuleName, dllInstance);
                                }
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  Let the call fails as requested.
                            retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, cohortsTemplate.WsRequested, newDirectoryWsRedirected, securityAttributes, dllInstance);
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
            else
            {
                // ILVAware
                std::wstring UseNewDir = DetermineIlvPathForWriteOperations(LogLevel_DebugIntermediate, cohortsNew, dllInstance);
                // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                // if-and-only-if they are present in the package.
                PreCreateLocalFoldersIfNeededForWrite(LogLevel_DebugBasic, UseNewDir, cohortsNew.WsPackage, dllInstance, L"CreateDirectoryExFixup");
                if (!cohortsNew.NativeIsValidOptionInScenario)
                {
                    PreCreatePackageFoldersIfIlvNeededForWrite(LogLevel_DebugBasic, UseNewDir, dllInstance, L"CreateDirectoryExFixup");
                }
                std::wstring UseTemplate = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohortsTemplate, dllInstance);
                UseTemplate = SelectLocalOrPackageForRead(UseTemplate, cohortsTemplate.WsPackage);

                retfinal = WRAPPER_CREATEDIRECTORYEX(LogLevel_DebugBasic, UseTemplate, UseNewDir, securityAttributes, dllInstance);
                return retfinal;
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateDirectoryExFixup")

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
    Log(LogLevel_DebugBasic, L"[%s%d] CreateDirectoryExFixup returns 0x%x", g_MfrModuleName, dllInstance, retfinal);
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateDirectoryEx, CreateDirectoryExFixup);


