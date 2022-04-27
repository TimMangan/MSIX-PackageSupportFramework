
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
