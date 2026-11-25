//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// GetFileInformationByName (kernelbase, Windows 11 24H2+): path based "stat" API.
// libuv (Node.js, Electron) uses it for stat() when available. Without this fixup the call sees only the native
// location: a folder that MFR created in the redirection area (e.g. %LOCALAPPDATA%\<App>) is reported as missing,
// so a recursive mkdir fails with ENOENT after CreateDirectory returned ERROR_ALREADY_EXISTS.
// The function is resolved at runtime; on older Windows versions the target is null and the fixup is not registered.

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineILVpaths.h"
#include "MFRConfiguration.h"

typedef BOOL(WINAPI* GetFileInformationByName_t)(PCWSTR FileName, int FileInformationClass, PVOID FileInfoBuffer, ULONG FileInfoBufferSize);

namespace kernelbaseimpl
{
    inline GetFileInformationByName_t GetFileInformationByNameImpl =
        reinterpret_cast<GetFileInformationByName_t>(::GetProcAddress(::LoadLibraryW(L"KernelBase.dll"), "GetFileInformationByName"));
}

BOOL WINAPI GetFileInformationByNameFixup(PCWSTR fileName, int fileInformationClass, PVOID fileInfoBuffer, ULONG fileInfoBufferSize) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard && fileName != nullptr)
        {
            dllInstance = ++g_InterceptInstance;
            std::wstring wfileName = AdjustSlashes(std::wstring(fileName), dllInstance);
            Log(LogLevel_DebugBasic, L"[%s%d] GetFileInformationByNameFixup class %d for '%s'", g_MfrModuleName, dllInstance, fileInformationClass, wfileName.c_str());
            wfileName = AdjustBadUNC(wfileName, dllInstance, L"GetFileInformationByNameFixup");

            Cohorts cohorts;
            DetermineCohorts(LogLevel_DebugIntermediate, wfileName, &cohorts, dllInstance, L"GetFileInformationByNameFixup");

            if (MFRConfiguration.Ilv_Aware)
            {
                // ILV mode: same handling as GetFileAttributesFixup. ILV merges package and user changes itself (and only
                // this path selection honors the ILV deletion markers), so ask once on the path chosen for read operations.
                std::wstring useFile = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohorts, dllInstance);
                useFile = SelectLocalOrPackageForRead(useFile, cohorts.WsPackage);
                std::wstring longUseFile = MakeLongPath(useFile);
                BOOL ilvRet = kernelbaseimpl::GetFileInformationByNameImpl(longUseFile.c_str(), fileInformationClass, fileInfoBuffer, fileInfoBufferSize);
                DWORD ilvError = ilvRet ? ERROR_SUCCESS : ::GetLastError();
                Log(LogLevel_DebugBasic, L"[%s%d] GetFileInformationByNameFixup in ILV Mode returns %d error 0x%x for '%s'", g_MfrModuleName, dllInstance, ilvRet, ilvError, longUseFile.c_str());
                ::SetLastError(ilvError);
                return ilvRet;
            }

            // Not ILV: same order as the other file information fixups: redirected area, package, native.
            std::vector<std::wstring> candidates;
            auto add = [&candidates](const std::wstring& p)
            {
                if (p.empty()) return;
                for (auto& c : candidates) { if (_wcsicmp(c.c_str(), p.c_str()) == 0) return; }
                candidates.push_back(p);
            };
            add(cohorts.WsRedirected);
            add(cohorts.WsPackage);
            if (cohorts.NativeIsValidOptionInScenario)
            {
                add(cohorts.WsNative);
            }
            add(cohorts.WsRequested);

            bool anyFileNotFound = false;
            DWORD lastError = ERROR_PATH_NOT_FOUND;
            for (auto& candidate : candidates)
            {
                std::wstring longName = MakeLongPath(candidate);
                BOOL ret = kernelbaseimpl::GetFileInformationByNameImpl(longName.c_str(), fileInformationClass, fileInfoBuffer, fileInfoBufferSize);
                DWORD error = ::GetLastError();
                if (ret)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileInformationByNameFixup returns SUCCESS for '%s'", g_MfrModuleName, dllInstance, longName.c_str());
                    ::SetLastError(ERROR_SUCCESS);
                    return ret;
                }
                Log(LogLevel_DebugBasic, L"[%s%d] GetFileInformationByNameFixup FAILED 0x%x for '%s'", g_MfrModuleName, dllInstance, error, longName.c_str());
                if (error == ERROR_FILE_NOT_FOUND)
                {
                    anyFileNotFound = true;
                }
                else if (error != ERROR_PATH_NOT_FOUND)
                {
                    // e.g. ERROR_INSUFFICIENT_BUFFER, ERROR_INVALID_PARAMETER: not a location problem, report as is
                    lastError = error;
                    ::SetLastError(lastError);
                    return FALSE;
                }
            }
            lastError = anyFileNotFound ? ERROR_FILE_NOT_FOUND : ERROR_PATH_NOT_FOUND;
            Log(LogLevel_DebugBasic, L"[%s%d] GetFileInformationByNameFixup returns FAILURE 0x%x", g_MfrModuleName, dllInstance, lastError);
            ::SetLastError(lastError);
            return FALSE;
        }
    }
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_MfrModuleName, dllInstance, L"GetFileInformationByNameFixup")

    return kernelbaseimpl::GetFileInformationByNameImpl(fileName, fileInformationClass, fileInfoBuffer, fileInfoBufferSize);
}
DECLARE_FIXUP(kernelbaseimpl::GetFileInformationByNameImpl, GetFileInformationByNameFixup);
