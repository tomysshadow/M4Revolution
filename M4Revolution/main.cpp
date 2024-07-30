#include "shared.h"
#include "M4Revolution.h"
#include <chrono>
#include <iostream>

#define MAIN_OUT 2
#define MAIN_ERR true, false, true, __FILE__, __LINE__

int main(int argc, char** argv) {
	consoleLog("Myst IV: Revolution 1.0.0");
	consoleLog("By Anthony Kleine", 2);

	// TODO TODO
	const int MIN_ARGC = 2;

	if (argc < MIN_ARGC) {
		return 1;
	}

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	M4Revolution m4Revolution(argv[1]);
	m4Revolution.fixLoading("data_out.m4b");

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Elapsed Seconds: " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	return 0;
}