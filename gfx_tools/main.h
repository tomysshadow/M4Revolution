#pragma once
#include "shared.h"
#include "PixelFormat.h"

namespace gfx_tools {
	GFX_TOOLS_API void GFX_TOOLS_CALL Init();
	GFX_TOOLS_API void GFX_TOOLS_CALL Shutdown();

	typedef unsigned long DIMENSION;
	typedef unsigned long STRIDE;

	GFX_TOOLS_API void GFX_TOOLS_CALL ConvertHeightMapIntoDuDvBumpMap(
		DIMENSION width, DIMENSION height,
		unsigned char* inputPointer, EnumPixelFormat inputEnumPixelFormat, STRIDE inputStride,
		unsigned char* outputPointer, EnumPixelFormat outputEnumPixelFormat, STRIDE outputStride
	);

	GFX_TOOLS_API void GFX_TOOLS_CALL ConvertHeightMapIntoNormalMap(
		DIMENSION width, DIMENSION height,
		unsigned char* inputPointer, EnumPixelFormat inputEnumPixelFormat, STRIDE inputStride,
		unsigned char* outputPointer, EnumPixelFormat outputEnumPixelFormat, STRIDE outputStride,
		float strength
	);
};