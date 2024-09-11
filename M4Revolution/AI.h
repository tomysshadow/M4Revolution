#pragma once
#include "shared.h"
#include "Ubi.h"
#include "Work.h"

namespace AI {
	static const Ubi::BigFile::Path::VECTOR TRANSITION_FADE_PATH_VECTOR = {
			{{"gamedata", "common"}, "common.m4b"},
			{{"common", "ai", "aitransitionfade"}, "ai_transition_fade.ai"}
	};

	static const Ubi::BigFile::Path::VECTOR USER_CONTROLS_PATH_VECTOR = {
			{{"gamedata", "common"}, "common.m4b"},
			{{"common", "ai", "aiusercontrols"}, "user_controls.ai"}
	};

	static const Locale LOCALE("English", LC_NUMERIC);

	void copyThread(std::ifstream &inputFileStream, const char* outputFileName, Work::Edit &edit);

	void editF32(
		std::ifstream &inputFileStream,
		const char* outputFileName,
		const std::string &name,
		const Ubi::BigFile::Path::VECTOR &pathVector,
		const std::string &key,
		float min = -FLT_MAX,
		float max = FLT_MAX,
		bool percentage = false
	);

	void editTransitionTime(std::ifstream &inputFileStream, const char* outputFileName);
};