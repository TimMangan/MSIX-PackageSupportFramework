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

#define MOREDEBUG 1

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
            if (auto overrideCOWValue = rootObject.try_get("overrideCOW"))
            {
                ///o& overrideCOWObject = overrideCOWValue->as_object();
                auto CowAsWstringView = overrideCOWValue->as_string().wstring();
                std::wstring CowAsWstring = CowAsWstringView.data();
#if MOREDEBUG
                Log(L"\t\tMFR CONFIG: Has overideCOW %s", CowAsWstring.c_str());
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
            Log(L"ERROR Reading config.json:  MFRTest in overrideLocalRedirections.");
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
                    std::wstring mode = ovMemberObj.get("name").as_string().wstring().data();

                    
                   for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
                   {
                       if (std::equal(folderid.begin(), folderid.end(), map.FolderId.c_str(), psf::path_compare{}))
                       {
                           if (std::equal(mode.begin(), mode.end(), L"disabled", psf::path_compare{}))
                           {
                               map.Valid_mapping = false;
                           }
                           else if (std::equal(mode.begin(), mode.end(), L"traditional", psf::path_compare{}))
                           {
                               map.Valid_mapping = false;
                           }
                           else if (std::equal(mode.begin(), mode.end(), L"default", psf::path_compare{}))
                           {
                               // Do nothing
                           }
                           else
                           {
                               Log(L"Bad json value ignored for overrideLocalredirections %s %s", folderid.c_str(), mode.c_str());
                           }
                           break;
                       }
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
                    std::wstring mode = ovMemberObj.get("name").as_string().wstring().data();


                    for (mfr::mfr_folder_mapping map : mfr::g_MfrFolderMappings)
                    {
                        if (std::equal(folderid.begin(), folderid.end(), map.FolderId.c_str(), psf::path_compare{}))
                        {
                            if (std::equal(mode.begin(), mode.end(), L"disabled", psf::path_compare{}))
                            {
                                map.Valid_mapping = false;
                            }
                            else if (std::equal(mode.begin(), mode.end(), L"default", psf::path_compare{}))
                            {
                                // Do nothing
                            }
                            else
                            {
                                Log(L"Bad json value ignored for overrideLocalTraditionalredirections %s %s", folderid.c_str(), mode.c_str());
                            }
                            break;
                        }
                    }
                }
            }
        }

        catch (...)
        {
            Log(L"ERROR Reading config.json:  MFRTest in overrideTraditionalRedirections.");
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

}
