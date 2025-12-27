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


#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"


#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:CopyHardLinkFixupAnsi_Fixup=impl::_CopyHardLinkFixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CopyHardLinkFixupWide_Fixup=impl::_CopyHardLinkFixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:CopyHardLinkFixupAnsi_Fixup=impl::CopyHardLinkFixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:CopyHardLinkFixupWide_Fixup=impl::CopyHardLinkFixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


template <typename CharT>
BOOL __stdcall CreateHardLinkFixup(
    _In_ const CharT* fileName,
    _In_ const CharT* existingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    
    BOOL retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyHardLinkFixup for", fileName);
            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CopyHardLinkFixup pointing to target", existingFileName);

            std::wstring wNewFileName = widen(fileName);
            std::wstring wExistingFileName = widen(existingFileName);
            wNewFileName = AdjustSlashes(wNewFileName, dllInstance);
            wExistingFileName = AdjustSlashes(wExistingFileName, dllInstance);

            wExistingFileName = AdjustBadUNC(wExistingFileName, dllInstance, L"CreateHardLinkFixup (existing)");
            wNewFileName = AdjustBadUNC(wNewFileName, dllInstance, L"CreateHardLinkFixup (new link)");

            Cohorts cohortsNew;
            DetermineCohorts(LogLevel_DebugIntermediate, wNewFileName, &cohortsNew, dllInstance, L"CreateHardLinkFixup");

            Cohorts cohortsExisting;
            DetermineCohorts(LogLevel_DebugIntermediate, wExistingFileName, &cohortsExisting, dllInstance, L"CreateHardLinkFixup");


            std::wstring UseExisting = cohortsExisting.WsRedirected;
            // Make a copy of existing into redirection area (if needed) so that all changes happen there
            if (!PathExists(cohortsExisting.WsRedirected.c_str()))
            {
                if (PathExists(cohortsExisting.WsPackage.c_str()))
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup:  Copy existing package file to redirection area.", g_MfrModuleName, dllInstance);
                    if (!Cow(LogLevel_DebugBasic, cohortsExisting.WsPackage, cohortsExisting.WsRedirected, dllInstance, L"CreateHardLinkFixup"))
                    {
                        UseExisting = cohortsExisting.WsPackage;
                        Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup:  Cow failure?", g_MfrModuleName, dllInstance);
                    }
                }
                else if (cohortsExisting.NativeIsValidOptionInScenario)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup:  Copy existing native file to redirection area.", g_MfrModuleName, dllInstance);
                    if (!Cow(LogLevel_DebugBasic, cohortsExisting.WsNative, cohortsExisting.WsRedirected, dllInstance, L"CreateHardLinkFixup"))
                    {
                        UseExisting = cohortsExisting.WsNative;
                        Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup:  Cow failure?", g_MfrModuleName, dllInstance);
                    }
                }
            }



            if (cohortsExisting.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
            {
                std::wstring rldNewFileNameRedirected;
                if (cohortsNew.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                {
                    rldNewFileNameRedirected = MakeLongPath(cohortsNew.WsRedirected);
                }
                else
                {
                    rldNewFileNameRedirected = MakeLongPath(cohortsNew.WsRequested);
                }
                std::wstring rldExistingFileNameRedirected = MakeLongPath(UseExisting);
                PreCreateFolders(rldNewFileNameRedirected, dllInstance, L"CreateHardLinkFixup");
                Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup: link is to   %s", g_MfrModuleName, dllInstance, rldNewFileNameRedirected.c_str());
                Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup: link is from %s", g_MfrModuleName, dllInstance, rldExistingFileNameRedirected.c_str());
                retfinal = impl::CreateHardLink(rldNewFileNameRedirected.c_str(), rldExistingFileNameRedirected.c_str(), securityAttributes);

                if (retfinal == 0)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup returns Failure 0x%x", g_MfrModuleName, dllInstance, GetLastError());
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup returns SUCCESS 0x%x", g_MfrModuleName, dllInstance, retfinal);
                }
                return retfinal;
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"CreateHardlinkFixup")

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
    Log(LogLevel_DebugBasic, L"[%s%d] CreateHardLinkFixup (default) returns %d", g_MfrModuleName, dllInstance, retfinal);
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::CreateHardLink, CreateHardLinkFixup);
