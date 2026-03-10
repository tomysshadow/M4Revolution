#pragma once
#include "Locale.h"

// ensures consistent interpretation of periods and commas in string to number conversions
inline const Locale STRING_TO_NUMBER_LOCALE_DEFAULT("C", LC_NUMERIC);

template<typename Char, typename Long>
size_t stringToLong(const Char* str, Long &value,
	int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);

template<typename Char, typename Float>
size_t stringToFloat(const Char* str, Float &value,
	const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);

template<typename Char, typename Long>
Long stringToLongOrDefaultValue(const Char* str, Long defaultValue,
	int base = 0, const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);

template<typename Char, typename Float>
Float stringToFloatOrDefaultValue(const Char* str, Float defaultValue,
	const Locale &locale = STRING_TO_NUMBER_LOCALE_DEFAULT);