
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#if _DEBUG
//#define MOREDEBUG 1
#endif

#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>
#include "PathUtilities.h"
#include "Detect_Pipe.h"

///
// detectPipe
// Given an input string, determines if the string could be a pipe.
// - Checks for form: \\.\pipe\<token>
// - Will only allow local pipes
// - Will not look for local pipes using localhost or 127.0.0.1
bool detectPipe(std::wstring pipeName);
bool detectPipe(std::string  pipeName);

const static int PipePrefixLen = 9;


std::wstring AdjustLocalPipeName(std::wstring pipeName)
{
    if (pipeName.length() >= PipePrefixLen)
    {
        if (detectPipe(pipeName))
        {
            std::wstring formattedPipe{ L"\\\\.\\pipe\\LOCAL\\" };
            // Stomp on the server name. In an app container, pipe name must be as follows
            formattedPipe.append(pipeName.substr(PipePrefixLen));
            return  formattedPipe ;
        }
    }
    // Not a valid pipe input so return input string
    return pipeName;
}
std::string AdjustLocalPipeName(std::string pipeName)
{
    if (pipeName.length() >= PipePrefixLen)
    {
        if (detectPipe(pipeName))
        {
            std::string formattedPipe{ "\\\\.\\pipe\\LOCAL\\" };
            // Stomp on the server name. In an app container, pipe name must be as follows
            formattedPipe.append(pipeName.substr(PipePrefixLen));
            return  formattedPipe;
        }
    }
    // Not a valid pipe input so return input string
    return pipeName;
}

 bool detectPipe(std::string pipeName)
{
    return detectPipe(widen(pipeName));
}

bool detectPipe(std::wstring pipeName)
{
    // Verify that the name can even fit
    if (pipeName.size() <= PipePrefixLen)
    {
        return false;
    }

    // Verify pipe path, makes assumptions on the input path as follows
    auto pathType = psf::path_type(pipeName.c_str());
    if ((pathType != psf::dos_path_type::local_device) &&
        (pathType != psf::dos_path_type::root_local_device))
    {
        return false;
    }

    if (comparei(pipeName.substr(3,5),L"\\pipe") &&
        pipeName.length() >= PipePrefixLen)
    {
        return true;
    }
    return false;

    // Verify that the name after the path root is pipe/
    //if (((pipeName[4] != 'p') && (pipeName[4] != 'P')) ||
    //    ((pipeName[5] != 'i') && (pipeName[5] != 'I')) ||
    //    ((pipeName[6] != 'p') && (pipeName[6] != 'P')) ||
    //    ((pipeName[7] != 'e') && (pipeName[7] != 'E')) ||
    //    !psf::is_path_separator(pipeName[8]))
    //{
    //    return false;
    //}

    //return true;
}
