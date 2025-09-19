//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <vector>
#include <known_folders.h>
#include <objbase.h>
#include <psf_framework.h>

#include <utilities.h>
#include <psf_logging.h>

#include <TraceLoggingProvider.h>
#include "Telemetry.h"

#include "ManagedFileMappings.h"
#include "MFRConfiguration.h"
#include "FunctionImplementations.h"

using namespace std::literals;

#if _DEBUG
#define MOREDEBUG 1
//#define EVENMOREDEBUG 1
#endif

TRACELOGGING_DECLARE_PROVIDER(g_Log_ETW_ComponentProvider);
TRACELOGGING_DEFINE_PROVIDER(
    g_Log_ETW_ComponentProvider,
    "Microsoft.Windows.PSFRuntime",
    (0xf7f4e8c4, 0x9981, 0x5221, 0xe6, 0xfb, 0xff, 0x9d, 0xd1, 0xcd, 0xa4, 0xe1),
    TraceLoggingOptionMicrosoftTelemetry());

mfr::mfr_configuration MFRConfiguration;

void InitializeConfiguration()
{
    TraceLoggingRegister(g_Log_ETW_ComponentProvider);
    std::wstringstream traceDataStream;

#if _ManualDebug
    Log(LogLevel_Exception, L"PsfLauncher waiting for debugger to attach to process...\n");
    manual_wait_for_debugger();
#endif
    
    Log(LogLevel_DebugIntermediate, "[%s%d]\t\tMFRFixup CONFIG: Look for config", g_MfrModuleName, 0);

    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
        Log(LogLevel_DebugIntermediate, "[%s%d]\t\t\tMFRFixup CONFIG: Has config", g_MfrModuleName, 0);
        auto& rootObject = rootConfig->as_object();
        traceDataStream << " config:\n";
        try
        {
            if (auto ilv = rootObject.try_get("ilvAware"))
            {
                if (ilv->type() == psf::json_type::string)
                {
                    auto ilvAsWStringView = ilv->as_string().wstring();
                    std::wstring ilvAsWstring = ilvAsWStringView.data();
                    if (ilvAsWstring.compare(L"true") == 0)
                    {
                        MFRConfiguration.Ilv_Aware = true;
                        Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\tMFR CONFIG: Has ilv-aware enabled", g_MfrModuleName, 0);
                    }
                }
                else if (ilv->type() == psf::json_type::boolean)
                {
                    auto ilvAsBoolean = ilv->as_boolean().get();
                    if (ilvAsBoolean)
                    {
                        MFRConfiguration.Ilv_Aware = true;
                        Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\tMFR CONFIG: Has ilv-aware enabled", g_MfrModuleName, 0);
                    }
                }
            }

            if (auto overrideCOWValue = rootObject.try_get("overrideCOW"))
            {
               std::wstring CowAsWstring = overrideCOWValue->as_string().wstring().data(); //CowAsWstringView.data();
                Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\tMFR CONFIG: Has overideCOW mode=%s", g_MfrModuleName, 0, CowAsWstring.c_str());
                if (CowAsWstring.compare(L"enablePe") == 0)
                {
                    MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWenablePe;
                }
                else if (CowAsWstring.compare(L"disableAll") == 0)
                {
                    MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWdisableAll;
                }
                else  if (CowAsWstring.compare(L"default") == 0)
                {
                    MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWdefault;
                }
                else
                {
                    Log(LogLevel_DebugBasic, L"[%s%d] Bad json value ignored for overrideCOW %s", g_MfrModuleName, 0, CowAsWstring.c_str());
                    MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWdefault;
                }
            }
        }
        catch (...)
        {
            Log(LogLevel_Exception, L"[%s%d] ERROR Reading config.json:  MFRTest in std options.", g_MfrModuleName, 0);
        }

        try
        {
            if (auto ovValue = rootObject.try_get("overrideLocalRedirections"))
            {
                Log(LogLevel_DebugIntermediate, "[%s%d]\t\t\tMFR CONFIG: Has overrideLocalRedirections", g_MfrModuleName, 0);
                const psf::json_array& ovArray = ovValue->as_array();

                for (auto& ovMemberValue : ovArray)
                {
                   auto& ovMemberObj = ovMemberValue.as_object();

                   std::wstring folderid = ovMemberObj.get("name").as_string().wstring().data();
                   std::wstring mode = ovMemberObj.get("mode").as_string().wstring().data();

                   Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tProcessing FolderId: %s mode:%s", g_MfrModuleName, 0, folderid.c_str(), mode.c_str());
                    
                   int MapIndex = 0;
                   for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
                   {
                       
                       //if (std::equal(folderid.begin(), folderid.end(), map.FolderId.c_str(), psf::path_compare{}))
                       if (folderid.length() == map.VFSFolderName.length() && 
                           std::equal(folderid.begin(), folderid.end(), map.VFSFolderName.c_str(), psf::path_compare{}))
                       {
                           if (std::equal(mode.begin(), mode.end(), L"disabled", psf::path_compare{}))
                           {
                               Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tDisabled: index=%d %s=%s", g_MfrModuleName, 0, MapIndex, folderid.c_str(), map.FolderId.c_str());
                               mfr::mfr_folder_mapping newMap = mfr::CloneFolderMapping(map);
                               newMap.IsAnExclusionToRedirect = mfr::mfr_exclusion_types::excluded;
                               mfr::g_MfrFolderMappings[MapIndex] = newMap;
                           }
                           else if (std::equal(mode.begin(), mode.end(), L"traditional", psf::path_compare{}))
                           {
                               Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tTraditional: index=%d %s", g_MfrModuleName, 0, MapIndex, folderid.c_str());
                               mfr::mfr_folder_mapping newMap = mfr::CloneFolderMapping(map);
                               //map.Valid_mapping = false;
                               newMap.RedirectionFlags = mfr::mfr_redirect_flags::prefer_redirection_if_package_vfs;
                               mfr::g_MfrFolderMappings[MapIndex] = newMap;
                           }
                           else if (std::equal(mode.begin(), mode.end(), L"default", psf::path_compare{}))
                           {

                               Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tDefault: index=%d %s", g_MfrModuleName, 0, MapIndex, folderid.c_str());
                               // Do nothing
                           }
                           else
                           {
                               Log(LogLevel_DebugBasic, L"[%s%d]Bad json value ignored for overrideLocalRedirections %s %s", g_MfrModuleName, 0, folderid.c_str(), mode.c_str());
                           }
                       }
                       MapIndex++;
                   }

                }
            }
        }
        catch (...)
        {
            Log(LogLevel_Exception, L"[%s%d] ERROR Reading config.json:  MFRTest in overrideLocalRedirections.", g_MfrModuleName, 0);
        }

        try
        {
            if (auto ovValue = rootObject.try_get("overrideTraditionalRedirections"))
            {
                Log(LogLevel_DebugIntermediate, "[%s%d]\t\t\tMFR CONFIG: Has overrideTraditionalRedirections", g_MfrModuleName, 0);
                const psf::json_array& ovArray = ovValue->as_array();

                for (auto& ovMemberValue : ovArray)
                {
                    auto& ovMemberObj = ovMemberValue.as_object();

                    std::wstring folderid = ovMemberObj.get("name").as_string().wstring().data();
                    std::wstring mode = ovMemberObj.get("mode").as_string().wstring().data();

                    Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tProcessing FolderId: %s mode:%s", g_MfrModuleName, 0, folderid.c_str(), mode.c_str());

                    int MapIndex = 0;
                    for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
                    {
                        //if (std::equal(folderid.begin(), folderid.end(), map.FolderId.c_str(), psf::path_compare{}))
                        if (folderid.length() == map.VFSFolderName.length() && 
                            std::equal(folderid.begin(), folderid.end(), map.VFSFolderName.c_str(), psf::path_compare{}))
                        {
                            if (std::equal(mode.begin(), mode.end(), L"disabled", psf::path_compare{}))
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tDisabled: index=%d %s=%s", g_MfrModuleName, 0, MapIndex, folderid.c_str(), map.FolderId.c_str());
                                mfr::mfr_folder_mapping newMap = mfr::CloneFolderMapping(map);
                                newMap.IsAnExclusionToRedirect = mfr::mfr_exclusion_types::excluded;
                                mfr::g_MfrFolderMappings[MapIndex] = newMap;
                            }
                            else if (std::equal(mode.begin(), mode.end(), L"local", psf::path_compare{}))
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tLocal: index=%d %s=%s", g_MfrModuleName, 0, MapIndex, folderid.c_str(), map.FolderId.c_str());
                                mfr::mfr_folder_mapping newMap = mfr::CloneFolderMapping(map);
                                newMap.RedirectionFlags = mfr::mfr_redirect_flags::prefer_redirection_local;
                                newMap.RedirectedPathBase = map.NativePathBase; // This is now the local redirection area.
                                mfr::g_MfrFolderMappings[MapIndex] = newMap;
                            }
                            else if (std::equal(mode.begin(), mode.end(), L"default", psf::path_compare{}))
                            {
                                Log(LogLevel_DebugIntermediate, L"[%s%d]\t\t\t\tDefault: index=%d %s", g_MfrModuleName, 0, MapIndex, folderid.c_str());
                                // Do nothing
                            }
                            else
                            {
                                Log(LogLevel_DebugBasic, L"[%s%d] Bad json value ignored for overrideTraditionalRedirections %s %s", g_MfrModuleName, 0, folderid.c_str(), mode.c_str());
                            }
                        }
                        MapIndex++;
                    }
                }
            }
        }

        catch (...)
        {
            Log(LogLevel_Exception, L"[%s%d] ERROR Reading config.json:  MFRTest in overrideTraditionalRedirections.", g_MfrModuleName, 0);
        }

        if (LogLevel_DebugMaximum <= g_JsonDebugLevel)
        {
            Log(LogLevel_DebugMaximum, L"[%s%d] ============== Dump ====================", g_MfrModuleName, 0);
            for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
            {
                Log(LogLevel_DebugMaximum, L"[%s%d] -----", g_MfrModuleName, 0);
                Log(LogLevel_DebugMaximum, L"[%s%d] Valid_Mapping=%d IsAnExclusionToRedirect=%d NativePathBase=%s", g_MfrModuleName, 0, map.Valid_mapping, map.IsAnExclusionToRedirect, map.NativePathBase.c_str());
                Log(LogLevel_DebugMaximum, L"[%s%d] FolderId=%s", g_MfrModuleName, 0, map.FolderId.c_str());
                Log(LogLevel_DebugMaximum, L"[%s%d] VFSFolderName=%s", g_MfrModuleName, 0, map.VFSFolderName.c_str());
                Log(LogLevel_DebugMaximum, L"[%s%d] PackagePathBase=%s", g_MfrModuleName, 0, map.PackagePathBase.c_str());
                Log(LogLevel_DebugMaximum, L"[%s%d] RedirectedPathBase=%s", g_MfrModuleName, 0, map.RedirectedPathBase.c_str());
                Log(LogLevel_DebugMaximum, L"[%s%d] DoesRuntimeMapNativeToVFS=%d", g_MfrModuleName, 0, map.DoesRuntimeMapNativeToVFS);
                Log(LogLevel_DebugMaximum, L"[%s%d] RedirectionFlags=%s", g_MfrModuleName, 0, RedirectFlagsName(map.RedirectionFlags));
            }
            Log(LogLevel_DebugMaximum, L"[%s%d] ============== Dump ====================", g_MfrModuleName, 0);
        }

        TraceLoggingWrite(
            g_Log_ETW_ComponentProvider,
            "MFRFixupConfigdata",
            TraceLoggingWideString(traceDataStream.str().c_str(), "MFRFixupConfig"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

    }

    TraceLoggingUnregister(g_Log_ETW_ComponentProvider);

    Log(LogLevel_DebugBasic, "[%s%d]\t\tMFRFixup CONFIG: Done processing.", g_MfrModuleName, 0);
            

}
