//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include <errno.h>
#include "FunctionImplementations.h"
#include <psf_logging.h>

#include "ManagedPathTypes.h"
#include "PathUtilities.h"
#include "FunctionImplementations.h"
#include <psf_logging.h>
#include <memory>
#include "FindData3.h"
#include "FindFirstHelpers.h"



DWORD copy_find_data(const WIN32_FIND_DATAW& from, WIN32_FIND_DATAA& to) noexcept
{
    to.dwFileAttributes = from.dwFileAttributes;
    to.ftCreationTime = from.ftCreationTime;
    to.ftLastAccessTime = from.ftLastAccessTime;
    to.ftLastWriteTime = from.ftLastWriteTime;
    to.nFileSizeHigh = from.nFileSizeHigh;
    to.nFileSizeLow = from.nFileSizeLow;
    to.dwReserved0 = from.dwReserved0;
    to.dwReserved1 = from.dwReserved1;

    if (auto len = ::WideCharToMultiByte(CP_ACP, 0, from.cFileName, -1, to.cFileName, sizeof(to.cFileName), nullptr, nullptr);
        !len || (len > sizeof(to.cFileName)))
    {
        return ::GetLastError();
    }

    if (auto len = ::WideCharToMultiByte(CP_ACP, 0, from.cAlternateFileName, -1, to.cAlternateFileName, sizeof(to.cAlternateFileName), nullptr, nullptr);
        !len || (len > sizeof(to.cAlternateFileName)))
    {
        return ::GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD copy_find_data(const WIN32_FIND_DATAW& from, WIN32_FIND_DATAW& to) noexcept
{
    to = from;
    return ERROR_SUCCESS;
}
DWORD copy_find_data(const WIN32_FIND_DATAA& from, WIN32_FIND_DATAW& to) noexcept
{
    to.dwFileAttributes = from.dwFileAttributes;
    to.ftCreationTime = from.ftCreationTime;
    to.ftLastAccessTime = from.ftLastAccessTime;
    to.ftLastWriteTime = from.ftLastWriteTime;
    to.nFileSizeHigh = from.nFileSizeHigh;
    to.nFileSizeLow = from.nFileSizeLow;
    to.dwReserved0 = from.dwReserved0;
    to.dwReserved1 = from.dwReserved1;

    if (auto len = ::MultiByteToWideChar(CP_ACP, 0, from.cFileName, -1, to.cFileName, sizeof(to.cFileName));
        !len || (len > sizeof(to.cFileName)))
    {
        return ::GetLastError();
    }

    if (auto len = ::MultiByteToWideChar(CP_ACP, 0, from.cAlternateFileName, -1, to.cAlternateFileName, sizeof(to.cAlternateFileName));
        !len || (len > sizeof(to.cAlternateFileName)))
    {
        return ::GetLastError();
    }

    return ERROR_SUCCESS;
}
