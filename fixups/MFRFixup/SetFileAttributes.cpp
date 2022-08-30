//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"

#if _DEBUG
//#define DEBUGPATHTESTING 1
#include "DebugPathTesting.h"
#endif

#include "FunctionImplementations.h"
#include <psf_logging.h>


#define WRAPPER_SETFILEATTRIBUTES(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::SetFileAttributesW(LongDestinationFilename.c_str(),fileAttributes); \
        if (debug) \
        { \
            Log(L"[%d] SetFileAttributes returns file '%s' and result 0x%x", DllInstance, LongDestinationFilename.c_str(), retfinal); \
        } \
        return retfinal; \
    }



template <typename CharT>
BOOL __stdcall SetFileAttributesFixup(_In_ const CharT* fileName, _In_ DWORD fileAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    BOOL retfinal;
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            LogString(DllInstance, L"SetFileAttributesFixup for fileName", wfileName.c_str());
#endif
            // This get is inheirently a write operation in all cases.
            // We may need to copy the file first.
            mfr::mfr_path file_mfr = mfr::create_mfr_path(wfileName);
            mfr::mfr_folder_mapping map;
            std::wstring testWsRequested = file_mfr.Request_NormalizedPath.c_str();
            std::wstring testWsNative;
            std::wstring testWsPackage;
            std::wstring testWsRedirected;
            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_native_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] SetFileAttributes    match on LocalRedirMapping", DllInstance);
#endif
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent using COW
                    testWsRedirected = testWsRequested;
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes"))
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsPackage, debug);
                        }
                    }
                    else if (PathParentExists(testWsPackage.c_str()))
                    {
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRequested.c_str(), DllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
#if MOREDEBUG
                    Log(L"[%d] SetFileAttributes    match on TraditionalRedirMapping", DllInstance);
#endif
                    // try the redirected path, then package (via COW), then native (possibly via COW).
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    testWsNative = testWsRequested.c_str();
#if MOREDEBUG
                    Log(L"[%d] SetFileAttributes      RedirPath=%s", DllInstance, testWsRedirected.c_str());
                    Log(L"[%d] SetFileAttributes    PackagePath=%s", DllInstance, testWsPackage.c_str());
                    Log(L"[%d] SetFileAttributes     NativePath=%s", DllInstance, testWsNative.c_str());
#endif
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes"))
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsPackage, debug);
                        }
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        // TODO: This might not be the best way to decide is COW is appropriate.  
                        //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                        //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            if (Cow(testWsNative, testWsRedirected, DllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(testWsNative, debug);
                            }
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsNative, debug);
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRequested.c_str(), DllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(testWsRequested, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_package_pvad_area", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package (COW), then don't need native.
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes"))
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsPackage, debug);
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_package_vfs_area", DllInstance);
#endif
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirection path, then the package (COW).
                    testWsPackage = testWsRequested;
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    if (PathExists(testWsRedirected.c_str()))
                    {
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes"))
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsPackage, debug);
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
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
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes"))
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsPackage, debug);
                        }
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        // TODO: This might not be the best way to decide is COW is appropriate.  
                        //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                        //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            if (Cow(testWsNative, testWsRedirected, DllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(testWsNative, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_redirection_area_writablepackageroot", DllInstance);
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
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                    else if (PathExists(testWsPackage.c_str()))
                    {
                        if (Cow(testWsPackage, testWsRedirected, DllInstance, L"SetFileAttributes"))
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsPackage, debug);
                        }
                    }
                    else if (PathExists(testWsNative.c_str()))
                    {
                        // TODO: This might not be the best way to decide is COW is appropriate.  
                        //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                        //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                        if (map.DoesRuntimeMapNativeToVFS)
                        {
                            if (Cow(testWsNative, testWsRedirected, DllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(testWsNative, debug);
                            }
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(testWsNative, debug);
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                        PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(testWsRedirected, debug);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] SetFileAttributes    in_redirection_area_other", DllInstance);
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
    LOGGED_CATCHHANDLER(DllInstance, L"SetFileAttributes")
#else
    catch (...)
    {
        Log(L"[%d] SetFileAttributes Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    std::wstring LongFileName = MakeLongPath(widen(fileName));
    retfinal = impl::SetFileAttributes(LongFileName.c_str(), fileAttributes);
#if _DEBUG
    Log(L"[%d] SetFileAttributes: returns retfinal=%d", DllInstance, retfinal);
    if (retfinal == 0)
    {
        Log(L"[%d] SetFileAttributes: returns GetLastError=0x%x", DllInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);

