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

// Microsoft documentation:https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilesectiona

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.


#define WRAPPER_WRITEPRIVATEPROFILESECTION(theDestinationFilename, debug ) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        if constexpr (psf::is_ansi<CharT>) \
        { \
            retfinal = impl::WritePrivateProfileSectionW(widen_argument(appName).c_str(), widen_argument(string).c_str(), LongDestinationFilename.c_str()); \
        } \
        else \
        { \
            retfinal =  impl::WritePrivateProfileSection(appName, string, LongDestinationFilename.c_str()); \
        } \
        if (debug) \
        { \
            Log(L"[%d] WritePrivateProfileSection returns %d on file %s", DllInstance, retfinal, LongDestinationFilename.c_str()); \
        } \
        return retfinal; \
    }

template <typename CharT>
BOOL __stdcall WritePrivateProfileSectionFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* string,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {

            if (fileName != NULL)
            {
#if _DEBUG
                LogString(DllInstance, L"WritePrivateProfileSectionFixup for fileName", fileName);
#endif
                // This get is inheirently a write operation in all cases.
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
                        // try the request path (which must be the local redirected version by definition), and then a package equivalent with COW if needed.
                        testWsRedirected = testWsRequested;
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            // no special acction, just write to redirected area
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if (Cow(testWsPackage, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsPackage, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"WritePrivateProfileSectionFixup");
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }                        
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package (COW), then native (possibly via COW).
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.RedirectedPathBase);
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.NativePathBase, map.PackagePathBase);
                        testWsNative = testWsRequested;
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if (Cow(testWsPackage, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsPackage, debug);
                            }
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            // TODO: This might not be the best way to decide is COW is appropriate.  
                            //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                            //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                            if (map.DoesRuntimeMapNativeToVFS)
                            {
                                if (Cow(testWsNative, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_WRITEPRIVATEPROFILESECTION(testWsNative, debug);
                                }
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsNative, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"WritePrivateProfileSectionFixup");
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
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
                        //// try the redirected path, then package with COW, then don't need native and create in redirected.
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsPackage = testWsRequested;
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if (Cow(testWsPackage, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsPackage, debug);
                            }
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            if (Cow(testWsNative, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsNative, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"WritePrivateProfileSectionFixup");
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
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
                        // try the redirected path, then package path (COW), then create redirected
                        testWsPackage = testWsRequested;
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if (Cow(testWsPackage, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsPackage, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"WritePrivateProfileSectionFixup");
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package (COW), then native (COW), then just create new in redirected.
                        testWsPackage = testWsRequested;
                        testWsRedirected = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.PackagePathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if (Cow(testWsPackage, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsPackage, debug);
                            }
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            if (Cow(testWsNative, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsNative, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"WritePrivateProfileSectionFixup");
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
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
                        // try the redirected path, then package (COW), then possibly native (COW), then create new in redirected.
                        testWsRedirected = testWsRequested;
                        testWsPackage = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                        testWsNative = ReplacePathPart(testWsRequested.c_str(), map.RedirectedPathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if (Cow(testWsPackage, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                               WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsPackage, debug);
                            }
                        }
                        else if (testWsPackage.find(L"\\VFS\\") != std::wstring::npos &&
                                 PathExists(testWsNative.c_str()))
                        {
                            if (Cow(testWsNative, testWsRedirected, DllInstance, L"WritePrivateProfileSectionFixup"))
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_WRITEPRIVATEPROFILESECTION(testWsNative, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call create the redirected file or write to the registry.
                            PreCreateFolders(testWsRedirected.c_str(), DllInstance, L"WritePrivateProfileSectionFixup");
                            WRAPPER_WRITEPRIVATEPROFILESECTION(testWsRedirected, debug);
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
                Log(L"[%d] WritePrivateProfileSectionFixup: null fileName, don't redirect", DllInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"WritePrivateProfileSectionFixup")
#else
    catch (...)
    {
        Log(L"[%d] WritePrivateProfileSectionFixup: Exception=0x%x", DllInstance, GetLastError());
    }
#endif 


    return impl::WritePrivateProfileSection(appName, string, fileName);
}
DECLARE_STRING_FIXUP(impl::WritePrivateProfileSection, WritePrivateProfileSectionFixup);
