#pragma once
#define _WIN32_WINNT 0x0500
#define NOMINMAX

#include <stdexcept>
#include <stdlib.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define M4IMAGE_API
#define M4IMAGE_CALL __cdecl

inline constexpr unsigned char clampToUCHAR(unsigned short number) {
	return __min(UCHAR_MAX, __max(0, number));
}

// forward declared so Pixman doesn't need to be included here
// (so you only need the static lib to include M4Image in another project)
extern "C" {
	typedef int pixman_bool_t;
	typedef union pixman_image pixman_image_t;
	pixman_bool_t pixman_image_unref(pixman_image_t* image);
}

inline bool unrefImage(pixman_image_t* &image) {
	if (image) {
		if (!pixman_image_unref(image)) {
			return false;
		}
	}

	image = NULL;
	return true;
}