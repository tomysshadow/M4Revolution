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
		struct ResizeInfo {
			int width = 0;
			int height = 0;
			size_t stride = 0;
			float quality = 0.90f;

			GFX_TOOLS_RD_API ResizeInfo();
			GFX_TOOLS_RD_API ResizeInfo(int width, int height, size_t stride, L_INT qFactor);

			static inline float getQuality(L_INT qFactor) {
				const L_INT Q_FACTOR_HIGHEST_QUALITY = 2;
				const L_INT Q_FACTOR_MOST_COMPRESSION = 255;

				if (qFactor < Q_FACTOR_HIGHEST_QUALITY) {
					return 1.0f;
				}
				return 1.0f - ((float)(qFactor - Q_FACTOR_HIGHEST_QUALITY) / (Q_FACTOR_MOST_COMPRESSION - Q_FACTOR_HIGHEST_QUALITY));
			}
		};

		std::optional<ResizeInfo> resizeInfoOptional = std::nullopt;

		GFX_TOOLS_RD_API RawBufferEx();
		GFX_TOOLS_RD_API RawBufferEx(POINTER pointer, SIZE size, bool owner, const std::optional<ResizeInfo> &resizeInfoOptional = std::nullopt);
		GFX_TOOLS_RD_API RawBufferEx(const RawBuffer &rawBuffer, const std::optional<ResizeInfo> &resizeInfoOptional = std::nullopt);
	};
}