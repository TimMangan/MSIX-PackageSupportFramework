//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>
#include "DebugPathTesting.h"

#if _DEBUG
#include "ManagedPathTypes.h"
#include "PathUtilities.h"
std::vector<std::wstring> DebugPathTestingList;

void DebugPathTesting(DWORD dllInstance)
{
    if (DebugPathTestingList.size() == 0)
    {
        //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: initialize test list.", g_MfrModuleName, dllInstance);
        DebugPathTestingList.push_back(L"C:\\Windows\\System32\\foo.xxx");
        DebugPathTestingList.push_back(L"C:\\Nonesuch\\OrSomething.yxy");
        DebugPathTestingList.push_back(L"C:Relative\\nonesuch.xxx");
        DebugPathTestingList.push_back(L"Relative\\nonesuch.xxx");
        DebugPathTestingList.push_back(L"\\\\?\\C:\\Windows\\System32\\foo.xxx");
        DebugPathTestingList.push_back(L"D:\\Nonesuch\\OrSomething.yxy");
        DebugPathTestingList.push_back(L"\\\\Server\\Share\\Nonesuch\\OrSomething.yxy");
        DebugPathTestingList.push_back(L"file:\\\\something");
        DebugPathTestingList.push_back(L"\\\\.\\namedpipelikething");
        DebugPathTestingList.push_back(L"CONOUT$");
        DebugPathTestingList.push_back(L"COM4:");

        DebugPathTestingList.push_back(L"\\\\?\\C:\\Windows\\System32\\Drivers\\..\\foo.xxx");
        DebugPathTestingList.push_back(L"C:\\Nonesuch\\Orthis\\..\\..\\OrSomething.yxy");

        //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: define g_packageRootPath=%s", g_MfrModuleName, dllInstance, g_packageRootPath.c_str());
        DebugPathTestingList.push_back(g_packageRootPath.c_str());

        auto testPvdFileWS = widen(g_packageRootPath.c_str());
        testPvdFileWS.append(L"\\packagepvad.txt");
        //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: define testPvdFileWS=%s", g_MfrModuleName, dllInstance, testPvdFileWS.c_str());
        DebugPathTestingList.push_back(testPvdFileWS.c_str());

        std::filesystem::path testPathVfs = g_packageVfsRootPath;
        testPathVfs /= L"ProgramFilesX64";
        testPathVfs /= L"packagedvfs.txt";
        //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: define testPathVfs=%s", g_MfrModuleName, dllInstance, testPathVfs.c_str());
        DebugPathTestingList.push_back(testPathVfs.c_str());

        std::filesystem::path testPathRedir = g_writablePackageRootPath;
        testPathRedir /= L"redirectedpad.txt";
        //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: define testPathRedir=%s", g_MfrModuleName, , testPathRedir.c_str());
        DebugPathTestingList.push_back(testPathRedir.c_str());

        std::filesystem::path testPathRedirVfs = g_writablePackageRootPath;
        testPathRedirVfs /= L"VFS\\ProgramFilesX64\\packagedvfs.txt";
        //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: define testPathRedirVfs=%s", g_MfrModuleName, dllInstance, testPathRedir.c_str());
        DebugPathTestingList.push_back(testPathRedirVfs.c_str());


        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: test list initialized for %d tests.", g_MfrModuleName, dllInstance, DebugPathTestingList.size());
        std::filesystem::path cwd = std::filesystem::current_path();
        //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: cwd=%s", g_MfrModuleName, dllInstance, cwd.c_str());
        Log(LogLevel_DebugBasic, L" ");
    }
    for (std::wstring testInput : DebugPathTestingList)
    {
        try
        {
            //Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: index at %d", g_MfrModuleName, dllInstance, index++);
            //LogString(LogLevel_DebugBasic, g_MfrModuleName, dllInstance, L"DEBUGPATHTESTING: testInput", testInput.c_str());
            mfr::mfr_path test_mfr = mfr::create_mfr_path(testInput);
            Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: request=%s", g_MfrModuleName, dllInstance, test_mfr.Request_OriginalPath.c_str());
            Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: dos_type=%s", g_MfrModuleName, dllInstance, psf::DosPathTypeName(test_mfr.Request_DosPathType));
            Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: mfr_type=%s", g_MfrModuleName, dllInstance, MfrPathTypeName(test_mfr.Request_MfrPathType));
            Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: normalized=%s", g_MfrModuleName, dllInstance, test_mfr.Request_NormalizedPath.c_str());
            mfr::mfr_folder_mapping map;
            std::wstring resultWS;
            switch (test_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                map = mfr::Find_RedirMapping_FromNativePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str(),dllInstance );
                if (map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map=LocalRedirection", g_MfrModuleName, dllInstance);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, dllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", g_MfrModuleName, dllInstance, map.NativePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", g_MfrModuleName, dllInstance, map.PackagePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", g_MfrModuleName, dllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map        PackagePath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                }
                else
                {
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str(), dllInstance);
                    if (map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", g_MfrModuleName, dllInstance);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, dllInstance, map.DoesRuntimeMapNativeToVFS);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", g_MfrModuleName, dllInstance, map.NativePathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", g_MfrModuleName, dllInstance, map.PackagePathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", g_MfrModuleName, dllInstance, map.RedirectedPathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", g_MfrModuleName, dllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map        PackagePath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                    }
                    else
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", g_MfrModuleName, dllInstance);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                map = mfr::Find_RedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str(), dllInstance);
                if (map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map=LocalRedirection", g_MfrModuleName, dllInstance);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, dllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", g_MfrModuleName, dllInstance, map.NativePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", g_MfrModuleName, dllInstance, map.PackagePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", g_MfrModuleName, dllInstance, map.RedirectedPathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", g_MfrModuleName, dllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    if (map.DoesRuntimeMapNativeToVFS)
                    {
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    }
                    else
                    {
                        resultWS = test_mfr.Request_NormalizedPath.c_str();
                    }
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                }
                else
                {
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str(), dllInstance);
                    if (map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", g_MfrModuleName, dllInstance);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, dllInstance, map.DoesRuntimeMapNativeToVFS);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", g_MfrModuleName, dllInstance, map.NativePathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", g_MfrModuleName, dllInstance, map.PackagePathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", g_MfrModuleName, dllInstance, map.RedirectedPathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", g_MfrModuleName, dllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                    }
                    else
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", g_MfrModuleName, dllInstance);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                map = mfr::Find_RedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str(), dllInstance);
                if (map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map=LocalRedirection", g_MfrModuleName, dllInstance);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, dllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", g_MfrModuleName, dllInstance, map.NativePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", g_MfrModuleName, dllInstance, map.PackagePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", g_MfrModuleName, dllInstance, map.RedirectedPathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map RedirectionFlags=%s", g_MfrModuleName, dllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                }
                else
                {
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str(), dllInstance);
                    if (map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", g_MfrModuleName, dllInstance);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, dllInstance, map.DoesRuntimeMapNativeToVFS);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", g_MfrModuleName, dllInstance, map.NativePathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", g_MfrModuleName, dllInstance, map.PackagePathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", g_MfrModuleName, dllInstance, map.RedirectedPathBase.c_str());
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map RedirectionFlags=%s", g_MfrModuleName, dllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                    }
                    else
                    {
                        Log(LogLevel_DebugBasic, L"[%s%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", g_MfrModuleName, dllInstance);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str(), dllInstance);
                if (map.Valid_mapping == mfr::mfr_enabled_types::enabled)
                {
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", g_MfrModuleName, dllInstance);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, dllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map     NativePathBase=%s", g_MfrModuleName, dllInstance, map.NativePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map    PackagePathBase=%s", g_MfrModuleName, dllInstance, map.PackagePathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map RedirectedPathBase=%s", g_MfrModuleName, dllInstance, map.RedirectedPathBase.c_str());
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map RedirectionFlags=%s", g_MfrModuleName, dllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map          DeVFSPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.NativePathBase);
                    Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING:   map        DeRedirPath=%s", g_MfrModuleName, dllInstance, resultWS.c_str());
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", g_MfrModuleName, dllInstance);
                }
                break;
            case mfr::mfr_path_types::is_Protocol:
                Log(LogLevel_DebugBasic, "[%s%d]  DEBUGPATHTESTING:   protocol; unspupported for redirection.", g_MfrModuleName, dllInstance);
                break;
            case mfr::mfr_path_types::is_DosSpecial:
                Log(LogLevel_DebugBasic, "[%s%d]  DEBUGPATHTESTING:   DOS Special path; unspupported for redirection.", g_MfrModuleName, dllInstance);
                break;
            case mfr::mfr_path_types::is_Shell:
                Log(LogLevel_DebugBasic, "[%s%d]  DEBUGPATHTESTING:   shell; unspupported for redirection.", g_MfrModuleName, dllInstance);
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                Log(LogLevel_DebugBasic, "[%s%d]  DEBUGPATHTESTING:   in microsoft-runtime redirection area; unspupported for redirection by us.", g_MfrModuleName, dllInstance);
                break;
            case mfr::mfr_path_types::in_other_drive_area:
                Log(LogLevel_DebugBasic, "[%s%d]  DEBUGPATHTESTING:   on a different drive letter; unspupported for redirection.", g_MfrModuleName, dllInstance);
                break;
            case mfr::mfr_path_types::is_UNC_path:
                Log(LogLevel_DebugBasic, "[%s%d]  DEBUGPATHTESTING:   unc path; unspupported for redirection.", g_MfrModuleName, dllInstance);
                break;
            case mfr::mfr_path_types::unsupported_for_intercepts:
                Log(LogLevel_DebugBasic, "[%s%d]  DEBUGPATHTESTING:   unspupported for redirection.", g_MfrModuleName, dllInstance);
                break;
            default:
                break;
            }
            Log(LogLevel_DebugBasic, L" ");
        }
        catch (...)
        {
            Log(LogLevel_DebugBasic, L"[%s%d]  DEBUGPATHTESTING: Exception 0x%x.", g_MfrModuleName, dllInstance, GetLastError());
        }
    }
}
#endif