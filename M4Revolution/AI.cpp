#include "AI.h"
#include <regex>
#include <sstream>
#include <iomanip>

namespace AI {
	static const Ubi::BigFile::Path::VECTOR TRANSITION_FADE_PATH_VECTOR = {
		{{"gamedata", "common"}, "common.m4b"},
		{{"common", "ai", "aitransitionfade"}, "ai_transition_fade.ai"}
	};

	static const Ubi::BigFile::Path::VECTOR BINARIZER_LOADER_PATH_VECTOR = {
		{{"gamedata", "common"}, "common.m4b"},
		{{"common"}, "binarizer_loader.log"}
	};

	static const Locale LOCALE("English", LC_NUMERIC);

	Ubi::BigFile::File::SIZE findFileSize(Work::Edit &edit, const Ubi::BigFile::Path::VECTOR &pathVector) {
		return Ubi::BigFile::findFile(edit.fileStream, pathVector)->size;
	}

	void editF32(
		Work::Edit &edit,
		const Ubi::BigFile::Path::VECTOR &pathVector,
		const std::string &name,
		const std::string &key,
		float min,
		float max
	) {
		std::fstream &fileStream = edit.fileStream;

		Ubi::BigFile::File::SIZE size = findFileSize(edit, pathVector);
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
		std::ostringstream outputStringStream;
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

	void toggleResource(
		Work::Edit &edit,
		const Ubi::BigFile::Path::VECTOR &pathVector,
		const std::string &name,
		const std::string &key
	) {
		std::fstream &fileStream = edit.fileStream;

		Ubi::BigFile::File::SIZE size = findFileSize(edit, pathVector);
		std::streampos position = fileStream.tellg();

		std::ostringstream outputStringStream;

		Ubi::Binary::BinarizerLoader::toggleResource(
			fileStream,
			size,
			name,
			key,
			outputStringStream
		);

		std::thread copyThread(Work::Edit::copyThread, std::ref(edit));
		edit.join(copyThread, position, outputStringStream.str());
	}

	void editTransitionTime(std::fstream &fileStream) {
		Work::Edit edit(fileStream, Work::Output::DATA_PATH);

		editF32(edit, TRANSITION_FADE_PATH_VECTOR, "Transition Time", "m_fadingTime", 0.0f, 500.0f);
	}

	void toggleSoundFading(std::fstream &fileStream) {
		Work::Edit edit(fileStream, Work::Output::DATA_PATH);

		toggleResource(edit, BINARIZER_LOADER_PATH_VECTOR, "Sound Fading", "/common/ai/aisndtransition/ai_snd_transition.ai");
	}
}