#pragma once
#include "PixelFormat.h"

namespace gfx_tools {
	GFX_TOOLS_API void GFX_TOOLS_CALL Init();
	GFX_TOOLS_API void GFX_TOOLS_CALL Shutdown();

	using Dimension = unsigned long;
	using Stride = unsigned long;

	GFX_TOOLS_API void GFX_TOOLS_CALL ConvertHeightMapIntoDuDvBumpMap(
		Dimension width, Dimension height,
		unsigned char* inputPointer, EnumPixelFormat inputEnumPixelFormat, Stride inputStride,
		unsigned char* outputPointer, EnumPixelFormat outputEnumPixelFormat, Stride outputStride
	);

	GFX_TOOLS_API void GFX_TOOLS_CALL ConvertHeightMapIntoNormalMap(
		Dimension width, Dimension height,
		unsigned char* inputPointer, EnumPixelFormat inputEnumPixelFormat, Stride inputStride,
		unsigned char* outputPointer, EnumPixelFormat outputEnumPixelFormat, Stride outputStride,
		float strength
	);
};