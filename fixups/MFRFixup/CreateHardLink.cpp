//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createhardlinka

/// NOTES:
///     This function creates an extra directory entry to an existing file, such that the file may be accessed in either way.
///     So for this intercept, determine the redirected path location (if any) for the existing file and the link to be created.
///     If there is not a copy of the existing file in the redirection area, perform a copy of it.
///     Make sure the folder in the redirected area for the link parent is present.
///     Then create the link in it's redirected area.

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
BOOL __stdcall CreateHardLinkFixup(
    _In_ const CharT* fileName,
    _In_ const CharT* existingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif    
    
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(dllInstance, L"CopyHardLinkFixup for", fileName);
            LogString(dllInstance, L"CopyHardLinkFixup pointing to target", existingFileName);
#endif
            std::wstring wNewFileName = widen(fileName);
            std::wstring wExistingFileName = widen(existingFileName);
            wNewFileName = AdjustSlashes(wNewFileName);
            wExistingFileName = AdjustSlashes(wExistingFileName);

            Cohorts cohortsNew;
            DetermineCohorts(wNewFileName, &cohortsNew, moredebug, dllInstance, L"CreateHardLinkFixup");

            Cohorts cohortsExisting;
            DetermineCohorts(wExistingFileName, &cohortsExisting, moredebug, dllInstance, L"CreateHardLinkFixup");


            std::wstring UseExisting = cohortsExisting.WsRedirected;
            // Make a copy of existing into redirection area (if needed) so that all changes happen there
            if (!PathExists(cohortsExisting.WsRedirected.c_str()))
            {
                if (PathExists(cohortsExisting.WsPackage.c_str()))
                {
#if _DEBUG
                    Log(L"[%d] CreateHardLinkFixup:  Copy existing package file to redirection area.", dllInstance);
#endif
                    if (!Cow(cohortsExisting.WsPackage, cohortsExisting.WsRedirected, dllInstance, L"CreateHardLinkFixup"))
                    {
                        UseExisting = cohortsExisting.WsPackage;
#if _DEBUG
                        Log(L"[%d] CreateHardLinkFixup:  Cow failure?", dllInstance);
#endif
                    }
                }
                else if (cohortsExisting.UsingNative)
                {
#if _DEBUG
                    Log(L"[%d] CreateHardLinkFixup:  Copy existing native file to redirection area.", dllInstance);
#endif
                    if (!Cow(cohortsExisting.WsNative, cohortsExisting.WsRedirected, dllInstance, L"CreateHardLinkFixup"))
                    {
                        UseExisting = cohortsExisting.WsNative;
#if _DEBUG
                        Log(L"[%d] CreateHardLinkFixup:  Cow failure?", dllInstance);
#endif
                    }
                }
            }



            if (cohortsExisting.map.Valid_mapping && cohortsNew.map.Valid_mapping)
            {
                std::wstring rldNewFileNameRedirected;
                if (!cohortsNew.map.IsAnExclusionToRedirect)
                {
                    rldNewFileNameRedirected = MakeLongPath(cohortsNew.WsRedirected);
                }
                else
                {
                    rldNewFileNameRedirected = MakeLongPath(cohortsNew.WsRequested);
                }
                std::wstring rldExistingFileNameRedirected = MakeLongPath(UseExisting);
                PreCreateFolders(rldNewFileNameRedirected, dllInstance, L"CreateHardLinkFixup");
#if MOREDEBUG
                Log(L"[%d] CreateHardLinkFixup: link is to   %s", dllInstance, rldNewFileNameRedirected.c_str());
                Log(L"[%d] CreateHardLinkFixup: link is from %s", dllInstance, rldExistingFileNameRedirected.c_str());
#endif
                retfinal = impl::CreateHardLink(rldNewFileNameRedirected.c_str(), rldExistingFileNameRedirected.c_str(), securityAttributes);
#if _DEBUG
                if (retfinal == 0)
                {
                    Log(L"[%d] CreateHardLinkFixup returns Failure 0x%x", dllInstance, GetLastError());
                }
                else
                {
                    Log(L"[%d] CreateHardLinkFixup returns SUCCESS 0x%x", dllInstance, retfinal);
                }
#endif
                return retfinal;
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"CreateHardlinkFixup")
#else
    catch (...)
    {
        Log(L"[%d] CreateHardLinkFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    if (fileName != nullptr && existingFileName != nullptr)
    {
        // Improve app compat by allowing long paths always
        std::wstring rldFileName = MakeLongPath(widen(fileName));
        std::wstring rldExistingFileName = MakeLongPath(widen(existingFileName));
        retfinal = impl::CreateHardLink(rldFileName.c_str(), rldExistingFileName.c_str(), securityAttributes);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; //impl::CreateHardLink(fileName, existingFileName, securityAttributes);
    }
#if _DEBUG
    Log(L"[%d] CreateHardLinkFixup (default) returns %d", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateHardLink, CreateHardLinkFixup);
