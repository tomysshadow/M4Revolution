#pragma once
#include <string>
#include <string.h>

struct IgnoreCaseComparer {
    bool operator()(const std::string &str, const std::string &str2) const noexcept {
        return _stricmp(str.c_str(), str2.c_str()) < 0;
    }

    bool operator()(const std::wstring &str, const std::wstring &str2) const noexcept {
        return _wcsicmp(str.c_str(), str2.c_str()) < 0;
    }
};