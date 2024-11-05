#include "AI.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <string>

static const Ubi::BigFile::Path::VECTOR BINARIZER_LOADER_PATH_VECTOR = {
	{{"gamedata", "common"}, "common.m4b"},
	{{"common"}, "binarizer_loader.log"}
};

static const Ubi::BigFile::Path::VECTOR TRANSITION_FADE_PATH_VECTOR = {
	{{"gamedata", "common"}, "common.m4b"},
	{{"common", "ai", "aitransitionfade"}, "ai_transition_fade.ai"}
};

static const Ubi::BigFile::Path::VECTOR USER_CONTROLS_PATH_VECTOR = {
	{{"gamedata", "common"}, "common.m4b"},
	{{"common", "ai", "aiusercontrols"}, "user_controls.ai"}
};

static const Locale LOCALE("English", LC_NUMERIC);

void editF32(
	Work::Edit &edit,
	const std::string &name,
	const Ubi::BigFile::Path::VECTOR &pathVector,
	const std::string &key,
	float min,
	float max
) {
	// find the AI file
	std::fstream &fileStream = edit.fileStream;
	Ubi::BigFile::File::SIZE size = Ubi::BigFile::findFile(fileStream, pathVector)->size;
	std::streampos position = fileStream.tellg();

	std::string ai = "";
	copyStreamToString(fileStream, ai, size);

	// find the line that the value is on
	const std::regex AI_LINE("^(\\s*([^\\s\\(]+)\\s*\\(\\s*([^\\s,]+)\\s*,\\s?)(.*)\\)[^\\S\\n]*(?:\\n|$)");
	const std::string TYPE_F32 = "f32";

	size_t f32Size = 0;
	float f32 = 0.0f;
	std::smatch matches = {};

	while (!f32Size
		&& std::regex_search(ai, matches, AI_LINE)
		&& matches.length() > 4) {
		MAKE_SCOPE_EXIT(aiScopeExit) {
			const std::string &LINE = matches[0];

			position += (std::streampos)matches.prefix().length() + (std::streampos)LINE.length();
			ai = matches.suffix();
		};

		const std::string &KEY = matches[2];

		if (KEY != key) {
			continue;
		}

		const std::string &TYPE = matches[3];

		if (TYPE != TYPE_F32) {
			continue;
		}

		const std::string &VALUE_STR = matches[4];

		f32Size = stringToFloat(VALUE_STR.c_str(), f32, LOCALE);

		if (!f32Size) {
			continue;
		}

		aiScopeExit.dismiss();
	}

	if (!f32Size) {
		throw std::runtime_error("Failed to Convert String To Float");
	}

	// log the current value
	std::ostringstream outputStringStream = {};
	outputStringStream.imbue(LOCALE);
	outputStringStream << "The current " << name << " is: " << f32 << ".";
	consoleLog(outputStringStream.str().c_str());

	// we've now found the position of the number to replace
	// create a new thread to begin copying the file in the background
	// while we ask the user to input the edited value
	std::thread copyThread(Work::Edit::copyThread, std::ref(edit));

	const std::string &VALUE_STR = matches[4];

	// get the number from the user and pad it to replace the existing number
	// ensure it is not too long and will not replace the end
	std::string::size_type outputStringLength = 0;
	std::string::size_type outputStringLengthMax = VALUE_STR.length();

	do {
		if (outputStringLength) {
			consoleLog("The number is too long. Please enter a shorter number.");
		}

		outputStringStream.str("");
		outputStringStream << "Please enter the new " << name << ".";

		f32 = consoleFloat(outputStringStream.str().c_str(), min, max, LOCALE);

		// std::left is used to left align because otherwise we'll shift the number over
		outputStringStream.str("");
		outputStringStream << std::left << std::setw(f32Size) << f32;

		outputStringLength = outputStringStream.str().length();
	} while (outputStringLength > outputStringLengthMax);

	// tell the edit to the copy thread
	const std::string &VALUE_STR_PREFIX = matches[1];

	edit.join(copyThread, position + (std::streamsize)VALUE_STR_PREFIX.length(), outputStringStream.str());
}

void AI::toggleSoundFading(Work::Edit &edit) {
	std::fstream &fileStream = edit.fileStream;
	Ubi::BigFile::File::SIZE size = Ubi::BigFile::findFile(fileStream, BINARIZER_LOADER_PATH_VECTOR)->size;
	std::streampos position = fileStream.tellg();

	bool enabled = false;
	const std::string &STR = Ubi::Binary::BinarizerLoader::toggleSoundFading(edit.fileStream, size, enabled).str();

	std::thread copyThread(Work::Edit::copyThread, std::ref(edit));
	edit.join(copyThread, position, STR);

	consoleLog((std::string("Sound Fading is now ") + (enabled ? "enabled" : "disabled") + ".").c_str());
}

void AI::editTransitionTime(Work::Edit &edit) {
	editF32(edit, "Transition Time", TRANSITION_FADE_PATH_VECTOR, "m_fadingTime", 0.0f, 500.0f);
}

void AI::editMouseControls(Work::Edit &edit) {
	editF32(edit, "Free Look Inertia Level", USER_CONTROLS_PATH_VECTOR, "m_freeLookInertiaLevel", 1.0f, 100.0f);
	editF32(edit, "Screen Mode Inertia Level", USER_CONTROLS_PATH_VECTOR, "m_screenModeInertiaLevel", 1.0f, 100.0f);
}