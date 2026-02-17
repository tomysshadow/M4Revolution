#pragma once
#include "utils.h"
#include "Work.h"

namespace AI {
	void editF32(
		Work::Edit &edit,
		const Ubi::BigFile::Path::VECTOR &pathVector,
		const std::string &name,
		const std::string &key,
		float min,
		float max
	);
};