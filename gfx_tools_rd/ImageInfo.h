#pragma once
#include "shared.h"
#include "PixelFormat.h"

namespace gfx_tools {
	union FormatHint {
		int value = 0;
	};

	static const FormatHint FORMAT_HINT_NONE = { 0 };
	static const FormatHint FORMAT_HINT_ALPHA = { 1 };
	static const FormatHint FORMAT_HINT_LUMINANCE = { 2 };

	typedef unsigned char LOD;

	static const LOD NUMBER_OF_LOD_MAX = 15;

	class GFX_TOOLS_RD_API ImageInfo {
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
		FormatHint formatHint = { 0 };

		void GFX_TOOLS_RD_CALL ComputeLODDimensions(DIMENSION &textureWidth, DIMENSION &textureHeight, DIMENSION &volumeExtent, LOD lod) const;
		BITS_PER_PIXEL GFX_TOOLS_RD_CALL GetBitsPerPixel() const;
		BITS_PER_PIXEL GFX_TOOLS_RD_CALL GetRequestedBitsPerPixel() const;
	};

	class GFX_TOOLS_RD_API ValidatedImageInfo : public ImageInfo {
		private:
		bool recomputeLodSizes = false;

		void GFX_TOOLS_RD_CALL MakePowerOfTwo(DIMENSION &dimension, bool reserved = false);
		void GFX_TOOLS_RD_CALL MakeSquare(DIMENSION &width, DIMENSION &height);
		void GFX_TOOLS_RD_CALL Clamp(DIMENSION &dimension, DIMENSION min, DIMENSION max);
		void GFX_TOOLS_RD_CALL RecomputeLodSize(LOD lod);
		void GFX_TOOLS_RD_CALL SetDimensions(DIMENSION textureWidth, DIMENSION textureHeight, DIMENSION volumeExtent);
		void GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		public:
		typedef unsigned long SIZE_IN_BYTES;

		ValidatedImageInfo();

		ValidatedImageInfo(
			DIMENSION textureWidth,
			DIMENSION textureHeight,
			DIMENSION volumeExtent,
			EnumPixelFormat enumPixelFormat,
			FormatHint formatHint
		);

		void GFX_TOOLS_RD_CALL OverwritePixelFormat(EnumPixelFormat enumPixelFormat);
		ImageInfo const GFX_TOOLS_RD_CALL &Get();
		void GFX_TOOLS_RD_CALL SetLodSizeInBytes(LOD lod, SIZE_IN_BYTES sizeInBytes);
		void GFX_TOOLS_RD_CALL SetNumberOfLOD(LOD numberOfLOD);
		void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);
	};
}