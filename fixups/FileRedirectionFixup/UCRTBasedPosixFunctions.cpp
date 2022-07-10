//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include "PathRedirection.h"
#include <psf_logging.h>


// NOTE: At this point, these fixups only intercept for diagnostic purposes.  But they are not
//       intercepting for unknown reasons.  But so far, evidence is that they are not needed as
//       the ucrt functions call into the Kernel32 functions anyway.
#if FIXUP_UCRT

FILE * __cdecl fopenFixup(
    _In_ const char* fileName,
    _In_ const char* mode) 
{
    auto guard = g_reentrancyGuard.enter(); 
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"fopen Fixup filename", fileName);
        LogString(Instance, L"fopen Fixup mode", mode);
#endif
        if (guard)
        {
            ; // if needed
        }
        FILE * file = impl::fopen(fileName, mode);
#if _DEBUG
        if (file == NULL)
        {
            Log(L"[%d]\tfopenFixup returns NULL", Instance);
        }
        else
        {
            Log(L"[%d]\tfopenFixup returns handle", Instance);
        }
#endif
        return file;
    }
    catch (...)
    {
        return NULL;
    }
}
DECLARE_FIXUP(impl::fopen, fopenFixup);



FILE * __cdecl _wfopenFixup(
    _In_ const wchar_t* fileName,
    _In_ const wchar_t* mode)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wfopenFixup filename", fileName);
        LogString(Instance, L"_wfopenFixup mode", mode);
#endif
        if (guard)
        {
            ; // if needed
        }
        FILE * file = impl::_wfopen(fileName, mode);
#if _DEBUG
        if (file == NULL)
        {
            Log(L"[%d]\t_wfopenFixup returns NULL", Instance);
        }
        else
        {
            Log(L"[%d]\t_wfopenFixup returns handle", Instance);
        }
#endif
        return file;
    }
    catch (...)
    {
        return NULL;
    }
}
DECLARE_FIXUP(impl::_wfopen, _wfopenFixup);



errno_t __cdecl fopen_sFixup(
    _Out_ FILE** pFile,
    _In_ const char* fileName,
    _In_ const char* mode)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"fopen_s Fixup filename", fileName);
        LogString(Instance, L"fopen_s Fixup mode", mode);
#endif
        if (guard)
        {
            ; // if needed
        }
        errno_t err = impl::fopen_s(pFile, fileName, mode);
#if _DEBUG
        if (err != ERROR_SUCCESS)
        {
            Log(L"[%d]\tfopen_s returns NULL with error=0x%x", Instance, err);
        }
        else
        {
            Log(L"[%d]\tfopen_s returns handle", Instance);
        }
#endif       
        return err;
    }
    catch (...)
    {
        return NULL;
    }
}
DECLARE_FIXUP(impl::fopen_s, fopen_sFixup);



errno_t __cdecl _wfopen_sFixup(
    _Out_ FILE** pFile,
    _In_ const wchar_t* fileName,
    _In_ const wchar_t* mode)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wfopen_s Fixup filename", fileName);
        LogString(Instance, L"_wfopen_s Fixup mode", mode);
#endif
        if (guard)
        {
            ; // if needed
        }
        errno_t err = impl::_wfopen_s(pFile, fileName, mode);
#if _DEBUG
        if (err != ERROR_SUCCESS)
        {
            Log(L"[%d]\t_wfopen_s returns NULL with error=0x%x", Instance, err);
        }
        else
        {
            Log(L"[%d]\t_wfopen_s returns handle", Instance);
        }
#endif           
        return err;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wfopen_s, _wfopen_sFixup);



intptr_t __cdecl _wfindfirst32Fixup(
    _In_ const wchar_t* filespec,
    _In_ _Out_ struct _wfinddata32_t* fileinfo)
{
    try
    {
        auto guard = g_reentrancyGuard.enter();
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wfindfirst32Fixup filename", filespec);
#endif
        if (guard)
        {
            ; // if needed
        }
        intptr_t iRet = impl::_wfindfirst32(filespec, fileinfo);
#if _DEBUG
        if (iRet == -1)
        {
            Log(L"[%d]\t_wfindfirst32Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wfindfirst32Fixup returns handle", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wfindfirst32, _wfindfirst32Fixup);



intptr_t __cdecl _wfindfirst32i64Fixup(
    _In_ const wchar_t* filespec,
    _In_ _Out_ struct _wfinddata32i64_t* fileinfo)
{
    try
    {
        auto guard = g_reentrancyGuard.enter();
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wfindfirst32i64Fixup filename", filespec);
#endif
            if (guard)
            {
                ; // if needed
            }
        intptr_t iRet = impl::_wfindfirst32i64(filespec, fileinfo);
#if _DEBUG
        if (iRet == -1)
        {
            Log(L"[%d]\t_wfindfirst32i64Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wfindfirst32i64Fixup returns handle", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wfindfirst32i64, _wfindfirst32i64Fixup);



intptr_t __cdecl _wfindfirst64Fixup(
    _In_ const wchar_t* filespec,
    _In_ _Out_ struct _wfinddata64_t* fileinfo)
{
    try
    {
        auto guard = g_reentrancyGuard.enter();
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wfindfirst64Fixup filename", filespec);
#endif
            if (guard)
            {
                ; // if needed
            }
        intptr_t iRet = impl::_wfindfirst64(filespec, fileinfo);
#if _DEBUG
        if (iRet == -1)
        {
            Log(L"[%d]\t_wfindfirst64Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wfindfirst64Fixup returns handle", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wfindfirst64, _wfindfirst64Fixup);



intptr_t __cdecl _wfindfirst64i32Fixup(
    _In_ const wchar_t* filespec,
    _In_ _Out_ struct _wfinddata64i32_t* fileinfo)
{
    try
    {
        auto guard = g_reentrancyGuard.enter();
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wfindfirst64i32Fixup filename", filespec);
#endif
            if (guard)
            {
                ; // if needed
            }
        intptr_t iRet = impl::_wfindfirst64i32(filespec, fileinfo);
#if _DEBUG
        if (iRet == -1)
        {
            Log(L"[%d]\t_wfindfirst64i32Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wfindfirst64i32Fixup returns handle", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wfindfirst64i32, _wfindfirst64i32Fixup);



int __cdecl _wmkdirFixup(
   _In_  const wchar_t* dirname)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wmkdir Fixup dirname", dirname);
#endif
        if (guard)
        {
            ; // if needed
        }
        int iRet = impl::_wmkdir(dirname);
#if _DEBUG
        if (iRet != ERROR_SUCCESS)
        {
            Log(L"[%d]\t_wmkdir Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wmkdir Fixup returns ERROR_SUCCESS", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wmkdir, _wmkdirFixup);



int __cdecl _wopenFixup(
    _In_ const wchar_t* filename,
    _In_ int oflag,
    _In_opt_  int pmode)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wopen Fixup filename", filename);
#endif
        if (guard)
        {
            ; // if needed
        }
        int iRet = impl::_wopen(filename, oflag, pmode);
#if _DEBUG
        if (iRet == -1)
        {
            Log(L"[%d]\t_wopen Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wopen Fixup returns handle", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wopen, _wopenFixup);



int __cdecl _wrmdirFixup(
    _In_  const wchar_t* dirname)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wrmdir Fixup dirname", dirname);
#endif
        if (guard)
        {
            ; // if needed
        }
        int iRet = impl::_wrmdir(dirname);
#if _DEBUG
        if (iRet != ERROR_SUCCESS)
        {
            Log(L"[%d]\t_wrmdir Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wrmdir Fixup returns ERROR_SUCCESS", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wrmdir, _wrmdirFixup);



int __cdecl _wsopenFixup(
    _In_ const wchar_t* filename,
    _In_ int oflag,
    _In_ int shflag,
    _In_opt_  int pmode)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wsopen Fixup filename", filename);
#endif
        if (guard)
        {
            ; // if needed
        }
        int iRet = impl::_wsopen(filename, oflag, shflag, pmode);
#if _DEBUG
        if (iRet == -1)
        {
            Log(L"[%d]\t_wsopen Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wsopen Fixup returns handle", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wsopen, _wsopenFixup);



errno_t  __cdecl _wsopen_sFixup(
    _Out_ int* pfh,
    _In_ const wchar_t* filename,
    _In_ int oflag,
    _In_ int shflag,
    _In_opt_  int pmode )
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wsopen_s Fixup filename", filename);
#endif
        if (guard)
        {
            ; // if needed
        }
        errno_t  iRet = impl::_wsopen_s(pfh, filename, oflag, shflag, pmode);
#if _DEBUG
        if (iRet == -1)
        {
            Log(L"[%d]\t_wsopen_s Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wsopen_s Fixup returns handle", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wsopen_s, _wsopen_sFixup);



#if NoMoreTrouble
int __cdecl _unlinkFixup(
    _In_  const char* dirname)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
        auto guard = g_reentrancyGuard.enter();
        DWORD Instance = ++g_FileIntceptInstance;
#if _DEBUG
        LogString(Instance, L"_unlink Fixup dirname", dirname);
#endif
        if (guard)
        {
            ; // if needed
        }
        int iRet = impl::_unlink(dirname);
#if _DEBUG
        if (iRet != ERROR_SUCCESS)
        {
            Log(L"[%d]\t_unlink Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_unlink Fixup returns ERROR_SUCCESS", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_unlink, _unlinkFixup);
#endif



int __cdecl _wunlinkFixup(
    _In_  const wchar_t* dirname)
{
    auto guard = g_reentrancyGuard.enter();
    try
    {
#if _DEBUG
        DWORD Instance = ++g_FileIntceptInstance;
        LogString(Instance, L"_wunlink Fixup dirname", dirname);
#endif
        if (guard)
        {
            ; // if needed
        }
        int iRet = impl::_wunlink(dirname);
#if _DEBUG
        if (iRet != ERROR_SUCCESS)
        {
            Log(L"[%d]\t_wunlink Fixup returns NULL with error=0x%x", Instance, GetLastError());
        }
        else
        {
            Log(L"[%d]\t_wunlink Fixup returns ERROR_SUCCESS", Instance);
        }
#endif           
        return iRet;
    }
    catch (...)
    {
        return EINVAL;
    }
}
DECLARE_FIXUP(impl::_wunlink, _wunlinkFixup);

#endif

