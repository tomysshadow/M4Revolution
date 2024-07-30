#pragma once
#include "shared.h"
#include "Ubi.h"

class M4Revolution {
	private:
	static const Ubi::BigFile::Path::VECTOR AI_TRANSITION_FADE_PATH_VECTOR;

	std::ifstream inputFileStream;

	public:
	M4Revolution(const std::string &inputFileName);
	M4Revolution(const M4Revolution &m4Revolution) = delete;
	M4Revolution &operator=(const M4Revolution &m4Revolution) = delete;
	void editTransitionSpeed(const std::string &outputFileName);
	void editInertia(const std::string &outputFileName);
	void fixLoading(const std::string &outputFileName);
};