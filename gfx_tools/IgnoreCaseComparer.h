#pragma once
#include <string>
#include <string.h>
#include <type_traits>

struct IgnoreCaseComparer {
	using is_transparent = void;

	template <typename T, typename T2>
    bool operator()(T str, T2 str2) const noexcept {
		if constexpr (std::is_same_v<decltype(c_str(str)), const wchar_t*>) {
			return _wcsicmp(c_str(str), c_str(str2)) < 0;
		} else {
			return _stricmp(c_str(str), c_str(str2)) < 0;
		}
    }

	private:
	static const char* c_str(const std::string &str) noexcept {
		return str.c_str();
	}

	static const char* c_str(const char* str) noexcept {
		return str;
	}

	static const wchar_t* c_str(const std::wstring &str) noexcept {
		return str.c_str();
	}

	static const wchar_t* c_str(const wchar_t* str) noexcept {
		return str;
	}
};