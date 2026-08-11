#pragma once
#include "PixelFormat.h"
#include "FormatHint.h"
#include <unordered_map>
#include <M4Image.h>

namespace gfx_tools {
	using Lod = unsigned char;

	static constexpr Lod NUMBER_OF_LOD_MAX = 15;

	class ImageInfo {
		public:
		using LodSize = unsigned long;
		using Dimension = unsigned short;
		using BitsPerPixel = unsigned long;

		LodSize lodSizesInBytes[NUMBER_OF_LOD_MAX] = {};
		Dimension textureWidth = 0;
		Dimension textureHeight = 0;
		Dimension requestedTextureWidth = 0;
		Dimension requestedTextureHeight = 0;
		Dimension volumeExtent = 0;
		Lod numberOfLOD = 0;
		EnumPixelFormat enumPixelFormat = EnumPixelFormat::PIXELFORMAT_UNKNOWN;
		EnumPixelFormat requestedEnumPixelFormat = EnumPixelFormat::PIXELFORMAT_UNKNOWN;
		FormatHint formatHint = { FormatHint::HINT_NONE };

		GFX_TOOLS_API ImageInfo();
		GFX_TOOLS_API void GFX_TOOLS_CALL ComputeLODDimensions(Dimension &textureWidth, Dimension &textureHeight, Dimension &volumeExtent, Lod lod) const;
		GFX_TOOLS_API BitsPerPixel GFX_TOOLS_CALL GetBitsPerPixel() const;
		GFX_TOOLS_API BitsPerPixel GFX_TOOLS_CALL GetRequestedBitsPerPixel() const;
		GFX_TOOLS_API M4Image::COLOR_FORMAT GFX_TOOLS_CALL GetColorFormat() const;
		GFX_TOOLS_API M4Image::COLOR_FORMAT GFX_TOOLS_CALL GetRequestedColorFormat() const;

		private:
		using ColorFormatMap = std::unordered_map<EnumPixelFormat, M4Image::COLOR_FORMAT>;

		static const ColorFormatMap PIXELFORMAT_COLOR_FORMAT_MAP;
		static const ColorFormatMap PIXELFORMAT_COLOR_FORMAT_TO_88_MAP;
		static const ColorFormatMap PIXELFORMAT_COLOR_FORMAT_TO_8888_MAP;
	};

	class ValidatedImageInfo : private ImageInfo {
		private:
		bool recomputeLodSizes = false;

		void GFX_TOOLS_CALL MakePowerOfTwo(Dimension &dimension, bool reserved = false);
		void GFX_TOOLS_CALL MakeSquare(Dimension &width, Dimension &height);
		void GFX_TOOLS_CALL Clamp(Dimension &dimension, Dimension min, Dimension max);
		void GFX_TOOLS_CALL RecomputeLodSize(Lod lod);
		void GFX_TOOLS_CALL SetDimensions(Dimension textureWidth, Dimension textureHeight, Dimension volumeExtent);
		void GFX_TOOLS_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		void create(
			Dimension textureWidth,
			Dimension textureHeight,
			Dimension volumeExtent,
			EnumPixelFormat enumPixelFormat,
			FormatHint formatHint
		);

		public:
		using SizeInBytes = unsigned long;

		GFX_TOOLS_API ValidatedImageInfo();
		GFX_TOOLS_API ValidatedImageInfo(const ImageInfo &imageInfo);

		GFX_TOOLS_API ValidatedImageInfo(
			Dimension textureWidth,
			Dimension textureHeight,
			Dimension volumeExtent,
			EnumPixelFormat enumPixelFormat,
			FormatHint formatHint
		);

		GFX_TOOLS_API void GFX_TOOLS_CALL OverwritePixelFormat(EnumPixelFormat enumPixelFormat);
		GFX_TOOLS_API ImageInfo const GFX_TOOLS_CALL &Get();
		GFX_TOOLS_API Lod GFX_TOOLS_CALL GetNumberOfLOD() const;
		GFX_TOOLS_API BitsPerPixel GFX_TOOLS_CALL GetBitsPerPixel() const;
		GFX_TOOLS_API BitsPerPixel GFX_TOOLS_CALL GetRequestedBitsPerPixel() const;
		GFX_TOOLS_API void GFX_TOOLS_CALL SetLodSizeInBytes(Lod lod, SizeInBytes sizeInBytes);
		GFX_TOOLS_API void GFX_TOOLS_CALL SetNumberOfLOD(Lod numberOfLOD);
		GFX_TOOLS_API void GFX_TOOLS_CALL SetHint(FormatHint formatHint);
	};
}