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

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilesection

#if _DEBUG
//#define MOREDEBUG 1
#endif

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.

#define WRAPPER_GETPRIVATEPROFILESECTION(theDestinationFilename, debug, moredebug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            auto wideString = std::make_unique<wchar_t[]>(stringLength); \
            retfinal = impl::GetPrivateProfileSectionW(widen_argument(appName).c_str(), wideString.get(), stringLength, LongDestinationFilename.c_str()); \
            if (_doserrno != ENOENT) \
            { \
                ::WideCharToMultiByte(CP_ACP, 0, wideString.get(), stringLength, string, stringLength, nullptr, nullptr); \
                if (debug) \
                { \
                    Log(L"[%d] GetPriviateProfileSectionFixup returns %x characters from %s.", DllInstance, retfinal, LongDestinationFilename.c_str()); \
                } \
                if (retfinal != 0 && moredebug) \
                { \
                    Log(L"[%d] GetPriviateProfileSectionFixup data %s.", DllInstance, string); \
                } \
                return retfinal; \
            } \
        } \
        else \
        { \
            retfinal = impl::GetPrivateProfileSectionW(appName, string, stringLength, LongDestinationFilename.c_str()); \
            if (debug) \
            { \
                Log(L"[%d] GetPriviateProfileSectionFixup returns %x characters from %s.", DllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
            if (retfinal != 0 && moredebug) \
            { \
                Log(L"[%d] GetPriviateProfileSectionFixup data %s.", DllInstance, string); \
            } \
            return retfinal; \
        } \
    }

template <typename CharT>
DWORD __stdcall GetPrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _Out_writes_to_opt_(stringSize, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    DWORD retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        bool debug = false;
#if _DEBUG
        debug = true;
#endif
        bool moredebug = false;
#if MOREDEBUG
        moredebug = true;
#endif
        if (guard)
        {
            if (fileName != NULL)
            {
#if _DEBUG
                LogString(DllInstance, L"GetPrivateProfileSectionFixup for fileName", widen(fileName, CP_ACP).c_str());
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
                    Log(L"[%d] GetPrivateProfileSectionFixup    in_native_area", DllInstance);
#endif
                    map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the request path (which must be the local redirected version by definition), and then a package equivalent, then return 0 characters as the fail.
                        testWsRedirected = testWsRequested;
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRedirected, debug, moredebug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsPackage, debug, moredebug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRequested, debug, moredebug);
                        }
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then native,  then return 0 characters as the fail.
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        testWsNative = testWsRequested;
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRedirected, debug, moredebug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsPackage, debug, moredebug);
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsNative, debug, moredebug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRequested, debug, moredebug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionFixup    in_package_pvad_area", DllInstance);
#endif
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        //// try the redirected path, then package, then don't need native and return 0 characters as the fail.
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRedirected, debug, moredebug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsPackage, debug, moredebug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRequested, debug, moredebug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionFixup    in_redirection_area_writablepackageroot", DllInstance);
#endif
                    map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package path, then return 0 characters as the fail.
                        testWsPackage = testWsRequested;
                        testWsRedirected = ReplacePathPart(testWsRequested, map.PackagePathBase, map.RedirectedPathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRedirected, debug, moredebug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsPackage, debug, moredebug);
                        }
                        else
                        {
                            
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRequested, debug, moredebug);
                        }
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then native, then return 0 characters as the fail.
                        testWsPackage = testWsRequested;
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRedirected, debug, moredebug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsPackage, debug, moredebug);
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsNative, debug, moredebug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRequested, debug, moredebug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionFixup    in_redirection_area_writablepackageroot", DllInstance);
#endif
                    map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then possibly native, then return 0 characters as the fail.
                        testWsRedirected = testWsRequested;
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRedirected, debug, moredebug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsPackage, debug, moredebug);
                        }
                        else if (testWsPackage.find(L"\\VFS\\") != std::wstring::npos &&
                                 PathExists(testWsNative.c_str()))
                        {
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsNative, debug, moredebug);
                        }
                        else
                        {
                            // No file, calling allows for default value or to get from registry.
                            WRAPPER_GETPRIVATEPROFILESECTION(testWsRequested, debug, moredebug);
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileSectionFixup    in_redirection_area_other", DllInstance);
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
            Log(L"[%d]  GetPrivateProfileSectionFixup: null filename, don't redirect as may be registry based or default.", DllInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"GetPrivateProfileSectionFixup")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileSectionFixup Exception=0x%x", DllInstance, GetLastError());
    }
#endif

    UINT uVal = impl::GetPrivateProfileSection(appName, string, stringLength, fileName);
#if MOREDEBUG
    Log(L"[%d] GetPrivateProfileSectionFixup Returning 0x%x from unfixed call.", DllInstance, uVal);
#endif
    return uVal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileSection, GetPrivateProfileSectionFixup);
