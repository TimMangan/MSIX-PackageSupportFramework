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

using namespace std::literals;

#if _DEBUG
//#define MOREDEBUG 1
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
    Log(L"PsfLauncher waiting for debugger to attach to process...\n");
    manual_wait_for_debugger();
#endif

#if MOREDEBUG
    Log("\t\tMFRFixup CONFIG: Look for config");
#endif            

    if (auto rootConfig = ::PSFQueryCurrentDllConfig())
    {
#if MOREDEBUG
        Log("\t\tMFRFixup CONFIG: Has config");
#endif            
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
#if MOREDEBUG
                        Log(L"\t\tMFR CONFIG: Has ilv-aware enabled");
#endif 
                    }
                }
                else if (ilv->type() == psf::json_type::boolean)
                {
                    auto ilvAsBoolean = ilv->as_boolean().get();
                    if (ilvAsBoolean)
                    {
                        MFRConfiguration.Ilv_Aware = true;
#if MOREDEBUG
                        Log(L"\t\tMFR CONFIG: Has ilv-aware enabled");
#endif 
                    }
                }
            }

            if (auto overrideCOWValue = rootObject.try_get("overrideCOW"))
            {
                ///o& overrideCOWObject = overrideCOWValue->as_object();
                auto CowAsWstringView = overrideCOWValue->as_string().wstring();
                std::wstring CowAsWstring = CowAsWstringView.data();
#if MOREDEBUG
                Log(L"\t\tMFR CONFIG: Has overideCOW mode=%s", CowAsWstring.c_str());
#endif 
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
                    Log(L"Bad json value ignored for overrideCOW %s", CowAsWstring.c_str());
                    MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWdefault;
                }
            }
        }
        catch (...)
        {
            Log(L"ERROR Reading config.json:  MFRTest in std options.");
        }

        try
        {
            if (auto ovValue = rootObject.try_get("overrideLocalRedirections"))
            {
#if MOREDEBUG
                Log("\t\tMFR CONFIG: Has overrideLocalRedirections");
#endif 
                const psf::json_array& ovArray = ovValue->as_array();

                for (auto& ovMemberValue : ovArray)
                {
                    auto& ovMemberObj = ovMemberValue.as_object();

                    std::wstring folderid = ovMemberObj.get("name").as_string().wstring().data();
                    std::wstring mode = ovMemberObj.get("mode").as_string().wstring().data();

#if MOREDEBUG
                    Log(L"\t\t\tProcessing FolderId: %s mode:%s", folderid.c_str(), mode.c_str());
#endif
                    int MapIndex = 0;
                   for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
                   {
                       
                       //if (std::equal(folderid.begin(), folderid.end(), map.FolderId.c_str(), psf::path_compare{}))
                       if (folderid.length() == map.VFSFolderName.length() && 
                           std::equal(folderid.begin(), folderid.end(), map.VFSFolderName.c_str(), psf::path_compare{}))
                       {
                           if (std::equal(mode.begin(), mode.end(), L"disabled", psf::path_compare{}))
                           {
#if MOREDEBUG
                               Log(L"\t\t\tDisabled: %s=%s", folderid.c_str(), map.FolderId.c_str());
#endif
                               mfr::mfr_folder_mapping newMap = mfr::CloneFolderMapping(map);
                               newMap.IsAnExclusionToRedirect = true;
                               mfr::g_MfrFolderMappings[MapIndex] = newMap;
                           }
                           else if (std::equal(mode.begin(), mode.end(), L"traditional", psf::path_compare{}))
                           {
#if MOREDEBUG
                               Log(L"\t\t\tTraditioal: %s", folderid.c_str());
#endif
                               mfr::mfr_folder_mapping newMap = mfr::CloneFolderMapping(map);
                               newMap.Valid_mapping = false;
                               mfr::g_MfrFolderMappings[MapIndex] = newMap;
                           }
                           else if (std::equal(mode.begin(), mode.end(), L"default", psf::path_compare{}))
                           {

#if MOREDEBUG
                               Log(L"\t\t\tDefault: %s", folderid.c_str());
#endif
                               // Do nothing
                           }
                           else
                           {
                               Log(L"Bad json value ignored for overrideLocalRedirections %s %s", folderid.c_str(), mode.c_str());
                           }
                       }
                       MapIndex++;
                   }

                }
            }
        }
        catch (...)
        {
            Log(L"ERROR Reading config.json:  MFRTest in overrideLocalRedirections.");
        }

        try
        {
            if (auto ovValue = rootObject.try_get("overrideTraditionalRedirections"))
            {
#if MOREDEBUG
                Log("\t\tMFR CONFIG: Has overrideTraditionalRedirections");
#endif 
                const psf::json_array& ovArray = ovValue->as_array();

                for (auto& ovMemberValue : ovArray)
                {
                    auto& ovMemberObj = ovMemberValue.as_object();

                    std::wstring folderid = ovMemberObj.get("name").as_string().wstring().data();
                    std::wstring mode = ovMemberObj.get("mode").as_string().wstring().data();

#if MOREDEBUG
                    Log(L"\t\t\tProcessing FolderId: %s mode:%s", folderid.c_str(), mode.c_str());
#endif

                    int MapIndex = 0;
                    for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
                    {
                        //if (std::equal(folderid.begin(), folderid.end(), map.FolderId.c_str(), psf::path_compare{}))
                        if (folderid.length() == map.VFSFolderName.length() && 
                            std::equal(folderid.begin(), folderid.end(), map.VFSFolderName.c_str(), psf::path_compare{}))
                        {
                            if (std::equal(mode.begin(), mode.end(), L"disabled", psf::path_compare{}))
                            {
#if MOREDEBUG
                                Log(L"\t\t\tDisabled: %s=%s", folderid.c_str(), map.FolderId.c_str());
#endif
                                mfr::mfr_folder_mapping newMap = mfr::CloneFolderMapping(map);
                                newMap.IsAnExclusionToRedirect = true;
                                mfr::g_MfrFolderMappings[MapIndex] = newMap;
                            }
                            else if (std::equal(mode.begin(), mode.end(), L"default", psf::path_compare{}))
                            {
#if MOREDEBUG
                                Log(L"\t\t\tDefault: %s", folderid.c_str());
#endif
                                // Do nothing
                            }
                            else
                            {
                                Log(L"Bad json value ignored for overrideLocalTraditionalredirections %s %s", folderid.c_str(), mode.c_str());
                            }
                        }
                        MapIndex++;
                    }
                }
            }
        }

        catch (...)
        {
            Log(L"ERROR Reading config.json:  MFRTest in overrideTraditionalRedirections.");
        }

#if EVENMOREDEBUG
        Log(L"============== Dump ====================");
        for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
        {
            Log(L"-----");
            Log(L"Valid_Mapping=%d IsAnExclusionToRedirect=%d NativePathBase=%s", map.Valid_mapping, map.IsAnExclusionToRedirect, map.NativePathBase.c_str());
            Log(L"FolderId=%s", map.FolderId.c_str());
            Log(L"VFSFolderName=%s", map.VFSFolderName.c_str());
            Log(L"PackagePathBase=%s", map.PackagePathBase.c_str());
            Log(L"RedirectedPathBase=%s", map.RedirectedPathBase.c_str());
            Log(L"DoesRuntimeMapNativeToVFS=%d", map.DoesRuntimeMapNativeToVFS);
            Log(L"RedirectionFlags=%s", RedirectFlagsName(map.RedirectionFlags));
        }
        Log(L"============== Dump ====================");
#endif

        TraceLoggingWrite(
            g_Log_ETW_ComponentProvider,
            "MFRFixupConfigdata",
            TraceLoggingWideString(traceDataStream.str().c_str(), "MFRFixupConfig"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
    }

    TraceLoggingUnregister(g_Log_ETW_ComponentProvider);

}
