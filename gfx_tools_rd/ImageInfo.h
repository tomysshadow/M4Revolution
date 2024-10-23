#pragma once
#include "shared.h"
#include "PixelFormat.h"

namespace gfx_tools {
	typedef int FormatHint;

	struct GFX_TOOLS_RD_API ImageInfo {
		static const unsigned char NUMBER_OF_LOD_MAX = 15;

		size_t lodSizesInBytes[NUMBER_OF_LOD_MAX] = {};
		unsigned short textureWidth = 0;
		unsigned short textureHeight = 0;
		unsigned short requestedTextureWidth = 0;
		unsigned short requestedTextureHeight = 0;
		unsigned short volumeExtent = 0;
		unsigned char numberOfLOD = 0;
		EnumPixelFormat enumPixelFormat = EnumPixelFormat::PIXELFORMAT_UNKNOWN;
		EnumPixelFormat requestedEnumPixelFormat = EnumPixelFormat::PIXELFORMAT_UNKNOWN;
		FormatHint formatHint = 0;

		void GFX_TOOLS_RD_CALL ComputeLODDimensions(unsigned short &textureWidth, unsigned short &textureHeight, unsigned short &volumeExtent, unsigned char lod);
		size_t GFX_TOOLS_RD_CALL GetBitsPerPixel();
		size_t GFX_TOOLS_RD_CALL GetRequestedBitsPerPixel();
	};

	struct GFX_TOOLS_RD_API ValidatedImageInfo : public ImageInfo {
		bool recomputeLodSizes = false;

		ValidatedImageInfo();

		ValidatedImageInfo(
			unsigned short textureWidth,
			unsigned short textureHeight,
			unsigned short volumeExtent,
			EnumPixelFormat enumPixelFormat,
			FormatHint formatHint
		);

		void GFX_TOOLS_RD_CALL MakePowerOfTwo(unsigned short &number, bool reserved = false);
		void GFX_TOOLS_RD_CALL MakeSquare(unsigned short &number, unsigned short &number2);
		void GFX_TOOLS_RD_CALL Clamp(unsigned short &number, unsigned short min, unsigned short max);
		void GFX_TOOLS_RD_CALL RecomputeLodSize(unsigned char lod);
		EnumPixelFormat GFX_TOOLS_RD_CALL OverwritePixelFormat(EnumPixelFormat enumPixelFormat);
		ValidatedImageInfo* GFX_TOOLS_RD_CALL Get();
		void GFX_TOOLS_RD_CALL SetLodSizeInBytes(unsigned char lod, size_t sizeInBytes);
		void GFX_TOOLS_RD_CALL SetDimensions(unsigned short textureWidth, unsigned short textureHeight, unsigned short volumeExtent);
		void GFX_TOOLS_RD_CALL SetNumberOfLOD(unsigned char numberOfLOD);
		EnumPixelFormat GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);
		FormatHint GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);
	};
}