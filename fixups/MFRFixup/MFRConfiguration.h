#pragma once
//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>


namespace mfr
{
    enum class mfr_COW_types
    {
        COWdefault = 0x0000,   //COW in use with winPE disabled
        COWdisableAll = 0x0001,   // COW fully disabled           
        COWenablePe = 0x0002,  // COW in use for all files.
    };
    DEFINE_ENUM_FLAG_OPERATORS(mfr_COW_types);

    struct mfr_configuration
    {
        bool Ilv_Aware = false;
        DWORD COW = 0;
    };

}

extern mfr::mfr_configuration MFRConfiguration;