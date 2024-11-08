#pragma once
#include "shared.h"
#include "Work.h"

namespace AI {
	void toggleSoundFading(std::fstream &fileStream);
	void editTransitionTime(std::fstream &fileStream);
	void editMouseControls(std::fstream &fileStream);
};