#pragma once
#define _WIN32_WINNT 0x0500
#define NOMINMAX
#include "M4Image\scope_guard.hpp"
#include <memory>
#include <stdexcept>
#include <vector>
#include <stdint.h>
#include <pixman.h>

#ifdef _WIN32
#include <windows.h>
#endif

inline unsigned char clampUCHAR(int channel) {
	return __min(UCHAR_MAX, __max(0, channel));
}

#define M4IMAGE_CALL __cdecl

#ifdef _WIN32
	#ifdef M4IMAGE_LIBRARY
		#define M4IMAGE_API __declspec(dllexport)
	#else
		#define M4IMAGE_API __declspec(dllimport)
	#endif
#else
	#define M4IMAGE_API
#endif

inline bool unrefImage(pixman_image_t* &image) {
	if (image) {
		if (!pixman_image_unref(image)) {
			return false;
		}
	}

	image = NULL;
	return true;
}