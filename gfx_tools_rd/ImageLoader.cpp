#include "ImageLoader.h"
#include <M4Image/M4Image.h>

namespace gfx_tools {
	RawBuffer::SIZE ImageLoader::GetRawBufferTotalSize() {
		return rawBufferTotalSize;
	}

	bool ImageLoader::GetImageInfo(ImageInfo &imageInfo) {
		MAKE_SCOPE_EXIT(imageInfoScopeExit) {
			imageInfo = ImageInfo();
		};

		if (!validatedImageInfoOptional.has_value()) {
			GetImageInfoImpEx();

			if (!validatedImageInfoOptional.has_value()) {
				return false;
			}
		}

		imageInfo = validatedImageInfoOptional.value().Get();
		imageInfoScopeExit.dismiss();
		return true;
	}

	void ImageLoader::SetPixelFormat(EnumPixelFormat enumPixelFormat) {
		this->enumPixelFormat = enumPixelFormat;
	}

	const ImageLoader::HINT_PIXELFORMAT_MAP ImageLoader::HINT_PIXELFORMAT_8_MAP = {
		{HINT_ALPHA, PIXELFORMAT_A_8},
		{HINT_LUMINANCE, PIXELFORMAT_L_8}
	};

	EnumPixelFormat ImageLoader::GetPixelFormatFromHint(uint32_t bits, bool hasAlpha) const {
		switch (bits) {
			case 8:
			if (!formatHint.hasHint) {
				return PIXELFORMAT_XRGB_8888;
			}

			{
				// try to find the pixel format from the hint
				// but if we don't recognize the hint it is an unknown format
				ImageLoader::HINT_PIXELFORMAT_MAP::const_iterator hintPixelFormatMapIterator = HINT_PIXELFORMAT_8_MAP.find(formatHint.hint);

				if (hintPixelFormatMapIterator != HINT_PIXELFORMAT_8_MAP.end()) {
					return hintPixelFormatMapIterator->second;
				}
			}
			break;
			case 16:
			return PIXELFORMAT_AL_88;
			case 32:
			if (hasAlpha) {
				return PIXELFORMAT_ARGB_8888;
			}
			return PIXELFORMAT_XRGB_8888;
		}
		return EnumPixelFormat::PIXELFORMAT_UNKNOWN;
	};

	void ImageLoaderMultipleBuffer::SetHint(FormatHint formatHint) {
		this->formatHint = formatHint;
	}

	bool ImageLoaderMultipleBuffer::GetImageInfoImp(ValidatedImageInfo &validatedImageInfo) {
		GetImageInfoImpEx();

		if (validatedImageInfoOptional.has_value()) {
			validatedImageInfo = validatedImageInfoOptional.value();
			return true;
		}

		validatedImageInfo = ValidatedImageInfo();
		return false;
	}

	void ImageLoaderMultipleBuffer::GetImageInfoImpEx() {
		validatedImageInfoOptional = std::nullopt;

		const RawBufferEx &RAW_BUFFER = rawBuffers[0];

		// the initial LOD is required
		if (!RAW_BUFFER.pointer) {
			return;
		}

		const char* extension = GetExtension();
		uint32_t bits = 0;
		int width = 0;
		int height = 0;

		if (RAW_BUFFER.uncompressed) {
			validatedImageInfoOptional.emplace(uncompressedImageInfo);
		} else {
			bool hasAlpha = false;

			try {
				M4Image::getInfo(RAW_BUFFER.pointer, RAW_BUFFER.size, extension, &bits, &hasAlpha, &width, &height);
			} catch (...) {
				return;
			}

			// TODO: format hint to pixel format conversion
			validatedImageInfoOptional.emplace(width, height, 1, GetPixelFormatFromHint(bits, hasAlpha), formatHint);
		}

		bool result = true;

		const size_t BYTES = 3;

		#define LOD_SIZE_IN_BYTES(bits, width, height) (((bits) >> BYTES) * (width) * (height))

		ValidatedImageInfo &validatedImageInfo = validatedImageInfoOptional.value();
		validatedImageInfo.SetNumberOfLOD(numberOfLod);
		bits = validatedImageInfo.GetBitsPerPixel();
		validatedImageInfo.SetLodSizeInBytes(0, LOD_SIZE_IN_BYTES(bits, width, height));

		// the following should fail only if we fail to get info
		// we are allowed to have buffers with null pointers, with zero sized images
		// it is only a failure if there is an invalid image
		for (LOD i = 1; i < numberOfLod; i++) {
			MAKE_SCOPE_EXIT(setLodSizeInBytesScopeExit) {
				validatedImageInfo.SetLodSizeInBytes(i++, 0);
			};

			const RawBufferEx &RAW_BUFFER = rawBuffers[i];

			if (!RAW_BUFFER.pointer) {
				continue;
			}

			if (RAW_BUFFER.uncompressed) {
				setLodSizeInBytesScopeExit.dismiss();
				validatedImageInfo.SetLodSizeInBytes(i, uncompressedImageInfo.lodSizesInBytes[i]);
			} else {
				try {
					M4Image::getInfo(RAW_BUFFER.pointer, RAW_BUFFER.size, extension, &bits, 0, &width, &height);
				} catch (...) {
					validatedImageInfoOptional = std::nullopt;
					return;
				}

				setLodSizeInBytesScopeExit.dismiss();
				validatedImageInfo.SetLodSizeInBytes(i, LOD_SIZE_IN_BYTES(bits, width, height));
			}
		}
	}
}