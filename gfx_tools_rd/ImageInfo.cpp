#include "ImageInfo.h"
#include "Configuration.h"
#include <stdlib.h>

namespace gfx_tools {
	void ImageInfo::ComputeLODDimensions(unsigned short &textureWidth, unsigned short &textureHeight, unsigned short &volumeExtent, unsigned char lod) {
		const unsigned short MIN_DIMENSION = 1;

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

	size_t ImageInfo::GetBitsPerPixel() {
		return PixelFormat::GetPixelFormat(enumPixelFormat)->GetBitsPerPixel();
	}

	size_t ImageInfo::GetRequestedBitsPerPixel() {
		return PixelFormat::GetPixelFormat(requestedEnumPixelFormat)->GetBitsPerPixel();
	}

	ValidatedImageInfo::ValidatedImageInfo() {
	}

	ValidatedImageInfo::ValidatedImageInfo(
		unsigned short textureWidth,
		unsigned short textureHeight,
		unsigned short volumeExtent,
		EnumPixelFormat enumPixelFormat,
		FormatHint formatHint
	) {
		SetPixelFormat(enumPixelFormat);
		SetDimensions(textureWidth, textureHeight, volumeExtent);
		SetHint(formatHint);
	}

	void ValidatedImageInfo::MakePowerOfTwo(unsigned short &value, bool reserved) {
		unsigned short powerOfTwo = 2;

		// if value is one, we want it to just become two
		// so don't allow powerOfTwo to be downscaled to one
		if (value > powerOfTwo) {
			do {
				powerOfTwo *= 2;
			} while (value > powerOfTwo);

			if (value == powerOfTwo) {
				return;
			}

			if (!Configuration::Get()->upscale) {
				powerOfTwo /= 2;
			}
		}

		value = powerOfTwo;
	}

	void ValidatedImageInfo::MakeSquare(unsigned short &value, unsigned short &value2) {
		unsigned short square = value;

		if (Configuration::Get()->upscale) {
			if (square < value2) {
				square = value2;
			}
		} else {
			if (square > value2) {
				square = value2;
			}
		}

		value = square;
		value2 = square;
	}

	void ValidatedImageInfo::Clamp(unsigned short &number, unsigned short min, unsigned short max) {
		number = __min(max, __max(number, min));
	}

	void ValidatedImageInfo::RecomputeLodSize(unsigned char lod) {
		const size_t BYTES = 3;

		ComputeLODDimensions(textureWidth, textureHeight, volumeExtent, lod);
		lodSizesInBytes[lod] = textureWidth * textureHeight * volumeExtent * (GetBitsPerPixel() >> BYTES);
	}

	EnumPixelFormat ValidatedImageInfo::OverwritePixelFormat(EnumPixelFormat enumPixelFormat) {
		this->enumPixelFormat = enumPixelFormat;
		return enumPixelFormat;
	}

	ValidatedImageInfo* ValidatedImageInfo::Get() {
		return this;
	}

	void ValidatedImageInfo::SetLodSizeInBytes(unsigned char lod, size_t sizeInBytes) {
		if (recompute && sizeInBytes) {
			RecomputeLodSize(lod);
			return;
		}

		lodSizesInBytes[lod] = sizeInBytes;
	}

	void ValidatedImageInfo::SetDimensions(unsigned short textureWidth, unsigned short textureHeight, unsigned short volumeExtent) {
		this->textureWidth = textureWidth;
		this->requestedTextureWidth = textureWidth;
		this->textureHeight = textureHeight;
		this->requestedTextureHeight = textureHeight;
		this->volumeExtent = volumeExtent;

		const Configuration &CONFIGURATION = *Configuration::Get();

		if (CONFIGURATION.dimensionsMakePowerOfTwo) {
			MakePowerOfTwo(this->textureWidth);
			MakePowerOfTwo(this->textureHeight);
			MakePowerOfTwo(this->volumeExtent);
		}

		if (CONFIGURATION.dimensionsMakeSquare) {
			MakeSquare(this->textureWidth, this->textureHeight);
		}

		Clamp(this->textureWidth, (unsigned short)CONFIGURATION.minTextureWidth, (unsigned short)CONFIGURATION.maxTextureWidth);
		Clamp(this->textureHeight, (unsigned short)CONFIGURATION.minTextureHeight, (unsigned short)CONFIGURATION.maxTextureHeight);
		Clamp(this->volumeExtent, (unsigned short)CONFIGURATION.minVolumeExtent, (unsigned short)CONFIGURATION.maxVolumeExtent);

		bool requested = this->textureWidth == textureWidth
			&& this->textureHeight == textureHeight
			&& this->volumeExtent == volumeExtent;

		recompute = recompute || !requested;

		if (recompute) {
			for (unsigned char i = 0; i < NUMBER_OF_LOD_MAX; i++) {
				if (lodSizesInBytes[i]) {
					RecomputeLodSize(i);
				}
			}
		}
	}

	void ValidatedImageInfo::SetNumberOfLOD(unsigned char numberOfLOD) {
		this->numberOfLOD = numberOfLOD;
	}

	EnumPixelFormat ValidatedImageInfo::SetPixelFormat(EnumPixelFormat enumPixelFormat) {
		if (PixelFormat::GetPixelFormat(enumPixelFormat)) {
			this->enumPixelFormat = enumPixelFormat;
			this->requestedEnumPixelFormat = enumPixelFormat;
			return enumPixelFormat;
		}

		const EnumPixelFormat DEFAULT_ENUM_PIXEL_FORMAT = PIXELFORMAT_ARGB_8888;

		recompute = true;
		this->enumPixelFormat = DEFAULT_ENUM_PIXEL_FORMAT;
		this->requestedEnumPixelFormat = DEFAULT_ENUM_PIXEL_FORMAT;
		return PIXELFORMAT_UNKNOWN;
	}

	FormatHint ValidatedImageInfo::SetHint(FormatHint formatHint) {
		this->formatHint = formatHint;
		return formatHint;
	}
}