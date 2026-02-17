#include "pch.h"
#include "versioninfo.h"
#include "M4Revolution.h"
#include <filesystem>

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 26454)
#include <steampp/steampp.h>
#pragma warning(pop)

void help() {
	openFile("https://github.com/tomysshadow/M4Revolution/blob/main/README.md");
}

std::string getAppInstallDir() {
	#ifdef WINDOWS
	try {
		// try for the DVD release first
		static const CHAR MYST_IV_REVELATION_DVD_SUBKEY[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{96F702F3-7CA4-41B5-A70A-4F348DF99A9A}";
		return getRegistryValueString(HKEY_LOCAL_MACHINE, MYST_IV_REVELATION_DVD_SUBKEY, "InstallLocation", KEY_WOW64_32KEY);
	} catch (const std::system_error&) {
		// fail silently
	}
	#endif

	{
		// try for the Steam release next
		static const steampp::AppID MYST_IV_REVELATION_APP_ID = 925940;

		steampp::Steam steam;
		std::string pathString = steam.getAppInstallDir(MYST_IV_REVELATION_APP_ID);

		if (!pathString.empty()) {
			return pathString;
		}
	}

	#ifdef WINDOWS
	try {
		// try for the GOG release next
		static const CHAR MYST_IV_REVELATION_GOG_GAME_ID_SUBKEY[] = "SOFTWARE\\GOG.com\\Games\\1956555724";
		return getRegistryValueString(HKEY_LOCAL_MACHINE, MYST_IV_REVELATION_GOG_GAME_ID_SUBKEY, "path", KEY_WOW64_32KEY);
	} catch (const std::system_error&) {
		// fail silently
	}
	#endif
	return std::filesystem::current_path().string();
}

std::optional<bool> performOperation(M4Revolution &m4Revolution) {
	static const long OPERATION_OPEN_ONLINE_HELP = 1;
	static const long OPERATION_TOGGLE_FULL_SCREEN = 2;
	static const long OPERATION_TOGGLE_CAMERA_INERTIA = 3;
	static const long OPERATION_EDIT_SOUND_FADE_OUT_TIME = 4;
	static const long OPERATION_EDIT_TRANSITION_TIME = 5;
	static const long OPERATION_FIX_LOADING = 6;
	static const long OPERATION_RESTORE_BACKUP = 7;
	static const long OPERATION_EXIT = 8;
	static const long OPERATION_MIN = OPERATION_OPEN_ONLINE_HELP;
	static const long OPERATION_MAX = OPERATION_EXIT;

	try {
		switch (consoleLong("Please enter the number corresponding to the operation you would like to perform.", OPERATION_MIN, OPERATION_MAX)) {
			case OPERATION_OPEN_ONLINE_HELP:
			help();
			break;
			case OPERATION_TOGGLE_FULL_SCREEN:
			m4Revolution.toggleFullScreen();
			break;
			case OPERATION_TOGGLE_CAMERA_INERTIA:
			m4Revolution.toggleCameraInertia();
			break;
			case OPERATION_EDIT_SOUND_FADE_OUT_TIME:
			m4Revolution.editSoundFadeOutTime();
			break;
			case OPERATION_EDIT_TRANSITION_TIME:
			m4Revolution.editTransitionTime();
			break;
			case OPERATION_FIX_LOADING:
			m4Revolution.fixLoading();
			break;
			case OPERATION_RESTORE_BACKUP:
			m4Revolution.restoreBackup();
			break;
			case OPERATION_EXIT:
			return std::nullopt;
		}
	} catch (const M4Revolution::Aborted &ex) {
		consoleLog(ex.what(), 2, false, true);
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	consoleLog(VERSIONINFO_FILE_DESCRIPTION " " VERSIONINFO_VERSION);
	consoleLog("By " VERSIONINFO_COPYRIGHT_NAME, 2);

	static const int MIN_ARGC = 1;

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
		pathStringOptional.emplace(getAppInstallDir());
	}

	M4Revolution m4Revolution(pathStringOptional.value(), logFileNames, disableHardwareAcceleration, maxThreads, maxFileTasks, configurationOptional);
	std::optional<bool> performedOperationOptional = std::nullopt;

	for(;;) {
		consoleLog("This menu may be used to perform the following operations.", 2);

		consoleLog("1. Open Online Help");
		consoleLog("2. Toggle Full Screen");
		consoleLog("3. Toggle Camera Inertia");
		consoleLog("4. Edit Sound Fade Out Time");
		consoleLog("5. Edit Transition Time");
		consoleLog("6. Fix Loading");
		consoleLog("7. Restore Backup");
		consoleLog("8. Exit", 2);

		try {
			performedOperationOptional = performOperation(m4Revolution);
		} catch (const std::exception &ex) {
			consoleLog(ex.what(), 2);
			
			consoleLog("The operation has not been performed because an unknown exception occurred.", true, false, true);
			throw;
		}

		if (!performedOperationOptional.has_value()) {
			break;
		}

		consoleLog("The operation has been ", false);
		consoleLog(performedOperationOptional.value() ? "performed." : "aborted.");
		consoleWait();
	};
	return 0;
}

// "And God said, 'Let there be light,' and there was light." - Genesis 1:3