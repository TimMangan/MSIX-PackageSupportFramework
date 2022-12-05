//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

//#include "attributes.h"
//#include "common_paths.h"
extern void Log(const char* fmt, ...);

auto GetFileAttributesFunc = &::GetFileAttributesW;


int DoPlaceholderTest()
{
    trace_message("Running a placeholder test.\n", info_color);
    auto attr = ::GetFileAttributesW(L"C:\\Program Files\\PlaceholderTest\\Placeholder.txt");
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        return trace_last_error(L"Failed to query the file attributes");
    }
    return ERROR_SUCCESS;
}



int PlaceholderTest()
{
    test_begin("Placeholder Test");
    Log("<<<<<MFRTests Placeholder Test");
    auto result = DoPlaceholderTest();
    Log("MFRTests Placeholder Test>>>>>");
    test_end(result);
    return result;
}
