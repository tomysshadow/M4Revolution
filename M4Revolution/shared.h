#pragma once
#include "scope_guard.hpp"
#include "Locale.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <fstream>
#include <stdint.h>
#include <string.h>

inline bool stringNullOrEmpty(const char* str) {
	return !str || !*str;
}

inline bool stringWhitespace(const char* str) {
	unsigned char space = *str;

	while (space && isspace(space)) {
		space = *++str;
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

inline bool memoryShift(void* mem, size_t memSize, void* sourceMem, size_t sourceMemSize, size_t shift, bool direction) {
	#pragma warning(push)
	#pragma warning(disable : 4133)
	if (sourceMem < mem || (char*)sourceMem + sourceMemSize > (char*)mem + memSize) {
		return false;
	}

	size_t destinationSize = (char*)mem + memSize - sourceMem;
	char* destination = (char*)sourceMem;

	if (direction) {
		destination += shift;
	} else {
		destination -= shift;
	}

	if (destination < mem || destination + destinationSize > (char*)mem + memSize) {
		return false;
	}
	return !memmove_s(destination, destinationSize, sourceMem, sourceMemSize);
	#pragma warning(pop)
}

// tries to ensure consistent interpretation of periods and commas in string to number conversions
// first tries the IETF name "en-US", then the ISO 15897 name "en_US"
// then tries the global locale (typically "C") if neither are available
static const Locale STRING_TO_NUMBER_LOCALE_DEFAULT({"en-US", "en_US"}, LC_NUMERIC, true);

inline size_t stringToDouble(const char* str, double &result, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0.0;
	};

	if (!str) {
		return 0;
	}

	char* endPointer = 0;

	errno = 0;
	result = _strtod_l(str, &endPointer, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline size_t stringToDoubleWide(const wchar_t* str, double &result, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0.0;
	};

	if (!str) {
		return 0;
	}

	wchar_t* endPointer = 0;

	errno = 0;
	result = _wcstod_l(str, &endPointer, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline double stringToDoubleOrDefaultValue(const char* str, double defaultValue = 0.0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	double result = 0.0;
	return stringToDouble(str, result, locale) ? result : defaultValue;
}

inline double stringToDoubleOrDefaultValueWide(const wchar_t* str, double defaultValue = 0.0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	double result = 0.0;
	return stringToDoubleWide(str, result, locale) ? result : defaultValue;
}

inline size_t stringToFloat(const char* str, float &result, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0.0f;
	};

	if (!str) {
		return 0;
	}

	char* endPointer = 0;

	errno = 0;
	result = _strtof_l(str, &endPointer, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline size_t stringToFloatWide(const wchar_t* str, float &result, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0.0f;
	};

	if (!str) {
		return 0;
	}

	wchar_t* endPointer = 0;

	errno = 0;
	result = _wcstof_l(str, &endPointer, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline float stringToFloatOrDefaultValue(const char* str, float defaultValue = 0.0f, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	float result = 0.0f;
	return stringToFloat(str, result, locale) ? result : defaultValue;
}

inline float stringToFloatOrDefaultValueWide(const wchar_t* str, float defaultValue = 0.0f, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	float result = 0.0f;
	return stringToFloatWide(str, result, locale) ? result : defaultValue;
}

inline size_t stringToLong(const char* str, long &result, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0;
	};

	if (!str) {
		return 0;
	}

	char* endPointer = 0;

	errno = 0;
	result = _strtol_l(str, &endPointer, base, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline size_t stringToLongWide(const wchar_t* str, long &result, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0;
	};

	if (!str) {
		return 0;
	}

	wchar_t* endPointer = 0;

	errno = 0;
	result = _wcstol_l(str, &endPointer, base, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline long stringToLongOrDefaultValue(const char* str, long defaultValue = 0, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	long result = 0;
	return stringToLong(str, result, base, locale) ? result : defaultValue;
}

inline long stringToLongOrDefaultValueWide(const wchar_t* str, long defaultValue = 0, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	long result = 0;
	return stringToLongWide(str, result, base, locale) ? result : defaultValue;
}

inline size_t stringToLongUnsigned(const char* str, unsigned long &result, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0;
	};

	if (!str) {
		return 0;
	}

	char* endPointer = 0;

	errno = 0;
	result = _strtoul_l(str, &endPointer, base, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline size_t stringToLongUnsignedWide(const wchar_t* str, unsigned long &result, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	MAKE_SCOPE_EXIT(resultScopeExit) {
		result = 0;
	};

	if (!str) {
		return 0;
	}

	wchar_t* endPointer = 0;

	errno = 0;
	result = _wcstoul_l(str, &endPointer, base, locale);
	size_t size = (result || !errno) ? (size_t)(endPointer - str) : 0;

	resultScopeExit.dismiss();
	return size;
}

inline long stringToLongUnsignedOrDefaultValue(const char* str, unsigned long defaultValue = 0, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	unsigned long result = 0;
	return stringToLongUnsigned(str, result, base, locale) ? result : defaultValue;
}

inline long stringToLongUnsignedOrDefaultValueWide(const wchar_t* str, unsigned long defaultValue = 0, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT) {
	unsigned long result = 0;
	return stringToLongUnsignedWide(str, result, base, locale) ? result : defaultValue;
}

void consoleLog(const char* str = 0, short newline = true, short tab = false, bool err = false, const char* file = 0, unsigned int line = 0);
void readFileStreamSafe(std::ifstream &inputFileStream, void* buffer, std::streamsize count);
void writeFileStreamSafe(std::ofstream &outputFileStream, const void* buffer, std::streamsize count);
void readFileStreamPartial(std::ifstream &inputFileStream, void* buffer, std::streamsize count, std::streamsize &gcount);
void writeFileStreamPartial(std::ofstream &outputFileStream, const void* buffer, std::streamsize count);
void copyFileStream(std::ifstream &inputFileStream, std::ofstream &outputFileStream, std::streamsize count = -1);