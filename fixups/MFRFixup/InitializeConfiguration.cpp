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
        [[maybe_unused]] auto& rootObject = rootConfig->as_object();
        traceDataStream << " config:\n";

        if (auto pathsValue = rootObject.try_get("overideCOW"))
        {
            auto& overideCOWObject = pathsValue->as_object();
            auto overideCOWValue = overideCOWObject.as_string().wstring();
#if MOREDEBUG
            Log("\t\tMFR CONFIG: Has overideCOW %s", overrideCOWValue);
#endif 
            if (overideCOWValue.compare(L"disablePe") == 0)
            {
                MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWdisablePe;
            }
            else if (overideCOWValue.compare(L"disableAll") == 0)
            {
                MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWdisableAll;
            }
            else
            {
                MFRConfiguration.COW = (DWORD)mfr::mfr_COW_types::COWdefault;
            }
        }

        if (auto pathsValue = rootObject.try_get("overrideLocalRedirections"))
        {
#if MOREDEBUG
            Log("\t\tMFR CONFIG: Has overrideLocalRedirections");
#endif 
        }


        if (auto pathsValue = rootObject.try_get("overrideTraditionalRedirections"))
        {
#if MOREDEBUG
            Log("\t\tMFR CONFIG: Has overrideTraditionalRedirections");
#endif 
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
