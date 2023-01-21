//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createdirectory

#if _DEBUG
//#define MOREDEBUG 1
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DESIGN NOTE:  It is debatiable what return to give when the app asks to create a directory and it is subject
//               to redirection.  It may or may not exist previously in some locations and differently in the 
//               redirection area, so the "correct" return code is debatable.
// 
//               We can provide a generally compatible answer, or success as long as it didn't exist in the redirection
//               area.  Because we always precreate the parent folders, PATH_NOT_FOUND becomes impossible, so either
//               success is returned or ERROR_ALREADY_EXISTS is returned should it already exist in the redirection area.
//               Generally, we think the apps trying to create a directory will ignore the error unless PATH_NOT_FOUND is
//               returned, because they only care that it really was created, making this a decent strategy.  But also 
//               consider that if the directory exists in the package path and the app tried to delete it (but can't 
//               because it is immutable), hiding the ERROR_ALREADY_EXISTS can be benificial as we would have deleted 
//               the redirected copy and are now creating it.  However if the purpose was to remove the things in the
//               package under that folder we can't remove the package flotsam, so there is no right answer. 
// 
//               The alternative is that we can provide a ERROR_ALREADY_EXISTS if it exists under any of the paths, 
//               whether or not we create the redirected folder.  This might make certain apps work better, but would
//               lead to more work by the app and not necessarily a good outcome.
//
//               The implementation, without the IMPROVE_RETURN_ACCURACY define, follows the first course.
//
//               Turn on the define to get the second behavior.  Consider CreateDirectoryEx which would need a similar
//               strategy if you do.
//   
//               NOTE: In an ILV scenario with traditional redirection, we want to add the directory to the package and not
//                     the redirection area.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define IMPROVE_RETURN_ACCURACY 1

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineIlvPaths.h"

#if _DEBUG
//#define MOREDEBUG 1
#endif

BOOL  WRAPPER_CREATEDIRECTORY(std::wstring theDestinationDirectory, LPSECURITY_ATTRIBUTES securityAttributes, DWORD dllInstance, bool debug)
{
    std::wstring LongDestinationDirectory = MakeLongPath(theDestinationDirectory);
    BOOL retfinal = impl::CreateDirectoryW(LongDestinationDirectory.c_str(), securityAttributes);
    if (debug)
    {
        if (retfinal == 0)
        {
            Log(L"[%d] CreateDirectory returns FAILURE 0x%x GetLastError=0x%x and file '%s'", dllInstance, retfinal, GetLastError(), LongDestinationDirectory.c_str());
        }
        else
        {
            Log(L"[%d] CreateDirectory returns SUCCESS 0x%x and file '%s'", dllInstance, retfinal, LongDestinationDirectory.c_str());
        }
    }
    return retfinal; 
}



template <typename CharT>
BOOL __stdcall CreateDirectoryFixup(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
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

    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;

    try
    {
        if (guard)
        {
            dllInstance = ++g_InterceptInstance;
            std::wstring wPathName = widen(pathName);
            wPathName = AdjustSlashes(wPathName);

#if _DEBUG
            LogString(dllInstance, L"CreateDirectoryFixup for path", pathName);
#endif
            
            // This get is inheirently a write operation in all cases.
            // There is no need to COW, just create the redirected folder, but may need to create parent folders first.
            Cohorts cohorts;
            DetermineCohorts(wPathName, &cohorts, moredebug, dllInstance, L"CreateDirectoryFixup");

            if (!MFRConfiguration.Ilv_Aware)
            {
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
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                    Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            // COW is not applicable, we don't need to copy the whole directory, just create it.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            return WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
                        }
                    }
                    else if (cohorts.map.Valid_mapping &&
                        (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        // try the redirected path, then package (via COW), then native (possibly via COW).
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohorts.WsPackage.c_str()) ||
                                    (cohorts.UsingNative && PathExists(cohorts.WsNative.c_str())))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                    Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else if (cohorts.UsingNative &&
                            PathExists(cohorts.WsNative.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            return WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    if (cohorts.map.Valid_mapping)
                    {
                        //// try the redirected path, then package (COW), then don't need native.
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                    Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            return WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
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
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohorts.WsPackage.c_str()))
                                {
                                    SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                    Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            return WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
                        }
                    }
                    else if (cohorts.map.Valid_mapping &&
                        (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_containerized ||
                            cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs))
                    {
                        // try the redirection path, then the package (COW), then native (possibly COW)
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohorts.WsPackage.c_str()) ||
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                    Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else if (cohorts.UsingNative &&
                            PathExists(cohorts.WsNative.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            return WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohorts.map.Valid_mapping)
                    {
                        // try the redirected path, then package (COW), then possibly native (Possibly COW).
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                if (PathExists(cohorts.WsPackage.c_str()) ||
                                    PathExists(cohorts.WsNative.c_str()))
                                {
                                    retfinal = FALSE;
                                    SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                    Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                                }
                            }
#endif
                            return retfinal;
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else if (cohorts.UsingNative &&
                            PathExists(cohorts.WsNative.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            retfinal = WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                            if (!retfinal)
                            {
                                SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                                Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                            }
#endif
                            return retfinal;
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"CreateDirectoryFixup");
                            return WRAPPER_CREATEDIRECTORY(cohorts.WsRedirected, securityAttributes, dllInstance, debug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    PreCreateFolders(cohorts.WsRequested.c_str(), dllInstance, L"CreateDirectoryFixup");
                    return WRAPPER_CREATEDIRECTORY(cohorts.WsRequested, securityAttributes, dllInstance, debug);
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
                //ILV aware
                std::wstring usePath = DetermineIlvPathForWriteOperations(cohorts, dllInstance, moredebug);
                // In a redirect to local scenario, we are responsible for pre-creating the local parent folders
                // if-and-only-if they are present in the package.
                PreCreateLocalFoldersIfNeededForWrite(usePath, cohorts.WsPackage, dllInstance, debug, L"CreateDirectoryFixup");
                
                retfinal = WRAPPER_CREATEDIRECTORY(usePath, securityAttributes, dllInstance, debug);
                return retfinal;
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"CreateDirectoryFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateDirectoryFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (pathName != nullptr)
    {
        std::wstring LongDirectory = MakeLongPath(widen(pathName));
        retfinal = impl::CreateDirectory(LongDirectory.c_str(), securityAttributes);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; // impl::CreateDirectory(pathName, securityAttributes);
    }
#if _DEBUG
    LogString(dllInstance, L"CreateDirectoryFixup (unguarded) for path", pathName);
#endif
#if _DEBUG
    if (retfinal == 0)
    {
        DWORD eCode = GetLastError();
        if (eCode  == ERROR_ALREADY_EXISTS)
        {
            Log(L"[%d] CreateDirectoryFixup (unguarded) returns 0x%x (ERROR_ALREADY_EXISTS)", dllInstance, retfinal);
        }
        else
        {
            Log(L"[%d] CreateDirectoryFixup (unguarded) returns 0x%x with error=0x%x", dllInstance, retfinal, eCode);
        }
    }
    else
    {
        Log(L"[%d] CreateDirectoryFixup (unguarded) returns 0x%x (ERROR_SUCCESS)", dllInstance, retfinal);
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateDirectory, CreateDirectoryFixup);
