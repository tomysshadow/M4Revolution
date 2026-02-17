#include "AI.h"
#include <regex>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace AI {
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
		static const std::regex AI_LINE("^(\\s*([^\\s\\(]+)\\s*\\(\\s*([^\\s,]+)\\s*,\\s?)(.*)\\)[^\\S\\n]*(?:\\n|$)");
		static const std::string TYPE_F32 = "f32";

		size_t f32Size = 0;
		float f32 = 0.0f;
		std::smatch matches = {};

		while (!f32Size
			&& std::regex_search(ai, matches, AI_LINE)
			&& matches.size() > 4) {
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
			throw std::runtime_error("f32Size must not be zero");
		}

		if (f32 < min) {
			throw std::runtime_error("f32 must be greater than or equal to min");
		}

		if (f32 > max) {
			throw std::runtime_error("f32 must be less than or equal to max");
		}

		std::ostringstream consoleOutputStringStream;
		consoleOutputStringStream.exceptions(std::ostringstream::badbit);
		consoleOutputStringStream.copyfmt(std::cout);

		// log the current value
		Work::Edit::outputCurrent(consoleOutputStringStream, name, f32);
		consoleLog(consoleOutputStringStream.str().c_str());

		// second stream so we get standard defaultfloat behaviour for the float
		// that we write to the file (no precision weirdness)
		// also, only apply the locale here
		// (use default locale for the write to the console)
		std::ostringstream fileOutputStringStream;
		fileOutputStringStream.exceptions(std::ostringstream::badbit);
		fileOutputStringStream.imbue(LOCALE);

		const std::string &VALUE_STR = matches[4];

		// get the number from the user and pad it to replace the existing number
		// ensure it is not too long and will not replace the end
		std::string::size_type fileOutputStringLength = 0;
		std::string::size_type fileOutputStringLengthMax = VALUE_STR.length();

		// we've now found the position of the number to replace
		// create a new thread to begin copying the file in the background
		// while we ask the user to input the edited value
		std::thread copyThread(Work::Edit::copyThread, std::ref(edit));

		do {
			if (fileOutputStringLength) {
				consoleLog("The number is too long. Please enter a shorter number.");
			}

			consoleOutputStringStream.str("");

			Work::Edit::outputNew(consoleOutputStringStream, name);
			f32 = consoleFloat(consoleOutputStringStream.str().c_str(), min, max, LOCALE);

			fileOutputStringStream.str("");

			// std::left is used to left align because otherwise we'll shift the number over
			fileOutputStringStream << std::left << std::setw((std::streamsize)f32Size) << f32;
			fileOutputStringLength = fileOutputStringStream.str().length();
		} while (fileOutputStringLength > fileOutputStringLengthMax);

		// tell the edit to the copy thread
		const std::string &VALUE_STR_PREFIX = matches[1];

		edit.apply(copyThread,
			
		{
			{
				position + (std::streamsize)VALUE_STR_PREFIX.length(),
				fileOutputStringStream.str()
			}
		});
	}
}