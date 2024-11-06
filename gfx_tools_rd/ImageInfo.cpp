#include "ImageInfo.h"
#include "Configuration.h"
#include <stdlib.h>

namespace gfx_tools {
	ImageInfo::ImageInfo() {
	}

	void ImageInfo::ComputeLODDimensions(DIMENSION &textureWidth, DIMENSION &textureHeight, DIMENSION &volumeExtent, LOD lod) const {
		const DIMENSION MIN_DIMENSION = 1;

		textureWidth = this->textureWidth >> lod;
		textureHeight = this->textureHeight >> lod;
		volumeExtent = this->volumeExtent >> lod;

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

	ImageInfo::BITS_PER_PIXEL ImageInfo::GetBitsPerPixel() const {
		return PixelFormat::GetPixelFormat(enumPixelFormat)->GetBitsPerPixel();
	}

	ImageInfo::BITS_PER_PIXEL ImageInfo::GetRequestedBitsPerPixel() const {
		return PixelFormat::GetPixelFormat(requestedEnumPixelFormat)->GetBitsPerPixel();
	}

	// if the two pixel formats are equivalent
	// then we allow BGRA/BGRX, BGR, LA, or L (no conversion happens)
	// if the pixel format must be 16-bit, we convert it to LA or AL
	// if the requested pixel format is 8-bit, we only convert to BGRA or XXXL
	// remember that the "requested" pixel format refers to the
	// pixel format before it was corrected, that is, of the original image
	// (similar to requestedTextureWidth/Height)
	// so enumPixelFormat determines the actual output format
	// but we only support these specific conversions
	// frankly, it doesn't make sense, but it's what the game does
	// so I try to rationalize it
	M4Image::COLOR_FORMAT ImageInfo::GetColorFormat() const {
		// equivalent
		if (enumPixelFormat == requestedEnumPixelFormat) {
			return PIXELFORMAT_COLOR_FORMAT_MAP.at(requestedEnumPixelFormat);
		}

		// 16-bit
		if (enumPixelFormat == PIXELFORMAT_AL_88) {
			return PIXELFORMAT_COLOR_FORMAT_8_TO_16_MAP.at(requestedEnumPixelFormat);
		}
		return PIXELFORMAT_COLOR_FORMAT_8_TO_32_MAP.at(requestedEnumPixelFormat); // 8-bit
	}

	M4Image::COLOR_FORMAT ImageInfo::GetRequestedColorFormat() const {
		return PIXELFORMAT_COLOR_FORMAT_MAP.at(requestedEnumPixelFormat);
	}

	const ImageInfo::COLOR_FORMAT_MAP ImageInfo::PIXELFORMAT_COLOR_FORMAT_MAP = {
		{PIXELFORMAT_ARGB_8888, M4Image::COLOR_FORMAT::BGRA32},
		{PIXELFORMAT_XRGB_8888, M4Image::COLOR_FORMAT::BGRX32},
		{PIXELFORMAT_RGB_888, M4Image::COLOR_FORMAT::BGR24},
		{PIXELFORMAT_AL_88, M4Image::COLOR_FORMAT::LA16},
		{PIXELFORMAT_A_8, M4Image::COLOR_FORMAT::L8},
		{PIXELFORMAT_L_8, M4Image::COLOR_FORMAT::L8}
	};

	const ImageInfo::COLOR_FORMAT_MAP ImageInfo::PIXELFORMAT_COLOR_FORMAT_8_TO_16_MAP = {
		{PIXELFORMAT_A_8, M4Image::COLOR_FORMAT::AL16},
		{PIXELFORMAT_L_8, M4Image::COLOR_FORMAT::LA16}
	};

	const ImageInfo::COLOR_FORMAT_MAP ImageInfo::PIXELFORMAT_COLOR_FORMAT_8_TO_32_MAP = {
		{PIXELFORMAT_A_8, M4Image::COLOR_FORMAT::XXXL32},
		{PIXELFORMAT_L_8, M4Image::COLOR_FORMAT::BGRA32}
	};

	void ValidatedImageInfo::MakePowerOfTwo(DIMENSION &dimension, bool reserved) {
		DIMENSION powerOfTwo = 2;

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

	void ValidatedImageInfo::MakeSquare(DIMENSION &width, DIMENSION &height) {
		DIMENSION square = width;

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

	void ValidatedImageInfo::Clamp(DIMENSION &dimension, DIMENSION min, DIMENSION max) {
		dimension = __min(max, __max(dimension, min));
	}

	void ValidatedImageInfo::RecomputeLodSize(LOD lod) {
		const BITS_PER_PIXEL BYTES = 3;

		ComputeLODDimensions(textureWidth, textureHeight, volumeExtent, lod);
		lodSizesInBytes[lod] = textureWidth * textureHeight * volumeExtent * (GetBitsPerPixel() >> BYTES);
	}

	void ValidatedImageInfo::SetDimensions(DIMENSION textureWidth, DIMENSION textureHeight, DIMENSION volumeExtent) {
		const Configuration &CONFIGURATION = Configuration::Get();

		this->textureWidth = textureWidth;
		this->requestedTextureWidth = textureWidth;
		this->textureHeight = textureHeight;
		this->requestedTextureHeight = textureHeight;
		this->volumeExtent = volumeExtent;

		if (CONFIGURATION.dimensionsMakePowerOfTwo) {
			MakePowerOfTwo(this->textureWidth);
			MakePowerOfTwo(this->textureHeight);
			MakePowerOfTwo(this->volumeExtent);
		}

		if (CONFIGURATION.dimensionsMakeSquare) {
			MakeSquare(this->textureWidth, this->textureHeight);
		}

		Clamp(this->textureWidth, (DIMENSION)CONFIGURATION.minTextureWidth, (DIMENSION)CONFIGURATION.maxTextureWidth);
		Clamp(this->textureHeight, (DIMENSION)CONFIGURATION.minTextureHeight, (DIMENSION)CONFIGURATION.maxTextureHeight);
		Clamp(this->volumeExtent, (DIMENSION)CONFIGURATION.minVolumeExtent, (DIMENSION)CONFIGURATION.maxVolumeExtent);

		recomputeLodSizes = recomputeLodSizes
			|| this->textureWidth != textureWidth
			|| this->textureHeight != textureHeight
			|| this->volumeExtent != volumeExtent;

		if (recomputeLodSizes) {
			for (LOD i = 0; i < NUMBER_OF_LOD_MAX; i++) {
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

		const EnumPixelFormat DEFAULT_ENUM_PIXEL_FORMAT = PIXELFORMAT_ARGB_8888;

		recomputeLodSizes = true;
		this->enumPixelFormat = DEFAULT_ENUM_PIXEL_FORMAT;
		this->requestedEnumPixelFormat = DEFAULT_ENUM_PIXEL_FORMAT;
	}

	void ValidatedImageInfo::create(
		DIMENSION textureWidth,
		DIMENSION textureHeight,
		DIMENSION volumeExtent,
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
		DIMENSION textureWidth,
		DIMENSION textureHeight,
		DIMENSION volumeExtent,
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

	LOD ValidatedImageInfo::GetNumberOfLOD() const {
		return numberOfLOD;
	}

	ImageInfo::BITS_PER_PIXEL ValidatedImageInfo::GetBitsPerPixel() const {
		return ImageInfo::GetBitsPerPixel();
	}

	ImageInfo::BITS_PER_PIXEL ValidatedImageInfo::GetRequestedBitsPerPixel() const {
		return ImageInfo::GetRequestedBitsPerPixel();
	}

	void ValidatedImageInfo::SetLodSizeInBytes(LOD lod, SIZE_IN_BYTES sizeInBytes) {
		if (recomputeLodSizes && sizeInBytes) {
			RecomputeLodSize(lod);
			return;
		}

		lodSizesInBytes[lod] = sizeInBytes;
	}

	void ValidatedImageInfo::SetNumberOfLOD(LOD numberOfLOD) {
		this->numberOfLOD = numberOfLOD;
	}

	void ValidatedImageInfo::SetHint(FormatHint formatHint) {
		this->formatHint = formatHint;
	}
}