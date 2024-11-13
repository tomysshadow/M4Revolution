#include "shared.h"
#include "M4Revolution.h"
#include <optional>
#include <filesystem>

#pragma warning(push)
#pragma warning(disable : 4244)
#include <steampp/steampp.h>
#pragma warning(pop)

void help() {
	openFile("https://github.com/tomysshadow/M4Revolution/blob/master/README.md");
}

bool performOperation(M4Revolution &m4Revolution) {
	const long OPERATION_OPEN_ONLINE_HELP = 1;
	const long OPERATION_EDIT_TRANSITION_TIME = 2;
	const long OPERATION_TOGGLE_SOUND_FADING = 3;
	const long OPERATION_TOGGLE_CAMERA_INERTIA = 4;
	const long OPERATION_FIX_LOADING = 5;
	const long OPERATION_RESTORE_BACKUP = 6;
	const long OPERATION_EXIT = 7;
	const long OPERATION_MIN = OPERATION_OPEN_ONLINE_HELP;
	const long OPERATION_MAX = OPERATION_EXIT;

	switch (consoleLong(
		"Please enter the number corresponding to the operation you would like to perform.",
		OPERATION_MIN,
		OPERATION_MAX
	)) {
		case OPERATION_OPEN_ONLINE_HELP:
		help();
		break;
		case OPERATION_EDIT_TRANSITION_TIME:
		m4Revolution.editTransitionTime();
		break;
		case OPERATION_TOGGLE_SOUND_FADING:
		m4Revolution.toggleSoundFading();
		break;
		case OPERATION_TOGGLE_CAMERA_INERTIA:
		m4Revolution.toggleCameraInertia();
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
	int argc7 = argc - 6;

	std::optional<std::string> pathStringOptional = std::nullopt;
	bool logFileNames = false;
	bool disableHardwareAcceleration = false;
	unsigned long maxThreads = 0;
	unsigned long maxFileTasks = 0;
	std::optional<Work::Convert::Configuration> configurationOptional = std::nullopt;

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
			if (arg == "-p" || arg == "--path") {
				pathStringOptional = argv[++i];
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
			} else if (i < argc7) {
				if (arg == "--dev-configuration") {
					Work::Convert::Configuration &configuration = configurationOptional.emplace();

					if (!stringToLongUnsigned(argv[++i], configuration.minTextureWidth)
						|| !stringToLongUnsigned(argv[++i], configuration.maxTextureWidth)
						|| !stringToLongUnsigned(argv[++i], configuration.minTextureHeight)
						|| !stringToLongUnsigned(argv[++i], configuration.maxTextureHeight)
						|| !stringToLongUnsigned(argv[++i], configuration.minVolumeExtent)
						|| !stringToLongUnsigned(argv[++i], configuration.maxVolumeExtent)) {
						consoleLog("Configuration must be six valid numbers", 2);
						help();
						return 1;
					}
				}
			}
		}
	}

	if (!pathStringOptional.has_value()) {
		const steampp::AppID MYST_IV_REVELATION_APP_ID = 925940;

		steampp::Steam steam;
		pathStringOptional.emplace(steam.getAppInstallDir(MYST_IV_REVELATION_APP_ID));

		if (pathStringOptional.value().empty()) {
			pathStringOptional.emplace(std::filesystem::current_path().string());
		}
	}

	M4Revolution m4Revolution(pathStringOptional.value(), logFileNames, disableHardwareAcceleration, maxThreads, maxFileTasks, configurationOptional);

	do {
		consoleLog("This menu may be used to perform the following operations.", 2);

		consoleLog("1) Open Online Help");
		consoleLog("2) Edit Transition Time");
		consoleLog("3) Toggle Sound Fading");
		consoleLog("4) Toggle Camera Inertia");
		consoleLog("5) Fix Loading");
		consoleLog("6) Restore Backup");
		consoleLog("7) Exit", 2);
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

// "And God said, 'Let there be light,' and there was light." - Genesis 1:3