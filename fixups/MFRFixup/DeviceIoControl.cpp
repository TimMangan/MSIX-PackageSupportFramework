
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api:https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol


#if _DEBUG
//#define MOREDEBUG 1
//#define INTERCEPT_DEVICEIOCONTROL 1
#endif

#include <errno.h>
#include <psf_logging.h>
#include "FunctionImplementations.h"
#include "FunctionImplementations_ntdll.h"

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include <Windows.h>
#include <IoapiSet.h>
#include <mountmgr.h>

#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

#define FILE_ANY_ACCESS 0
#define FILE_READ_ACCESS  0x0001
#define FILE_WRITE_ACCESS 0x0002
#define METHOD_BUFFERED 0

#if INTERCEPT_DEVICEIOCONTROL

BOOL __stdcall DeviceIoControlFixup(
    IN HANDLE hDevice,
    IN DWORD dwIoControlCode,
    IN OPTIONAL LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    IN OPTIONAL LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT OPTIONAL LPDWORD lpBytesReturned,
    IN OUT OPTIONAL LPOVERLAPPED lpOverlapped)
{
    DWORD dllInstance = ++g_InterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            std::wstring wsCode = L"";
            switch (dwIoControlCode)
            {
            case IOCTL_MOUNTMGR_CREATE_POINT:
                wsCode = L"IOCTL_MOUNTMGR_CREATE_POINTS";
                break;
            case IOCTL_MOUNTMGR_DELETE_POINTS:
                wsCode = L"IOCTL_MOUNTMGR_DELETE_POINTS";
                break;
            case IOCTL_MOUNTMGR_QUERY_POINTS:
                    wsCode = L"IOCTL_MOUNTMGR_QUERY_POINTS";
                    break;
            case IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY:
            case IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER:
            case IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS:
            case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED:
            case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED:
            case IOCTL_MOUNTMGR_CHANGE_NOTIFY:
            case IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE:
            case IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES:
            case IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION:
                wsCode = L"Known IOCTL Code";
                break;
            default:
                wsCode = L"Unknown IOCTL Code";
                break;
            }
            Log(LogLevel_DebugBasic, L"[%s%d] DeviceIoControlFixup: Handle=0x%x IoControlCode=0x%x=%s", g_MfrModuleName, dllInstance, hDevice, dwIoControlCode,wsCode.c_str());
            Log(LogLevel_DebugBasic, "[%s%d] DeviceIoControlFixup: InBuffer=0x%x Size=%d OutBuffer=0x%x Size=%d",
                g_MfrModuleName, dllInstance, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize);

            ;  // normally you will return here after calling the native function...
        }
    }
    catch (...)
    {
        Log(LogLevel_Exception, L"[%s%d] DeviceIoControlFixup: Exception=0x%x", g_MfrModuleName, dllInstance, GetLastError());
    }
    BOOL bVal = impl::DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

    if (bVal)
    {
        try
        {
            if (lpInBuffer != NULL && nInBufferSize > 0)
            {
                Loghexdump(LogLevel_DebugBasic, lpInBuffer, nInBufferSize, g_MfrModuleName, dllInstance);
                std::string lpInBufferStr(reinterpret_cast<const char*>(lpInBuffer), nInBufferSize);
                LogString(LogLevel_DebugMaximum,g_MfrModuleName, dllInstance, "DeviceIoControlFixup: Input Buffer", lpInBufferStr.c_str());
            }
            if (lpOutBuffer != NULL && *lpBytesReturned > 0)
            {
                Loghexdump(LogLevel_DebugBasic, lpOutBuffer, *lpBytesReturned, g_MfrModuleName, dllInstance);
                std::string lpOutBufferStr(reinterpret_cast<const char*>(lpOutBuffer), *lpBytesReturned);
                LogString(LogLevel_DebugMaximum,g_MfrModuleName, dllInstance, "DeviceIoControlFixup: Output Buffer", lpOutBufferStr.c_str());
            }
        }
        catch (...)
        {
        }
    }
    Log(LogLevel_DebugBasic, L"[%s%d] DeviceIoControlFixup: returns BOOL %d with error=0x%x", g_MfrModuleName, dllInstance, bVal, GetLastError());
    return bVal;
}
DECLARE_FIXUP(impl::DeviceIoControl, DeviceIoControlFixup);


#endif