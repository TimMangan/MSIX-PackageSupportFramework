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


#include "FunctionImplementations.h"
#include <psf_logging.h>



HANDLE  WRAPPER_CREATEFILE2(std::wstring theDestinationFile,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_ DWORD creationDisposition,
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams,
    DWORD DllInstance, bool debug)
{
    std::wstring LongDestinationFile = MakeLongPath(theDestinationFile);
    HANDLE retfinal = impl::CreateFile2(LongDestinationFile.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
    if (debug)
    {
        Log(L"[%d] CreateFile2 returns file '%s' and result 0x%x", DllInstance, LongDestinationFile.c_str(), retfinal);
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
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    HANDLE retfinal;


    try
    {
        if (guard)
        {
            std::wstring wPathName = fileName;

#if _DEBUG
            LogString(DllInstance, L"CreateFile2Fixup for ", fileName);
            Log(L"[%d]        DesiredAccess 0x%x  ShareMode 0x%x", DllInstance, desiredAccess, shareMode);
#endif
            std::replace(wPathName.begin(), wPathName.end(), L'/', L'\\');

            // This get is may or may not be a write operation.
            // There may be a need to COW, jand may need to create parent folders in redirection area first.
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
                Log(L"[%d] CreateFile2Fixup    in_native_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CreateFile2Fixup    match on LocalRedirMapping", DllInstance);
#endif
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent
                    testWsRedirected = testWsRequested;
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            //PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (PathParentExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] CreateFile2Fixup    match on TraditionalRedirMapping", DllInstance);
#endif
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    testWsNative = testWsRequested.c_str();
#if MOREDEBUG
                    Log(L"[%d] CreateFile2Fixup      RedirPath=%s", DllInstance, testWsRedirected.c_str());
                    Log(L"[%d] CreateFile2Fixup    PackagePath=%s", DllInstance, testWsPackage.c_str());
                    Log(L"[%d] CreateFile2Fixup     NativePath=%s", DllInstance, testWsNative.c_str());
#endif
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            //PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (PathExists(testWsNative.c_str()))
                    {
                        if (Cow(testWsNative, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            ///PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                Log(L"[%d] CreateFile2Fixup    in_package_pvad_area", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            //PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFileFixup");
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                Log(L"[%d] CreateFile2Fixup    in_package_vfs_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                    return retfinal;
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
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (PathExists(testWsNative.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsNative, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] CreateFile2Fixup    in_redirection_area_writablepackageroot", DllInstance);
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
                        retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                        return retfinal;
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    if (PathExists(testWsNative.c_str()))
                    {
                        // COW is applicable first.
                        if (Cow(testWsNative, testWsRedirected, DllInstance, L"CreateFile2Fixup"))
                        {
                            retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                            return retfinal;
                        }
                    }
                    // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                    PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                    retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] CreateFile2Fixup    in_redirection_area_other", DllInstance);
#endif
                PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"CreateFile2Fixup");
                retfinal = WRAPPER_CREATEFILE2(testWsRedirected, desiredAccess, shareMode, creationDisposition, createExParams, DllInstance, debug);
                return retfinal;
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
    LOGGED_CATCHHANDLER(DllInstance, L"CreateFile2Fixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateFile2Fixup Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    std::wstring LongDirectory = MakeLongPath(fileName);
    return impl::CreateFile2(LongDirectory.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
}
DECLARE_FIXUP(impl::CreateFile2, CreateFile2Fixup);
