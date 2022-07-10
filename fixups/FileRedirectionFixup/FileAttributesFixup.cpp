//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
DWORD __stdcall GetFileAttributesFixup(_In_ const CharT* fileName) noexcept
{
    DWORD GetFileAttributesInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            LogString(GetFileAttributesInstance,L"GetFileAttributesFixup for fileName", wfileName.c_str());
#endif
            std::replace(wfileName.begin(), wfileName.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(wfileName.c_str()))
            {
                wfileName = ReverseRedirectedToPackage(wfileName.c_str());
#if _DEBUG
                LogString(GetFileAttributesInstance, L"GetFileAttributesFixup Use ReverseRedirected fileName", wfileName.c_str());
#endif
            }

            if (!IsUnderUserAppDataLocalPackages(wfileName.c_str()))
            {
                path_redirect_info  pri = ShouldRedirectV2(wfileName.c_str(), redirect_flags::check_file_presence, GetFileAttributesInstance);
                if (pri.should_redirect)
                {
#if _DEBUG
                    Log(L"[%d] GetFileAttributes: Should Redirect says yes.", GetFileAttributesInstance);
#endif
                    SetLastError(0);
                    DWORD attributes = impl::GetFileAttributes(pri.redirect_path.c_str());
                    if (attributes == INVALID_FILE_ATTRIBUTES)
                    {
                        // Might be file/dir has not been copied yet, but might also be funky ADL/ADR.
                        if (IsUnderUserAppDataLocal(wfileName.c_str()) ||
                            IsUnderUserAppDataRoaming(wfileName.c_str()))
                        {
                            // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                            std::filesystem::path PackageVersion = GetPackageVFSPath(wfileName.c_str());
                            if (wcslen(PackageVersion.c_str()) > 0)
                            {
#if _DEBUG
                                Log(L"[%d] GetFileAttributes: uncopied ADL/ADR case %ls", GetFileAttributesInstance,PackageVersion.c_str());
#endif
                                attributes = impl::GetFileAttributes(PackageVersion.c_str());
                                if (attributes == INVALID_FILE_ATTRIBUTES)
                                {
#if _DEBUG
                                    Log(L"[%d] GetFileAttributes: fall back to original request location.", GetFileAttributesInstance);
#endif
                                    attributes = impl::GetFileAttributesW(wfileName.c_str());
                                }
                            }
                        }
                        else
                        {
#if _DEBUG
                            Log(L"[%d] GetFileAttributes: other not yet redirected case", GetFileAttributesInstance);
#endif
                            attributes = impl::GetFileAttributesW(wfileName.c_str());
                        }
                    }
                    else if (attributes != INVALID_FILE_ATTRIBUTES)
                    {
                        if (pri.shouldReadonly)
                        {
                            if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                                attributes |= FILE_ATTRIBUTE_READONLY;
                        }
                        else
                        {
                            attributes &= ~FILE_ATTRIBUTE_READONLY;
                        }
                    }
#if _DEBUG
                    Log(L"[%d] GetFileAttributes: returns att=0x%x", GetFileAttributesInstance, attributes);
                    Log(L"[%d] GetFileAttributes: returns GetLastError=0x%x", GetFileAttributesInstance, GetLastError());
#endif
                    return attributes;
                }
                else
                {
#if _DEBUG
                    Log(L"[%d] GetFileAttributes: No Redirect, try original call ", GetFileAttributesInstance);
#endif
                    SetLastError(0);
                    DWORD attributes = impl::GetFileAttributes(fileName);
                    if (attributes == INVALID_FILE_ATTRIBUTES)
                    {
                        DWORD rememberError = GetLastError();
                        // If this was a native path and folder is in the package, we might need to try the package
                        // just to set the LastError correctly.
                        std::filesystem::path PackageVersion = GetPackageVFSPath(wfileName.c_str());
                        if (wcslen(PackageVersion.c_str()) > 0)
                        {
#if _DEBUG
                            Log(L"[%d] GetFileAttributes: Retry in actual package anyway %ls", GetFileAttributesInstance, PackageVersion.c_str());
#endif
                            attributes = impl::GetFileAttributes(PackageVersion.c_str());
#if _DEBUG
                            Log(L"[%d] GetFileAttributes: No Redirect returns att=0x%x", GetFileAttributesInstance, attributes);
                            Log(L"[%d] GetFileAttributes: No Redirect returns GetLastError=0x%x", GetFileAttributesInstance, GetLastError());
#endif
                        }
                        else
                        {
#if _DEBUG
                            Log(L"[%d] GetFileAttributes: No Redirect returns Invalid and GetLastError=0x%x", GetFileAttributesInstance, rememberError);
#endif
                            SetLastError(rememberError);
                        }
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributes: No Redirect returns att=0x%x", GetFileAttributesInstance, attributes);
                        Log(L"[%d]GetFileAttributes: No Redirect GetLastError=0x%x", GetFileAttributesInstance, GetLastError());
#endif
                    }
                    
                    return attributes;
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d] GetFileAttributes: Under LocalAppData\\Packages, don't redirect, make original call", GetFileAttributesInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(GetFileAttributesInstance, L"DeleGetFileAttributesteFile")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributes Exception=0x%x", GetFileAttributesInstance, GetLastError());
    }
#endif

    DWORD retfinal = impl::GetFileAttributes(fileName);
#if _DEBUG
    Log(L"[%d] GetFileAttributes: returns retfinal=%d", GetFileAttributesInstance, retfinal);
    if (retfinal == INVALID_FILE_ATTRIBUTES)
    {
        Log(L"[%d] GetFileAttributes: No Redirect returns GetLastError=0x%x", GetFileAttributesInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributes, GetFileAttributesFixup);


template <typename CharT>
BOOL __stdcall GetFileAttributesExFixup(
    _In_ const CharT* fileName,
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation) noexcept
{
    DWORD GetFileAttributesExInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            LogString(GetFileAttributesExInstance,L"GetFileAttributesExFixup for fileName", wfileName.c_str());
#endif
            std::replace(wfileName.begin(), wfileName.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(wfileName.c_str()))
            {
                wfileName = ReverseRedirectedToPackage(wfileName.c_str());
#if _DEBUG
                LogString(GetFileAttributesExInstance, L"GetFileAttributesEx: Use ReverseRedirected fileName", wfileName.c_str());
#endif
            }

            if (!IsUnderUserAppDataLocalPackages(fileName))
            {
                path_redirect_info  pri = ShouldRedirectV2(wfileName.c_str(), redirect_flags::check_file_presence, GetFileAttributesExInstance);
                if (pri.should_redirect)
                {
#if _DEBUG
                    Log(L"[%d] GetFileAttributesEx: Should Redirect says yes.", GetFileAttributesExInstance);
#endif
                    BOOL retval = impl::GetFileAttributesExW(pri.redirect_path.c_str(), infoLevelId, fileInformation);
                    if (retval == 0)
                    {
                        // We know it exists, so must be file/dir has not been copied yet.
                        if (IsUnderUserAppDataLocal(wfileName.c_str()) ||
                            IsUnderUserAppDataRoaming(wfileName.c_str()))
                        {
                            // special case.  Need to do the copy ourselves if present in the package as MSIX Runtime doesn't take care of these cases.
                            std::filesystem::path PackageVersion = GetPackageVFSPath(wfileName.c_str());
                            if (wcslen(PackageVersion.c_str()) > 0)
                            {
#if _DEBUG
                                Log(L"[%d] GetFileAttributesEx: uncopied ADL/ADR case %ls", GetFileAttributesExInstance,PackageVersion.c_str());
#endif
                                retval = impl::GetFileAttributesExW(PackageVersion.c_str(), infoLevelId, fileInformation);
                                if (retval == 0)
                                {
#if _DEBUG
                                    Log(L"[%d] GetFileAttributesEx: fall back to original location.", GetFileAttributesExInstance);
#endif
                                    retval = impl::GetFileAttributesExW(wfileName.c_str(), infoLevelId, fileInformation);
                                }
                            }
                        }
                        else
                        {
#if _DEBUG
                            Log(L"[%d] GetFileAttributesEx: other uncopied other case", GetFileAttributesExInstance);
#endif
                            retval = impl::GetFileAttributesExW(wfileName.c_str(), infoLevelId, fileInformation);
                        }
                    }
                    else if (retval != 0)
                    {
                        if (pri.shouldReadonly)
                        {
                            if (infoLevelId == GetFileExInfoStandard)
                            {
                                if ((((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                                    ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
                            }
                        }
                        else
                        {
                            if (infoLevelId == GetFileExInfoStandard)
                            {
                                ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                            }
                        }
                    }
                    if (retval != 0)
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesExInstance: returns att=0x%x", GetFileAttributesExInstance,
                            ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
                        Log(L"[%d] GetFileAttributesEx: returns retval=%d", GetFileAttributesExInstance, retval);
                        //Log(L"[%d]GetFileAttributesEx: returns GetLastError=0x%x", GetFileAttributesExInstance, GetLastError());
#endif
                        SetLastError(0);
                    }
                    else
                    {
#if _DEBUG
                        Log(L"[%d] GetFileAttributesEx: returns retval=%d att=%d", GetFileAttributesExInstance, retval, ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
                        Log(L"[%d] GetFileAttributesEx: returns GetLastError=0x%x", GetFileAttributesExInstance, GetLastError());
#endif
                    }
                    return retval;
                }
            }
            else
            {
#if _DEBUG
                Log(L"[%d] GetFileAttributesEx Under LocalAppData\\Packages, don't redirect", GetFileAttributesExInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(GetFileAttributesExInstance, L"GetFileAttributesEx")
#else
    catch (...)
    {
        Log(L"[%d] GetFileAttributesEx Exception=0x%x", GetFileAttributesExInstance, GetLastError());
    }
#endif

    SetLastError(0);
    DWORD retfinal =  impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
#if _DEBUG
    Log(L"[%d] GetFileAttributesEx: returns retfinal=%d", GetFileAttributesExInstance, retfinal);
    if (retfinal == 0)
    {
        Log(L"[%d] GetFileAttributesEx: returns GetLastError=0x%x", GetFileAttributesExInstance, GetLastError());
    }
    else
    {
        Log(L"[%d] GetFileAttributesExInstance: returns att=0x%x", GetFileAttributesExInstance,
            ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributesEx, GetFileAttributesExFixup);

template <typename CharT>
BOOL __stdcall SetFileAttributesFixup(_In_ const CharT* fileName, _In_ DWORD fileAttributes) noexcept
{
    auto guard = g_reentrancyGuard.enter();
    DWORD SetFileAttributesInstance = ++g_FileIntceptInstance;
    try
    {
        if (guard)
        {
            std::wstring wfileName = widen(fileName);
#if _DEBUG
            LogString(SetFileAttributesInstance,L"SetFileAttributesFixup for fileName", wfileName.c_str());
#endif

            if (!IsUnderUserAppDataLocalPackages(fileName))
            {
                path_redirect_info  pri = ShouldRedirectV2(wfileName.c_str(), redirect_flags::copy_on_read, SetFileAttributesInstance);
                if (pri.should_redirect)
                {
                    DWORD redirectedAttributes = fileAttributes;
                    if (pri.shouldReadonly)
                    {
                        if ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                            redirectedAttributes |= FILE_ATTRIBUTE_READONLY;
                    }
#if _DEBUG
                    Log(L"[%d] SetFileAttributes Setting on redirected Equivalent with 0x%x", SetFileAttributesInstance, redirectedAttributes);
#endif
                    std::wstring rldRedirectPath = TurnPathIntoRootLocalDevice(widen_argument(pri.redirect_path.c_str()).c_str());
                    BOOL retval = impl::SetFileAttributesW(rldRedirectPath.c_str(), redirectedAttributes);
#if _DEBUG
                    Log(L"[%d] SetFileAttributes: returns retval=%d", SetFileAttributesInstance, retval);
                    if (retval == 0)
                    {
                        Log(L"[%d] SetFileAttributes: returns GetLastError=0x%x", SetFileAttributesInstance, GetLastError());
                    }
#endif
                    return retval;
                }
            }
            else
            {
                // We don't treat WritablePackageRoot different when setting attributes, only when getting them.
#if _DEBUG
                Log(L"[%d] SetFileAttributes Under LocalAppData\\Packages, don't redirect", SetFileAttributesInstance);
#endif
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(SetFileAttributesInstance, L"SetFileAttributes")
#else
    catch (...)
    {
        Log(L"[%d] SetFileAttributes Exception=0x%x", SetFileAttributesInstance, GetLastError());
    }
#endif

    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
    BOOL retfinal = impl::SetFileAttributes(rldFileName.c_str(), fileAttributes);
#if _DEBUG
    Log(L"[%d] SetFileAttributes: returns retfinal=%d", SetFileAttributesInstance, retfinal);
    if (retfinal == 0)
    {
        Log(L"[%d] SetFileAttributes: returns GetLastError=0x%x", SetFileAttributesInstance, GetLastError());
    }
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);
