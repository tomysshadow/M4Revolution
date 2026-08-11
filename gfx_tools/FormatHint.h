#pragma once
#include "PixelFormat.h"
#include <array>

namespace gfx_tools {
	union FormatHint {
		static constexpr int HINT_NONE = 0;
		static constexpr int HINT_ALPHA = 1;
		static constexpr int HINT_LUMINANCE = 2;

		int hint : 3;

		EnumPixelFormat GetEnumPixelFormat(bool isAlpha, uint32_t bits) const;

		private:
		using HintPixelFormatArray = std::array<EnumPixelFormat, 3>;

		static constexpr HintPixelFormatArray HINT_PIXELFORMAT_8_ARRAY = {
			PIXELFORMAT_XRGB_8888, // None
			PIXELFORMAT_A_8, // Alpha
			PIXELFORMAT_L_8 // Luminance
		};
	};
}