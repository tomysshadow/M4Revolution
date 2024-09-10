#include "shared.h"
#include <iostream>

#define SHARED_OUT true, 2
#define SHARED_ERR true, 2, true, __FILE__, __LINE__

void consoleLog(const char* str, short newline, short tab, bool err, const char* file, unsigned int line) {
	if (!str) {
		str = "";
	}

	if (err) {
		for (short i = 0; i < tab; i++) {
			std::cerr << "\t";
		}

		std::cerr << str;

		if (line || !stringNullOrEmpty(file)) {
			if (!file) {
				file = "";
			}

			std::cerr << " [" << file << ":" << line << "]";
		}

		std::cerr << " (" << GetLastError() << ")";

		for (short i = 0; i < newline; i++) {
			std::cerr << std::endl;
		}
		return;
	}

	for (short i = 0; i < tab; i++) {
		std::cout << "\t";
	}

	std::cout << str;

	if (line || !stringNullOrEmpty(file)) {
		if (!file) {
			file = "";
		}

		std::cout << " [" << file << ":" << line << "]";
	}

	for (short i = 0; i < newline; i++) {
		std::cout << std::endl;
	}
}

#define CONSOLE_NUMBER_LOCALE(locale) do {\
	std::locale coutLocale = std::cout.imbue(locale);\
	\
	SCOPE_EXIT {\
		std::cout.imbue(coutLocale);\
	};\
} while (0)

#define CONSOLE_NUMBER_BETWEEN(minValue, minValueDefault, maxValue, maxValueDefault) ((minValue) != (minValueDefault) && (maxValue) != (maxValueDefault))
#define CONSOLE_NUMBER_LESS(minValue, minValueDefault, maxValue, maxValueDefault) ((minValue) == (minValueDefault) && (maxValue) != (maxValueDefault))
#define CONSOLE_NUMBER_GREATER(minValue, minValueDefault, maxValue, maxValueDefault) ((minValue) != (minValueDefault) && (maxValue) == (maxValueDefault))

#define CONSOLE_NUMBER_BETWEEN_OUT(minValue, maxValue) do { std::cout << "[" << (minValue) << " - " << (maxValue) << "]" << std::endl; } while (0)
#define CONSOLE_NUMBER_LESS_OUT(maxValue) do { std::cout << "[<" << (maxValue) << "]" << std::endl; } while (0)
#define CONSOLE_NUMBER_GREATER_OUT(minValue) do { std::cout << "[>" << (minValue) << "]" << std::endl; } while (0)
#define CONSOLE_NUMBER_VALID_OUT do { std::cout << std::endl; } while (0)

#define CONSOLE_NUMBER_BETWEEN_OUT_RETRY(minValue, maxValue) do { std::cout << "Please enter a valid number between " << (minValue) << " and " << (maxValue) << "." << std::endl; } while (0)
#define CONSOLE_NUMBER_LESS_OUT_RETRY(maxValue) do { std::cout << "Please enter a valid number less than " << (maxValue) << "." << std::endl; } while (0)
#define CONSOLE_NUMBER_GREATER_OUT_RETRY(minValue) do { std::cout << "Please enter a valid number greater than " << (minValue) << "." << std::endl; } while (0)
#define CONSOLE_NUMBER_VALID_RETRY_OUT do { std::cout << "Please enter a valid number." << std::endl; } while (0)

#define CONSOLE_NUMBER_OUT(str, minValue, minValueDefault, maxValue, maxValueDefault, resultString) do {\
	if (str) {\
		std::cout << str << " ";\
		\
		if (CONSOLE_NUMBER_BETWEEN(minValue, minValueDefault, maxValue, maxValueDefault)) {\
			CONSOLE_NUMBER_BETWEEN_OUT(minValue, maxValue);\
		} else if (CONSOLE_NUMBER_LESS(minValue, minValueDefault, maxValue, maxValueDefault)) {\
			CONSOLE_NUMBER_LESS_OUT(maxValue);\
		} else if (CONSOLE_NUMBER_GREATER(minValue, minValueDefault, maxValue, maxValueDefault)) {\
			CONSOLE_NUMBER_GREATER_OUT(minValue);\
		} else {\
			CONSOLE_NUMBER_VALID_OUT;\
		}\
		\
		str = 0;\
	} else {\
		if (CONSOLE_NUMBER_BETWEEN(minValue, minValueDefault, maxValue, maxValueDefault)) {\
			CONSOLE_NUMBER_BETWEEN_OUT_RETRY(minValue, maxValue);\
		} else if (CONSOLE_NUMBER_LESS(minValue, minValueDefault, maxValue, maxValueDefault)) {\
			CONSOLE_NUMBER_LESS_OUT_RETRY(maxValue);\
		} else if (CONSOLE_NUMBER_GREATER(minValue, minValueDefault, maxValue, maxValueDefault)) {\
			CONSOLE_NUMBER_GREATER_OUT_RETRY(minValue);\
		} else {\
			CONSOLE_NUMBER_VALID_RETRY_OUT;\
		}\
	}\
	\
	std::getline(std::cin, resultString);\
	std::cout << std::endl;\
} while (0)

double consoleDouble(const char* str, double minValue, double maxValue, const Locale &locale) {
	CONSOLE_NUMBER_LOCALE(locale);

	std::string resultString = "";
	double result = 0.0;

	do {
		CONSOLE_NUMBER_OUT(str, minValue, -DBL_MAX, maxValue, DBL_MAX, resultString);
	} while (!stringToDouble(resultString.c_str(), result, locale) || result < minValue || result > maxValue);
	return result;
}

float consoleFloat(const char* str, float minValue, float maxValue, const Locale &locale) {
	CONSOLE_NUMBER_LOCALE(locale);

	std::string resultString = "";
	float result = 0.0f;

	do {
		CONSOLE_NUMBER_OUT(str, minValue, -FLT_MAX, maxValue, FLT_MAX, resultString);
	} while (!stringToFloat(resultString.c_str(), result, locale) || result < minValue || result > maxValue);
	return result;
}

long consoleLong(const char* str, long minValue, long maxValue, int base, const Locale &locale) {
	CONSOLE_NUMBER_LOCALE(locale);

	std::string resultString = "";
	long result = 0;

	do {
		CONSOLE_NUMBER_OUT(str, minValue, -LONG_MAX, maxValue, LONG_MAX, resultString);
	} while (!stringToLong(resultString.c_str(), result, base, locale) || result < minValue || result > maxValue);
	return result;
}

unsigned long consoleLongUnsigned(const char* str, unsigned long minValue, unsigned long maxValue, int base, const Locale &locale) {
	CONSOLE_NUMBER_LOCALE(locale);

	std::string resultString = "";
	unsigned long result = 0;

	do {
		CONSOLE_NUMBER_OUT(str, minValue, 0, maxValue, ULONG_MAX, resultString);
	} while (!stringToLongUnsigned(resultString.c_str(), result, base, locale) || result < minValue || result > maxValue);
	return result;
}

bool consoleBool(const char* str, const std::optional<bool> &defaultValueOptional) {
	const char YES = 'Y';
	const char NO = 'N';

	char yes = 'y';
	char no = 'n';

	if (defaultValueOptional.has_value()) {
		if (defaultValueOptional.value()) {
			yes = YES;
		} else {
			no = NO;
		}
	}

	std::string resultString = "";
	char result = 0;

	do {
		if (str) {
			std::cout << str << " [" << yes << "/" << no << "]" << std::endl;
			str = 0;
		} else {
			std::cout << "Please enter " << yes << " or " << no << "." << std::endl;
		}

		std::getline(std::cin, resultString);
		std::cout << std::endl;

		result = charWhitespaceTrim(resultString.c_str());

		if (!result && defaultValueOptional.has_value()) {
			return defaultValueOptional.value();
		}

		result = toupper(result);
	} while (result != YES && result != NO);
	return result == YES;
}

void readFileStreamSafe(std::ifstream &inputFileStream, void* buffer, std::streamsize count) {
	if (!inputFileStream.read((char*)buffer, count) || count != inputFileStream.gcount()) {
		throw std::runtime_error("Failed to Read File Stream");
	}
}

void writeFileStreamSafe(std::ofstream &outputFileStream, const void* buffer, std::streamsize count) {
	if (!outputFileStream.write((const char*)buffer, count)) {
		throw std::runtime_error("Failed to Write File Stream");
	}
}

void readFileStreamPartial(std::ifstream &inputFileStream, void* buffer, std::streamsize count, std::streamsize &gcount) {
	gcount = 0;
	inputFileStream.read((char*)buffer, count);
	gcount = inputFileStream.gcount();
}

void writeFileStreamPartial(std::ofstream &outputFileStream, const void* buffer, std::streamsize count) {
	outputFileStream.write((const char*)buffer, count);
}

void copyFileStream(std::ifstream &inputFileStream, std::ofstream &outputFileStream, std::streamsize count) {
	if (!count) {
		return;
	}

	const size_t BUFFER_SIZE = 0x10000;
	char buffer[BUFFER_SIZE] = {};

	std::streamsize countRead = BUFFER_SIZE;
	std::streamsize gcountRead = 0;

	do {
		countRead = (std::streamsize)min((size_t)count, (size_t)countRead);

		readFileStreamPartial(inputFileStream, buffer, countRead, gcountRead);

		if (!gcountRead) {
			break;
		}

		writeFileStreamSafe(outputFileStream, buffer, gcountRead);

		if (count != -1) {
			count -= gcountRead;

			if (!count) {
				break;
			}
		}
	} while (countRead == gcountRead);

	if (count != -1) {
		if (count) {
			throw std::logic_error("count must not be greater than file size");
		}
	}
}