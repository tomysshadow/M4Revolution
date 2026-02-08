#include "shared.h"
#include <iostream>
#include <sstream>
#include <iomanip>

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

		#ifdef WINDOWS
		std::cerr << " (" << GetLastError() << ")";
		#endif

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

void consoleWait(short newline) {
	#ifdef MACINTOSH
	static const char* COMMAND = "read";
	#endif
	#ifdef WINDOWS
	static const char* COMMAND = "pause";
	#endif

	// we only want to know if we failed to create the process
	// processing the command may fail if the user closes the window (which is fine)
	errno = 0;
	
	if (system(COMMAND) == -1 && errno) {
		throw std::runtime_error("Failed to Create Child Process");
	}

	consoleLog(0, newline);
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
#define CONSOLE_NUMBER_LESS_OUT(maxValue) do { std::cout << "[<=" << (maxValue) << "]" << std::endl; } while (0)
#define CONSOLE_NUMBER_GREATER_OUT(minValue) do { std::cout << "[>=" << (minValue) << "]" << std::endl; } while (0)
#define CONSOLE_NUMBER_VALID_OUT do { std::cout << std::endl; } while (0)

#define CONSOLE_NUMBER_BETWEEN_OUT_RETRY(minValue, maxValue) do { std::cout << "Please enter a valid number from " << (minValue) << " to " << (maxValue) << "." << std::endl; } while (0)
#define CONSOLE_NUMBER_LESS_OUT_RETRY(maxValue) do { std::cout << "Please enter a valid number less than or equal to " << (maxValue) << "." << std::endl; } while (0)
#define CONSOLE_NUMBER_GREATER_OUT_RETRY(minValue) do { std::cout << "Please enter a valid number greater than or equal to " << (minValue) << "." << std::endl; } while (0)
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
	static const char YES = 'Y';
	static const char NO = 'N';

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

		result = toupper((unsigned char)result);
	} while (result != YES && result != NO);
	return result == YES;
}

std::string consoleString(const char* str) {
	if (str) {
		std::cout << str << std::endl;
	}

	std::string result = "";
	std::getline(std::cin, result);
	std::cout << std::endl;
	return result;
}

void readStream(std::istream &inputStream, void* buffer, std::streamsize count) {
	inputStream.read((char*)buffer, count);
}

void writeStream(std::ostream &outputStream, const void* buffer, std::streamsize count) {
	outputStream.write((const char*)buffer, count);
}

void readStreamPartial(std::istream &inputStream, void* buffer, std::streamsize count, std::streamsize &gcount) {
	MAKE_SCOPE_EXIT(gcountScopeExit) {
		gcount = 0;
	};

	try {
		inputStream.read((char*)buffer, count);
	} catch (const std::istream::failure&) {
		inputStream.clear();
	}

	gcount = inputStream.gcount();
	gcountScopeExit.dismiss();
}

void writeStreamPartial(std::ostream &outputStream, const void* buffer, std::streamsize count) {
	try {
		outputStream.write((const char*)buffer, count);
	} catch (const std::ostream::failure&) {
		outputStream.clear();
	}
}

void copyStream(std::istream &inputStream, std::ostream &outputStream, std::streamsize count) {
	copyStreamToWriteDestination(
		inputStream,
	
		[&outputStream](void* buffer, std::streamsize count){
			writeStream(outputStream, buffer, count);
		},
	
		count
	);
}

void copyStreamToString(std::istream &inputStream, std::string &outputString, std::streamsize count) {
	if (count != -1) {
		outputString.reserve(outputString.length() + (std::string::size_type)count);
	}

	copyStreamToWriteDestination(
		inputStream,

		[&outputString](void* buffer, std::streamsize count) {
			outputString.append((char*)buffer, (std::string::size_type)count);
		},

		count
	);
}

void openFile(const char* path) {
	std::ostringstream outputStringStream;
	outputStringStream.exceptions(std::ostringstream::badbit);

	#ifdef MACINTOSH
	outputStringStream << "open";
	#endif
	#ifdef WINDOWS
	outputStringStream << "start \"\"";
	#endif

	outputStringStream << " " << std::quoted(path);

	if (system(outputStringStream.str().c_str())) {
		throw std::runtime_error("Failed to Process Command");
	}
}

void toggleLog(const std::string &name, bool toggledOn) {
	std::cout << name << " has now been toggled " << (toggledOn ? "on." : "off.") << std::endl << std::endl;
}

#ifdef WINDOWS
void readPipePartial(HANDLE pipe, LPVOID buffer, DWORD numberOfBytesToRead, DWORD &numberOfBytesRead) {
	numberOfBytesRead = 0;

	DWORD numberOfBytesCopied = 0;

	while (ReadFile(pipe, (LPSTR)buffer + numberOfBytesRead, numberOfBytesToRead - numberOfBytesRead, &numberOfBytesCopied, NULL)) {
		if ((numberOfBytesRead += numberOfBytesCopied) == numberOfBytesToRead) {
			return;
		}
	}

	osErr(GetLastError() == ERROR_BROKEN_PIPE);
}

std::string getRegistryValueString(HKEY baseRegistryKey, LPCSTR subkeyPointer, LPCSTR valuePointer, DWORD keyFlags) {
	if (!baseRegistryKey) {
		throw std::invalid_argument("baseRegistryKey must not be NULL");
	}

	if (!subkeyPointer) {
		throw std::invalid_argument("subkeyPointer must not be NULL");
	}

	if (!valuePointer) {
		throw std::invalid_argument("valuePointer must not be NULL");
	}

	// open the key, because we will be using it more than once
	HKEY registryKey = NULL;
	osErr(RegOpenKeyExA(baseRegistryKey, subkeyPointer, 0, keyFlags | KEY_QUERY_VALUE, &registryKey));

	SCOPE_EXIT {
		osErr(closeRegistryKey(registryKey));
	};

	// get the data size
	DWORD dataSize = 0;
	osErr(RegGetValueA(registryKey, NULL, valuePointer, RRF_RT_REG_SZ, NULL, NULL, &dataSize));

	// get the data
	std::unique_ptr<CHAR[]> dataPointer(new CHAR[(size_t)dataSize]);
	osErr(RegGetValueA(registryKey, NULL, valuePointer, RRF_RT_REG_SZ, NULL, dataPointer.get(), &dataSize));
	return dataPointer.get();
}

void setFileAttributeHidden(bool hidden, LPCSTR pathStringPointer) {
	if (!pathStringPointer) {
		throw std::invalid_argument("pathStringPointer must not be NULL");
	}

	DWORD fileAttributes = GetFileAttributesA(pathStringPointer);
	osErr(fileAttributes != INVALID_FILE_ATTRIBUTES);

	fileAttributes = hidden
		? fileAttributes | FILE_ATTRIBUTE_HIDDEN
		: fileAttributes & ~FILE_ATTRIBUTE_HIDDEN;
	
	osErr(SetFileAttributesA(pathStringPointer, fileAttributes));
}
#endif