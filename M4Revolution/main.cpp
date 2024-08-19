#include "shared.h"
#include "M4Revolution.h"
#include <chrono>
#include <iostream>

#define MAIN_OUT 2
#define MAIN_ERR true, false, true, __FILE__, __LINE__

void help() {
	consoleLog("Usage: M4Revolution path [-lfn -nohw -mt maxThreads]");
}

int main(int argc, char** argv) {
	consoleLog("Myst IV: Revolution 1.0.0");
	consoleLog("By Anthony Kleine", 2);

	const int MIN_ARGC = 2;

	if (argc < MIN_ARGC) {
		help();
		return 1;
	}

	std::string arg = "";
	int argc2 = argc - 1;

	bool logFileNames = false;
	bool disableHardwareAcceleration = false;
	unsigned long maxThreads = 0;
	unsigned long maxFileTasks = 0;

	for (int i = MIN_ARGC; i < argc; i++) {
		arg = std::string(argv[i]);

		if (arg == "-h" || arg == "--help") {
			help();
			return 0;
		} else if (arg == "-lfn" || arg == "--log-file-names") {
			logFileNames = true;
		} else if (arg == "-nohw" || arg == "--disable-hardware-acceleration") {
			disableHardwareAcceleration = true;
		} else if (i < argc2) {
			if (arg == "-mt" || arg == "--max-threads") {
				if (!stringToLongUnsigned(argv[++i], maxThreads)) {
					consoleLog("Max Threads must be a valid number", 2);
					help();
					return 1;
				}
			} else if (arg == "--dev-max-file-tasks") {
				if (!stringToLongUnsigned(argv[++i], maxFileTasks)) {
					consoleLog("Max File Tasks must be a valid number", 2);
					help();
					return 1;
				}
			}
		}
	}

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	M4Revolution m4Revolution(argv[1], logFileNames, disableHardwareAcceleration, maxThreads, maxFileTasks);
	m4Revolution.fixLoading();

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Elapsed Seconds: " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	return 0;
}