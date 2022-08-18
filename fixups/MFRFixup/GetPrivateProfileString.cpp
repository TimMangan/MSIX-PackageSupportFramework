//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"

// Microsoft documentation: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilestring

// NOTE: In addition to file based configuration, apps map put this data into the registry.  The app may call this with a null filename, or a filename that does not exist.
//       In that case, we should call the native implementation which will return the registry or default result.


template <typename CharT>
DWORD __stdcall GetPrivateProfileStringFixup(
    _In_opt_ const CharT* appName,
    _In_opt_ const CharT* keyName,
    _In_opt_ const CharT* defaultString,
    _Out_writes_to_opt_(returnStringSizeInChars, return +1) CharT* string,
    _In_ DWORD stringLength,
    _In_opt_ const CharT* fileName) noexcept
{
    DWORD DllInstance = ++g_InterceptInstance;
    DWORD retfinal;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {

            if (fileName != NULL)
            {
#if _DEBUG
                if constexpr (psf::is_ansi<CharT>)
                {
                    if (fileName != NULL)
                    {
                        LogString(DllInstance, L"GetPrivateProfileStringFixup (A) for fileName", widen(fileName, CP_ACP).c_str());
                    }
                    else
                    {
                        Log(L"[%d] GetPrivateProfileStringFixup for null file.", DllInstance);
                    }
                    if (appName != NULL)
                    {

                        LogString(DllInstance, L" Section", widen_argument(appName).c_str());
                    }
                    if (keyName != NULL)
                    {
                        LogString(DllInstance, L" Key", widen_argument(keyName).c_str());
                    }
                }
                else
                {
                    if (fileName != NULL)
                    {
                        LogString(DllInstance, L"GetPrivateProfileStringFixup (W) for fileName", widen(fileName, CP_ACP).c_str());
                    }
                    else
                    {
                        Log(L"[%d] GetPrivateProfileStringFixup for null file.", DllInstance);
                    }
                    if (appName != NULL)
                    {

                        LogString(DllInstance, L" Section", appName);
                    }
                    if (keyName != NULL)
                    {
                        LogString(DllInstance, L" Key", keyName);
                    }
                }
#endif
                // This get is inheirently a read-only operation in all cases.
                // We prefer to use the redirecton case, if present.
                std::wstring wfileName = widen(fileName);
                mfr::mfr_path file_mfr = mfr::create_mfr_path(wfileName);
                mfr::mfr_folder_mapping map;
                std::wstring testWsNative;
                std::wstring testWsRedirected;
                std::wstring testWsPackage;
                switch (file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileStringFixup    in_native_area", DllInstance);
#endif
                    map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the request path, which must be the local redirected version by definition, and then a package equivalent, then default 
                        testWsRedirected = file_mfr.Request_NormalizedPath.c_str();
                        testWsPackage = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength,  narrow(testWsRedirected.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                        }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsRedirected.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsPackage.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsPackage.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                        }
                        }
                        else
                        {
                            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if _DEBUG
                            Log(L"[%d] GetPrivateProfileStringFixup Returned uint: %d from registry or default ", DllInstance, retfinal);
#endif
                            return retfinal;
                        }
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then native, then default
                        testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                        testWsPackage = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                        testWsNative = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsRedirected.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsRedirected.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsPackage.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsPackage.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsNative.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsNative.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if _DEBUG
                            Log(L"[%d] GetPrivateProfileStringFixup Returned uint: %d from registry or default ", DllInstance, retfinal);
#endif
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileStringFixup    in_package_pvad_area", DllInstance);
#endif
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        //// try the redirected path, then package, then don't need native, so default
                        testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsPackage = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsRedirected.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsRedirected.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsPackage.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsPackage.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if _DEBUG
                            Log(L"[%d] GetPrivateProfileStringFixup Returned uint: %d from registry or default ", DllInstance, retfinal);
#endif
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileStringFixup    in_package_vfs_area", DllInstance);
#endif
                    map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package path, then default.
                        testWsPackage = file_mfr.Request_NormalizedPath.c_str();
                        testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsRedirected.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsRedirected.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsPackage.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsPackage.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if _DEBUG
                            Log(L"[%d] GetPrivateProfileStringFixup Returned uint: %d from registry or default ", DllInstance, retfinal);
#endif
                            return retfinal;
                        }
                    }
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then native, then default
                        testWsPackage = file_mfr.Request_NormalizedPath.c_str();
                        testWsRedirected = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        testWsNative = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsRedirected.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsRedirected.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsPackage.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsPackage.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsNative.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsNative.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsNative.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if _DEBUG
                            Log(L"[%d] GetPrivateProfileStringFixup Returned uint: %d from registry or default ", DllInstance, retfinal);
#endif
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileStringFixup    in_redirection_area_writablepackageroot", DllInstance);
#endif
                    map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(file_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        // try the redirected path, then package, then possibly native, then default.
                        testWsRedirected = file_mfr.Request_NormalizedPath.c_str();
                        testWsPackage = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                        testWsNative = ReplacePathPart(file_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.NativePathBase);
                        if (PathExists(testWsRedirected.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsRedirected.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsRedirected.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (PathExists(testWsPackage.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsPackage.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                        }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsPackage.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else if (testWsPackage.find(L"\\VFS\\") != std::wstring::npos &&
                                 PathExists(testWsNative.c_str()))
                        {
                            if constexpr (psf::is_ansi<CharT>)
                            {

                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, narrow(testWsNative.c_str()).c_str());
#if _DEBUG
                                Log(L"[%d] GetPrivateProfileStringFixup: Ansi Returned length=0x%x", DllInstance, retfinal);
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Ansi Returned string", string);
#endif
                                return retfinal;
                            }
                            else
                            {
                                retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, testWsNative.c_str());
#if _DEBUG
                                if (retfinal > 0)
                                    LogString(DllInstance, L"GetPrivateProfileStringFixup: Returned string", string);
                                else
                                    Log(L"[%d] GetPrivateProfileStringFixup: Returned string zero length", DllInstance);
#endif
                                return retfinal;
                            }
                        }
                        else
                        {
                            retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if _DEBUG
                            Log(L"[%d] GetPrivateProfileStringFixup Returned uint: %d from registry or default ", DllInstance, retfinal);
#endif
                            return retfinal;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
#if MOREDEBUG
                    Log(L"[%d] GetPrivateProfileStringFixup    in_redirection_area_other", DllInstance);
#endif
                    break;
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_protocol_path:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    break;
                }
            }
            else
            {
                Log(L"[%d] GetPrivateProfileStringFixup: null fileName, don't redirect", DllInstance);
            }
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(DllInstance, L"GetPrivateProfileString")
#else
    catch (...)
    {
        Log(L"[%d] GetPrivateProfileString: Exception=0x%x", DllInstance, GetLastError());
    }
#endif 


    retfinal = impl::GetPrivateProfileString(appName, keyName, defaultString, string, stringLength, fileName);
#if MOREDEBUG
    LogString(DllInstance, L" Returning from unfixed call.", string);
#endif
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetPrivateProfileString, GetPrivateProfileStringFixup);
