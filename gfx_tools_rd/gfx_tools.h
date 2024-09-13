#pragma once
#include "shared.h"
#include "PixelFormat.h"

namespace gfx_tools {
	/*
	void GFX_TOOLS_RD_API Init();
	void GFX_TOOLS_RD_API Shutdown();
	*/

	GFX_TOOLS_RD_API unsigned char* ConvertHeightMapIntoDuDvBumpMap(
		unsigned long width,
		unsigned long height,
		unsigned char* inputBuffer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputStride,
		unsigned char* outputBuffer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputStride
	);

	GFX_TOOLS_RD_API unsigned char* ConvertHeightMapIntoNormalMap(
		unsigned long width,
		unsigned long height,
		unsigned char* inputBuffer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputStride,
		unsigned char* outputBuffer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputStride,
		float
	);
};