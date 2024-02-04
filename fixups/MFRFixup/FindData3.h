#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "FunctionImplementations.h"
#include <filesystem>
#include <dos_paths.h>
#include "fancy_handle.h"


struct find_deleter
{
    using pointer = psf::fancy_handle;

    void operator()(const pointer& findHandle) noexcept
    {
        if (findHandle)
        {
            [[maybe_unused]] auto result = impl::FindClose(findHandle);
            if (result)
            {
                // removing assert as the condition has been seen to occur afteas part of an application where the app
                // has already crashed and we are cleaning up.  We don't need to announce the crash more than once.
                //assert(result);
                ;
            }
        }
    }
};

using unique_find_handle = std::unique_ptr<void, find_deleter>;

enum FindDataResult
{
    Result_Redirected = 0,
    Result_Package = 1,
    Result_Native = 2
};

struct FindData3
{
    // Redirected path directory so that we can avoid returning duplicate filenames. This value will be empty if the
    // path does not exist/match any existing files at the start of the enumeration, for a slight optimization
    std::wstring requested_path;

    //std::wstring wsNative_path;
    //std::wstring wsPackage_path;
    //std::wstring wsRedirected_path;
    std::list<std::wstring> wsAlready_returned_list = {};

    // There are three different locations where we might find things, stored in the find_handles array and used in the following order:
    //    The first (optional) value is the find handle for the "redirected path" (typically to the user's profile). Set when original is a package path.
    //    The second (optional) value is for a "package" location equivalent to the requested information.
    //    The third (optional) value is the find handle for the equivalend local path.
    // Some of these values will be not used (empty) when not appropriate for the requested type of request.

    // The values are set to INVALID_HANDLE_VALUE as enumeration completes.

    // Notes: When the a path does not exist, the associated handle is NULL,.
    unique_find_handle find_handles[3];

    // We need to hold on to the results of FindFirstFile for find_handles[0/1/2/3/4] 
    WIN32_FIND_DATAW cached_data;

    DWORD RememberedInstance = 0;
};
