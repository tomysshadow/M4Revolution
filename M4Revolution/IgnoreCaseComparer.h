#pragma once
#include <string>

struct IgnoreCaseComparer {
    bool operator()(const std::string &str, const std::string &str2) const noexcept {
        int result = _strnicmp(str.c_str(), str2.c_str(), str.size() + 1);
        
        return result
            ? result < 0
            : str.size() < str2.size();
    }
};