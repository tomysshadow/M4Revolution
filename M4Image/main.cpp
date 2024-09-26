#include "M4Image\shared.h"

#ifdef _WIN32
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);
	}
	return TRUE;
}
#endif