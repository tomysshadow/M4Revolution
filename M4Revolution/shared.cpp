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