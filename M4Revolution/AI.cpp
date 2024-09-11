#include "AI.h"
#include <regex>
#include <sstream>
#include <iomanip>

void AI::copyThread(Work::Edit &edit) {
	std::ofstream &outputFileStream = edit.outputFileStream;

	if (!edit.copied) {
		std::ifstream &inputFileStream = edit.inputFileStream;
		inputFileStream.seekg(0, std::ios::beg);

		copyStream(inputFileStream, outputFileStream);
		edit.copied = true;
	}

	edit.event.wait(true);

	outputFileStream.seekp(edit.position);
	writeStreamSafe(outputFileStream, edit.str.c_str(), edit.str.length());
}

void AI::editF32(
	Work::Edit &edit,
	const std::string &name,
	const Ubi::BigFile::Path::VECTOR &pathVector,
	const std::string &key,
	float min,
	float max
) {
	// find the AI file
	std::ifstream &inputFileStream = edit.inputFileStream;
	inputFileStream.clear();
	inputFileStream.seekg(0, std::ios::beg);

	Ubi::BigFile::File::POINTER filePointer = 0;
	std::streampos position = 0;

	for (
		Ubi::BigFile::Path::VECTOR::const_iterator pathVectorIterator = pathVector.begin();
		pathVectorIterator != pathVector.end();
		pathVectorIterator++
	) {
		Ubi::BigFile bigFile(inputFileStream, *pathVectorIterator, filePointer);

		if (!filePointer) {
			throw std::runtime_error("filePointer must not be zero");
		}

		inputFileStream.seekg(position + (std::streampos)filePointer->position);
		position = inputFileStream.tellg();
	}

	std::string ai = "";
	copyStreamToString(inputFileStream, ai, filePointer->size);

	// find the line that the value is on
	const std::regex AI_LINE("^(\\s*([^\\s\\(]+)\\s*\\(\\s*(\\w+)\\s*,\\s*)(.*)\\)[^\\S\\n]*(?:\\n|$)");
	const std::string TYPE_F32 = "f32";

	size_t f32Size = 0;
	float f32 = 0.0f;
	std::smatch matches = {};

	while (!f32Size
		&& std::regex_search(ai, matches, AI_LINE)
		&& matches.length() > 4) {
		MAKE_SCOPE_EXIT(aiScopeExit) {
			const std::string &LINE = matches[0];

			position += matches.prefix().length() + LINE.length();
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
	std::thread copyThread(AI::copyThread, std::ref(edit));

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

		outputStringStream.str("");
		outputStringStream << std::left << std::setw(f32Size) << f32;

		outputStringLength = outputStringStream.str().length();
	} while (outputStringLength > outputStringLengthMax);

	// tell the edit to the copy thread
	const std::string &VALUE_STR_PREFIX = matches[1];

	edit.position = position + (std::streamsize)VALUE_STR_PREFIX.length();
	edit.str = outputStringStream.str();
	edit.event.set();

	if (!edit.copied) {
		consoleLog("Please wait...", 2);
	}

	copyThread.join();
}

void AI::editTransitionTime(Work::Edit &edit) {
	editF32(edit, "Transition Time", TRANSITION_FADE_PATH_VECTOR, "m_fadingTime", 0.0f, 500.0f);
}

void AI::editInertiaLevels(Work::Edit &edit) {
	editF32(edit, "Free Look Inertia Level", USER_CONTROLS_PATH_VECTOR, "m_freeLookInertiaLevel", 1.0f, 100.0f);
	editF32(edit, "Screen Mode Inertia Level", USER_CONTROLS_PATH_VECTOR, "m_screenModeInertiaLevel", 1.0f, 100.0f);
}