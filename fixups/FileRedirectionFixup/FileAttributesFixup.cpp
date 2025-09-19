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
            LogString(LogLevel_DebugBasic, g_FrfModuleName, GetFileAttributesInstance,L"GetFileAttributesFixup for fileName", wfileName.c_str());
            std::replace(wfileName.begin(), wfileName.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(wfileName.c_str()))
            {
                wfileName = ReverseRedirectedToPackage(wfileName.c_str());
                LogString(LogLevel_DebugBasic, g_FrfModuleName, GetFileAttributesInstance, L"GetFileAttributesFixup Use ReverseRedirected fileName", wfileName.c_str());
            }

            if (!IsUnderUserAppDataLocalPackages(wfileName.c_str()))
            {
                path_redirect_info  pri = ShouldRedirectV2(wfileName.c_str(), redirect_flags::check_file_presence | redirect_flags::ok_if_parent_in_pkg, GetFileAttributesInstance);
                if (pri.should_redirect)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: Should Redirect says yes.", g_FrfModuleName, GetFileAttributesInstance);
                    SetLastError(0);
                    DWORD attributes = INVALID_FILE_ATTRIBUTES;
                    if (pri.doesRedirectedExist)
                    {
                        attributes = impl::GetFileAttributes(pri.redirect_path.c_str());
                    }
                    else if (pri.doesVFSExist)
                    {
                        attributes = impl::GetFileAttributes(pri.vfs_path.c_str());
                    }
                    else
                    {
                        attributes = impl::GetFileAttributes(wfileName.c_str());
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
                                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: uncopied ADL/ADR case %ls", g_FrfModuleName, GetFileAttributesInstance, PackageVersion.c_str());
                                    attributes = impl::GetFileAttributes(PackageVersion.c_str());
                                    if (attributes == INVALID_FILE_ATTRIBUTES)
                                    {
                                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: fall back to original request location.", g_FrfModuleName, GetFileAttributesInstance);
                                        attributes = impl::GetFileAttributesW(wfileName.c_str());
                                    }
                                }
                            }
                            else
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: other not yet redirected case", g_FrfModuleName, GetFileAttributesInstance);
                                attributes = impl::GetFileAttributesW(wfileName.c_str());
                            }
                        }
                    }
                    if (attributes != INVALID_FILE_ATTRIBUTES)
                    {
                        if (pri.doesRedirectedExist && pri.shouldReadonly)
                        {
                            if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                                attributes |= FILE_ATTRIBUTE_READONLY;
                        }
                        else
                        {
                            attributes &= ~FILE_ATTRIBUTE_READONLY;
                        }
                    }
                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: returns att=0x%x", g_FrfModuleName, GetFileAttributesInstance, attributes);
                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: returns GetLastError=0x%x", g_FrfModuleName, GetFileAttributesInstance, GetLastError());
                    return attributes;
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: No Redirect, try original call ", g_FrfModuleName, GetFileAttributesInstance);
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
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: Retry in actual package anyway %ls", g_FrfModuleName, GetFileAttributesInstance, PackageVersion.c_str());
                            attributes = impl::GetFileAttributes(PackageVersion.c_str());
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: No Redirect returns att=0x%x", g_FrfModuleName, GetFileAttributesInstance, attributes);
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: No Redirect returns GetLastError=0x%x", g_FrfModuleName, GetFileAttributesInstance, GetLastError());
                        }
                        else
                        {
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: No Redirect returns Invalid and GetLastError=0x%x", g_FrfModuleName, GetFileAttributesInstance, rememberError);
                            SetLastError(rememberError);
                        }
                    }
                    else
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: No Redirect returns att=0x%x", g_FrfModuleName, GetFileAttributesInstance, attributes);
                        Log(LogLevel_DebugBasic, L"[%s%d]GetFileAttributes: No Redirect GetLastError=0x%x", g_FrfModuleName, GetFileAttributesInstance, GetLastError());
                    }
                    
                    return attributes;
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: Under LocalAppData\\Packages, don't redirect, make original call", g_FrfModuleName, GetFileAttributesInstance);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, GetFileAttributesInstance, L"DeleGetFileAttributesteFile")


    DWORD retfinal = impl::GetFileAttributes(fileName);
    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: returns retfinal=%d", g_FrfModuleName, GetFileAttributesInstance, retfinal);
    if (retfinal == INVALID_FILE_ATTRIBUTES)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributes: No Redirect returns GetLastError=0x%x", g_FrfModuleName, GetFileAttributesInstance, GetLastError());
    }
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
            LogString(LogLevel_DebugBasic, g_FrfModuleName, GetFileAttributesExInstance,L"GetFileAttributesExFixup for fileName", wfileName.c_str());
            std::replace(wfileName.begin(), wfileName.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(wfileName.c_str()))
            {
                wfileName = ReverseRedirectedToPackage(wfileName.c_str());
                LogString(LogLevel_DebugBasic, g_FrfModuleName, GetFileAttributesExInstance, L"GetFileAttributesEx: Use ReverseRedirected fileName", wfileName.c_str());
            }

            if (!IsUnderUserAppDataLocalPackages(fileName))
            {
                path_redirect_info  pri = ShouldRedirectV2(wfileName.c_str(), redirect_flags::check_file_presence | redirect_flags::ok_if_parent_in_pkg, GetFileAttributesExInstance);
                if (pri.should_redirect)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: Should Redirect says yes.", g_FrfModuleName, GetFileAttributesExInstance);

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
                                Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: uncopied ADL/ADR case %ls", g_FrfModuleName, GetFileAttributesExInstance,PackageVersion.c_str());
                                retval = impl::GetFileAttributesExW(PackageVersion.c_str(), infoLevelId, fileInformation);
                                if (retval == 0)
                                {
                                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: fall back to original location.", g_FrfModuleName, GetFileAttributesExInstance);
                                    retval = impl::GetFileAttributesExW(wfileName.c_str(), infoLevelId, fileInformation);
                                }
                            }
                        }
                        else
                        {
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: other uncopied other case", g_FrfModuleName, GetFileAttributesExInstance);
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
                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExInstance: returns att=0x%x", g_FrfModuleName, GetFileAttributesExInstance,
                            ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns retval=%d", g_FrfModuleName, GetFileAttributesExInstance, retval);
                        //Log(LogLevel_DebugBasic, L"[%s%d]GetFileAttributesEx: returns GetLastError=0x%x", g_FrfModuleName, GetFileAttributesExInstance, GetLastError());

                        SetLastError(0);
                    }
                    else
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns retval=%d att=%d", g_FrfModuleName, GetFileAttributesExInstance, retval, ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns GetLastError=0x%x", g_FrfModuleName, GetFileAttributesExInstance, GetLastError());
                    }
                    return retval;
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, GetFileAttributesExInstance);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, GetFileAttributesExInstance, L"GetFileAttributesEx")

    SetLastError(0);
    DWORD retfinal =  impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns retfinal=%d", g_FrfModuleName, GetFileAttributesExInstance, retfinal);
    if (retfinal == 0)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns GetLastError=0x%x", g_FrfModuleName, GetFileAttributesExInstance, GetLastError());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExInstance: returns att=0x%x", g_FrfModuleName, GetFileAttributesExInstance,
            ((WIN32_FILE_ATTRIBUTE_DATA*)fileInformation)->dwFileAttributes);
    }
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
            LogString(LogLevel_DebugBasic, g_FrfModuleName, SetFileAttributesInstance,L"SetFileAttributesFixup for fileName", wfileName.c_str());

            if (!IsUnderUserAppDataLocalPackages(fileName))
            {
                path_redirect_info  pri = ShouldRedirectV2(wfileName.c_str(), redirect_flags::copy_on_read | redirect_flags::ok_if_parent_in_pkg, SetFileAttributesInstance);
                if (pri.should_redirect)
                {
                    DWORD redirectedAttributes = fileAttributes;
                    if (pri.shouldReadonly)
                    {
                        if ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                            redirectedAttributes |= FILE_ATTRIBUTE_READONLY;
                    }
                    Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes Setting on redirected Equivalent with 0x%x", g_FrfModuleName, SetFileAttributesInstance, redirectedAttributes);
                    std::wstring rldRedirectPath = TurnPathIntoRootLocalDevice(widen_argument(pri.redirect_path.c_str()).c_str());
                    BOOL retval = impl::SetFileAttributesW(rldRedirectPath.c_str(), redirectedAttributes);
                    Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes: returns retval=%d", g_FrfModuleName, SetFileAttributesInstance, retval);
                    if (retval == 0)
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes: returns GetLastError=0x%x", g_FrfModuleName, SetFileAttributesInstance, GetLastError());
                    }
                    return retval;
                }
            }
            else
            {
                // We don't treat WritablePackageRoot different when setting attributes, only when getting them.
                Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, SetFileAttributesInstance);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, SetFileAttributesInstance, L"SetFileAttributes")


    std::wstring rldFileName = TurnPathIntoRootLocalDevice(widen_argument(fileName).c_str());
    BOOL retfinal = impl::SetFileAttributes(rldFileName.c_str(), fileAttributes);
    Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes: returns retfinal=%d", g_FrfModuleName, SetFileAttributesInstance, retfinal);
    if (retfinal == 0)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] SetFileAttributes: returns GetLastError=0x%x", g_FrfModuleName, SetFileAttributesInstance, GetLastError());
    }
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::SetFileAttributes, SetFileAttributesFixup);
