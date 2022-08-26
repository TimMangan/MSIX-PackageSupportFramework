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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define IMPROVE_RETURN_ACCURACY 1

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"


#include "FunctionImplementations.h"
#include <psf_logging.h>


BOOL  WRAPPER_CREATEDIRECTORY(std::wstring theDestinationDirectory, LPSECURITY_ATTRIBUTES securityAttributes, DWORD DllInstance, bool debug)
{
    std::wstring LongDestinationDirectory = MakeLongPath(theDestinationDirectory);
    BOOL retfinal = impl::CreateDirectoryW(LongDestinationDirectory.c_str(), securityAttributes);
    if (debug)
    {
        Log(L"[%d] CreateDirectory returns file '%s' and result 0x%x", DllInstance, LongDestinationDirectory.c_str(), retfinal);
    }
    return retfinal;
}



template <typename CharT>
BOOL __stdcall CreateDirectoryFixup(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;

    try
    {
        if (guard)
        {
            std::wstring wPathName = widen(pathName);
#if _DEBUG
            LogString(DllInstance, L"CreateDirectoryFixup for path", pathName);
#endif
            std::replace(wPathName.begin(), wPathName.end(), L'/', L'\\');

            // This get is inheirently a write operation in all cases.
            // There is no need to COW, just create the redirected folder, but may need to create parent folders first.
            mfr::mfr_path file_mfr = mfr::create_mfr_path(wPathName);
            mfr::mfr_folder_mapping map;
            std::wstring testWsRequested = file_mfr.Request_NormalizedPath.c_str();
            std::wstring testWsNative;
            std::wstring testWsPackage;
            std::wstring testWsRedirected;

            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryFixup    in_native_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CreateDirectoryFixup    match on LocalRedirMapping", DllInstance);
#endif
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent
                    testWsRedirected = testWsRequested;
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        // Still do this to set attributes
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(testWsPackage.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                            Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        // COW is not applicable, we don't need to copy the whole directory, just create it.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                        return retfinal;
                    }
                    else if (PathParentExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        return WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CreateDirectoryFixup    match on TraditionalRedirMapping", DllInstance);
#endif
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    testWsNative = testWsRequested.c_str();
#if MOREDEBUG
                    Log(L"[%d] CreateDirectoryFixup      RedirPath=%s", DllInstance, testWsRedirected.c_str());
                    Log(L"[%d] CreateDirectoryFixup    PackagePath=%s", DllInstance, testWsPackage.c_str());
                    Log(L"[%d] CreateDirectoryFixup     NativePath=%s", DllInstance, testWsNative.c_str());
#endif
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(testWsPackage.c_str()) ||
                            PathExists(testWsNative.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                            Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal =  WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        return WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryFixup    in_package_pvad_area", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(testWsPackage.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                            Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        return WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryFixup    in_package_vfs_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(testWsPackage.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                            Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        return WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW), then native (possibly COW)
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(testWsPackage.c_str()) ||
                            PathExists(testWsNative.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                            Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        return WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryFixup    in_redirection_area_writablepackageroot", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    testWsRedirected = testWsRequested;
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    testWsNative = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.NativePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        if (PathExists(testWsPackage.c_str()) ||
                            PathExists(testWsNative.c_str()))
                        {
                            retfinal = FALSE;
                            SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                            Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
                        }
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                        return retfinal;
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        retfinal = WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
#if IMPROVE_RETURN_ACCURACY
                        retfinal = FALSE;
                        SetLastError(ERROR_ALREADY_EXISTS);
#if _DEBUG
                        Log("[%d] CreateDirectoryFixup: Resetting return code to ERROR_ALREADY_EXISTS.");
#endif
#endif
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateDirectoryFixup");
                        return WRAPPER_CREATEDIRECTORY(testWsRedirected, securityAttributes, DllInstance, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] CreateDirectoryFixup    in_redirection_area_other", DllInstance);
#endif
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
    LOGGED_CATCHHANDLER(DllInstance, L"CreateDirectoryFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateDirectoryFixup Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    return impl::CreateDirectory(pathName, securityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateDirectory, CreateDirectoryFixup);
