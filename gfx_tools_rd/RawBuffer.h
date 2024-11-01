#pragma once
#include "shared.h"
#include <optional>

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
		struct LoadedInfo {
			int width = 0;
			int height = 0;
			size_t stride = 0;
			float quality = 0.90f;

			GFX_TOOLS_RD_API LoadedInfo(int width, int height, size_t stride, L_INT qFactor);

			inline float getQuality(L_INT qFactor) {
				if (qFactor < 2) {
					return 1.0f;
				}
				return 1.0f - ((float)(qFactor - 2) / 253.0f);
			}
		};

		std::optional<LoadedInfo> loadedInfoOptional = std::nullopt;

		GFX_TOOLS_RD_API RawBufferEx();
		GFX_TOOLS_RD_API RawBufferEx(POINTER pointer, SIZE size, bool owner, const std::optional<LoadedInfo> &loadedInfoOptional = std::nullopt);
		GFX_TOOLS_RD_API RawBufferEx(const RawBuffer &rawBuffer, const std::optional<LoadedInfo> &loadedInfoOptional = std::nullopt);
	};
}