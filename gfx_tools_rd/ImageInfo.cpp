#include "ImageInfo.h"
#include "Configuration.h"
#include <stdlib.h>

namespace gfx_tools {
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

	ValidatedImageInfo::ValidatedImageInfo() {
	}

	ValidatedImageInfo::ValidatedImageInfo(
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

	void ValidatedImageInfo::OverwritePixelFormat(EnumPixelFormat enumPixelFormat) {
		this->enumPixelFormat = enumPixelFormat;
	}

	ImageInfo const &ValidatedImageInfo::Get() {
		return *this;
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