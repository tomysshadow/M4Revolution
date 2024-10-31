#include "FormatHint.h"

namespace gfx_tools {
	EnumPixelFormat FormatHint::GetPixelFormat(uint32_t bits, bool hasAlpha) const {
		switch (bits) {
			case 8:
			{
				// try to find the pixel format from the hint
				// but if we don't recognize the hint it is an unknown format
				HINT_PIXELFORMAT_MAP::const_iterator hintPixelFormatMapIterator = HINT_PIXELFORMAT_8_MAP.find(hint);

				if (hintPixelFormatMapIterator != HINT_PIXELFORMAT_8_MAP.end()) {
					return hintPixelFormatMapIterator->second;
				}
			}
			break;
			case 16:
			return PIXELFORMAT_AL_88;
			case 32:
			return hasAlpha ? PIXELFORMAT_ARGB_8888 : PIXELFORMAT_XRGB_8888;
		}
		return EnumPixelFormat::PIXELFORMAT_UNKNOWN;
	}

	const FormatHint::HINT_PIXELFORMAT_MAP FormatHint::HINT_PIXELFORMAT_8_MAP = {
		{HINT_NONE, PIXELFORMAT_XRGB_8888},
		{HINT_ALPHA, PIXELFORMAT_A_8},
		{HINT_LUMINANCE, PIXELFORMAT_L_8}
	};
}