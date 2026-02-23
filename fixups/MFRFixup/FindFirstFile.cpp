//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Microsoft documentation on this api: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilew

// This implementation of FindFileFirst/Next supports finding files under three different possible locations, all layered together.
// 
// The general design is roughly as follows. 
// When a find is requested, it is possible that in addition to the location requested, there may be files that we should find in up to three other locations:
//   * The (equivalent) native location
//   * The (equivalent) package location
//   * The (equivalent) redirected location.
//
// First, we develope a "clean" version of the requested path that looks like C:\whatever to aid in creating the alternate paths:
// A) An area where package file changes may have been previously redirected to.
// B) A VFS path in the package (when the location requested is outside of the package boundary).
// C) The normalized path as requested.
// D) A devirtualized native path (when the location requested is inside a package VFS folder, this is the native location).
// E) A deredirected package path (when the requested location is in the writablepackageroot area).
// 
// So we calculate all five possible paths (or deterine the path is not possible/applicable),
// Then we build up results in a logical order, presenting in order of: Redirection, VFS, requested, and devirtualized native, deredirected package.
//
// NOTE: If we ever address the "delete package file" problem, we'll need to address that here, too.
// NOTE2: If successful, this replacement form might be a model needed in other file operations.
// NOTE2: Unlike many MFR intercepts, this function returns strings to the caller.

// =======================================================================================================  
// NOTE3: ****VERY IMPORTANT*** Changes to this file MUST be made in parallel with FindFirstFileEx.cpp
// =======================================================================================================



#include <errno.h>
#include "FunctionImplementations.h"

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include <psf_logging.h>
#include <memory>
#include "FindData3.h"
#include "FindFirstHelpers.h"
#include "DetermineCohorts.h"
#include "FID.h"
#include "FindFirstFile.h"


//#pragma comment(linker, "/EXPORT:FindFirstFile_Ansi_Fixup=impl::FindFirstFileEx.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#pragma comment(linker, "/EXPORT:FindFirstFile_Wide_Fixup=impl::FindFirstFileEx.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.

//#pragma comment(linker, "/EXPORT:FindFirstFileFixupAnsi_Fixup=impl::FindFirstFileW<char>")  // A test to see if exporting these names helps ProcessMonitor stack traces.
//#pragma comment(linker, "/EXPORT:FindFirstFileFixupWide_Fixup=impl::FindFirstFileW<wchar_t>")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:FindFirstFileA_FixupHelper=_FindFirstFileAFixupHelper@8")
#pragma comment(linker, "/EXPORT:FindFirstFileW_FixupHelper=_FindFirstFileWFixupHelper@8")
#pragma comment(linker, "/EXPORT:FindFirstFileFixupAnsi_Fixup=impl:_FindFirstFileFixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:FindFirstFileFixupWide_Fixup=impl:_FindFirstFileFixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#else
#pragma comment(linker, "/EXPORT:FindFirstFileA_FixupHelper=FindFirstFileAFixupHelper")
#pragma comment(linker, "/EXPORT:FindFirstFileW_FixupHelper=FindFirstFileWFixupHelper")
#pragma comment(linker, "/EXPORT:FindFirstFileFixupAnsi_Fixup=impl::FindFirstFileFixup.ansi")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#pragma comment(linker, "/EXPORT:FindFirstFileFixupWide_Fixup=impl::FindFirstFileFixup.wide")  // A test to see if exporting these names helps ProcessMonitor stack traces.
#endif


extern "C" HANDLE __stdcall FindFirstFileAFixupHelper(_In_ const char* fileName, _Out_ win32_find_data_t<char>* findFileData)
{
    DWORD dllInstance = ++g_InterceptInstance;

    if (!findFileData)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    findFileData->cFileName[0] = '\0'; // Clear the output buffer to ensure we don't return garbage if we fail
    findFileData->cAlternateFileName[0] = '\0'; // Clear the alternate file name buffer as well



    LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, "FindFirstFileAFixup: for fileName", fileName);


    std::string afileName = AdjustSlashes(fileName, dllInstance);

    auto result = std::make_unique<FindData3A>();
    result->RememberedInstance = dllInstance;
    result->requested_path = afileName;


    afileName = AdjustBadUNC(afileName, dllInstance, "FindFirstFileAFixup");

    // Adjust for the bad Winzip issue of asking for C:\ProgramFilesX64\WindowsApps...
    afileName = AdjustPFx64Path(afileName, dllInstance, L"FindFirstFileAFixup");

    // Determine possible paths involved
    Cohorts cohorts;
    DetermineCohorts(LogLevel_DebugIntermediate, widen(fileName), &cohorts, dllInstance, L"FindFirstFileAFixup");


    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup:  Adjusted Path=%s", g_MfrModuleName, dllInstance, afileName.c_str());
    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup:      RedirPath=%s", g_MfrModuleName, dllInstance, cohorts.WsRedirected.c_str());
    Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup:    PackagePath=%s", g_MfrModuleName, dllInstance, cohorts.WsPackage.c_str());
    if (cohorts.NativeIsValidOptionInScenario)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup:     NativePath=%s", g_MfrModuleName, dllInstance, cohorts.WsNative.c_str());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup:  NO NativePath", g_MfrModuleName, dllInstance);
    }

    

    DWORD initialFindError = ERROR_PATH_NOT_FOUND;

    // First find the redirected area results
    std::string rldUseFile = narrow(MakeLongPath(cohorts.WsRedirected));
    result->find_handles[Result_Redirected].reset(impl::FindFirstFile(rldUseFile.c_str(), &result->cached_data[Result_Redirected]));
    // Some applications really care about the failure reason. Try and make this the best that we can, preferring
    // something like "file not found" over "path does not exist"
    initialFindError = ::GetLastError();


    if (result->find_handles[Result_Redirected])
    {      
        if (result->cached_data[Result_Redirected].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from redirected): had results=\'%ls\' \'%ls\'", g_MfrModuleName, dllInstance, Result_Redirected, result->cached_data[Result_Redirected].cFileName, result->cached_data[Result_Redirected].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from redirected): had result=\'%ls\'", g_MfrModuleName, dllInstance, Result_Redirected, result->cached_data[Result_Redirected].cFileName);
        }

    }
    else
    {
        //if (initialFindError == ERROR_FILE_NOT_FOUND)
        //    AnyValidPath = true;

        // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
        //result->redirect_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from redirected): had no results", g_MfrModuleName, dllInstance, Result_Redirected);
    }
    // save for next level

    rldUseFile = narrow(MakeLongPath(cohorts.WsPackage));
    result->find_handles[Result_Package].reset(impl::FindFirstFile(rldUseFile.c_str(), &result->cached_data[Result_Package]));

    if (result->find_handles[Result_Package])
    {
        if (result->cached_data[Result_Package].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from package):   had results=\'%ls\' \'%ls\'", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName, result->cached_data[Result_Package].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from package):   had result=\'%ls\'", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName);
        }
        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
            initialFindError = ERROR_FILE_NOT_FOUND;
        ///result->package_vfs_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from package):   had no results", g_MfrModuleName, dllInstance, Result_Package);
    }

 

    if (cohorts.NativeIsValidOptionInScenario)
    {
        rldUseFile = narrow(MakeLongPath(cohorts.WsNative));
        result->find_handles[Result_Native].reset(impl::FindFirstFile(rldUseFile.c_str(), &result->cached_data[Result_Native]));
        if (result->find_handles[Result_Native])
        {
            if (result->cached_data[Result_Native].cAlternateFileName != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from native)    had results=\'%ls\' \'%ls\'", g_MfrModuleName, dllInstance, Result_Native, result->cached_data[Result_Native].cFileName, result->cached_data[Result_Native].cAlternateFileName);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from native)    had result=\'%ls\'", g_MfrModuleName, dllInstance, Result_Native, result->cached_data[Result_Native].cFileName);
            }
            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
                initialFindError = ERROR_FILE_NOT_FOUND;
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from native):   had no results", g_MfrModuleName, dllInstance, Result_Native);
        }
        
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup[%d] (from native):    no results possible", g_MfrModuleName, dllInstance, Result_Native);
    }

    if (result->find_handles[Result_Redirected] ||
        result->find_handles[Result_Package] ||
        result->find_handles[Result_Native])
    {
        int UseIndex = ChooseIndexForFindResult(cohorts, result.get());

        // return first result found
        copy_find_data(result->cached_data[UseIndex], *findFileData);

        // Consider the case where the name is a variablized MSIX name, such as ProgramFilesX64 and the app asked for "C:\Program Files".
        // In that case, we really want to return what they asked for, because although they already knew the name, they might use the value
        // we return instead, and bad things happen.
        if (UseIndex == Result_Redirected || UseIndex == Result_Package)
        {
            if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_native_area)
            {
                if (afileName.find_first_of("*?") == std::string::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::string origNameOnly = afileName;
                    if (origNameOnly.length() > 0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == "\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of("\\") != std::string::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of("\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup SWAP %s for %s", g_MfrModuleName, dllInstance, widen(origNameOnly).c_str(), widen(result->cached_data[UseIndex].cFileName).c_str());
                        strcpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        strcpy_s(findFileData->cFileName, MAX_PATH, origNameOnly.c_str());

                    }
                }
            }
            else if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_package_pvad_area)
            {
                // This means that the requested path is in a package area, but not in the VFS area.
                // Most likely this is the root path of the package, but possibly not.
                if (afileName.find_first_of("*?") == std::string::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::string origNameOnly = afileName;
                    if (origNameOnly.length() > 0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == "\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of("\\") != std::string::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of("\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup SWAP %s for %s", g_MfrModuleName, dllInstance, widen(origNameOnly).c_str(), widen(result->cached_data[UseIndex].cFileName).c_str());

                        strcpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        strcpy_s(findFileData->cFileName, MAX_PATH, origNameOnly.c_str());

                    }
                }
            }
        }
        if (result->cached_data[UseIndex].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup returns from index=%d \'%ls\' \'%ls\'", g_MfrModuleName, dllInstance, UseIndex, result->cached_data[UseIndex].cFileName, result->cached_data[UseIndex].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup return from index=%d \'%ls\'", g_MfrModuleName, dllInstance, UseIndex, result->cached_data[UseIndex].cFileName);
        }
        result->sAlready_returned_list.push_back(result->cached_data[UseIndex].cFileName);
        ::SetLastError(ERROR_SUCCESS);
        
        return reinterpret_cast<HANDLE>(result.release());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileAFixup returns 0x%x", g_MfrModuleName, dllInstance, initialFindError);

        ::SetLastError(initialFindError);
        return INVALID_HANDLE_VALUE;
    }

}



extern "C" HANDLE __stdcall FindFirstFileWFixupHelper(_In_ const wchar_t* fileName, _Out_ win32_find_data_t<wchar_t>* findFileData)
{
    DWORD dllInstance = ++g_InterceptInstance;

    if (!findFileData)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    // Clear the output buffer to ensure we don't return garbage if we fail
    findFileData->cFileName[0] = L'\0';
    findFileData->cAlternateFileName[0] = L'\0';


    auto result = std::make_unique<FindData3W>();
    result->RememberedInstance = dllInstance;

    LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, "FindFirstFileFixup: for fileName", fileName);


    std::wstring wfileName = AdjustSlashes(fileName, dllInstance);
    result->requested_path = wfileName;


    // Adjust the file name to handle bad UNC paths
    wfileName = AdjustBadUNC(wfileName, dllInstance, L"FindFirstFileWFixup");

    // Adjust for the bad Winzip issue of asking for C:\ProgramFilesX64\WindowsApps...
    wfileName = AdjustPFx64Path(wfileName, dllInstance, L"FindFirstFileWFixup");

    // Determine possible paths involved
    Cohorts cohorts;
    DetermineCohorts(LogLevel_DebugMaximum, wfileName, &cohorts, dllInstance, L"FindFirstFileWFixup");


    Log(LogLevel_DebugIntermediate, L"[%s%d] FindFirstFileWFixup:  Adjusted Path=%s", g_MfrModuleName, dllInstance, wfileName.c_str());
    Log(LogLevel_DebugIntermediate, L"[%s%d] FindFirstFileWFixup:      RedirPath=%s", g_MfrModuleName, dllInstance, cohorts.WsRedirected.c_str());
    Log(LogLevel_DebugIntermediate, L"[%s%d] FindFirstFileWFixup:    PackagePath=%s", g_MfrModuleName, dllInstance, cohorts.WsPackage.c_str());
    if (cohorts.NativeIsValidOptionInScenario)
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup:     NativePath=%s", g_MfrModuleName, dllInstance, cohorts.WsNative.c_str());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup:  NO NativePath", g_MfrModuleName, dllInstance);
    }




    DWORD initialFindError = ERROR_PATH_NOT_FOUND;

    // First find the redirected area results
    std::wstring rldUseFile = MakeLongPath(cohorts.WsRedirected);
    result->find_handles[Result_Redirected].reset(impl::FindFirstFile(rldUseFile.c_str(), &result->cached_data[Result_Redirected]));
    // Some applications really care about the failure reason. Try and make this the best that we can, preferring
    // something like "file not found" over "path does not exist"
    initialFindError = ::GetLastError();


    if (result->find_handles[Result_Redirected])
    {
        if (result->cached_data[Result_Redirected].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from redirected): had results=\'%s\' \'%s\'", g_MfrModuleName, dllInstance, Result_Redirected, result->cached_data[Result_Redirected].cFileName, result->cached_data[Result_Redirected].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from redirected): had results=\'%s\'", g_MfrModuleName, dllInstance, Result_Redirected, result->cached_data[Result_Redirected].cFileName);
        }
    }
    else
    {
        //if (initialFindError == ERROR_FILE_NOT_FOUND)
        //    AnyValidPath = true;

        // Path doesn't exist or match any files. We can safely get away without the redirected file exists check
        //result->redirect_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from redirected): had no results", g_MfrModuleName, dllInstance, Result_Redirected);

    }

    rldUseFile = MakeLongPath(cohorts.WsPackage);
    result->find_handles[Result_Package].reset(impl::FindFirstFile(rldUseFile.c_str(), &result->cached_data[Result_Package]));

    if (result->find_handles[Result_Package])
    {
        if (result->cached_data[Result_Package].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from package):   had results=\'%s\' \'%s\'", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName, result->cached_data[Result_Package].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from package):   had result=\'%s\'", g_MfrModuleName, dllInstance, Result_Package, result->cached_data[Result_Package].cFileName);
        }
        initialFindError = ERROR_SUCCESS;
    }
    else
    {
        if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
            initialFindError = ERROR_FILE_NOT_FOUND;
        ///result->package_vfs_path.clear();
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from package):   had no results", g_MfrModuleName, dllInstance, Result_Package);
    }



    if (cohorts.NativeIsValidOptionInScenario)
    {
        rldUseFile = MakeLongPath(cohorts.WsNative);
        result->find_handles[Result_Native].reset(impl::FindFirstFile(rldUseFile.c_str(), &result->cached_data[Result_Native]));
        if (result->find_handles[Result_Native])
        {
            if (result->cached_data[Result_Native].cAlternateFileName != NULL)
            {
                Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from native)    had results=\'%s\' \'%s\'", g_MfrModuleName, dllInstance, Result_Native, result->cached_data[Result_Native].cFileName, result->cached_data[Result_Native].cAlternateFileName);
            }
            else
            {
                Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from native)    had result=\'%s\'", g_MfrModuleName, dllInstance, Result_Native, result->cached_data[Result_Native].cFileName);
            }

            initialFindError = ERROR_SUCCESS;
        }
        else
        {
            if (initialFindError != ERROR_SUCCESS && GetLastError() == ERROR_FILE_NOT_FOUND)
                initialFindError = ERROR_FILE_NOT_FOUND;
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from native):   had no results", g_MfrModuleName, dllInstance, Result_Native);
        }

    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup[%d] (from native):    no results possible", g_MfrModuleName, dllInstance, Result_Native);
    }

    if (result->find_handles[Result_Redirected] ||
        result->find_handles[Result_Package] ||
        result->find_handles[Result_Native])
    {
        int UseIndex = ChooseIndexForFindResult(cohorts,  result.get());
        
        // return first result found
        copy_find_data(result->cached_data[UseIndex], *findFileData);

        // Consider the case where the name is a variablized MSIX name, such as ProgramFilesX64 and the app asked for "C:\Program Files".
        // In that case, we really want to return what they asked for, because although they already knew the name, they might use the value
        // we return instead, and bad things happen.
        if (UseIndex == Result_Redirected || UseIndex == Result_Package)
        {
            if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_native_area)
            {
                if (wfileName.find_first_of(L"*?") == std::wstring::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::wstring origNameOnly = wfileName;
                    if (origNameOnly.length() >0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == L"\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of(L"\\") != std::wstring::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of(L"\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup SWAP %s for %s", g_MfrModuleName, dllInstance, origNameOnly.c_str(), result->cached_data[UseIndex].cFileName);

                        wcscpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        wcscpy_s(findFileData->cFileName, MAX_PATH, origNameOnly.c_str());

                    }
                }
            }
            else if (cohorts.file_mfr.Request_MfrPathType == mfr::mfr_path_types::in_package_pvad_area)
            {
                // This means that the requested path is in a package area, but not in the VFS area.
                // Most likely this is the root path of the package, but possibly not.  If it is, UseIndex will be Result_Package already, so we are OK.  Might not need this code?
                if (wfileName.find_first_of(L"*?") == std::wstring::npos)
                {
                    // If the original file name had no wildcards, we might need to swap out.
                    std::wstring origNameOnly = wfileName;
                    if (origNameOnly.length() > 0 && origNameOnly.substr(origNameOnly.length() - 1, 1) == L"\\")
                    {
                        origNameOnly = origNameOnly.substr(0, origNameOnly.length() - 1);
                    }
                    if (origNameOnly.find_last_of(L"\\") != std::wstring::npos)
                    {
                        origNameOnly = origNameOnly.substr(origNameOnly.find_last_of(L"\\") + 1);
                    }
                    if (result->cached_data[UseIndex].cFileName != origNameOnly)
                    {
                        // If the name we found is not the same as the original name, then we need to swap it out.
                        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup SWAP %s for %s", g_MfrModuleName, dllInstance, origNameOnly.c_str(), result->cached_data[UseIndex].cFileName);

                        wcscpy_s(result->cached_data[UseIndex].cFileName, MAX_PATH, origNameOnly.c_str());
                        wcscpy_s(findFileData->cFileName, MAX_PATH, origNameOnly.c_str());
                    }
                }
            }

        }

        if (result->cached_data[UseIndex].cAlternateFileName != NULL)
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup returns from index=%d \'%s\' \'%s\'", g_MfrModuleName, dllInstance, UseIndex, result->cached_data[UseIndex].cFileName, result->cached_data[UseIndex].cAlternateFileName);
        }
        else
        {
            Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup return from index=%d \'%s\'", g_MfrModuleName, dllInstance, UseIndex, result->cached_data[UseIndex].cFileName);
        }
        result->wsAlready_returned_list.push_back(result->cached_data[UseIndex].cFileName);
        ::SetLastError(ERROR_SUCCESS);
        return reinterpret_cast<HANDLE>(result.release());
    }
    else
    {
        Log(LogLevel_DebugBasic, L"[%s%d] FindFirstFileWFixup returns 0x%x", g_MfrModuleName, dllInstance, initialFindError);
        ::SetLastError(initialFindError);
        return INVALID_HANDLE_VALUE;
    }

}


template <typename CharT>
HANDLE __stdcall FindFirstFileFixup(_In_ const CharT* fileName, _Out_ win32_find_data_t<CharT>* findFileData) noexcept try
{
    DWORD dllInstance = ++g_InterceptInstance;

    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        if constexpr (psf::is_ansi<CharT>)
        {
            return FindFirstFileAFixupHelper(fileName, findFileData);
        }
        else
        {
            return FindFirstFileWFixupHelper(fileName, findFileData);
        }

    }

    // If still here, call original.
    LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"\tFindFirstFileFixup: (unguarded) for fileName", fileName);
    return impl::FindFirstFile(fileName, findFileData);
}
catch (...)
{
    // NOTE: Since we allocate our own "find handle" memory, we can't just forward on to the implementation
    ::SetLastError(win32_from_caught_exception());
    Log(LogLevel_Exception, L"***FindFirstFileFixup Exception***");
    return INVALID_HANDLE_VALUE;
}
DECLARE_STRING_FIXUP(impl::FindFirstFile, FindFirstFileFixup);

