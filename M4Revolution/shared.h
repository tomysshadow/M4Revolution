#pragma once
#define _WIN32_WINNT 0x0600
#define NOMINMAX
#include "Locale.h"
#include <memory>
#include <functional>
#include <stdexcept>
#include <string>
#include <optional>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libzap.h>
#include <scope_guard.hpp>

#ifdef _WIN32
	#define WINDOWS
#else
	#ifdef _WIN16
		#define WINDOWS
	#endif
#endif
#ifndef WINDOWS
	#define MACINTOSH
#endif

#ifdef WINDOWS
	#include <windows.h>
#endif

#include "resource.h"

inline char charWhitespaceTrim(const char* str) {
	unsigned char space = *str;

	while (space && isspace(space)) {
		space = *++str;
	}
	return space;
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

#define RETRY_ERR(retry) do {\
	consoleLog((retry), true, false, true);\
	consoleWait();\
} while (0)

#define OPERATION_EXCEPTION_RETRY_ERR(operation, exception, retry) do {\
	try {\
		(operation);\
	} catch (exception ex) {\
		consoleLog(ex.what(), 2);\
		\
		RETRY_ERR(retry);\
		continue;\
	}\
	\
	break;\
} while (1)

#ifdef WINDOWS
inline void osErrThrow(bool doserrno = false) {
	std::error_code errorCode(doserrno ? _doserrno : GetLastError(), std::system_category());
	throw std::system_error(errorCode);
}

inline void osErr(DWORD dw, bool doserrno = false) {
	if (dw) {
		return;
	}
	
	osErrThrow(doserrno);
}

inline void osErr(WORD w, bool doserrno = false) {
	osErr((DWORD)w, doserrno);
}

inline void osErr(BYTE by, bool doserrno = false) {
	osErr((DWORD)by, doserrno);
}

inline void osErr(BOOL b, bool doserrno = false) {
	osErr((DWORD)b, doserrno);
}

inline void osErr(HANDLE h, bool doserrno = false) {
	if (h != NULL && h != INVALID_HANDLE_VALUE) {
		return;
	}
	
	osErrThrow(doserrno);
}

inline void osErr(LSTATUS err) {
	if (err == ERROR_SUCCESS) {
		return;
	}

	SetLastError(err);
	osErrThrow(false);
}
#endif

template <typename Number>
inline constexpr Number clamp(Number number, Number min, Number max) {
	return __min(max, __max(number, min));
}

template <typename Number>
inline constexpr bool isPowerOfTwo(Number number) {
	return number > 0 && !(number & (number - 1));
}

inline bool freeZAP(zap_byte_t* &out) {
	if (out) {
		if (zap_free(out) != ZAP_ERROR_NONE) {
			return false;
		}
	}

	out = 0;
	return true;
}

#ifdef WINDOWS
inline bool closeHandle(HANDLE &handle) {
	if (handle && handle != INVALID_HANDLE_VALUE) {
		if (!CloseHandle(handle)) {
			return false;
		}
	}

	handle = NULL;
	return true;
}

inline bool closeProcess(HANDLE &process) {
	if (process) {
		if (!CloseHandle(process)) {
			return false;
		}
	}

	process = NULL;
	return true;
}

inline bool closeThread(HANDLE &thread) {
	return closeProcess(thread);
}

inline bool destroyWindow(HWND &windowHandle) {
	if (windowHandle) {
		if (!DestroyWindow(windowHandle)) {
			return false;
		}
	}

	windowHandle = NULL;
	return true;
}

inline bool closeRegistryKey(HKEY &registryKey) {
	if (registryKey) {
		LSTATUS err = RegCloseKey(registryKey);

		if (err != ERROR_SUCCESS) {
			SetLastError(err);
			return false;
		}
	}

	registryKey = NULL;
	return true;
}
#endif

void consoleLog(const char* str = 0, short newline = true, short tab = false, bool err = false, const char* file = 0, unsigned int line = 0);
void consoleWait(short newline = true);
double consoleDouble(const char* str = 0, double minValue = -DBL_MAX, double maxValue = DBL_MAX, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);
float consoleFloat(const char* str = 0, float minValue = -FLT_MAX, float maxValue = FLT_MAX, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);
long consoleLong(const char* str = 0, long minValue = -LONG_MAX, long maxValue = LONG_MAX, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);
unsigned long consoleLongUnsigned(const char* str = 0, unsigned long minValue = 0, unsigned long maxValue = ULONG_MAX, int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);
bool consoleBool(const char* str = 0, const std::optional<bool> &defaultValueOptional = std::nullopt);
std::string consoleString(const char* str = 0);

void readStream(std::istream &inputStream, void* buffer, std::streamsize count);
void writeStream(std::ostream &outputStream, const void* buffer, std::streamsize count);
void readStreamPartial(std::istream &inputStream, void* buffer, std::streamsize count, std::streamsize &gcount);
void writeStreamPartial(std::ostream &outputStream, const void* buffer, std::streamsize count);

template <typename WriteDestination>
void copyStreamToWriteDestination(std::istream &inputStream, WriteDestination writeDestination, std::streamsize count = -1) {
	if (!count) {
		return;
	}

	const size_t BUFFER_SIZE = 0x10000;
	char buffer[BUFFER_SIZE] = {};

	std::streamsize countRead = BUFFER_SIZE;
	std::streamsize gcountRead = 0;

	do {
		countRead = (std::streamsize)__min((size_t)count, (size_t)countRead);

		readStreamPartial(inputStream, buffer, countRead, gcountRead);

		if (!gcountRead) {
			break;
		}

		writeDestination(buffer, gcountRead);

		if (count != -1) {
			count -= gcountRead;

			if (!count) {
				break;
			}
		}
	} while (countRead == gcountRead);

	if (count != -1) {
		if (count) {
			throw std::logic_error("count must not be greater than stream size");
		}
	}
}

void copyStream(std::istream &inputStream, std::ostream &outputStream, std::streamsize count = -1);
void copyStreamToString(std::istream &inputStream, std::string &outputString, std::streamsize count = -1);
void openFile(const char* path);
void toggleLog(const std::string &name, bool toggledOn);

#ifdef WINDOWS
void readPipePartial(HANDLE pipe, LPVOID buffer, DWORD numberOfBytesToRead, DWORD &numberOfBytesRead);
std::string getRegistryValueString(HKEY baseRegistryKey, LPCSTR subkeyPointer, LPCSTR valuePointer, DWORD keyFlags = 0);
void setFileAttributeHidden(bool hidden, LPCSTR pathStringPointer);
#endif