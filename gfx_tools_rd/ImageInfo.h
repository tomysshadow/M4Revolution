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

		void ComputeLODDimensions(unsigned short &textureWidth, unsigned short &textureHeight, unsigned short &volumeExtent, unsigned char lod);
		size_t GetBitsPerPixel();
		size_t GetRequestedBitsPerPixel();
	};

	struct GFX_TOOLS_RD_API ValidatedImageInfo : public ImageInfo {
		bool recompute = false;

		ValidatedImageInfo();

		ValidatedImageInfo(
			unsigned short textureWidth,
			unsigned short textureHeight,
			unsigned short volumeExtent,
			EnumPixelFormat enumPixelFormat,
			FormatHint formatHint
		);

		void MakePowerOfTwo(unsigned short &value, bool reserved = false);
		void MakeSquare(unsigned short &value, unsigned short &value2);
		void Clamp(unsigned short &value, unsigned short min, unsigned short max);
		void RecomputeLodSize(unsigned char lod);
		EnumPixelFormat OverwritePixelFormat(EnumPixelFormat enumPixelFormat);
		ValidatedImageInfo* Get();
		void SetLodSizeInBytes(unsigned char lod, size_t sizeInBytes);
		void SetDimensions(unsigned short textureWidth, unsigned short textureHeight, unsigned short volumeExtent);
		void SetNumberOfLOD(unsigned char numberOfLOD);
		EnumPixelFormat SetPixelFormat(EnumPixelFormat enumPixelFormat);
		FormatHint SetHint(FormatHint formatHint);
	};
}