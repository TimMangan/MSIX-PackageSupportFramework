//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>

template <typename CharT>
BOOL __stdcall CreateDirectoryFixup(_In_ const CharT* pathName, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD CreateDirectoryInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    
    try
    {
        if (guard)
        {
            std::wstring wPathName = widen(pathName);
            LogString(LogLevel_DebugBasic, g_FrfModuleName, CreateDirectoryInstance,L"CreateDirectoryFixup for path", pathName);
            std::replace(wPathName.begin(), wPathName.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(wPathName.c_str()))
            {
                wPathName = ReverseRedirectedToPackage(wPathName.c_str());
                LogString(LogLevel_DebugBasic, g_FrfModuleName, CreateDirectoryInstance, L"Use ReverseRedirected fileName", wPathName.c_str());
            }

            if (!IsUnderUserAppDataLocalPackages(wPathName.c_str()))
            { 
                path_redirect_info  pri = ShouldRedirectV2(wPathName.c_str(), redirect_flags::ensure_directory_structure, CreateDirectoryInstance);
                //path_redirect_info  pri = ShouldRedirectV2(pathName, redirect_flags::ensure_directory_structure, CreateDirectoryInstance);
                //path_redirect_info  pri = ShouldRedirectV2(pathName, redirect_flags::none, CreateDirectoryInstance);
                if (pri.should_redirect)
                {
                    LogString(LogLevel_DebugBasic, g_FrfModuleName, CreateDirectoryInstance, L"CreateDirectoryFixup Use Folder", pri.redirect_path.c_str());
                    BOOL bRet = impl::CreateDirectory(pri.redirect_path.c_str(), securityAttributes);
                    if (bRet != 0)
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] CreateDirectory returns SUCCESS", g_FrfModuleName, CreateDirectoryInstance);

                    }
                    else
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] CreateDirectory returns FAIL with GetLastError=0x%x", g_FrfModuleName, CreateDirectoryInstance, GetLastError());
                    }
                    return bRet;
                }
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d]Under LocalAppData\\Packages, don't redirect", g_FrfModuleName, CreateDirectoryInstance);
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, CreateDirectoryInstance, L"CreateDirectory")


    // In the spirit of app compatability, make the path long formed just in case.
    if constexpr (psf::is_ansi<CharT>)
    {
        std::string sPathName = TurnPathIntoRootLocalDevice(pathName);
        return impl::CreateDirectory(sPathName.c_str(), securityAttributes);
    }
    else
    {
        std::wstring wPathName = TurnPathIntoRootLocalDevice(pathName);
        return impl::CreateDirectory(wPathName.c_str(), securityAttributes);
    }

}
DECLARE_STRING_FIXUP(impl::CreateDirectory, CreateDirectoryFixup);

template <typename CharT>
BOOL __stdcall CreateDirectoryExFixup(
    _In_ const CharT* templateDirectory,
    _In_ const CharT* newDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes) noexcept
{
    DWORD CreateDirectoryExInstance = ++g_FileIntceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            LogString(LogLevel_DebugBasic, g_FrfModuleName, CreateDirectoryExInstance,L"CreateDirectoryExFixup using template", templateDirectory);
            LogString(LogLevel_DebugBasic, g_FrfModuleName, CreateDirectoryExInstance,L"CreateDirectoryExFixup to",  newDirectory);
            std::wstring WtemplateDirectory = widen(templateDirectory);
            std::wstring WnewDirectory = widen(newDirectory);
            std::replace(WtemplateDirectory.begin(), WtemplateDirectory.end(), L'/', L'\\');
            std::replace(WnewDirectory.begin(), WnewDirectory.end(), L'/', L'\\');

            if (IsUnderUserPackageWritablePackageRoot(WtemplateDirectory.c_str()))
            {
                WtemplateDirectory = ReverseRedirectedToPackage(WtemplateDirectory.c_str());
                LogString(LogLevel_DebugBasic, g_FrfModuleName, CreateDirectoryExInstance, L"Use ReverseRedirected templateDirectory", WtemplateDirectory.c_str());
            }
            if (IsUnderUserPackageWritablePackageRoot(WnewDirectory.c_str()))
            {
                WnewDirectory = ReverseRedirectedToPackage(WtemplateDirectory.c_str());
                LogString(LogLevel_DebugBasic, g_FrfModuleName, CreateDirectoryExInstance, L"Use ReverseRedirected newDirectory", WnewDirectory.c_str());
            }

            
            path_redirect_info  priSource = ShouldRedirectV2(WtemplateDirectory.c_str(), redirect_flags::check_file_presence, CreateDirectoryExInstance);
            path_redirect_info  priDest = ShouldRedirectV2(WnewDirectory.c_str(), redirect_flags::ensure_directory_structure | redirect_flags::ok_if_parent_in_pkg, CreateDirectoryExInstance);
            if (priSource.should_redirect || priDest.should_redirect)
            {
                std::wstring rldRedirectTemplate = TurnPathIntoRootLocalDevice(priSource.should_redirect ? priSource.redirect_path.c_str() : WtemplateDirectory.c_str());
                std::wstring rldDest = TurnPathIntoRootLocalDevice(priDest.should_redirect ? priDest.redirect_path.c_str() : WnewDirectory.c_str());
                BOOL bRet = impl::CreateDirectoryEx(rldRedirectTemplate.c_str(), rldDest.c_str(), securityAttributes);
                if (bRet != 0)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateDirectoryEx returns SUCCESS", g_FrfModuleName, CreateDirectoryExInstance);

            }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] CreateDirectoryEx returns FAIL with GetLastError=0x%x", g_FrfModuleName, CreateDirectoryExInstance, GetLastError());
                }
                return bRet;
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_FrfModuleName, CreateDirectoryExInstance, L"CreateDirectoryEx")



    // In the spirit of app compatability, make the path long formed just in case.
    try
    {
        std::wstring WtemplateDirectory = widen(templateDirectory);
        std::wstring WnewDirectory = widen(newDirectory);
        std::wstring rldTemplateDirectory = TurnPathIntoRootLocalDevice(WtemplateDirectory.c_str());
        std::wstring rldNewDirectory = TurnPathIntoRootLocalDevice(WnewDirectory.c_str());
        return impl::CreateDirectoryEx(rldTemplateDirectory.c_str(), rldNewDirectory.c_str(), securityAttributes);
    }
    catch (...)
    {
        // Fall back to assuming no redirection is necessary
        LogString(LogLevel_Exception, g_FrfModuleName, g_FileIntceptInstance, L"CreateDirectoryExFixup ", L"*** Exception; use requested folder.***");
    }
    return impl::CreateDirectoryEx(templateDirectory, newDirectory, securityAttributes);
}
DECLARE_STRING_FIXUP(impl::CreateDirectoryEx, CreateDirectoryExFixup);
