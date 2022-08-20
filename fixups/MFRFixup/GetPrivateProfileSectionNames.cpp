//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilesectionnames


// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#define WRAPPER_GETPRIVATEPROFILESECTIONNAME(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            auto wideString = std::make_unique<wchar_t[]>(stringLength); \
            retfinal = impl::GetPrivateProfileSectionNamesW(wideString.get(), stringLength, LongDestinationFilename.c_str()); \
            if (_doserrno != ENOENT) \
            { \
                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr); \
                if (debug) \
                { \
                    Log(L"[%d] GetPrivateProfileSectionsNames returns length 0x%x from %s", DllInstance, retfinal, LongDestinationFilename.c_str()); \
                } \
                return retfinal; \
            } \
        } \
        else \
        { \
            retfinal = impl::GetPrivateProfileSectionNamesW(string, stringLength, LongDestinationFilename.c_str()); \
            if (debug) \
            { \
                Log(L"[%d] GetPrivateProfileSectionsNames returns length 0x%x from %x", DllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
        } \
    }

template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionNamesFixup(
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
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
            if (fileName != NULL)
            {
#if _DEBUG
                LogString(DllInstance, L"GetPrivateProfileSectionNamesFixup for fileName", widen(fileName, CP_ACP).c_str());
#endif
                // This get is inheirently a read-only operation in all cases.
// We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);
                mfr::mfr_path file_mfr = mfr::create_mfr_path(wfileName);
                mfr::mfr_folder_mapping map;
                std::wstring testWsRequested = file_mfr.Request_NormalizedPath.c_str();
                std::wstring testWsNative;
                std::wstring testWsRedirected;
                std::wstring testWsPackage;
                switch (file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionNamesFixup    in_native_area", DllInstance);
#endif
                    map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent, then default 
                        testWsRedirected = testWsRequested.c_str();
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsPackage, debug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRequested, debug);
                        }
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then native, then default
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsPackage, debug);
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsNative, debug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRequested, debug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionNamesFixup    in_package_pvad_area", DllInstance);
#endif
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        //// try the redirected path, then package, then don't need native, so default
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsPackage = testWsRequested;
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsPackage, debug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRequested, debug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionNamesFixup    in_package_vfs_area", DllInstance);
#endif
                    map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package path, then default.
                        testWsPackage = testWsRequested;
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsPackage, debug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRequested, debug);
                        }
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then native, then default
                        testWsPackage = testWsRequested;
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsPackage, debug);
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsNative, debug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRequested, debug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionNamesFixup    in_redirection_area_writablepackageroot", DllInstance);
#endif
                    map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then possibly native, then default.
                        testWsRedirected = testWsRequested;
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsPackage, debug);
                        }
                        else if (testWsPackage.find(L"\\VFS\\") != std::wstring::npos &&
                                 PathExists(testWsNative.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsNative, debug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTIONNAME(testWsRequested, debug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionNamesFixup    in_redirection_area_other", DllInstance);
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
            else
            {
#if _DEBUG
                Log(L"[%d] GetPrivateProfileNamesFixup: null fileName, don't redirect as may be registry based or default.", DllInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"GetPrivateProfileSectionNamesFixup")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileSectionNamesFixup: Exception=0x%x", DllInstance, GetLastError());
    }
#endif


    retfinal = impl::GetPrivateProfileSectionNames(string, stringLength, fileName);
#if MOREDEBUG
    Log(L"[%d] GetPrivateProfileSectionNamesFixup Returned uint: %d from unfixed call.", DllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSectionNames, GetPrivateProfileSectionNamesFixup);
