#pragma once

#include <string_view>
#include <windows.h>

extern bool findStringIC(const std::string& strHaystack, const std::string& strNeedle);
extern bool findStringIC(const std::wstring& strHaystack, const std::wstring& strNeedle);