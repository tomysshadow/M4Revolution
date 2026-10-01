#pragma once

inline char charWhitespaceTrim(const char* str) {
	unsigned char space = (unsigned char)*str;

	while (space && isspace(space)) {
		space = (unsigned char)*++str;
	}
	return (char)space;
}

inline wchar_t wcharWhitespaceTrim(const wchar_t* wcs) {
	wchar_t space = *wcs;

	while (space && iswspace(space)) {
		space = *++wcs;
	}
	return space;
}

inline bool strNullOrEmpty(const char* str) {
	return !str || !*str;
}

inline bool strWhitespace(const char* str) {
	unsigned char space = (unsigned char)*str;

	while (space && isspace(space)) {
		space = (unsigned char)*++str;
	}
	return !space;
}

inline bool wcsWhitespace(const wchar_t* wcs) {
	wchar_t space = *wcs;

	while (space && iswspace(space)) {
		space = *++wcs;
	}
	return !space;
}

inline size_t strSize(const char* str) {
	return strlen(str) + 1;
}

inline size_t wcsSize(const wchar_t* wcs) {
	return wcslen(wcs) + 1;
}

inline size_t strSizeMax(const char* str, size_t sizeMax) {
	return strnlen_s(str, sizeMax) + 1;
}

inline size_t wcsSizeMax(const wchar_t* wcs, size_t sizeMax) {
	return wcsnlen_s(wcs, sizeMax) + 1;
}

inline bool strTruncated(const char* str, size_t size) {
	return size <= strnlen_s(str, size) + 1;
}

inline bool wcsTruncated(const wchar_t* wcs, size_t size) {
	return size <= wcsnlen_s(wcs, size) + 1;
}

inline bool strEquals(const char* str, const char* str2) {
	return !strcmp(str, str2);
}

inline bool wcsEquals(const wchar_t* wcs, const wchar_t* wcs2) {
	return !wcscmp(wcs, wcs2);
}

inline bool strEqualsIgnoreCase(const char* str, const char* str2) {
	return !_stricmp(str, str2);
}

inline bool wcsEqualsIgnoreCase(const wchar_t* wcs, const wchar_t* wcs2) {
	return !_wcsicmp(wcs, wcs2);
}

inline bool memEquals(const void* mem, const void* mem2, size_t size) {
	return !memcmp(mem, mem2, size);
}

#define GFX_TOOLS_CALL

#ifdef _WIN32
	#ifdef GFX_TOOLS_LIBRARY
		#define GFX_TOOLS_API __declspec(dllexport)
	#else
		#define GFX_TOOLS_API __declspec(dllimport)
	#endif
#else
	#define GFX_TOOLS_API
#endif

using L_INT = int;
static constexpr L_INT SUCCESS = 1;

#ifdef FOR_UNICODE
	using L_TCHAR = TCHAR;
#else
	using L_TCHAR = char;
#endif

inline bool freeZAP(zap_byte_t* &out) {
	if (out) {
		if (zap_free(out) != ZAP_ERROR_NONE) {
			return false;
		}
	}

	out = nullptr;
	return true;
}