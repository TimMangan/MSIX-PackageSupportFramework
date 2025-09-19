//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#if _DEBUG
//#define MOREDEBUG
#endif

std::string ReplaceAppRegistrySyntax(std::string regPath);

REGSAM RegFixupSam(Json_Debug_Levels debugRequestLevel, std::string keypath, REGSAM samDesired, DWORD RegLocalInstance);

bool RegFixupFakeDelete(Json_Debug_Levels debugRequestLevel, std::string keypath, [[maybe_unused]] DWORD RegLocalInstance);

LSTATUS RegFixupDeletionMarker(Json_Debug_Levels debugRequestLevel, std::string keyPath,std::string Value, [[maybe_unused]] DWORD RegLocalInstance);

bool RegFixupJavaBlocker(Json_Debug_Levels debugRequestLevel, std::string keypath, [[maybe_unused]] DWORD RegLocalInstance);

#if TRYHKLM2HKCU
bool HasHKLM2HKCUSpecified();

std::string HKLM2HKCU_Replacement(std::string path);
#endif

