//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsymboliclinka

/// NOTES:
///     This function creates a file that resolves as pointing to an target file or directory.
///     This means that the app might use this link in part of paths in subsequent calls that we would
///     interpret as called.
///     So for this intercept, determine the redirected path location (if any) for the target target and the link to be created.
///     If the target target is a file, treat it the same as for CreateHardLink, in other words
///     if there is not a copy of the target file in the redirection area, perform a copy of it.
///     If the target target is a folder, we could copy the whole folder, but for now we'll just make
///     the link point to an empty folder there (which would get layered in subsequent calls anyway.
///     Make sure the folder in the redirected area for the link parent is present.
///     Then create the link in it's redirected area.
///
///     ERROR_PRIVILEGE_NOT_HELD gets returned inside MSIX, possibly due to a need for SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
///     to be part of the flag.  This is an adaption to the container to try to protect things.  Basically
///     symbolic links have been deemed bad (but not hard links) and this won't work. The extra flag only
///     works if running in developer mode according to documentation, but not developer mode not required.  
///     So we will automatically add the flag to all calls in the spirit of app-compat.

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"


template <typename CharT>
BOOLEAN __stdcall CreateSymbolicLinkFixup(
    _In_ const CharT* symlinkFileName,
    _In_ const CharT* targetFileName,
    _In_ DWORD flags) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif    
    
    BOOLEAN retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(dllInstance, L"CreateSymbolicLinkFixup for", symlinkFileName);
            LogString(dllInstance, L"CreateSymbolicLinkFixup target", targetFileName);
            Log(L"[%d] CreateSymbolicLinkFixup flags=0x%x", dllInstance, flags);
#endif

            std::wstring wSymlinkFileName = widen(symlinkFileName);
            std::wstring wTargetFileName = widen(targetFileName);
            wSymlinkFileName = AdjustSlashes(wSymlinkFileName);
            wTargetFileName = AdjustSlashes(wTargetFileName);

            wSymlinkFileName = AdjustBadUNC(wSymlinkFileName, dllInstance, L"CreateSymbolicLinkFixup (new link)");
            wTargetFileName = AdjustBadUNC(wTargetFileName, dllInstance, L"CreateSymbolicLinkFixup (existing)");

            Cohorts cohortsSymlink;
            DetermineCohorts(wSymlinkFileName, &cohortsSymlink, moredebug, dllInstance, L"CreateSymbolicLinkFixup");

            Cohorts cohortsTarget;
            DetermineCohorts(wTargetFileName, &cohortsTarget, moredebug, dllInstance, L"CreateSymbolicLinkFixup");


            std::wstring UseTarget = cohortsTarget.WsRedirected;
            // If file case, make a copy if needed so that all changes happen there
            if (flags == 0)
            {
                if (!PathExists(cohortsTarget.WsRedirected.c_str()))
                {
                    if (PathExists(cohortsTarget.WsPackage.c_str()))
                    {
#if _DEBUG
                        Log(L"[%d] CreateSymbolicLinkFixup:  Copy target package file to redirection area.", dllInstance);
#endif
                        if (!Cow(cohortsTarget.WsPackage, cohortsTarget.WsRedirected, dllInstance, L"CreateSymbolicLinkFixup"))
                        {
                            UseTarget = cohortsTarget.WsPackage;
#if _DEBUG
                            Log(L"[%d] CreateSymbolicLinkFixup:  Cow failure?", dllInstance);
#endif
                        }
                    }
                    else if (cohortsTarget.UsingNative)
                    {
#if _DEBUG
                        Log(L"[%d] CreateSymbolicLinkFixup:  Copy target native file to redirection area.", dllInstance);
#endif
                        if (!Cow(cohortsTarget.WsNative, cohortsTarget.WsRedirected, dllInstance, L"CreateSymbolicLinkFixup"))
                        {
                            UseTarget = cohortsTarget.WsNative;
#if _DEBUG
                            Log(L"[%d] CreateSymbolicLinkFixup:  Cow failure?", dllInstance);
#endif
                        }
                    }
                }
            }
            else
            {
                // Until evidence says otherwise, skip any copy for the directory case.
                // Ignoring SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE as it isn't a call a production app would make.
            }

 
            if (cohortsTarget.map.Valid_mapping && cohortsSymlink.map.Valid_mapping)
            {
                std::wstring rldSymlinkFileNameRedirected;
                if (!cohortsSymlink.map.IsAnExclusionToRedirect)
                {
                    rldSymlinkFileNameRedirected = MakeLongPath(cohortsSymlink.WsRedirected);
                }
                else
                {
                    rldSymlinkFileNameRedirected = MakeLongPath(cohortsSymlink.WsRequested);
                }
                std::wstring rldTargetFileNameRedirected = MakeLongPath(UseTarget);
                PreCreateFolders(rldSymlinkFileNameRedirected, dllInstance, L"CreateSymbolicLinkFixup");
#if MOREDEBUG
                Log(L"[%d] CreateSymbolicLinkFixup: link is to   %s", dllInstance, rldSymlinkFileNameRedirected.c_str());
                Log(L"[%d] CreateSymbolicLinkFixup: link is from %s", dllInstance, rldTargetFileNameRedirected.c_str());
#endif
                //retfinal = impl::CreateSymbolicLink(rldSymlinkFileNameRedirected.c_str(), rldTargetFileNameRedirected.c_str(), flags);
                retfinal = impl::CreateSymbolicLink(rldSymlinkFileNameRedirected.c_str(), rldTargetFileNameRedirected.c_str(), flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
#if _DEBUG
                if (retfinal == 0)
                {
                    Log(L"[%d] CreateSymbolicLinkFixup returns FAILURE 0x%x", dllInstance, GetLastError());
                }
                else
                {
                    Log(L"[%d] CreateSymbolicLinkFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                }
#endif
                return retfinal;

            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"CreateSymbolicLinkFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateSymbolicLinkFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    if (symlinkFileName != nullptr && targetFileName != nullptr)
    {
        std::wstring rldFileName = MakeLongPath(widen(symlinkFileName));
        std::wstring rldTargetFileName = MakeLongPath(widen(targetFileName));
        retfinal = impl::CreateSymbolicLink(rldFileName.c_str(), rldTargetFileName.c_str(), flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; //impl::CreateSymbolicLink(symlinkFileName, targetFileName, flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
    }
#if _DEBUG
    Log(L"[%d] CreateSymbolicLinkFixup (default) returns %d", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateSymbolicLink, CreateSymbolicLinkFixup);
