//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall CopyFileFixup(_In_ const CharT* existingFileName, _In_ const CharT* newFileName, _In_ BOOL failIfExists) noexcept
{
    DWORD CopyFileInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(CopyFileInstance,L"CopyFileFixup from", existingFileName);
            LogString(CopyFileInstance,L"CopyFileFixup to",   newFileName);
#endif
            
            // NOTE: We don't want to copy either file in the event one/both exist. Copying the source file would be
            //       wasteful since it's not the file that we care about (nor do we need write permissions to it); we
            //       just need to know its redirect path so that we can copy the most up to date one. It wouldn't
            //       necessarily be a complete waste of time to copy the destination file, particularly if
            //       'failIfExists' is true, but we'll go ahead and make the assumption that no application will try and
            //       copy to a file it wrote on install with 'failIfExists' as true. One possible option would be to
            //       manually fail out ourselves if 'failIfExists' is true and the file exists in the package, but
            //       that's arguably worse since we currently aren't handling the case where an application tries to
            //       delete a file in its package path.
            path_redirect_info  priSource = ShouldRedirectV2(existingFileName, redirect_flags::check_file_presence, CopyFileInstance);
            path_redirect_info  priDest = ShouldRedirectV2(newFileName, redirect_flags::ensure_directory_structure, CopyFileInstance);
            if (priSource.should_redirect )
            {
                std::wstring rldSourceRedirectPath = TurnPathIntoRootLocalDevice(widen(priSource.redirect_path).c_str());
                std::wstring rldRedirectDest = TurnPathIntoRootLocalDevice(priDest.should_redirect ? priDest.redirect_path.c_str() : widen_argument(newFileName).c_str());
                return impl::CopyFile(
                    rldSourceRedirectPath.c_str(),
                    rldRedirectDest.c_str(),
                    failIfExists);
            }
            else
            {
                ///auto path = widen(existingFileName, CP_ACP);
                auto path = widen(existingFileName);
                std::filesystem::path vfspath = GetPackageVFSPath(path.c_str());
                std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(vfspath.has_filename() ? vfspath.c_str() : path.c_str());
                std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(priDest.should_redirect ? priDest.redirect_path.c_str() : widen_argument(newFileName).c_str());
                return impl::CopyFile(
                    rldExistingFileName.c_str(),
                    rldNewDirectory.c_str(),
                    failIfExists);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(CopyFileInstance, L"CopyFile")
#else
    catch (...)
    {
        Log(L"[%d] CopyFile Exception=0x%x", CopyFileInstance, GetLastError());
    }
#endif

    // In the spirit of app compatability, make the path long formed just in case.
    if constexpr (psf::is_ansi<CharT>)
    {
        std::string rldExistingFileName = TurnPathIntoRootLocalDevice(existingFileName);
        std::string rldNewFileName = TurnPathIntoRootLocalDevice(newFileName);
        return impl::CopyFile(rldExistingFileName.c_str(), rldNewFileName.c_str(), failIfExists);
    }
    else
    {
        std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(existingFileName);
        std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(newFileName);
        return impl::CopyFile(rldExistingFileName.c_str(), rldNewDirectory.c_str(), failIfExists);
    }
    /////return impl::CopyFile(existingFileName, newFileName, failIfExists);
}
DECLARE_STRING_FIXUP(impl::CopyFile, CopyFileFixup);

template <typename CharT>
BOOL __stdcall CopyFileExFixup(
    _In_ const CharT* existingFileName,
    _In_ const CharT* newFileName,
    _In_opt_ LPPROGRESS_ROUTINE progressRoutine,
    _In_opt_ LPVOID data,
    _When_(cancel != NULL, _Pre_satisfies_(*cancel == FALSE)) _Inout_opt_ LPBOOL cancel,
    _In_ DWORD copyFlags) noexcept
{
    DWORD CopyFileExInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(CopyFileExInstance,L"CopyFileExFixup from", existingFileName);
            LogString(CopyFileExInstance,L"CopyFileExFixup to",   newFileName);
#endif

            // See note in CopyFileFixup for commentary on copy-on-read policy
            path_redirect_info  priSource = ShouldRedirectV2(existingFileName, redirect_flags::check_file_presence, CopyFileExInstance);
            path_redirect_info  priDest = ShouldRedirectV2(newFileName, redirect_flags::ensure_directory_structure, CopyFileExInstance);
            if (priSource.should_redirect)
            {
                std::wstring rldSourceRedirectPath = TurnPathIntoRootLocalDevice(widen(priSource.redirect_path).c_str());
                std::wstring rldRedirectDest = TurnPathIntoRootLocalDevice(priDest.should_redirect ? priDest.redirect_path.c_str() : widen_argument(newFileName).c_str());
                return impl::CopyFileEx(
                    rldSourceRedirectPath.c_str(),
                    rldRedirectDest.c_str(),
                    progressRoutine,
                    data,
                    cancel,
                    copyFlags);
            }
            else
            {
                ///auto path = widen(existingFileName, CP_ACP);
                auto path = widen(existingFileName);
                std::filesystem::path vfspath = GetPackageVFSPath(path.c_str());
                std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(vfspath.has_filename() ? vfspath.c_str() : path.c_str());
                std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(priDest.should_redirect ? priDest.redirect_path.c_str() : widen_argument(newFileName).c_str());
                return impl::CopyFileEx(rldExistingFileName.c_str(), rldNewDirectory.c_str(),
                    progressRoutine,
                    data,
                    cancel,
                    copyFlags);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(CopyFileExInstance, L"CopyFileEx")
#else
    catch (...)
    {
        Log(L"[%d] CopyFileEx Exception=0x%x", CopyFileExInstance, GetLastError());
    }
#endif

    if constexpr (psf::is_ansi<CharT>)
    {
        std::string rldExistingFileName = TurnPathIntoRootLocalDevice(existingFileName);
        std::string rldNewFileName = TurnPathIntoRootLocalDevice(newFileName);
        return impl::CopyFileEx(rldExistingFileName.c_str(), rldNewFileName.c_str(), progressRoutine, data, cancel, copyFlags);
    }
    else
    {
        std::wstring rldExistingFileName = TurnPathIntoRootLocalDevice(existingFileName);
        std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(newFileName);
        return impl::CopyFileEx(rldExistingFileName.c_str(), rldNewDirectory.c_str(), progressRoutine, data, cancel, copyFlags);
    }
    /////return impl::CopyFileEx(existingFileName, newFileName, progressRoutine, data, cancel, copyFlags);
}
DECLARE_STRING_FIXUP(impl::CopyFileEx, CopyFileExFixup);

HRESULT __stdcall CopyFile2Fixup(
    _In_ PCWSTR existingFileName,
    _In_ PCWSTR newFileName,
    _In_opt_ COPYFILE2_EXTENDED_PARAMETERS* extendedParameters) noexcept
{
    DWORD CopyFile2Instance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
#if _DEBUG
            LogString(CopyFile2Instance,L"CopyFile2Fixup from", existingFileName);
            LogString(CopyFile2Instance,L"CopyFile2Fixup to",   newFileName);
#endif

            // See note in CopyFileFixup for commentary on copy-on-read policy
            path_redirect_info  priSource = ShouldRedirectV2(existingFileName, redirect_flags::check_file_presence, CopyFile2Instance);
            path_redirect_info  priDest = ShouldRedirectV2(newFileName, redirect_flags::ensure_directory_structure, CopyFile2Instance);
            if (priSource.should_redirect)
            {
                return impl::CopyFile2(
                    priSource.redirect_path.c_str(),
                    priDest.should_redirect ? priDest.redirect_path.c_str() : newFileName,
                    extendedParameters);
            }

            else
            {
                ///auto path = widen(existingFileName, CP_ACP);
                auto path = widen(existingFileName);
                std::filesystem::path vfspath = GetPackageVFSPath(path.c_str());
                return impl::CopyFile2(
                    vfspath.has_filename() ? vfspath.c_str() : existingFileName,
                    priDest.should_redirect ? priDest.redirect_path.c_str() : newFileName,
                    extendedParameters);
            }
        }
    }
#if _DEBUG
        // Fall back to assuming no redirection is necessary if exception
        LOGGED_CATCHHANDLER(CopyFile2Instance, L"CopyFile2")
#else
catch (...)
{
    Log(L"[%d] CopyFile2 Exception=0x%x", CopyFile2Instance, GetLastError());
}
#endif

    return impl::CopyFile2(existingFileName, newFileName, extendedParameters);
}
DECLARE_FIXUP(impl::CopyFile2, CopyFile2Fixup);
