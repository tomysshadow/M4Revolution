#include "shared.h"
#include "M4Revolution.h"

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

	M4Revolution m4Revolution(argv[1]);
	m4Revolution.fixLoading("data_out.m4b");
	return 0;
}