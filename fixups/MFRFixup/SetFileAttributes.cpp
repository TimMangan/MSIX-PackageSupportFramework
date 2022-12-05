//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation for this API: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileattributesa

#if _DEBUG
#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

#if _DEBUG
//#define DEBUGPATHTESTING 1
#include "DebugPathTesting.h"
#endif



#define WRAPPER_SETFILEATTRIBUTES(theDestinationFilename, debug) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::SetFileAttributesW(LongDestinationFilename.c_str(),fileAttributes); \
        if (debug) \
        { \
            if (retfinal == 0) \
            { \
                Log(L"[%d] SetFileAttributes returns FAILURE 0x%x and file '%s'", dllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
            else \
            { \
                Log(L"[%d] SetFileAttributes returns SUCCESS 0x%x and file '%s'", dllInstance, retfinal, LongDestinationFilename.c_str()); \
            } \
        } \
        return retfinal; \
    }



template <typename CharT>
BOOL __stdcall SetFileAttributesFixup(_In_ const CharT* fileName, _In_ DWORD fileAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD dllInstance = g_InterceptInstance;
    bool debug = false;
    bool moreDebug = false;
#if _DEBUG
    debug = true;
#endif
#if MOREDEBUG
    moreDebug = true;
#endif
    BOOL retfinal;
    try
    {
        if (guard)
        {
            dllInstance = ++g_InterceptInstance;
            std::wstring wfileName = widen(fileName);
            wfileName = AdjustSlashes(wfileName);

#if _DEBUG
            LogString(dllInstance, L"SetFileAttributesFixup for fileName", wfileName.c_str());
#endif
            // This get is inheirently a write operation in all cases.
            // We may need to copy the file first.
            Cohorts cohorts;
            DetermineCohorts(wfileName, &cohorts, moreDebug, dllInstance, L"SetFileAttributesFixup");

            switch (cohorts.file_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                if (cohorts.map.Valid_mapping)
                {
                    switch (cohorts.map.RedirectionFlags)
                    {
                    case mfr::mfr_redirect_flags::prefer_redirection_local:
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent using COW
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);  // always returns
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug); // always returns
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsPackage, debug); // always returns
                            }
                        }
                        else if (PathParentExists(cohorts.WsPackage.c_str()))
                        {
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug) // always returns
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRequested.c_str(), dllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug); // always returns
                        }
                        break;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:

                        // try the redirected path, then package (via COW), then native (possibly via COW).
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsPackage, debug);
                            }
                        }
                        else if (PathExists(cohorts.WsNative.c_str()))
                        {
                            // TODO: This might not be the best way to decide is COW is appropriate.  
                            //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                            //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                            if (cohorts.map.DoesRuntimeMapNativeToVFS)
                            {
                                if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                {
                                    WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_SETFILEATTRIBUTES(cohorts.WsNative, debug);
                                }
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsNative, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRequested.c_str(), dllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRequested, debug);
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
                if (cohorts.map.Valid_mapping)
                {
                    switch (cohorts.map.RedirectionFlags)
                    {
                    case mfr::mfr_redirect_flags::prefer_redirection_local:
                        // not possible, fall through
                        break;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        //// try the redirected path, then package (COW), then don't need native.
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsPackage, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
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
                if (cohorts.map.Valid_mapping)
                {
                    switch (cohorts.map.RedirectionFlags)
                    {
                    case mfr::mfr_redirect_flags::prefer_redirection_local:
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsPackage, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                        }
                        break;
                    case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                    case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                        // try the redirection path, then the package (COW), then native (possibly COW)
                        if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                        {
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                        }
                        else if (PathExists(cohorts.WsPackage.c_str()))
                        {
                            if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsPackage, debug);
                            }
                        }
                        else if (PathExists(cohorts.WsNative.c_str()))
                        {
                            // TODO: This might not be the best way to decide is COW is appropriate.  
                            //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                            //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                            if (cohorts.map.DoesRuntimeMapNativeToVFS)
                            {
                                if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                {
                                    WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_SETFILEATTRIBUTES(cohorts.WsNative, debug);
                                }
                            }
                            else
                            {
                                // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                                PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
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
                if (cohorts.map.Valid_mapping)
                {
                    // try the redirected path, then package (COW), then possibly native (Possibly COW).
                    if (!cohorts.map.IsAnExclusionToRedirect && PathExists(cohorts.WsRedirected.c_str()))
                    {
                        WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        if (Cow(cohorts.WsPackage, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                        {
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                        }
                        else
                        {
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsPackage, debug);
                        }
                    }
                    else if (cohorts.UsingNative)
                    {
                        if (PathExists(cohorts.WsNative.c_str()))
                        {
                            // TODO: This might not be the best way to decide is COW is appropriate.  
                            //       Possibly we should always do it, or possibly only if the equivalent VFS folder exists in the package.
                            //       Setting attributes on an external file subject to traditional redirection seems an unlikely scenario that we need COW, but it might make an old app work.
                            if (cohorts.map.DoesRuntimeMapNativeToVFS)
                            {
                                if (Cow(cohorts.WsNative, cohorts.WsRedirected, dllInstance, L"SetFileAttributes"))
                                {
                                    WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                                }
                                else
                                {
                                    WRAPPER_SETFILEATTRIBUTES(cohorts.WsNative, debug);
                                }
                            }
                            else
                            {
                                WRAPPER_SETFILEATTRIBUTES(cohorts.WsNative, debug);
                            }
                        }
                        else
                        {
                            // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                            PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                            WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
                        }
                    }
                    else
                    {
                        // There isn't such a file anywhere.  We want to create the redirection parent folder and let this call against the redirected file to create there or update registry.
                        PreCreateFolders(cohorts.WsRedirected.c_str(), dllInstance, L"SetFileAttributes");
                        WRAPPER_SETFILEATTRIBUTES(cohorts.WsRedirected, debug);
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
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"SetFileAttributes")
#else
    catch (...)
    {
        Log(L"[%d] SetFileAttributes Exception=0x%x", dllInstance, GetLastError());
    }
#endif
    if (fileName != nullptr)
    {
        std::wstring LongFileName = MakeLongPath(widen(fileName));
        retfinal = impl::SetFileAttributes(LongFileName.c_str(), fileAttributes);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; //impl::SetFileAttributes(fileName, fileAttributes);
    }
#if _DEBUG
    Log(L"[%d] SetFileAttributes: returns retfinal=%d", dllInstance, retfinal);
    if (retfinal == 0)
    {
        Log(L"[%d] SetFileAttributes: returns GetLastError=0x%x", dllInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);

