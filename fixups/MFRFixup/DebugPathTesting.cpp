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

void DebugPathTesting(DWORD DllInstance)
{
    if (DebugPathTestingList.size() == 0)
    {
        //Log(L"[%d]  DEBUGPATHTESTING: initialize test list.", DllInstance);
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

        //Log(L"[%d]  DEBUGPATHTESTING: define g_packageRootPath=%s", DllInstance, g_packageRootPath.c_str());
        DebugPathTestingList.push_back(g_packageRootPath.c_str());

        auto testPvdFileWS = widen(g_packageRootPath.c_str());
        testPvdFileWS.append(L"\\packagepvad.txt");
        //Log(L"[%d]  DEBUGPATHTESTING: define testPvdFileWS=%s", DllInstance, testPvdFileWS.c_str());
        DebugPathTestingList.push_back(testPvdFileWS.c_str());

        std::filesystem::path testPathVfs = g_packageVfsRootPath;
        testPathVfs /= L"ProgramFilesX64";
        testPathVfs /= L"packagedvfs.txt";
        //Log(L"[%d]  DEBUGPATHTESTING: define testPathVfs=%s", DllInstance, testPathVfs.c_str());
        DebugPathTestingList.push_back(testPathVfs.c_str());

        std::filesystem::path testPathRedir = g_writablePackageRootPath;
        testPathRedir /= L"redirectedpad.txt";
        //Log(L"[%d]  DEBUGPATHTESTING: define testPathRedir=%s", DllInstance, testPathRedir.c_str());
        DebugPathTestingList.push_back(testPathRedir.c_str());

        std::filesystem::path testPathRedirVfs = g_writablePackageRootPath;
        testPathRedirVfs /= L"VFS\\ProgramFilesX64\\packagedvfs.txt";
        //Log(L"[%d]  DEBUGPATHTESTING: define testPathRedirVfs=%s", DllInstance, testPathRedir.c_str());
        DebugPathTestingList.push_back(testPathRedirVfs.c_str());


        Log(L"[%d]  DEBUGPATHTESTING: test list initialized for %d tests.", DllInstance, DebugPathTestingList.size());
        std::filesystem::path cwd = std::filesystem::current_path();
        //Log(L"[%d]  DEBUGPATHTESTING: cwd=%s", DllInstance, cwd.c_str());
        Log(L" ");
    }
    for (std::wstring testInput : DebugPathTestingList)
    {
        try
        {
            //Log(L"[%d]  DEBUGPATHTESTING: index at %d", DllInstance, index++);
            //LogString(DllInstance, L"DEBUGPATHTESTING: testInput", testInput.c_str());
            mfr::mfr_path test_mfr = mfr::create_mfr_path(testInput);
            Log(L"[%d]  DEBUGPATHTESTING: request=%s", DllInstance, test_mfr.Request_OriginalPath.c_str());
            Log(L"[%d]  DEBUGPATHTESTING: dos_type=%s", DllInstance, psf::DosPathTypeName(test_mfr.Request_DosPathType));
            Log(L"[%d]  DEBUGPATHTESTING: mfr_type=%s", DllInstance, MfrPathTypeName(test_mfr.Request_MfrPathType));
            Log(L"[%d]  DEBUGPATHTESTING: normalized=%s", DllInstance, test_mfr.Request_NormalizedPath.c_str());
            mfr::mfr_folder_mapping map;
            std::wstring resultWS;
            switch (test_mfr.Request_MfrPathType)
            {
            case mfr::mfr_path_types::in_native_area:
                map = mfr::Find_LocalRedirMapping_FromNativePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    Log(L"[%d]  DEBUGPATHTESTING:   map=LocalRedirection", DllInstance);
                    Log(L"[%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", DllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(L"[%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", DllInstance, map.NativePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", DllInstance, map.PackagePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", DllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                    Log(L"[%d]  DEBUGPATHTESTING:       map        PackagePath=%s", DllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                    Log(L"[%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", DllInstance, resultWS.c_str());
                }
                else
                {
                    map = mfr::Find_TraditionalRedirMapping_FromNativePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        Log(L"[%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", DllInstance);
                        Log(L"[%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", DllInstance, map.DoesRuntimeMapNativeToVFS);
                        Log(L"[%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", DllInstance, map.NativePathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", DllInstance, map.PackagePathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", DllInstance, map.RedirectedPathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", DllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.PackagePathBase);
                        Log(L"[%d]  DEBUGPATHTESTING:       map        PackagePath=%s", DllInstance, resultWS.c_str());
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.NativePathBase, map.RedirectedPathBase);
                        Log(L"[%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", DllInstance, resultWS.c_str());
                    }
                    else
                    {
                        Log(L"[%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", DllInstance);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_pvad_area:
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    Log(L"[%d]  DEBUGPATHTESTING:   map=LocalRedirection", DllInstance);
                    Log(L"[%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", DllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(L"[%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", DllInstance, map.NativePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", DllInstance, map.PackagePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", DllInstance, map.RedirectedPathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", DllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    if (map.DoesRuntimeMapNativeToVFS)
                    {
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    }
                    else
                    {
                        resultWS = test_mfr.Request_NormalizedPath.c_str();
                    }
                    Log(L"[%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", DllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    Log(L"[%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", DllInstance, resultWS.c_str());
                }
                else
                {
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        Log(L"[%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", DllInstance);
                        Log(L"[%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", DllInstance, map.DoesRuntimeMapNativeToVFS);
                        Log(L"[%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", DllInstance, map.NativePathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", DllInstance, map.PackagePathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", DllInstance, map.RedirectedPathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map   RedirectionFlags=%s", DllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                        Log(L"[%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", DllInstance, resultWS.c_str());
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        Log(L"[%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", DllInstance, resultWS.c_str());
                    }
                    else
                    {
                        Log(L"[%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", DllInstance);
                    }
                }
                break;
            case mfr::mfr_path_types::in_package_vfs_area:
                map = mfr::Find_LocalRedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    Log(L"[%d]  DEBUGPATHTESTING:   map=LocalRedirection", DllInstance);
                    Log(L"[%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", DllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(L"[%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", DllInstance, map.NativePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", DllInstance, map.PackagePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", DllInstance, map.RedirectedPathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:       map RedirectionFlags=%s", DllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                    Log(L"[%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", DllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                    Log(L"[%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", DllInstance, resultWS.c_str());
                }
                else
                {
                    map = mfr::Find_TraditionalRedirMapping_FromPackagePath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str());
                    if (map.Valid_mapping)
                    {
                        Log(L"[%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", DllInstance);
                        Log(L"[%d]  DEBUGPATHTESTING:       map DoesRuntimeMapNativeToVFS=%d", DllInstance, map.DoesRuntimeMapNativeToVFS);
                        Log(L"[%d]  DEBUGPATHTESTING:       map     NativePathBase=%s", DllInstance, map.NativePathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map    PackagePathBase=%s", DllInstance, map.PackagePathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map RedirectedPathBase=%s", DllInstance, map.RedirectedPathBase.c_str());
                        Log(L"[%d]  DEBUGPATHTESTING:       map RedirectionFlags=%s", DllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.NativePathBase);
                        Log(L"[%d]  DEBUGPATHTESTING:       map          DeVFSPath=%s", DllInstance, resultWS.c_str());
                        resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.PackagePathBase, map.RedirectedPathBase);
                        Log(L"[%d]  DEBUGPATHTESTING:       map     RedirectedPath=%s", DllInstance, resultWS.c_str());
                    }
                    else
                    {
                        Log(L"[%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", DllInstance);
                    }
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_writablepackageroot:
                map = mfr::Find_TraditionalRedirMapping_FromRedirectedPath_ForwardSearch(test_mfr.Request_NormalizedPath.c_str());
                if (map.Valid_mapping)
                {
                    Log(L"[%d]  DEBUGPATHTESTING:   map=TraditionalRedirection", DllInstance);
                    Log(L"[%d]  DEBUGPATHTESTING:   map DoesRuntimeMapNativeToVFS=%d", DllInstance, map.DoesRuntimeMapNativeToVFS);
                    Log(L"[%d]  DEBUGPATHTESTING:   map     NativePathBase=%s", DllInstance, map.NativePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:   map    PackagePathBase=%s", DllInstance, map.PackagePathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:   map RedirectedPathBase=%s", DllInstance, map.RedirectedPathBase.c_str());
                    Log(L"[%d]  DEBUGPATHTESTING:   map RedirectionFlags=%s", DllInstance, mfr::RedirectFlagsName(map.RedirectionFlags));
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.PackagePathBase);
                    Log(L"[%d]  DEBUGPATHTESTING:   map          DeVFSPath=%s", DllInstance, resultWS.c_str());
                    resultWS = ReplacePathPart(test_mfr.Request_NormalizedPath.c_str(), map.RedirectedPathBase, map.NativePathBase);
                    Log(L"[%d]  DEBUGPATHTESTING:   map        DeRedirPath=%s", DllInstance, resultWS.c_str());
                }
                else
                {
                    Log(L"[%d] DEBUGPATHTESTING:    ERROR NO MAPPING.", DllInstance);
                }
                break;
            case mfr::mfr_path_types::in_redirection_area_other:
                Log("[%d]  DEBUGPATHTESTING:   in microsoft-runtime redirection area; unspupported for redirection by us.", DllInstance);
                break;
            case mfr::mfr_path_types::in_other_drive_area:
                Log("[%d]  DEBUGPATHTESTING:   on a different drive letter; unspupported for redirection.", DllInstance);
                break;
            case mfr::mfr_path_types::is_protocol_path:
                Log("[%d]  DEBUGPATHTESTING:   protocol path; unspupported for redirection.", DllInstance);
                break;
            case mfr::mfr_path_types::is_UNC_path:
                Log("[%d]  DEBUGPATHTESTING:   unc path; unspupported for redirection.", DllInstance);
                break;
            case mfr::mfr_path_types::unsupported_for_intercepts:
                Log("[%d]  DEBUGPATHTESTING:   unspupported for redirection.", DllInstance);
                break;
            default:
                break;
            }
            Log(L" ");
        }
        catch (...)
        {
            Log(L"[%d]  DEBUGPATHTESTING: Exception 0x%x.", DllInstance, GetLastError());
        }
    }
}
#endif