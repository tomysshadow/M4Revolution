#pragma once
#include "shared.h"

namespace gfx_tools {
	struct RawBuffer {
		typedef unsigned char* POINTER;
		typedef unsigned long SIZE;

		POINTER pointer = 0;
		SIZE size = 0;
		bool owner = true;

		GFX_TOOLS_RD_API RawBuffer();
		GFX_TOOLS_RD_API RawBuffer(POINTER pointer, SIZE size, bool owner);
	};

	struct RawBufferEx : public RawBuffer {
		// zero if image is compressed, non-zero if uncompressed
		size_t stride = 0;
		float quality = 0.90f;

		GFX_TOOLS_RD_API RawBufferEx();
		GFX_TOOLS_RD_API RawBufferEx(POINTER pointer, SIZE size, bool owner, size_t stride = 0, L_INT qFactor = 0);
		GFX_TOOLS_RD_API RawBufferEx(const RawBuffer &rawBuffer, size_t stride = 0, L_INT qFactor = 0);

		inline float getQuality(L_INT qFactor) {
			if (qFactor < 2) {
				return 1.0f;
			}
			return 1.0f - ((float)(qFactor - 2) / 253.0f);
		}
	};
}