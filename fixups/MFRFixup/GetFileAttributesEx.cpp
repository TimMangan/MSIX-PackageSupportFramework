//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#if _DEBUG
#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "DetermineCohorts.h"
#include "DetermineILVpaths.h"

//#define DEBUGPATHTESTING 1
#include "DebugPathTesting.h"


void LogAttributesEx(Json_Debug_Levels debugRequestLevel, const wchar_t* MfrModuleName, DWORD dllInstance, LPVOID fileInformation)
{
    if (fileInformation != NULL)
    {
        Log(debugRequestLevel, L"[%s%d] GetFileAttributesEx         Attributes %s  Size 0x%I64x 0x%I64x  Creation 0x%x 0x%x  Access 0x%x 0x%x  Write 0x%x 0x%x",
            MfrModuleName, dllInstance,
            Log_FlagsAndAttributes(debugRequestLevel, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->dwFileAttributes).c_str(),
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeHigh, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->nFileSizeLow,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwHighDateTime, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftCreationTime.dwLowDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwHighDateTime, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastAccessTime.dwLowDateTime,
            ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwHighDateTime, ((LPWIN32_FILE_ATTRIBUTE_DATA)fileInformation)->ftLastWriteTime.dwLowDateTime);
    }
}




#define WRAPPER_GETFILEATTRIBUTESEX(theDestinationFilename, operationString) \
    { \
        std::wstring LongDestinationFilename = MakeLongPath(theDestinationFilename); \
        retfinal = impl::GetFileAttributesEx(LongDestinationFilename.c_str(), infoLevelId, fileInformation); \
        DWORD error = GetLastError(); \
        if (retfinal != 0) \
        { \
            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup for %s returns result SUCCESS and Attr 0x%x on file '%s'", g_MfrModuleName, dllInstance , operationString, retfinal, LongDestinationFilename.c_str()); \
            LogAttributesEx(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, fileInformation); \
            SetLastError(0); \
            return retfinal; \
        } \
        if (error == ERROR_FILE_NOT_FOUND) \
        { \
            anyFileNotFound = true; \
        } \
        else if (error == ERROR_PATH_NOT_FOUND) \
        { \
            anyPathNotFound = true; \
        } \
        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup for %s FAILED 0x%x on file %s.", g_MfrModuleName, dllInstance, operationString, error, LongDestinationFilename.c_str() ); \
    }


template <typename CharT>
BOOL __stdcall GetFileAttributesExFixup(
    _In_ const CharT* fileName,
    _In_ GET_FILEEX_INFO_LEVELS infoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID fileInformation) noexcept
{
    DWORD dllInstance = g_InterceptInstance;
    DWORD retfinal = 0;
    auto guard = g_reentrancyGuard.enter();
    try
    {
        if (guard)
        {
            dllInstance = ++g_InterceptInstance;
            std::wstring wfileName = widen(fileName);
            wfileName = AdjustSlashes(wfileName, dllInstance);
            
            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup level 0x%x for fileName '%s' ", g_MfrModuleName, dllInstance, infoLevelId, wfileName.c_str());
            wfileName = AdjustBadUNC(wfileName, dllInstance, L"GetFileAttributesExFixup");
            

            Cohorts cohorts;
            DetermineCohorts(LogLevel_DebugIntermediate, wfileName, &cohorts, dllInstance, L"GetFileAttributesExFixup");
            bool anyFileNotFound = false;
            bool anyPathNotFound = false;

            if (!MFRConfiguration.Ilv_Aware)
            {
                switch (cohorts.file_mfr.Request_MfrPathType)
                {
                case mfr::mfr_path_types::in_native_area:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                            {
                                // try the request path, which must be the local redirected version by definition, and then a package equivalent  
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, L"WsRedirected");   // returns if successful.
                                if (cohorts.WsPackage.compare(cohorts.WsRedirected) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");   // returns if successful.
                                }

                                if (cohorts.WsRequested.compare(cohorts.WsRedirected) != 0 &&
                                    cohorts.WsRequested.compare(cohorts.WsPackage) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRequested, L"WsRequested");   // returns if successful.
                                }
                            }
                            else
                            {
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");   // returns if successful.\

                                if (cohorts.WsRequested.compare(cohorts.WsPackage) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRequested, L"WsRequested");   // returns if successful.
                                }
                            }
                            // Everything failed if here
                            if (anyFileNotFound)
                            {
                                SetLastError(ERROR_FILE_NOT_FOUND);
                            }
                            else if (anyPathNotFound)
                            {
                                SetLastError(ERROR_PATH_NOT_FOUND);
                            }
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup returns with result 0x%x and error =0x%x", g_MfrModuleName, dllInstance, retfinal, GetLastError());
                            return retfinal;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                            {
                                // try the redirected path, then package, then native.
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, L"WsRedirected");   // returns if successful.
                                if (cohorts.WsPackage.compare(cohorts.WsRedirected) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");   // returns if successful.
                                }
                                if (cohorts.WsNative.compare(cohorts.WsRedirected) != 0 &&
                                    cohorts.WsNative.compare(cohorts.WsPackage) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, L"WsNative");   // returns if successful.
                                }
                            }
                            else
                            {
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");   // returns if successful.
                                if (cohorts.WsNative.compare(cohorts.WsPackage) != 0 )
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, L"WsNative");   // returns if successful.
                                }
                            }

                            // All failed if here
                            if (anyFileNotFound)
                            {
                                SetLastError(ERROR_FILE_NOT_FOUND);
                            }
                            else if (anyPathNotFound)
                            {
                                SetLastError(ERROR_PATH_NOT_FOUND);
                            }
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup returns with result 0x%x and error =0x%x", g_MfrModuleName, dllInstance, retfinal, GetLastError());
                            return retfinal;
                        case mfr::mfr_redirect_flags::prefer_redirection_none:
                        case mfr::mfr_redirect_flags::disabled:
                        default:
                            // just fall through to unguarded code
                            break;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_package_pvad_area:
                    /// NOTE: Ilv does not allow accessing PVAD files in the package.  PERIOD!!!  So this call will always fail.
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                        {
                            //// try the redirected path, then package, then don't need native.
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, L"WsRedirected");   // returns if successful.
                            if (cohorts.WsPackage.compare(cohorts.WsRedirected) != 0)
                            {
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, "WsPackage");   // returns if successful.
                            }
                        }
                        else
                        {
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, "WsPackage");   // returns if successful.
                        }

                        // Both failed if here
                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup returns with result 0x%x and error =0x%x", g_MfrModuleName, dllInstance, retfinal, GetLastError());
                        return retfinal;
                    }
                    break;
                case mfr::mfr_path_types::in_package_vfs_area:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        switch (cohorts.map.RedirectionFlags)
                        {
                        case mfr::mfr_redirect_flags::prefer_redirection_local:
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                            {
                                // try the request path, which must be the local redirected version by definition, and then a package equivalent.
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, L"WsRedirected");   // returns if successful.
                            }
                            if (cohorts.WsPackage.compare(cohorts.WsRedirected) != 0)
                            {
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");   // returns if successful.
                            }

                            // Both failed if here
                            if (anyFileNotFound)
                            {
                                SetLastError(ERROR_FILE_NOT_FOUND);
                            }
                            else if (anyPathNotFound)
                            {
                                SetLastError(ERROR_PATH_NOT_FOUND);
                            }
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup returns with result 0x%x and error =0x%x", g_MfrModuleName, dllInstance, retfinal, GetLastError());
                            return retfinal;
                        case mfr::mfr_redirect_flags::prefer_redirection_containerized:
                        case mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs:
                            if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                            {
                                // try the redirected path, then package, then native.
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, L"WsRedirected");  // returns if successful.
                                if (cohorts.WsPackage.compare(cohorts.WsRedirected) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");  // returns if successful.
                                }
                                if (cohorts.WsNative.compare(cohorts.WsRedirected) != 0 &&
                                    cohorts.WsNative.compare(cohorts.WsPackage) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, L"WsNative");  // returns if successful.
                                }
                            }
                            else
                            {
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");  // returns if successful.
                                if (cohorts.WsNative.compare(cohorts.WsPackage) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, L"WsNative");  // returns if successful.
                                }
                            }


                            // All failed if here
                            if (anyFileNotFound)
                            {
                                SetLastError(ERROR_FILE_NOT_FOUND);
                            }
                            else if (anyPathNotFound)
                            {
                                SetLastError(ERROR_PATH_NOT_FOUND);
                            }
                            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup returns with result 0x%x and error =0x%x", g_MfrModuleName, dllInstance, retfinal, GetLastError());
                            return retfinal;
                        case mfr::mfr_redirect_flags::prefer_redirection_none:
                        case mfr::mfr_redirect_flags::disabled:
                        default:
                            // just fall through to unguarded code
                            break;
                        }
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                    if (cohorts.map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        if (cohorts.map.IsAnExclusionToRedirect == mfr::mfr_exclusion_types::not_excluded)
                        {
                            // try the redirected path, then package, then native if relevant.
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsRedirected, L"WsRedirected");  // returns if successful
                            if (cohorts.WsPackage.compare(cohorts.WsRedirected) != 0)
                            {
                                WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");  // returns if successful.
                            }
                            if (cohorts.NativeIsValidOptionInScenario)
                            {
                                if (!cohorts.WsNative.compare(cohorts.WsRedirected) &&
                                    !cohorts.WsNative.compare(cohorts.WsPackage))
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, L"WsNative");  // returns if successful.
                                }
                            }
                        }
                        else
                        {
                            WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsPackage, L"WsPackage");  // returns if successful.

                            if (cohorts.NativeIsValidOptionInScenario)
                            {
                                if (cohorts.WsNative.compare(cohorts.WsPackage) != 0)
                                {
                                    WRAPPER_GETFILEATTRIBUTESEX(cohorts.WsNative, L"WsNative");  // returns if successful.
                                }
                            }
                        }
                        

                        if (anyFileNotFound)
                        {
                            SetLastError(ERROR_FILE_NOT_FOUND);
                        }
                        else if (anyPathNotFound)
                        {
                            SetLastError(ERROR_PATH_NOT_FOUND);
                        }
                        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup returns with result 0x%x and error =0x%x", g_MfrModuleName, dllInstance, retfinal, GetLastError());
                        return retfinal;
                    }
                    break;
                case mfr::mfr_path_types::in_redirection_area_other:
                    break;
                case mfr::mfr_path_types::is_Protocol:
                case mfr::mfr_path_types::is_DosSpecial:
                case mfr::mfr_path_types::is_Shell:
                case mfr::mfr_path_types::in_other_drive_area:
                case mfr::mfr_path_types::is_UNC_path:
                case mfr::mfr_path_types::unsupported_for_intercepts:
                case mfr::mfr_path_types::unknown:
                default:
                    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesExFixup has mfr_path_type 0x%x", g_MfrModuleName, dllInstance, cohorts.file_mfr.Request_MfrPathType);
                    break;
                }
            }
            else
            {
                // ILV 
                std::wstring UseFile = DetermineIlvPathForReadOperations(LogLevel_DebugIntermediate, cohorts, dllInstance);
                
                // In a redirect to local scenario, we are responsible for determining if source is local or in package
                UseFile = SelectLocalOrPackageForRead(UseFile, cohorts.WsPackage);

                WRAPPER_GETFILEATTRIBUTESEX(UseFile, L"IlvMode");  // returns if successful.
                return retfinal;
            }
        }
    }
    // Fall back to assuming no redirection is necessary if exception
    LOGGED_CATCHHANDLER_MIN(LogLevel_Exception, g_MfrModuleName, dllInstance, L"GetFileAttributesExFixup")

    SetLastError(0);
    if (fileName != nullptr)
    {
        std::wstring LongFileName = MakeLongPath(widen(fileName));
        Log(LogLevel_DebugIntermediate, L"[%s%d] GetFileAttributesEx: unfixed versus %s", g_MfrModuleName, dllInstance, LongFileName.c_str());

        retfinal = impl::GetFileAttributesEx(LongFileName.c_str(), infoLevelId, fileInformation);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        retfinal = INVALID_FILE_ATTRIBUTES; //impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
    }
    Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns retfinal=%d", g_MfrModuleName, dllInstance, retfinal);
    if (retfinal == 0)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns GetLastError=0x%x", g_MfrModuleName, dllInstance, GetLastError());
        if (GetLastError() == 2)
        {
            retfinal = impl::GetFileAttributesEx(fileName, infoLevelId, fileInformation);
            Log(LogLevel_DebugBasic, L"[%s%d] GetFileAttributesEx: returns retry retfinal=%d", g_MfrModuleName, dllInstance, retfinal);
        }
    }
    else
    {
        LogAttributesEx(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, fileInformation);
    }
    return retfinal;
}
DECLARE_STRING_FIXUP(impl::GetFileAttributesEx, GetFileAttributesExFixup);

