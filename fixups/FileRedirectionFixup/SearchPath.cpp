//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//

// Design: Like other file based calls, we will need to look in various folders for the requested file other than the requested one.
//         However, in this case the returned buffer might not be big enough to actually return a different path (and calling code
//         might not retry with a bigger buffer if we ask).  As we aren't actually opening the file, even if the file is found under 
//         a different path we'll return using the requested path; other file utililities will redo the redirections and find it when they are called.
//       

// NOTE: If we ever address the "delete package file" problem, we'll need to address that here, too

#include <array>

#include <dos_paths.h>
#include <fancy_handle.h>
#include <psf_framework.h>

#include "FunctionImplementations.h"
#include "PathRedirection.h"

#include <psf_logging.h>
extern std::filesystem::path g_writablePackageRootPath;


template <typename CharT>
DWORD __stdcall SearchPathFixup(
    _In_opt_   const CharT* lpPath,
    _In_       const CharT* lpFileName,
    _In_opt_   const CharT* lpExtension,
    _In_       DWORD  nBufferLength,
    _Out_      CharT* lpBuffer,
    _Out_opt_  CharT** lpFilePart)
{
    auto guard = g_reentrancyGuard.enter();
    DWORD SearchPathInstance = 0;
    try
    {
        if (!guard)
        {

#if _DEBUG
            if (lpPath != NULL)
            {
                if (lpExtension != NULL)
                {
                    Log(L"[%d]\tSearchPathFixup (unguarded): for folder=%s fileName=%s ext=%s", SearchPathInstance,lpPath, lpFileName, lpExtension);
                }
                else
                {
                    Log(L"[%d]\tSearchPathFixup (unguarded): for folder=%s fileName=%s", SearchPathInstance, lpPath, lpFileName);
                }
            }
            else
            {
                if (lpExtension != NULL)
                {
                    Log(L"[%d]\tSearchPathFixup (unguarded): for  fileName=%s ext=%s", SearchPathInstance, lpFileName, lpExtension);
                }
                else
                {
                    Log(L"[%d]\tSearchPathFixup (unguarded): for  fileName=%s", SearchPathInstance, lpFileName);
                }
            }
#endif
            return impl::SearchPath(lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
        }

        
        SearchPathInstance = ++g_FileIntceptInstance;
#if _DEBUG
        if (lpPath != NULL)
        {
            if (lpExtension != NULL)
            {
                Log(L"[%d]\tSearchPathFixup: for folder=%s fileName=%s ext=%s", SearchPathInstance, lpPath, lpFileName, lpExtension);
            }
            else
            {
                Log(L"[%d]\tSearchPathFixup: for folder=%s fileName=%s", SearchPathInstance, lpPath, lpFileName);
            }
        }
        else
        {
            if (lpExtension != NULL)
            {
                Log(L"[%d]\tSearchPathFixup: for  fileName=%s ext=%s", SearchPathInstance, lpFileName, lpExtension);
            }
            else
            {
                Log(L"[%d]\tSearchPathFixup: for  fileName=%s", SearchPathInstance, lpFileName);
            }
        }
#endif
        //TODO
        DWORD dRet = impl::SearchPath(lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
#if _DEBUG
        if (dRet == 0)
        {
            Log("[%d]\t\tSearchPathFixup: native not found.", SearchPathInstance);
        }
        else
        {
            Log("[%d]\t\tSearchPathFixup: native found.", SearchPathInstance);
        }
#endif

        std::wstring wPathRequested;
        normalized_pathV2 wPathNormalized;   // Cleaned up version of original path reference.
        std::wstring wPathVirtualized;       // VFS path in the package.
        std::wstring wPathDeVirtualized;     // If original was VFS in package, this is the normal system version.
        std::wstring wPathRedirected;        // If redirection is in order, the folder in the redirected area.
        std::wstring wPathDeRedirected;      // If original was in the redirected area, where in the package path does that related to.
        
        if (lpPath == NULL)
        {
            wPathRequested = std::filesystem::current_path().c_str();
        }
        else
        {
            wPathRequested = widen(lpPath);
        }
        wPathNormalized = NormalizePathV2(wPathRequested.c_str(), SearchPathInstance);
        if constexpr (psf::is_ansi<CharT>)
        {
            Log("[%d]\t\tSearchPathFixup: ansi.", SearchPathInstance);
            dRet = impl::SearchPath(narrow(wPathNormalized.full_path).c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
        }
        else
        {
            Log("[%d]\t\tSearchPathFixup: wide.", SearchPathInstance);
            dRet = impl::SearchPath(wPathNormalized.full_path.c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
        }
        if (dRet == 0)
        {
            wPathVirtualized = VirtualizePathV2(wPathNormalized, SearchPathInstance);
            if constexpr (psf::is_ansi<CharT>)
            {
                dRet = impl::SearchPath(narrow(wPathVirtualized).c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
            }
            else
            {
                dRet = impl::SearchPath(wPathVirtualized.c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
            }
            if (dRet == 0)
            {
                wPathDeVirtualized = DeVirtualizePathV2(wPathNormalized);
                if constexpr (psf::is_ansi<CharT>)
                {
                    dRet = impl::SearchPath(narrow(wPathDeVirtualized).c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
                }
                else
                {
                    dRet = impl::SearchPath(wPathDeVirtualized.c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
                }
                if (dRet == 0)
                {
                    wPathRedirected = RedirectedPathV2(wPathNormalized, false, g_writablePackageRootPath.native(), SearchPathInstance);
                    if constexpr (psf::is_ansi<CharT>)
                    {
                        dRet = impl::SearchPath(narrow(wPathRedirected).c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
                    }
                    else
                    {
                        dRet = impl::SearchPath(wPathRedirected.c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
                    }
                    if (dRet == 0)
                    {
                        wPathDeRedirected = ReverseRedirectedToPackage(wPathNormalized.original_path.c_str());
                        if constexpr (psf::is_ansi<CharT>)
                        {
                            dRet = impl::SearchPath(narrow(wPathDeRedirected).c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
                        }
                        else
                        {
                            dRet = impl::SearchPath(wPathDeRedirected.c_str(), lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
                        }
                        if (dRet == 0)
                        {
                            Log(L"[%d]\tSearchPathFixup: Not found under any path.", SearchPathInstance);
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else
                        {
                            LogString(SearchPathInstance, L"\tSearchPathFixup: Found under path", wPathDeRedirected.c_str());
                        }
                    }
                    else
                    {
                        LogString(SearchPathInstance, L"\tSearchPathFixup: Found under path", wPathRedirected.c_str());
                    }
                }
                else
                {
                    LogString(SearchPathInstance, L"\tSearchPathFixup: Found under path", wPathDeVirtualized.c_str());
                }
            }
            else
            {
                LogString(SearchPathInstance, L"\tSearchPathFixup: Found under path", wPathVirtualized.c_str());
            }
        }
        else
        {
            LogString(SearchPathInstance, L"\tSearchPathFixup: Found under path", wPathNormalized.full_path.c_str());
        }
        

        Log(L"[%d]\tSearchPathFixup: return value=0x%x GetLastError=0x%x", SearchPathInstance, dRet, GetLastError());
        return dRet;
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(SearchPathInstance, L"SearchPath")
#else
    catch (...)
    {
        Log(L"[%d] SearchPath Exception=0x%x", SearchPathInstance, GetLastError());
    }
#endif 
    return 0;
    
}
DECLARE_STRING_FIXUP(impl::SearchPath, SearchPathFixup);