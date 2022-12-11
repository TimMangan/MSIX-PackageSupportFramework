//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft Documentation on this API: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory

#if _DEBUG
//#define MOREDEBUG 1
#endif



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


BOOL  WRAPPER_SETCURRENTDIRECTORY(std::wstring thePath, DWORD dllInstance, bool debug)
{
    std::wstring LongThePath = MakeLongPath(thePath);
    BOOL retfinal = impl::SetCurrentDirectoryW(LongThePath.c_str());
    if (debug)
    {
        if (retfinal == 0)
        {
            Log(L"[%d] SetCurrentDirectory returns result FAILURE 0x%x on file '%s'", dllInstance, GetLastError(), LongThePath.c_str());
        }
        else
        {
            Log(L"[%d] SetCurrentDirectory returns result SUCCESS 0x%x on file '%s'", dllInstance, retfinal, LongThePath.c_str());
        }
    }
    return retfinal;
}


template <typename CharT>
BOOL __stdcall SetCurrentDirectoryFixup(_In_ const CharT* pathName) noexcept
{
    DWORD dllInstance = ++g_InterceptInstance;
    bool debug = false;
#if _DEBUG
    debug = true;
#endif
    bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;
    try
    {
        if (guard)
        {
            std::wstring wPathName = widen(pathName);
            wPathName = AdjustSlashes(wPathName);


#if _DEBUG
            LogString(dllInstance, L"SetCurrentDirectory for pathName", wPathName.c_str());
#endif
            if (MFRConfiguration.Ilv_Aware)
            {
                Cohorts cohorts;
                DetermineCohorts(wPathName, &cohorts, moredebug, dllInstance, L"DeleteFileFixup");

                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohorts.map.Valid_mapping &&
                        cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_local)
                    {
                        // treat as is; don't redirect
                    }
                    else if (PathExists(cohorts.WsPackage.c_str()))
                    {
                        retfinal = WRAPPER_SETCURRENTDIRECTORY(cohorts.WsPackage, dllInstance,debug);
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
                    // treat as is for now, no need to reverse since ILV handles backwards too
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
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"SetCurrentDirectoryFixup")
#else
    catch (...)
    {
        Log(L"[%d] SetCurrentDirectoryFixup Exception=0x%x", dllInstance, GetLastError());
    }
#endif
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
#if _DEBUG
    Log(L"[%d] SetCurrentDirectoryFixup returns 0x%x", dllInstance, retfinal);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetCurrentDirectory, SetCurrentDirectoryFixup);

