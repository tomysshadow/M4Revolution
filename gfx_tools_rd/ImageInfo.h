#pragma once
#include "shared.h"
#include "PixelFormat.h"
#include "FormatHint.h"
#include <map>
#include <M4Image/M4Image.h>

namespace gfx_tools {
	typedef unsigned char LOD;

	static const LOD NUMBER_OF_LOD_MAX = 15;

	class ImageInfo {
		public:
		typedef unsigned long LOD_SIZE;
		typedef unsigned short DIMENSION;
		typedef unsigned long BITS_PER_PIXEL;

		LOD_SIZE lodSizesInBytes[NUMBER_OF_LOD_MAX] = {};
		DIMENSION textureWidth = 0;
		DIMENSION textureHeight = 0;
		DIMENSION requestedTextureWidth = 0;
		DIMENSION requestedTextureHeight = 0;
		DIMENSION volumeExtent = 0;
		LOD numberOfLOD = 0;
		EnumPixelFormat enumPixelFormat = EnumPixelFormat::PIXELFORMAT_UNKNOWN;
		EnumPixelFormat requestedEnumPixelFormat = EnumPixelFormat::PIXELFORMAT_UNKNOWN;
		FormatHint formatHint = { FormatHint::HINT_NONE };

		GFX_TOOLS_RD_API ImageInfo();
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL ComputeLODDimensions(DIMENSION &textureWidth, DIMENSION &textureHeight, DIMENSION &volumeExtent, LOD lod) const;
		GFX_TOOLS_RD_API BITS_PER_PIXEL GFX_TOOLS_RD_CALL GetBitsPerPixel() const;
		GFX_TOOLS_RD_API BITS_PER_PIXEL GFX_TOOLS_RD_CALL GetRequestedBitsPerPixel() const;
		GFX_TOOLS_RD_API M4Image::COLOR_FORMAT GFX_TOOLS_RD_CALL GetColorFormat() const;
		GFX_TOOLS_RD_API M4Image::COLOR_FORMAT GFX_TOOLS_RD_CALL GetRequestedColorFormat() const;

		private:
		typedef std::map<EnumPixelFormat, M4Image::COLOR_FORMAT> COLOR_FORMAT_MAP;

		static const COLOR_FORMAT_MAP PIXELFORMAT_COLOR_FORMAT_MAP;
		static const COLOR_FORMAT_MAP PIXELFORMAT_COLOR_FORMAT_8_TO_16_MAP;
		static const COLOR_FORMAT_MAP PIXELFORMAT_COLOR_FORMAT_8_TO_32_MAP;
	};

	class ValidatedImageInfo : private ImageInfo {
		private:
		bool recomputeLodSizes = false;

		void GFX_TOOLS_RD_CALL MakePowerOfTwo(DIMENSION &dimension, bool reserved = false);
		void GFX_TOOLS_RD_CALL MakeSquare(DIMENSION &width, DIMENSION &height);
		void GFX_TOOLS_RD_CALL Clamp(DIMENSION &dimension, DIMENSION min, DIMENSION max);
		void GFX_TOOLS_RD_CALL RecomputeLodSize(LOD lod);
		void GFX_TOOLS_RD_CALL SetDimensions(DIMENSION textureWidth, DIMENSION textureHeight, DIMENSION volumeExtent);
		void GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		void create(
			DIMENSION textureWidth,
			DIMENSION textureHeight,
			DIMENSION volumeExtent,
			EnumPixelFormat enumPixelFormat,
			FormatHint formatHint
		);

		public:
		typedef unsigned long SIZE_IN_BYTES;

		GFX_TOOLS_RD_API ValidatedImageInfo();
		GFX_TOOLS_RD_API ValidatedImageInfo(const ImageInfo &imageInfo);

		GFX_TOOLS_RD_API ValidatedImageInfo(
			DIMENSION textureWidth,
			DIMENSION textureHeight,
			DIMENSION volumeExtent,
			EnumPixelFormat enumPixelFormat,
			FormatHint formatHint
		);

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL OverwritePixelFormat(EnumPixelFormat enumPixelFormat);
		GFX_TOOLS_RD_API ImageInfo const GFX_TOOLS_RD_CALL &Get();
		GFX_TOOLS_RD_API LOD GFX_TOOLS_RD_CALL GetNumberOfLOD() const;
		GFX_TOOLS_RD_API BITS_PER_PIXEL GFX_TOOLS_RD_CALL GetBitsPerPixel() const;
		GFX_TOOLS_RD_API BITS_PER_PIXEL GFX_TOOLS_RD_CALL GetRequestedBitsPerPixel() const;
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetLodSizeInBytes(LOD lod, SIZE_IN_BYTES sizeInBytes);
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetNumberOfLOD(LOD numberOfLOD);
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);
	};
}