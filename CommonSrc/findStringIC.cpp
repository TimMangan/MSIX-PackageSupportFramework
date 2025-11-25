
#include <string_view>
#include <vector>

#include <windows.h>
#include <detours.h>
#include <psf_constants.h>
#include <psf_framework.h>
#include <psf_logging.h>

#include "Config.h"
#include <StartInfo_helper.h>
#include <TlHelp32.h>
#include <shellapi.h>

using namespace std::literals;


bool findStringIC(const std::string& strHaystack, const std::string& strNeedle)
{
    auto it = std::search(
        strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(), strNeedle.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return (it != strHaystack.end());
}
bool findStringIC(const std::wstring& strHaystack, const std::wstring& strNeedle)
{
    auto it = std::search(
        strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(), strNeedle.end(),
        [](wchar_t ch1, wchar_t ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return (it != strHaystack.end());
}

std::wstring wStringToLower(const std::wstring& str) {
    std::wstring lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::towlower);
    return lowerStr;
}

std::wstring wStringToUpper(const std::wstring& str) {
    std::wstring upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::towupper);
    return upperStr;
}

std::wstring caseInsensitiveReplace(const std::wstring& str, const std::wstring& from, const std::wstring& to) {
    std::wstring lowerStr = wStringToLower(str);
    std::wstring lowerFrom = wStringToLower(from);

    size_t pos = 0;
    std::wstring result = str;
    while ((pos = lowerStr.find(lowerFrom, pos)) != std::wstring::npos) {
        result.replace(pos, from.length(), to);
        lowerStr.replace(pos, from.length(), wStringToLower(to));
        pos += to.length();
    }

    return result;
}