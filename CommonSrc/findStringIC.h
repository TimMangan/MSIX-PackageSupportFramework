#pragma once

#include <string_view>
#include <windows.h>

extern bool findStringIC(const std::string& strHaystack, const std::string& strNeedle);
extern bool findStringIC(const std::wstring& strHaystack, const std::wstring& strNeedle);

extern std::wstring wStringToLower(const std::wstring& str);
extern std::wstring wStringToUpper(const std::wstring& str);

extern std::wstring caseInsensitiveReplace(const std::wstring& str, const std::wstring& from, const std::wstring& to);