#include "pch.h"
#include "FormatHint.h"

namespace gfx_tools {
	EnumPixelFormat FormatHint::GetEnumPixelFormat(bool isAlpha, uint32_t bits) const {
		switch (bits) {
			case 8:
			{
				// try to find the pixel format from the hint
				// but if we don't recognize the hint it is an unknown format
				if ((HintPixelFormatArray::size_type)hint < HINT_PIXELFORMAT_8_ARRAY.size()) {
					return HINT_PIXELFORMAT_8_ARRAY[hint];
				}
			}
			break;
			case 16:
			return PIXELFORMAT_AL_88;
			case 32:
			return isAlpha ? PIXELFORMAT_ARGB_8888 : PIXELFORMAT_XRGB_8888;
		}
		return PIXELFORMAT_UNKNOWN;
	}
}