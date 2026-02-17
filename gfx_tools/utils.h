#pragma once

inline char charWhitespaceTrim(const char* str) {
	unsigned char space = (unsigned char)*str;

	while (space && isspace(space)) {
		space = (unsigned char)*++str;
	}
	return (char)space;
}

inline wchar_t charWhitespaceTrimWide(const wchar_t* str) {
	wchar_t space = *str;

	while (space && iswspace(space)) {
		space = *++str;
	}
	return space;
}

inline bool stringNullOrEmpty(const char* str) {
	return !str || !*str;
}

inline bool stringWhitespace(const char* str) {
	unsigned char space = (unsigned char)*str;

	while (space && isspace(space)) {
		space = (unsigned char)*++str;
	}
	return !space;
}

inline bool stringWhitespaceWide(const wchar_t* str) {
	wchar_t space = *str;

	while (space && iswspace(space)) {
		space = *++str;
	}
	return !space;
}

inline size_t stringSize(const char* str) {
	return strlen(str) + 1;
}

inline size_t stringSizeWide(const wchar_t* str) {
	return wcslen(str) + 1;
}

inline size_t stringSizeMax(const char* str, size_t sizeMax) {
	return strnlen_s(str, sizeMax) + 1;
}

inline size_t stringSizeMaxWide(const wchar_t* str, size_t sizeMax) {
	return wcsnlen_s(str, sizeMax) + 1;
}

inline bool stringTruncated(const char* str, size_t size) {
	return size <= strnlen_s(str, size) + 1;
}

inline bool stringTruncatedWide(const wchar_t* str, size_t size) {
	return size <= wcsnlen_s(str, size) + 1;
}

inline bool stringEquals(const char* str, const char* str2) {
	return !strcmp(str, str2);
}

inline bool stringEqualsWide(const wchar_t* str, const wchar_t* str2) {
	return !wcscmp(str, str2);
}

inline bool stringEqualsCaseInsensitive(const char* str, const char* str2) {
	return !_stricmp(str, str2);
}

inline bool stringEqualsCaseInsensitiveWide(const wchar_t* str, const wchar_t* str2) {
	return !_wcsicmp(str, str2);
}

inline bool memoryEquals(const void* mem, const void* mem2, size_t size) {
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

typedef int L_INT;
static const L_INT SUCCESS = 1;

#ifdef FOR_UNICODE
	typedef TCHAR L_TCHAR;
#else
	typedef char L_TCHAR;
#endif

inline bool freeZAP(zap_byte_t* &out) {
	if (out) {
		if (zap_free(out) != ZAP_ERROR_NONE) {
			return false;
		}
	}

	out = 0;
	return true;
}