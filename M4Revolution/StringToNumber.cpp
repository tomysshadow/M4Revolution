#include "pch.h"
#include "StringToNumber.h"

template<typename Char, typename Number>
struct StringToNumberTraits;

template<>
struct StringToNumberTraits<char, long> {
	static long strtox(const char* str, char** endPointer,
		int base, _locale_t locale) {
		return _strtol_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<wchar_t, long> {
	static long strtox(const wchar_t* str, wchar_t** endPointer,
		int base, _locale_t locale) {
		return _wcstol_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<char, long long> {
	static long long strtox(const char* str, char** endPointer,
		int base, _locale_t locale) {
		return _strtoll_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<wchar_t, long long> {
	static long long strtox(const wchar_t* str, wchar_t** endPointer,
		int base, _locale_t locale) {
		return _wcstoll_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<char, unsigned long> {
	static unsigned long strtox(const char* str, char** endPointer,
		int base, _locale_t locale) {
		return _strtoul_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<wchar_t, unsigned long> {
	static unsigned long strtox(const wchar_t* str, wchar_t** endPointer,
		int base, _locale_t locale) {
		return _wcstoul_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<char, unsigned long long> {
	static unsigned long long strtox(const char* str, char** endPointer,
		int base, _locale_t locale) {
		return _strtoull_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<wchar_t, unsigned long long> {
	static unsigned long long strtox(const wchar_t* str, wchar_t** endPointer,
		int base, _locale_t locale) {
		return _wcstoull_l(str, endPointer, base, locale);
	}
};

template<>
struct StringToNumberTraits<char, float> {
	static float strtox(const char* str, char** endPointer,
		int base, _locale_t locale) {
		return _strtof_l(str, endPointer, locale);
	}
};

template<>
struct StringToNumberTraits<wchar_t, float> {
	static float strtox(const wchar_t* str, wchar_t** endPointer,
		int base, _locale_t locale) {
		return _wcstof_l(str, endPointer, locale);
	}
};

template<>
struct StringToNumberTraits<char, double> {
	static double strtox(const char* str, char** endPointer,
		int base, _locale_t locale) {
		return _strtod_l(str, endPointer, locale);
	}
};

template<>
struct StringToNumberTraits<wchar_t, double> {
	static double strtox(const wchar_t* str, wchar_t** endPointer,
		int base, _locale_t locale) {
		return _wcstod_l(str, endPointer, locale);
	}
};

template<>
struct StringToNumberTraits<char, long double> {
	static long double strtox(const char* str, char** endPointer,
		int base, _locale_t locale) {
		return _strtold_l(str, endPointer, locale);
	}
};

template<>
struct StringToNumberTraits<wchar_t, long double> {
	static long double strtox(const wchar_t* str, wchar_t** endPointer,
		int base, _locale_t locale) {
		return _wcstold_l(str, endPointer, locale);
	}
};

template<typename Char, typename Number>
size_t stringToNumber(const Char* str, Number &value,
	int base, const Locale &locale) {
	value = 0;
	errno = 0;

	if (!str) {
		return 0;
	}

	Char* endPointer = 0;

	value = StringToNumberTraits<Char, Number>::strtox(str,
		&endPointer, base, locale);

	return (value || !errno)
		? (size_t)(endPointer - str)
		: 0;
}

template<typename Char, typename Long>
size_t stringToLong(const Char* str, Long &value,
	int base, const Locale &locale) {
	return stringToNumber(str, value, base, locale);
}

template<typename Char, typename Float>
size_t stringToFloat(const Char* str, Float &value,
	const Locale &locale) {
	return stringToNumber(str, value, 0, locale);
}

template<typename Char, typename Long>
Long stringToLongOrDefaultValue(const Char* str, Long defaultValue,
	int base, const Locale &locale) {
	Long value = 0;

	return stringToNumber(str, value, base, locale)
		? value : defaultValue;
}

template<typename Char, typename Float>
Float stringToFloatOrDefaultValue(const Char* str, Float defaultValue,
	const Locale &locale) {
	Float value = 0;

	return stringToNumber(str, value, 0, locale)
		? value : defaultValue;
}

template size_t stringToLong<char, long>(
	const char* str, long &value,
	int base, const Locale &locale
);

template size_t stringToLong<wchar_t, long>(
	const wchar_t* str, long &value,
	int base, const Locale &locale
);

template size_t stringToLong<char, long long>(
	const char* str, long long &value,
	int base, const Locale &locale
);

template size_t stringToLong<wchar_t, long long>(
	const wchar_t* str, long long &value,
	int base, const Locale &locale
);

template size_t stringToLong<char, unsigned long>(
	const char* str, unsigned long &value,
	int base, const Locale &locale
);

template size_t stringToLong<wchar_t, unsigned long>(
	const wchar_t* str, unsigned long &value,
	int base, const Locale &locale
);

template size_t stringToLong<char, unsigned long long>(
	const char* str, unsigned long long &value,
	int base, const Locale &locale
);

template size_t stringToLong<wchar_t, unsigned long long>(
	const wchar_t* str, unsigned long long &value,
	int base, const Locale &locale
);

template size_t stringToFloat<char, float>(
	const char* str, float &value,
	const Locale &locale
);

template size_t stringToFloat<wchar_t, float>(
	const wchar_t* str, float &value,
	const Locale &locale
);

template size_t stringToFloat<char, double>(
	const char* str, double &value,
	const Locale &locale
);

template size_t stringToFloat<wchar_t, double>(
	const wchar_t* str, double &value,
	const Locale &locale
);

template size_t stringToFloat<char, long double>(
	const char* str, long double &value,
	const Locale &locale
);

template size_t stringToFloat<wchar_t, long double>(
	const wchar_t* str, long double &value,
	const Locale &locale
);

template long stringToLongOrDefaultValue<char, long>(
	const char* str, long defaultValue,
	int base, const Locale &locale
);

template long stringToLongOrDefaultValue<wchar_t, long>(
	const wchar_t* str, long defaultValue,
	int base, const Locale &locale
);

template long long stringToLongOrDefaultValue<char, long long>(
	const char* str, long long defaultValue,
	int base, const Locale &locale
);

template long long stringToLongOrDefaultValue<wchar_t, long long>(
	const wchar_t* str, long long defaultValue,
	int base, const Locale &locale
);

template unsigned long stringToLongOrDefaultValue<char, unsigned long>(
	const char* str, unsigned long defaultValue,
	int base, const Locale &locale
);

template unsigned long stringToLongOrDefaultValue<wchar_t, unsigned long>(
	const wchar_t* str, unsigned long defaultValue,
	int base, const Locale &locale
);

template unsigned long long stringToLongOrDefaultValue<char, unsigned long long>(
	const char* str, unsigned long long defaultValue,
	int base, const Locale &locale
);

template unsigned long long stringToLongOrDefaultValue<wchar_t, unsigned long long>(
	const wchar_t* str, unsigned long long defaultValue,
	int base, const Locale &locale
);

template float stringToFloatOrDefaultValue<char, float>(
	const char* str, float defaultValue,
	const Locale &locale
);

template float stringToFloatOrDefaultValue<wchar_t, float>(
	const wchar_t* str, float defaultValue,
	const Locale &locale
);

template double stringToFloatOrDefaultValue<char, double>(
	const char* str, double defaultValue,
	const Locale &locale
);

template double stringToFloatOrDefaultValue<wchar_t, double>(
	const wchar_t* str, double defaultValue,
	const Locale &locale
);

template long double stringToFloatOrDefaultValue<char, long double>(
	const char* str, long double defaultValue,
	const Locale &locale
);

template long double stringToFloatOrDefaultValue<wchar_t, long double>(
	const wchar_t* str, long double defaultValue,
	const Locale &locale
);