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


#define WRAPPER_GETFILEATTRIBUTES(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::GetFileAttributesW(LongDestinationFilename.c_str()); \
        if (retfinal != INVALID_FILE_ATTRIBUTES) \
        { \
            if (debug) \
            { \
                Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x", DllInstance, LongDestinationFilename.c_str(), retfinal); \
            } \
            return retfinal; \
        } \
    }

template <typename CharT>
DWORD __stdcall GetFileAttributesFixup(_In_ const CharT* fileName) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    DWORD retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            if constexpr (psf::is_ansi<CharT>)
            {
                LogString(DllInstance, L"GetFileAttributesFixupA for fileName", wfileName.c_str());
            }
            else
            {
                LogString(DllInstance, L"GetFileAttributesFixupW for fileName", wfileName.c_str());
            }
#endif

#if DEBUGPATHTESTING
            if (wfileName.compare(L"C:\\Program Files\\PlaceholderTest\\Placeholder.txt") == 0)
            {
                DebugPathTesting(DllInstance);
            }
#endif
            // This get is inheirently a read-only operation in all cases.
            // We prefer to use the redirecton case, if present.
            mfr::mfr_path file_mfr = mfr::create_mfr_path(wfileName);
            mfr::mfr_folder_mapping map;
            std::wstring testWsRequested = file_mfr.Request_NormalizedPath.c_str();
            std::wstring testWsNative;
            std::wstring testWsPackage;
            std::wstring testWsRedirected;
            switch (file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent = 
                    testWsRedirected = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTES(testWsRedirected, debug);

                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsPackage, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsRedirected, debug);

                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsPackage, debug);

                    testWsNative = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTES(testWsNative, debug);
                    
#if _DEBUG
                    Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsNative.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    //// try the redirected path, then package, then don't need native.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsRedirected, debug);

                    testWsPackage = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTES(testWsPackage, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the request path, which must be the local redirected version by definition, and then a package equivalent.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsRedirected, debug);

                    testWsPackage = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTES(testWsPackage, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsRedirected, debug);

                    testWsPackage = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTES(testWsPackage, debug);

                    testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsNative, debug);

#if _DEBUG
                    Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsNative.c_str(), retfinal, GetLastError());
#endif
                    return retfinal;
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                Log(L"[%d] GetFileAttributes    in_redirection_area_writablepackageroot", DllInstance);
#endif
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    // try the redirected path, then package, then native.
                    testWsRedirected = testWsRequested;
                    WRAPPER_GETFILEATTRIBUTES(testWsRedirected, debug);

                    testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    WRAPPER_GETFILEATTRIBUTES(testWsPackage, debug);

                    if (map.DoesRuntimeMapNativeToVFS)
                    {
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.NativePathBase);
                        WRAPPER_GETFILEATTRIBUTES(testWsNative, debug);

#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsNative.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes returns file '%s' and result 0x%x and error =0x%x", DllInstance, testWsPackage.c_str(), retfinal, GetLastError());
#endif
                        return retfinal;
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                Log(L"[%d] GetFileAttributes    in_redirection_area_other", DllInstance);
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
    LOGGED_CATCHHANDLER(DllInstance, L"GetFileAttributesTest")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributes Exception=0x%x", DllInstance, GetLastError());
    }
#endif
    std::wstring LongFileName = MakeLongPath(widen(fileName));
    retfinal = impl::GetFileAttributes(LongFileName.c_str());
#if _DEBUG
    Log(L"[%d] GetFileAttributes: returns retfinal=%d", DllInstance, retfinal);
    if (retfinal == INVALID_FILE_ATTRIBUTES)
    {
        Log(L"[%d] GetFileAttributes: No Redirect returns GetLastError=0x%x", DllInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributes, GetFileAttributesFixup);

