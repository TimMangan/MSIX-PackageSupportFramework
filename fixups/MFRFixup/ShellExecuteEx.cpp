//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecuteexw

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// One of the uses of this API it to display a file picker like dialog that internally will want to display files.
// This can be an issue for MSIX that the PSF is not designed to handle.  The issue is that a number of VFS folders in the
// package are not treated by the underlying MSIX runtime as folders layered over the equivalent native location.
// For Example: VFS\AppVPackageDrive.  If the picker looks at the package path, such files may be seen, but looking for the folder from the root of C will not.
// Currently, there is no solution to these issues, so we are simply logging the occurrence so that the cause may be more easily detected.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include <psf_logging.h>
#include "FunctionImplementations.h"

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"

#if FIXUP_ORIGINAL_SHELLEXECUTEEX

 BOOL __stdcall  ShellExecuteExAFixup(_Inout_ SHELLEXECUTEINFOA* pExecInfo) 
{
    DWORD dllInstance = g_InterceptInstance;
    [[maybe_unused]] bool debug = false;
#if _DEBUG
    debug = true;
#endif
    [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
    moredebug = true;
#endif

    auto guard = g_reentrancyGuard.enter();
    BOOL retfinal;

    try
    {
        if (guard)
        {
            if (pExecInfo)
            {
                if ((pExecInfo->fMask & SEE_MASK_WAITFORINPUTIDLE) == 0)  // used at the start of some apps to signal app is ready without any real filepickers
                {
                    // Release level logging for detection
                    bool temp = g_psf_NoLogging;
                    g_psf_NoLogging = false;
                    Log(L"[%d] ShellExecuteExA unguarded. Known compatibility issues exist in certain usages!", dllInstance);
                    LogString(dllInstance, L"ShellExecuteExA: file", pExecInfo->lpFile);
                    LogString(dllInstance, L"ShellExecuteExA: verb", pExecInfo->lpVerb);
                    LogString(dllInstance, L"ShellExecuteExA: directory", pExecInfo->lpDirectory);
#ifdef MOREDEBUG
                    Log(L"[%d] ShellExecuteExA fMask=0x%x", dllInstance, pExecInfo->fMask);
#endif
                    LogCallingModule();
                    g_psf_NoLogging = temp;
                }
            }
            retfinal = impl::ShellExecuteExA(pExecInfo);
            return retfinal;
        }
    }
#if _DEBUG
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER(dllInstance, L"ShellExecuteExA")
#else
    catch (...)
    {
        Log(L"[%d] ShellExecuteExA Exception=0x%x", dllInstance, GetLastError());
    }
#endif

    retfinal = impl::ShellExecuteExA(pExecInfo);
    return retfinal;
}
 DECLARE_FIXUP(impl::ShellExecuteExA, ShellExecuteExAFixup);




 BOOL __stdcall  ShellExecuteExWFixup(_Inout_ SHELLEXECUTEINFOW* pExecInfo)
 {
     DWORD dllInstance = g_InterceptInstance;
     [[maybe_unused]] bool debug = false;
#if _DEBUG
      debug = true;
#endif
     [[maybe_unused]] bool moredebug = false;
#if MOREDEBUG
     moredebug = true;
#endif

     auto guard = g_reentrancyGuard.enter();
     BOOL retfinal;

     try
     {
         if (guard)
         {
             if (pExecInfo)
             {
                 if ((pExecInfo->fMask & SEE_MASK_WAITFORINPUTIDLE) == 0)  // used at the start of some apps to signal app is ready without any real filepickers
                 {
                     // Release level logging for detection
                     bool temp = g_psf_NoLogging;
                     g_psf_NoLogging = false;
                     Log(L"[%d] ShellExecuteExW unguarded. Known compatibility issues exist in certain usages!", dllInstance);
                     LogString(dllInstance, L"ShellExecuteExW: verb", pExecInfo->lpVerb);
#if MOREDEBUG
                     Log(L"[%d] ShellExecuteExW fMask=0x%x", dllInstance, pExecInfo->fMask);
#endif
                     LogCallingModule();
                     g_psf_NoLogging = temp;
                 }
             }
             retfinal = impl::ShellExecuteExW(pExecInfo);
             return retfinal;
         }
     }
#if _DEBUG
     // Fall back to assuming no redirection is necessary if exception
     LOGGED_CATCHHANDLER(dllInstance, L"ShellExecuteExW")
#else
     catch (...)
     {
         Log(L"[%d] ShellExecuteExW Exception=0x%x", dllInstance, GetLastError());
     }
#endif

     retfinal = impl::ShellExecuteExW(pExecInfo);
     return retfinal;
 }
 DECLARE_FIXUP(impl::ShellExecuteExW, ShellExecuteExWFixup);

#endif