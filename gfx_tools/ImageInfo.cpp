#include "pch.h"
#include "ImageInfo.h"
#include "Configuration.h"
#include <stdlib.h>

namespace gfx_tools {
	ImageInfo::ImageInfo() {
	}

	void ImageInfo::ComputeLODDimensions(Dimension &textureWidth, Dimension &textureHeight, Dimension &volumeExtent, Lod lod) const {
		static constexpr Dimension MIN_DIMENSION = 1;

		textureWidth = (Dimension)(this->textureWidth >> lod);
		textureHeight = (Dimension)(this->textureHeight >> lod);
		volumeExtent = (Dimension)(this->volumeExtent >> lod);

		if (!textureWidth) {
			textureWidth = MIN_DIMENSION;
		}

		if (!textureHeight) {
			textureHeight = MIN_DIMENSION;
		}

		if (!volumeExtent) {
			volumeExtent = MIN_DIMENSION;
		}
	}

	ImageInfo::BitsPerPixel ImageInfo::GetBitsPerPixel() const {
		return PixelFormat::GetPixelFormat(enumPixelFormat)->GetBitsPerPixel();
	}

	ImageInfo::BitsPerPixel ImageInfo::GetRequestedBitsPerPixel() const {
		return PixelFormat::GetPixelFormat(requestedEnumPixelFormat)->GetBitsPerPixel();
	}

	// remember that the "requested" pixel format refers to the
	// pixel format before it was corrected, that is, of the input image (as loaded by M4Image)
	// (similar to requestedTextureWidth/Height)
	// so enumPixelFormat determines the actual output format (that the GPU wants)
	// but we only support these specific conversions
	// frankly, it doesn't make sense, but it's what the game does
	// so I try to rationalize it
	M4Image::COLOR_FORMAT ImageInfo::GetColorFormat() const {
		if (enumPixelFormat == requestedEnumPixelFormat) {
			return PIXELFORMAT_COLOR_FORMAT_MAP.at(requestedEnumPixelFormat);
		}

		if (enumPixelFormat == PIXELFORMAT_AL_88) {
			return PIXELFORMAT_COLOR_FORMAT_TO_88_MAP.at(requestedEnumPixelFormat);
		}
		return PIXELFORMAT_COLOR_FORMAT_TO_8888_MAP.at(requestedEnumPixelFormat);
	}

	M4Image::COLOR_FORMAT ImageInfo::GetRequestedColorFormat() const {
		return PIXELFORMAT_COLOR_FORMAT_MAP.at(requestedEnumPixelFormat);
	}

	const ImageInfo::ColorFormatMap ImageInfo::PIXELFORMAT_COLOR_FORMAT_MAP = {
		{PIXELFORMAT_ARGB_8888, M4Image::COLOR_FORMAT::BGRA},
		{PIXELFORMAT_XRGB_8888, M4Image::COLOR_FORMAT::BGRX},
		{PIXELFORMAT_RGB_888, M4Image::COLOR_FORMAT::BGR},
		{PIXELFORMAT_AL_88, M4Image::COLOR_FORMAT::LA},
		{PIXELFORMAT_A_8, M4Image::COLOR_FORMAT::L},
		{PIXELFORMAT_L_8, M4Image::COLOR_FORMAT::L}
	};

	// mappings to convert non-16bit formats into 16-bit
	// 8-bit input is assumed to be luminance
	// 32-bit input not included here (would be strange for GPU to ask for alpha/luminance only from them)
	const ImageInfo::ColorFormatMap ImageInfo::PIXELFORMAT_COLOR_FORMAT_TO_88_MAP = {
		{PIXELFORMAT_A_8, M4Image::COLOR_FORMAT::AL}, // L -> L(A)
		{PIXELFORMAT_L_8, M4Image::COLOR_FORMAT::LA}, // L -> (L)A
	};

	// mappings to convert non-32bit formats into 32-bit
	// 8-bit input is assumed to be luminance
	const ImageInfo::ColorFormatMap ImageInfo::PIXELFORMAT_COLOR_FORMAT_TO_8888_MAP = {
		{PIXELFORMAT_AL_88, M4Image::COLOR_FORMAT::BGRA}, // fixes dgVoodoo2 compatibility (doesn't support 16-bit textures)
		{PIXELFORMAT_A_8, M4Image::COLOR_FORMAT::XXXL}, // L -> BGR(A)
		{PIXELFORMAT_L_8, M4Image::COLOR_FORMAT::BGRA} // L -> (BGR)A
	};

	void ValidatedImageInfo::MakePowerOfTwo(Dimension &dimension, bool reserved) {
		Dimension powerOfTwo = 2;

		// if number is one, we want it to just become two
		// so don't allow powerOfTwo to be downscaled to one
		if (dimension > powerOfTwo) {
			do {
				powerOfTwo *= 2;
			} while (dimension > powerOfTwo);

			if (dimension == powerOfTwo) {
				return;
			}

			if (!Configuration::Get().toNext) {
				powerOfTwo /= 2;
			}
		}

		dimension = powerOfTwo;
	}

	void ValidatedImageInfo::MakeSquare(Dimension &width, Dimension &height) {
		Dimension square = width;

		if (Configuration::Get().toNext) {
			if (square < height) {
				square = height;
			}
		} else {
			if (square > height) {
				square = height;
			}
		}

		width = square;
		height = square;
	}

	void ValidatedImageInfo::Clamp(Dimension &dimension, Dimension min, Dimension max) {
		dimension = __min(max, __max(min, dimension));
	}

	void ValidatedImageInfo::RecomputeLodSize(Lod lod) {
		static constexpr BitsPerPixel BYTES = 3;

		ComputeLODDimensions(textureWidth, textureHeight, volumeExtent, lod);
		lodSizesInBytes[lod] = textureWidth * textureHeight * volumeExtent * (GetBitsPerPixel() >> BYTES);
	}

	void ValidatedImageInfo::SetDimensions(Dimension textureWidth, Dimension textureHeight, Dimension volumeExtent) {
		const Configuration &configuration = Configuration::Get();

		this->textureWidth = textureWidth;
		this->requestedTextureWidth = textureWidth;
		this->textureHeight = textureHeight;
		this->requestedTextureHeight = textureHeight;
		this->volumeExtent = volumeExtent;

		if (configuration.dimensionsMakePowerOfTwo) {
			MakePowerOfTwo(this->textureWidth);
			MakePowerOfTwo(this->textureHeight);
			MakePowerOfTwo(this->volumeExtent);
		}

		if (configuration.dimensionsMakeSquare) {
			MakeSquare(this->textureWidth, this->textureHeight);
		}

		Clamp(this->textureWidth, (Dimension)configuration.minTextureWidth, (Dimension)configuration.maxTextureWidth);
		Clamp(this->textureHeight, (Dimension)configuration.minTextureHeight, (Dimension)configuration.maxTextureHeight);
		Clamp(this->volumeExtent, (Dimension)configuration.minVolumeExtent, (Dimension)configuration.maxVolumeExtent);

		recomputeLodSizes = recomputeLodSizes
			|| this->textureWidth != textureWidth
			|| this->textureHeight != textureHeight
			|| this->volumeExtent != volumeExtent;

		if (recomputeLodSizes) {
			for (Lod i = 0; i < NUMBER_OF_LOD_MAX; i++) {
				if (lodSizesInBytes[i]) {
					RecomputeLodSize(i);
				}
			}
		}
	}

	void ValidatedImageInfo::SetPixelFormat(EnumPixelFormat enumPixelFormat) {
		if (PixelFormat::GetPixelFormat(enumPixelFormat)) {
			this->enumPixelFormat = enumPixelFormat;
			this->requestedEnumPixelFormat = enumPixelFormat;
			return;
		}

		static constexpr EnumPixelFormat DEFAULT_ENUM_PIXELFORMAT = PIXELFORMAT_ARGB_8888;

		recomputeLodSizes = true;
		this->enumPixelFormat = DEFAULT_ENUM_PIXELFORMAT;
		this->requestedEnumPixelFormat = DEFAULT_ENUM_PIXELFORMAT;
	}

	void ValidatedImageInfo::create(
		Dimension textureWidth,
		Dimension textureHeight,
		Dimension volumeExtent,
		EnumPixelFormat enumPixelFormat,
		FormatHint formatHint
	) {
		SetPixelFormat(enumPixelFormat);
		SetDimensions(textureWidth, textureHeight, volumeExtent);
		SetHint(formatHint);
	}

	ValidatedImageInfo::ValidatedImageInfo() {
	}

	ValidatedImageInfo::ValidatedImageInfo(const ImageInfo &imageInfo) {
		create(imageInfo.textureWidth, imageInfo.textureHeight, imageInfo.volumeExtent, imageInfo.enumPixelFormat, imageInfo.formatHint);
	}

	ValidatedImageInfo::ValidatedImageInfo(
		Dimension textureWidth,
		Dimension textureHeight,
		Dimension volumeExtent,
		EnumPixelFormat enumPixelFormat,
		FormatHint formatHint
	) {
		create(textureWidth, textureHeight, volumeExtent, enumPixelFormat, formatHint);
	}

	void ValidatedImageInfo::OverwritePixelFormat(EnumPixelFormat enumPixelFormat) {
		this->enumPixelFormat = enumPixelFormat;
	}

	ImageInfo const &ValidatedImageInfo::Get() {
		return *this;
	}

	Lod ValidatedImageInfo::GetNumberOfLOD() const {
		return numberOfLOD;
	}

	ImageInfo::BitsPerPixel ValidatedImageInfo::GetBitsPerPixel() const {
		return ImageInfo::GetBitsPerPixel();
	}

	ImageInfo::BitsPerPixel ValidatedImageInfo::GetRequestedBitsPerPixel() const {
		return ImageInfo::GetRequestedBitsPerPixel();
	}

	void ValidatedImageInfo::SetLodSizeInBytes(Lod lod, SizeInBytes sizeInBytes) {
		if (recomputeLodSizes && sizeInBytes) {
			RecomputeLodSize(lod);
			return;
		}

		lodSizesInBytes[lod] = sizeInBytes;
	}

	void ValidatedImageInfo::SetNumberOfLOD(Lod numberOfLOD) {
		this->numberOfLOD = numberOfLOD;
	}

	void ValidatedImageInfo::SetHint(FormatHint formatHint) {
		this->formatHint = formatHint;
	}
}