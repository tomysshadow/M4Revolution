#include "shared.h"
#include "M4Revolution.h"

bool performOperation(M4Revolution &m4Revolution) {
	const long OPERATION_TOGGLE_SOUND_FADING = 1;
	const long OPERATION_EDIT_TRANSITION_TIME = 2;
	const long OPERATION_EDIT_MOUSE_CONTROLS = 3;
	const long OPERATION_FIX_LOADING = 4;
	const long OPERATION_RESTORE_BACKUP = 5;
	const long OPERATION_EXIT = 6;
	const long OPERATION_MIN = OPERATION_TOGGLE_SOUND_FADING;
	const long OPERATION_MAX = OPERATION_EXIT;

	switch (consoleLong(
			"Please enter the number corresponding to the operation you would like to perform.",
			OPERATION_MIN,
			OPERATION_MAX
		)) {
		case OPERATION_TOGGLE_SOUND_FADING:
		m4Revolution.toggleSoundFading();
		break;
		case OPERATION_EDIT_TRANSITION_TIME:
		m4Revolution.editTransitionTime();
		break;
		case OPERATION_EDIT_MOUSE_CONTROLS:
		m4Revolution.editMouseControls();
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
	consoleLog("Usage: M4Revolution [-p path -lfn -nohw -mt maxThreads]");
}

int main(int argc, char** argv) {
	consoleLog("Myst IV: Revolution 1.0.0");
	consoleLog("By Anthony Kleine", 2);

	const int MIN_ARGC = 1;

	if (argc < MIN_ARGC) {
		help();
		return 1;
	}

	std::string arg = "";
	int argc2 = argc - 1;

	std::string path = "data.m4b"; // TODO: find install location automatically
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
			if (arg == "-p" || arg == "-path") {
				path = argv[++i];
			} else if (arg == "-mt" || arg == "--max-threads") {
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

	M4Revolution m4Revolution(path.c_str(), logFileNames, disableHardwareAcceleration, maxThreads, maxFileTasks);

	do {
		consoleLog("This menu may be used to perform the following operations.", 2);

		consoleLog("1) Toggle Sound Fading");
		consoleLog("2) Edit Transition Time");
		consoleLog("3) Edit Mouse Controls");
		consoleLog("4) Fix Loading");
		consoleLog("5) Restore Backup");
		consoleLog("6) Exit", 2);
	} while (consoleBool(
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
	));
	return 0;
}