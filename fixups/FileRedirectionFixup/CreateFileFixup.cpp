//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

/// ConvertToReadOnlyAccess: Modify a file operation call if it requests write access to one without write access.
DWORD inline ConvertToReadOnlyAccess(DWORD desiredAccess)
{
    DWORD redirectedAccess = desiredAccess;
    if ((desiredAccess & GENERIC_ALL) != 0)
    {
        redirectedAccess &= ~GENERIC_ALL;
    }
    if ((desiredAccess & GENERIC_WRITE) != 0)
    {
        redirectedAccess &= ~GENERIC_WRITE;
    }
    if ((desiredAccess & (GENERIC_ALL | GENERIC_WRITE)) != 0 &&
        (desiredAccess & ~(GENERIC_ALL | GENERIC_WRITE)) == 0)
    {
        redirectedAccess |= GENERIC_READ;
    }
    if ((desiredAccess & FILE_GENERIC_WRITE) != 0)
    {
        redirectedAccess &= ~FILE_GENERIC_WRITE;
        redirectedAccess |= FILE_GENERIC_READ;
    }
    if ((desiredAccess & FILE_WRITE_DATA) != 0)
    {
        redirectedAccess &= ~FILE_WRITE_DATA;
    }
    if ((desiredAccess & FILE_WRITE_ATTRIBUTES) != 0)
    {
        redirectedAccess &= FILE_WRITE_ATTRIBUTES;
    }
    if ((desiredAccess & FILE_WRITE_EA) != 0)
    {
        redirectedAccess &= FILE_WRITE_EA;
    }
    return redirectedAccess;
}

template <typename CharT>
HANDLE __stdcall CreateFileFixup(
    _In_ const CharT* fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes,
    _In_ DWORD creationDisposition,
    _In_ DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD CreateFileInstance = ++g_FileIntceptInstance;
#if _DEBUG
    if (fileName != NULL)
    {
        LogString(CreateFileInstance, L"CreateFileFixup for ", fileName);
    }
    else
    {
        Log(L"[%d] CreateFileFixup for NULL", CreateFileInstance);
    }
    Log(L"[%d]   DesiredAccess 0x%x  ShareMode 0x%x Disposition 0x%x flags 0x%x", CreateFileInstance, desiredAccess, shareMode, creationDisposition, flagsAndAttributes);
    if ((flagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) != 0)
    {
        Log(L"[%d]   Looks like a possible directory request (FILE_FLAG_BACKUP_SEMANTICS)", CreateFileInstance);
        // We probably should not redirect a request to open a directory, only create one
    }  
    
#endif
    
    std::string FileNameString;
    std::wstring WFileNameString;
    const CharT* FixedFileName = fileName;
    try
    {
        if (guard)
        {
            if constexpr (psf::is_ansi<CharT>)
            {
                FileNameString = fileName;
                std::replace(FileNameString.begin(), FileNameString.end(), '/', '\\');
                FileNameString = RemoveAnyFinalDoubleSlash(FileNameString);
                FixedFileName = FileNameString.c_str();
                //LogString(CreateFileInstance, L"CreateFileFixup A for fileName", widen(fileName).c_str()); 
            }
            else
            {
                WFileNameString = fileName;
                std::replace(WFileNameString.begin(), WFileNameString.end(), L'/', L'\\');
                WFileNameString = RemoveAnyFinalDoubleSlash(WFileNameString);
                FixedFileName = WFileNameString.c_str();
                //LogString(CreateFileInstance, L"CreateFileFixup W for fileName", fileName);
            }
            //if ((flagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) != 0)
            if (false)
            {
                // Create file uses this attribute to open a directory
                // Do not redirect this call, let it open whatever folder it wanted.  FindFirst will cover redirection later, if needed.
                HANDLE hRes;
                    hRes = impl::CreateFile(FixedFileName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
#if _DEBUG
                    if (hRes == INVALID_HANDLE_VALUE)
                        LogString(CreateFileInstance, L"CreateFileFixup fall through for directory call returns FAILURE.", FixedFileName);
                    else
                        LogString(CreateFileInstance, L"CreateFileFixup fall through for directory call returns SUCCESS.", FixedFileName);
                    Log(L"[%d]    Error is 0x%x", CreateFileInstance, GetLastError());
#endif
                
                return hRes;
            }
            else
            {
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (IsUnderUserPackageWritablePackageRoot(FileNameString.c_str()))
                    {
                        if (creationDisposition && CREATE_ALWAYS == 0 &&
                            !std::filesystem::exists(FixedFileName))
                        {
                            // The request was already in the redirected area and the file didn't exist and
                            // the request wasn't to CREATE_ALWAYS, so let's reverse the path back to the package and try from there.
                            WFileNameString = ReverseRedirectedToPackage(widen(FileNameString));
                            std::string tname = narrow(WFileNameString);
                            FixedFileName = tname.c_str();
#if _DEBUG
                            LogString(CreateFileInstance, L"Use ReverseRedirected fileName", FixedFileName);
#endif
                        }
                    }
                }
                else
                {
                    if (IsUnderUserPackageWritablePackageRoot(WFileNameString.c_str()))
                    {
                        if (creationDisposition && CREATE_ALWAYS == 0 &&
                            !std::filesystem::exists(FixedFileName))
                        {
                            // The request was already in the redirected area and the file didn't exist and
                            // the request wasn't to CREATE_ALWAYS, so let's reverse the path back to the package and try from there.
                            WFileNameString = ReverseRedirectedToPackage(WFileNameString.c_str());
                            FixedFileName = WFileNameString.c_str();
#if _DEBUG
                            LogString(CreateFileInstance, L"Use ReverseRedirected fileName", FixedFileName);
#endif
                        }
                    }
                }

                if (!IsUnderUserAppDataLocalPackages(FixedFileName))
                {
                    // FUTURE: If 'creationDisposition' is something like 'CREATE_ALWAYS', we could get away with something
                    //         cheaper than copy-on-read, but we'd also need to be mindful of ensuring the correct error if so
                    path_redirect_info  pri = ShouldRedirectV2(FixedFileName, redirect_flags::copy_on_read, CreateFileInstance);
                    if (pri.should_redirect)
                    {
                        if (IsUnderUserAppDataLocal(FixedFileName))
                        {
#if _DEBUG
                            Log(L"[%d]Under LocalAppData", CreateFileInstance);
#endif
                            // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                            std::filesystem::path PackageVersion = GetPackageVFSPath(FixedFileName);
                            if (wcslen(PackageVersion.c_str()) >= 0)
                            {
                                if (impl::PathExists(PackageVersion.c_str()))
                                {
                                    if (!impl::PathExists(pri.redirect_path.c_str()))
                                    {
                                        // Need to copy now
#if _DEBUG
                                        LogString(CreateFileInstance, L"\tFRF CreateFile COA from ADL to", pri.redirect_path.c_str());
#endif
                                        impl::CopyFileW(PackageVersion.c_str(), pri.redirect_path.c_str(), true);
                                    }
                                }
                            }
                        }
                        else if (IsUnderUserAppDataRoaming(FixedFileName))
                        {
#if _DEBUG
                            Log(L"[%d]\tUnder AppData(roaming)", CreateFileInstance);
#endif
                            // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                            std::filesystem::path PackageVersion = GetPackageVFSPath(FixedFileName);
                            if (wcslen(PackageVersion.c_str()) >= 0)
                            {
                                if (impl::PathExists(PackageVersion.c_str()))
                                {
                                    if (!impl::PathExists(pri.redirect_path.c_str()))
                                    {
                                        // Need to copy now
#if _DEBUG
                                        LogString(CreateFileInstance, L"\tFRF CreateFile COA from ADR to", pri.redirect_path.c_str());
#endif
                                        impl::CopyFileW(PackageVersion.c_str(), pri.redirect_path.c_str(), true);
                                    }
                                }
                            }
                        }

                        DWORD redirectedAccess = desiredAccess;
                        if (pri.shouldReadonly)
                        {
                            redirectedAccess = ConvertToReadOnlyAccess(desiredAccess);
#if _DEBUG
                            if (redirectedAccess != desiredAccess)
                            {
                                Log(L"[%d] CreateFile2: Modified desired access in redirection area to 0x%x", CreateFileInstance, redirectedAccess);
                            }
#endif
                        }
#if MOREDEBUG
                        HKEY keyS;
                        RegOpenKey(HKEY_CURRENT_USER, L"MarkerStart", &keyS);
#endif
                        HANDLE hRet = impl::CreateFile(pri.redirect_path.c_str(), redirectedAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
                        if (hRet == INVALID_HANDLE_VALUE)
                        {
                            DWORD ecode = GetLastError();
#if MOREDEBUG
                            HKEY keyE;
                            RegOpenKey(HKEY_CURRENT_USER, L"MarkerEnd", &keyE);
#endif
#if _DEBUG
                            Log(L"[%d]CreateFile redirected uses %ls. FAILURE=0x%x.", CreateFileInstance, pri.redirect_path.c_str(), ecode);
#endif
                            // Fall back to original request, but keep this error if needed (might be file not found instead of path not found/access denied).
                            HANDLE hRes3;
                            if constexpr (psf::is_ansi<CharT>)
                            {
                                std::string sFileName = TurnPathIntoRootLocalDevice(FixedFileName);
                                hRes3 = impl::CreateFile(sFileName.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
                            }
                            else
                            {
                                std::wstring wFileName = TurnPathIntoRootLocalDevice(FixedFileName);
                                hRes3 = impl::CreateFile(wFileName.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
                            }
                            if (hRes3 == INVALID_HANDLE_VALUE)
                            {
#if _DEBUG                      
                                DWORD ecode2 = GetLastError();
                                Log(L"[%d]CreateFile fall-through to original request also failed 0x%x, return redirected result of 0x%x and reset error to redirected case.", CreateFileInstance, ecode2, ecode);
#endif
                                SetLastError(ecode);
                            }
                            else
                            {
#if _DEBUG
                                Log(L"[%d]CreateFile2 fall-through to original request SUCCESS", CreateFileInstance);
#endif
                            }
                            return hRes3;
                        }
                        else
                        {
#if _DEBUG
                            Log(L"[%d]CreateFile redirected uses %ls. SUCCESS.", CreateFileInstance, pri.redirect_path.c_str());
#endif
                            return hRet;
                        }
                    }
                }
                else
                {
#if _DEBUG
                    Log(L"[%d]Under LocalAppData\\Packages, don't redirect", CreateFileInstance);
#endif          
                }
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(CreateFileInstance,L"CreateFile")
#else
    catch (...)
    {
        Log(L"[%d] CreateFile Exception=0x%x", CreateFileInstance, GetLastError());
    }
#endif


    // In the spirit of app compatability, make the path long formed just in case.
    if constexpr (psf::is_ansi<CharT>)
    {
        std::string sFileName = TurnPathIntoRootLocalDevice(FixedFileName);
        HANDLE hRes = impl::CreateFile(sFileName.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
#if _DEBUG
        if (hRes == INVALID_HANDLE_VALUE)
            LogString(CreateFileInstance, L"CreateFileFixup fall through call returns FAILURE.", sFileName.c_str());
        else
            LogString(CreateFileInstance, L"CreateFileFixup fall through call returns SUCCESS.", sFileName.c_str());
#endif
        return hRes;
    }
    else
    {
        std::wstring wFileName = TurnPathIntoRootLocalDevice(FixedFileName);
        HANDLE hRes = impl::CreateFile(wFileName.c_str(), desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
#if _DEBUG
        if (hRes == INVALID_HANDLE_VALUE)
            LogString(CreateFileInstance,L"CreateFileFixup fall through call returns FAILURE.", wFileName.c_str());
        else
            LogString(CreateFileInstance,L"CreateFileFixup fall through call returns SUCCESS.", wFileName.c_str());
#endif
        return hRes;
    }
}
DECLARE_STRING_FIXUP(impl::CreateFile, CreateFileFixup);

HANDLE __stdcall CreateFile2Fixup(
    _In_ LPCWSTR fileName,
    _In_ DWORD desiredAccess,
    _In_ DWORD shareMode,
    _In_ DWORD creationDisposition,
    _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS createExParams) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD CreateFile2Instance = ++g_FileIntceptInstance;
#if _DEBUG
    LogString(CreateFile2Instance, L"CreateFile2Fixup for ", fileName);
    Log(L"[%d] DesiredAccess 0x%x  ShareMode 0x%x", CreateFile2Instance, desiredAccess, shareMode);
#endif
    std::wstring WFileNameString = fileName;
    try
    {
        if (guard)
        {
            std::replace( WFileNameString.begin(), WFileNameString.end(), L'/', L'\\');
            WFileNameString = RemoveAnyFinalDoubleSlash(WFileNameString);

            ///Log(L"[%d]CreateFile2Fixup for %ls", CreateFile2Instance, widen(fileName, CP_ACP).c_str());
            //Log(L"[%d]CreateFile2Fixup for %ls", CreateFile2Instance, fileName);
            

            if (IsUnderUserPackageWritablePackageRoot(WFileNameString.c_str()))
            {
                WFileNameString = ReverseRedirectedToPackage(WFileNameString.c_str());
#if _DEBUG
                LogString(CreateFile2Instance, L"Use ReverseRedirected fileName", WFileNameString.c_str());
#endif
            }

            if (!IsUnderUserAppDataLocalPackages(WFileNameString.c_str()))
            {
                // FUTURE: See comment in CreateFileFixup about using 'creationDisposition' to choose a potentially better
                //         redirect flags value
                path_redirect_info  pri = ShouldRedirectV2(WFileNameString.c_str(), redirect_flags::copy_on_read, CreateFile2Instance);
                if (pri.should_redirect)
                {
                    if (IsUnderUserAppDataLocal(WFileNameString.c_str()))
                    {
#if _DEBUG
                        Log(L"[%d]Under LocalAppData", CreateFile2Instance);
#endif
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            if (impl::PathExists(PackageVersion.c_str()))
                            {
                                if (!impl::PathExists(pri.redirect_path.c_str()))
                                {
                                    // Need to copy now
#if _DEBUG
                                    LogString(CreateFile2Instance, L"\tFRF CreateFile2 COA from ADL to", pri.redirect_path.c_str());
#endif
                                    impl::CopyFileW(PackageVersion.c_str(), pri.redirect_path.c_str(), true);
                                }
                            }
                        }
                    }
                    else if (IsUnderUserAppDataRoaming(WFileNameString.c_str()))
                    {
#if _DEBUG
                        Log(L"[%d]\tUnder AppData(roaming)", CreateFile2Instance);
#endif
                        // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(fileName);
                        if (wcslen(PackageVersion.c_str()) >= 0)
                        {
                            if (impl::PathExists(PackageVersion.c_str()))
                            {
                                if (!impl::PathExists(pri.redirect_path.c_str()))
                                {
                                    // Need to copy now
#if _DEBUG
                                    LogString(CreateFile2Instance, L"\tFRF CreateFile2 COA from ADR to", pri.redirect_path.c_str());
#endif
                                    impl::CopyFileW(PackageVersion.c_str(), pri.redirect_path.c_str(), true);
                                }
                            }
                        }
                    }
                    DWORD redirectedAccess = desiredAccess;
                    if (pri.shouldReadonly)
                    {
                        redirectedAccess = ConvertToReadOnlyAccess(desiredAccess);
#if _DEBUG
                        if (redirectedAccess != desiredAccess)
                        {
                            Log(L"[%d] CreateFile2: Modified desired access in redirection area to 0x%x", CreateFile2Instance, redirectedAccess);
                        }
#endif
                    }

                    HANDLE hRet = impl::CreateFile2(pri.redirect_path.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
                    if (hRet == INVALID_HANDLE_VALUE)
                    {
#if _DEBUG
                        Log(L"[%d]CreateFile2 redirected uses %ls. FAILURE.", CreateFile2Instance, pri.redirect_path.c_str());
#endif
                        // Fall back to original request, but keep this error if needed (might be file not found instead of path not found/access denied).
                        DWORD ecode = GetLastError();
                        std::wstring wFileName = TurnPathIntoRootLocalDevice(WFileNameString.c_str());
                        HANDLE hRet3 = impl::CreateFile2(wFileName.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
                        if (hRet3 == INVALID_HANDLE_VALUE)
                        {
#if _DEBUG
                            Log(L"[%d]CreateFile2 Fall through to original request also failed return redirected result 0x%x", CreateFile2Instance, ecode);
#endif
                            SetLastError(ecode);
                        }
                        else
                        {
#if _DEBUG
                            Log(L"[%d]CreateFile2 Fall through to original request SUCCESS", CreateFile2Instance);
#endif
                        }
                        return hRet3;
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d]CreateFile2 redirected uses %ls. SUCCESS.", CreateFile2Instance, pri.redirect_path.c_str());
#endif
                        return hRet;
                    }
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d]Under LocalAppData\\Packages, don't redirect", CreateFile2Instance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(CreateFile2Instance, L"CreateFile2")
#else
    catch (...)
    {
        Log(L"[%d] CreateFile2 Exception=0x%x", CreateFile2Instance, GetLastError());
    }
#endif


    // In the spirit of app compatability, make the path long formed just in case.
    std::wstring wFileName = TurnPathIntoRootLocalDevice(WFileNameString.c_str());
    /////return impl::CreateFile2(fileName, desiredAccess, shareMode, creationDisposition, createExParams);
    HANDLE hRet2= impl::CreateFile2(wFileName.c_str(), desiredAccess, shareMode, creationDisposition, createExParams);
    if (hRet2 == INVALID_HANDLE_VALUE)
    {
#if _DEBUG
        LogString(CreateFile2Instance, L"CreateFile2 fallthrough FAILURE.",  wFileName.c_str());
#endif
        // Fall back to original request
    }
    else
    {
#if _DEBUG
        LogString(CreateFile2Instance, L"CreateFile2 fallthrough SUCCESS.",  wFileName.c_str());
#endif
    }
    return hRet2;
}
DECLARE_FIXUP(impl::CreateFile2, CreateFile2Fixup);
