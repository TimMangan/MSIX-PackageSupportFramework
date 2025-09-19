//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation for this API: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileattributesa


#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineILVpaths.h"

#if _DEBUG
//#define DEBUGPATHTESTING 1
#include "DebugPathTesting.h"
#endif



BOOL WRAPPER_SETFILEATTRIBUTES(Json_Debug_Levels debugRequestLevel, std::wstring theDestinationFilename, DWORD fileAttributes, DWORD dllInstance)
    { 
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); 
        bool retfinal = impl::SetFileAttributesW(LongDestinationFilename.c_str(),fileAttributes); 
        if (retfinal == 0)
        {
            Log(debugRequestLevel, L"[%s%d] SetFileAttributes wrapper returns FAILURE 0x%x and file '%s'", g_MfrModuleName, dllInstance, GetLastError(), LongDestinationFilename.c_str());
        }
        else
        {
            Log(debugRequestLevel, L"[%s%d] SetFileAttributes wrapper returns SUCCESS and file '%s'", g_MfrModuleName, dllInstance, LongDestinationFilename.c_str());
        }
        return retfinal; 
    }



template <typename CharT>
BOOL __stdcall SetFileAttributesFixup(_In_ const CharT* fileName, _In_ DWORD fileAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD dllInstance = g_InterceptInstance;
    BOOL retfinal;
    try
    {
        if (guard)
        {
            dllInstance = ++g_InterceptInstance;
            std::wstring wfileName = widen(fileName);
            wfileName = AdjustSlashes(wfileName, dllInstance);

            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"SetFileAttributesFixup for fileName", wfileName.c_str());

            wfileName = AdjustBadUNC(wfileName, dllInstance, L"SetFileAttributesFixup");
            

            // This get is inherently a write operation in all cases.
            // We may need to copy the file first.
            Cohorts cohorts;
            DetermineCohorts(LogLevel_DebugIntermediate, wfileName, &cohorts, dllInstance, L"SetFileAttributesFixup");

            if (!MFRConfiguration.Ilv_Aware)
            {
                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            if (!MFRConfiguration.Ilv_Aware)
                            {
                                // try the request path, which must be the local redirected version by definition, and then a package equivalent using COW
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (LogLevel_DebugBasic, Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsPackage, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else if (PathParentExists(cohorts.WsPackage.c_str()))
                                { 
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                    PreCreateFolders(cohorts.WsRequested.c_str(), dllInstance, L"SetFileAttributes");
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributesFixup: Native Local with ILV", g_MfrModuleName, dllInstance);
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsPackage.c_str()))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRequested, fileAttributes, dllInstance);
                                    if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                    {
                                        // ILV has issues with this, fake it.
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: can't access package file; return fake success.", g_MfrModuleName, dllInstance);
                                        SetLastError(0);
                                        return TRUE;
                                    }
                                    else
                                    {
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRequested, fileAttributes, dllInstance);
                                    if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                    {
                                        // ILV has issues with this, fake it.
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: can't access requested file; return fake success.", g_MfrModuleName, dllInstance);
                                        SetLastError(0);
                                        return TRUE;
                                    }
                                    else
                                    {
                                        return retfinal;
                                    }
                                }
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            if (!MFRConfiguration.Ilv_Aware)
                            {
                                // try the redirected path, then package (via COW), then native (possibly via COW).
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsPackage, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else if (PathExists(cohorts.WsNative.c_str()))
                                {
                                    // TODO: This might not be the best way to decide is COW is appropriate.  
                                    //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                                    //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                                    if (cohorts.map.DoesRuntimeMapNativeToVFS)
                                    {
                                        if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                        {
                                            retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                            return retfinal;
                                        }
                                        else
                                        {
                                            retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsNative, fileAttributes, dllInstance);
                                            return retfinal;
                                        }
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsNative, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                    PreCreateFolders(cohorts.WsRequested.c_str(), dllInstance, L"SetFileAttributes");
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRequested, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: Native Traditional with ILV", g_MfrModuleName, dllInstance);
                                // WIth IlvAware, we can't set the attribute and get this specific error if the file is in the package.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                }
                                else
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRequested, fileAttributes, dllInstance);
                                }
                                if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                {
                                    // ILV has issues with this, fake it.
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: Can't access file; return fake success.", g_MfrModuleName, dllInstance);
                                    SetLastError(0);
                                    return TRUE;
                                }
                                else
                                {
                                    return retfinal;
                                }
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_none:
                        case mfr::mfr_redirect_flags::disabled:
                        default:
                            // just fall through to unguarded code
                            break;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            // not possible, fall through
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            if (!MFRConfiguration.Ilv_Aware)
                            {
                                //// try the redirected path, then package (COW), then don't need native.
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsPackage, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributesFixup: PVAD Traditional with ILV", g_MfrModuleName, dllInstance);
                                retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                {
                                    // ILV has issues with this, fake it.
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: can't access redirected file; return fake success.", g_MfrModuleName, dllInstance);
                                    SetLastError(0);
                                    return TRUE;
                                }
                                else
                                {
                                    return retfinal;
                                }
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_none:
                        case mfr::mfr_redirect_flags::disabled:
                        default:
                            // just fall through to unguarded code
                            break;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            if (!MFRConfiguration.Ilv_Aware)
                            {
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsPackage, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributesFixup: VFS Local with ILV", g_MfrModuleName, dllInstance);
                                if (cohorts.NativeIsValidOptionInScenario)
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                    {
                                        // ILV has issues with this, fake it.
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: can't access file; return fake success.", g_MfrModuleName, dllInstance );
                                        SetLastError(0);
                                        return TRUE;
                                    }
                                    else
                                    {
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRequested, fileAttributes, dllInstance);
                                    if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                    {
                                        // ILV has issues with this, fake it.
                                        Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: can't access requested file; return fake success.", g_MfrModuleName, dllInstance);
                                        SetLastError(0);
                                        return TRUE;
                                    }
                                    else
                                    {
                                        return retfinal;
                                    }
                                }
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            if (!MFRConfiguration.Ilv_Aware)
                            {
                                // try the redirection path, then the package (COW), then native (possibly COW)
                                if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                                else if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsPackage, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else if (PathExists(cohorts.WsNative.c_str()))
                                {
                                    // TODO: This might not be the best way to decide is COW is appropriate.  
                                    //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                                    //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                                    if (cohorts.map.DoesRuntimeMapNativeToVFS)
                                    {
                                        if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                        {
                                            retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                            return retfinal;
                                        }
                                        else
                                        {
                                            retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsNative, fileAttributes, dllInstance);
                                            return retfinal;
                                        }
                                    }
                                    else
                                    {
                                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                                        PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributes: VFS Traditional with ILV", g_MfrModuleName, dllInstance);
                                retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                                {
                                    // ILV has issues with this, fake it.
                                    Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: can't access redirected file; return fake success.", g_MfrModuleName, dllInstance);
                                    SetLastError(0);
                                    return TRUE;
                                }
                                else
                                {
                                    return retfinal;
                                }
                            }
                            break;
                        case mfr::mfr_redirect_flags::prefer_redirection_none:
                        case mfr::mfr_redirect_flags::disabled:
                        default:
                            // just fall through to unguarded code
                            break;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        if (!MFRConfiguration.Ilv_Aware)
                        {
                            // try the redirected path, then package (COW), then possibly native (Possibly COW).
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded && PathExists(cohorts.WsRedirected.c_str()))
                            {
                                retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                return retfinal;
                            }
                            else if (PathExists(cohorts.WsPackage.c_str()))
                            {
                                if (Cow(LogLevel_DebugBasic, cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                                else
                                {
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsPackage, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                            }
                            else if (cohorts.NativeIsValidOptionInScenario)
                            {
                                if (PathExists(cohorts.WsNative.c_str()))
                                {
                                    // TODO: This might not be the best way to decide is COW is appropriate.  
                                    //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                                    //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                                    if (cohorts.map.DoesRuntimeMapNativeToVFS)
                                    {
                                        if (Cow(LogLevel_DebugBasic, cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                        {
                                            retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                            return retfinal;
                                        }
                                        else
                                        {
                                            retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsNative, fileAttributes, dllInstance);
                                            return retfinal;
                                        }
                                    }
                                    else
                                    {
                                        retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsNative, fileAttributes, dllInstance);
                                        return retfinal;
                                    }
                                }
                                else
                                {
                                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                                    PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                    retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                    return retfinal;
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRedirected, fileAttributes, dllInstance);
                                return retfinal;
                            }
                        }
                        else
                        {
                            Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributes: writablepackageroot area with ILV", g_MfrModuleName, dllInstance);
                            retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, cohorts.WsRequested, fileAttributes, dllInstance);
                            if (!retfinal && GetLastError() == ERROR_CANT_ACCESS_FILE)
                            {
                                // ILV has issues with this, fake it.
                                Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributeFixups: can't access requested file; return fake success.", g_MfrModuleName, dllInstance);
                                SetLastError(0);
                                return TRUE;
                            }
                            else
                            {
                                return retfinal;
                            }
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
                // We can use the mapping for ReadOperations since no files are added or removed.
                std::wstring UseName = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohorts, dllInstance);
                // In a redirect to local scenario, we are responsible for determining if source is local or in package
                UseName = SelectLocalOrPackageForRead(UseName, cohorts.WsPackage);

                retfinal = WRAPPER_SETFILEATTRIBUTES(LogLevel_DebugBasic, UseName, fileAttributes, dllInstance);
                if (retfinal)
                {
                    // ILV doesn't clear out the error on this call.
                    SetLastError(0);
                }
                return retfinal;
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"SetFileAttributes")

    if (fileName != nullptr)
    {
        std::wstring LongFileName = MakeLongPath(widen(fileName));
        Log(LogLevel_DebugIntermediate, L"[%s%d] SetFileAttributesFixup:unguarded call for %s", g_MfrModuleName, dllInstance, LongFileName.c_str());
        retfinal = impl::SetFileAttributes(LongFileName.c_str(), fileAttributes);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; //impl::SetFileAttributes(fileName, fileAttributes);
    }
    Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes: returns retfinal=%d", g_MfrModuleName, dllInstance, retfinal);
    if (retfinal == 0)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes: returns GetLastError=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);

