//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

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



#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:ChooseIndexForFindResultA=_ChooseIndexForFindResult.ansi")
#pragma comment(linker, "/EXPORT:ChooseIndexForFindResultW=_ChooseIndexForFindResult.wide")
#else
#pragma comment(linker, "/EXPORT:ChooseIndexForFindResultA=ChooseIndexForFindResult.ansi")
#pragma comment(linker, "/EXPORT:ChooseIndexForFindResultW=ChooseIndexForFindResult.wide")
#endif


int ChooseIndexForFindResult(const Cohorts cohorts, FindData3A* result)
{
    int UseIndex = Result_Redirected;
    switch (cohorts.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_package_pvad_area:
        if (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_none)
        {
            // This covers the case where the requested path is in a package area, but not in the VFS area.
            UseIndex = Result_Package;
        }
        break;
    case mfr::mfr_path_types::in_native_area:
    default:
        if (result->find_handles[Result_Redirected])
        {
            UseIndex = Result_Redirected;
        }
        else if (result->find_handles[Result_Package])
        {
            UseIndex = Result_Package;
        }
        else
        {
            UseIndex = Result_Native;
        }
        break;
    }
    return UseIndex;
}
int ChooseIndexForFindResult(const Cohorts cohorts, FindData3W* result)
{
    int UseIndex = Result_Redirected;
    switch (cohorts.file_mfr.Request_MfrPathType)
    {
    case mfr::mfr_path_types::in_package_pvad_area:
        if (cohorts.map.RedirectionFlags == mfr::mfr_redirect_flags::prefer_redirection_none)
        {
            // This covers the case where the requested path is in a package area, but not in the VFS area.
            UseIndex = Result_Package;
        }
        break;
    case mfr::mfr_path_types::in_native_area:
    default:
        if (result->find_handles[Result_Redirected])
        {
            UseIndex = Result_Redirected;
        }
        else if (result->find_handles[Result_Package])
        {
            UseIndex = Result_Package;
        }
        else
        {
            UseIndex = Result_Native;
        }
        break;
    }
    return UseIndex;
}