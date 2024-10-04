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

	void copyThread(Work::Edit &edit);

	void editF32(
		Work::Edit &edit,
		const std::string &name,
		const Ubi::BigFile::Path::VECTOR &pathVector,
		const std::string &key,
		float min = -FLT_MAX,
		float max = FLT_MAX
	);

	void editTransitionTime(Work::Edit &edit);
	void editMouseControls(Work::Edit &edit);
};