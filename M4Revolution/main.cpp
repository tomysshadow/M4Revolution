#include "shared.h"
#include "M4Revolution.h"
#include <chrono>
#include <iostream>
#include <stdlib.h>

bool performOperation(M4Revolution &m4Revolution) {
	const long OPERATION_EDIT_TRANSITION_TIME = 1;
	const long OPERATION_EDIT_INERTIA_LEVELS = 2;
	const long OPERATION_FIX_LOADING = 3;
	const long OPERATION_RESTORE_BACKUP = 4;
	const long OPERATION_EXIT = 5;
	const long OPERATION_MIN = OPERATION_EDIT_TRANSITION_TIME;
	const long OPERATION_MAX = OPERATION_EXIT;

	switch (
			consoleLong(
				"Please enter the number corresponding to the operation you would like to perform.",
				OPERATION_MIN,
				OPERATION_MAX
			)
		) {
		case OPERATION_EDIT_TRANSITION_TIME:
		m4Revolution.editTransitionTime();
		break;
		case OPERATION_EDIT_INERTIA_LEVELS:
		m4Revolution.editInertiaLevels();
		break;
		case OPERATION_FIX_LOADING:
		m4Revolution.fixLoading();
		break;
		case OPERATION_RESTORE_BACKUP:
		return m4Revolution.restoreBackup();
		case OPERATION_EXIT:
		exit(0);
	}
	return true;
}

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

	M4Revolution m4Revolution(argv[1], logFileNames, disableHardwareAcceleration, maxThreads, maxFileTasks);

	do {
		consoleLog("This menu may be used to perform the following operations.", 2);

		consoleLog("1) Edit Transition Time");
		consoleLog("2) Edit Inertia Levels");
		consoleLog("3) Fix Loading");
		consoleLog("4) Restore Backup");
		consoleLog("5) Exit", 2);
	} while (
		consoleBool(
			(
				std::string("The operation has been ")

				+ (
					performOperation(m4Revolution)
					
					? "performed"
					: "aborted"
				)
				
				+ ". Would you like to return to the menu? If not, the application will exit."
			).c_str(),
			
			true
		)
	);
	return 0;
}