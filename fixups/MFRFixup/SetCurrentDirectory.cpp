//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory



#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

// IMPLEMENTATION NOTES:
// =====================
// Because of how InstalledLocationVirtualization works, it is desireable to
// alter an attempt to set the current directory to a local path if the path
// exists inside the package.  


BOOL  WRAPPER_SETCURRENTDIRECTORY(std::wstring thePath, DWORD dllInstance)
{
    std::wstring LongThePath = MakeLongPath(thePath);
    BOOL retfinal = impl::SetCurrentDirectoryW(LongThePath.c_str());
    if (retfinal == 0)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] SetCurrentDirectory returns result FAILURE 0x%x on file '%s'", g_MfrModuleName, dllInstance, GetLastError(), LongThePath.c_str());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] SetCurrentDirectory returns result SUCCESS 0x%x on file '%s'", g_MfrModuleName, dllInstance, retfinal, LongThePath.c_str());
    }
    return retfinal;
}


template <typename CharT>
BOOL __stdcall SetCurrentDirectoryFixup(_In_ const CharT* pathName) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;

    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;
    try
    {
        if (guard)
        {
            std::wstring wPathName = widen(pathName);
            wPathName = AdjustSlashes(wPathName, dllInstance);

            LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"SetCurrentDirectory for pathName", wPathName.c_str());

            ///if (MFRConfiguration.Ilv_Aware)
            {
                Cohorts cohorts;
                DetermineCohorts(LogLevel_DebugIntermediate, wPathName, &cohorts, dllInstance, L"SetCurrentDirectory");

                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled &&
                        cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        // treat as is; don't redirect
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = WRAPPER_SETCURRENTDIRECTORY(cohorts.WsPackage, dllInstance);
                        return retfinal;
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    // treat as is
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    // treat as is
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    // treat as is for now in the package (necessary if not ILV)
                    if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = WRAPPER_SETCURRENTDIRECTORY(cohorts.WsPackage, dllInstance);
                        return retfinal;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    // treat as is
                    break;
                case mfr::mfr_path_types::is_Protocol:
                case mfr::mfr_path_types::is_DosSpecial:
                case mfr::mfr_path_types::is_Shell:
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    // treat as is
                    break;
                }

            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"SetCurrentDirectoryFixup")

    if (pathName != nullptr)
    {
        std::wstring LongDeletingFile = MakeLongPath(widen(pathName));
        retfinal = impl::SetCurrentDirectory(LongDeletingFile.c_str());
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = 0; 
    }
    Log(LogLevel_DebugBasic, L"[%s%d] SetCurrentDirectoryFixup returns 0x%x", g_MfrModuleName, dllInstance, retfinal);
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetCurrentDirectory, SetCurrentDirectoryFixup);

