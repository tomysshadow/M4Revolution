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

	void ImageLoaderMultipleBuffer::SetHint(FormatHint formatHint) {
		this->formatHint = formatHint;
	}

	bool ImageLoaderMultipleBuffer::GetImageInfoImp(ValidatedImageInfo &validatedImageInfo) {
		MAKE_SCOPE_EXIT(validatedImageInfoScopeExit) {
			validatedImageInfo = ValidatedImageInfo();
		};

		GetImageInfoImpEx();

		if (validatedImageInfoOptional.has_value()) {
			validatedImageInfo = validatedImageInfoOptional.value();
			validatedImageInfoScopeExit.dismiss();
			return true;
		}
		return false;
	}

	void ImageLoaderMultipleBuffer::GetImageInfoImpEx() {
		MAKE_SCOPE_EXIT(validatedImageInfoOptionalScopeExit) {
			validatedImageInfoOptional = std::nullopt;
		};

		const LOD MAIN_LOD = 0;
		const RawBufferEx &MAIN_RAW_BUFFER = rawBuffers[MAIN_LOD];

		// the main raw buffer's pointer is required
		if (!MAIN_RAW_BUFFER.pointer) {
			return;
		}

		const size_t BYTES = 3;
		const int VOLUME_EXTENT = 1;

		#define LOD_SIZE_IN_BYTES(bits, textureWidth, textureHeight, volumeExtent) (((bits) >> BYTES) * (textureWidth) * (textureHeight) * (volumeExtent))

		const char* extension = GetExtension();
		uint32_t bits = 0;
		int textureWidth = 0;
		int textureHeight = 0;
		ValidatedImageInfo::SIZE_IN_BYTES sizeInBytes = 0;

		if (MAIN_RAW_BUFFER.uncompressed) {
			validatedImageInfoOptional.emplace(uncompressedImageInfo);
			sizeInBytes = uncompressedImageInfo.lodSizesInBytes[MAIN_LOD];
		} else {
			bool isAlpha = false;

			try {
				M4Image::getInfo(MAIN_RAW_BUFFER.pointer, MAIN_RAW_BUFFER.size, extension, &isAlpha, &bits, &textureWidth, &textureHeight);
			} catch (...) {
				return;
			}

			validatedImageInfoOptional.emplace(textureWidth, textureHeight, VOLUME_EXTENT, formatHint.GetEnumPixelFormat(isAlpha, bits), formatHint);
			bits = validatedImageInfoOptional.value().GetBitsPerPixel();
			sizeInBytes = LOD_SIZE_IN_BYTES(bits, textureWidth, textureHeight, VOLUME_EXTENT);
		}

		ValidatedImageInfo &validatedImageInfo = validatedImageInfoOptional.value();
		validatedImageInfo.SetNumberOfLOD(numberOfLOD);
		validatedImageInfo.SetLodSizeInBytes(MAIN_LOD, sizeInBytes);

		// the following should fail only if we fail to get info
		// we are allowed to have buffers with null pointers, with zero sized images
		// it is only a failure if there is an invalid image
		for (LOD i = MAIN_LOD + 1; i < numberOfLOD; i++) {
			MAKE_SCOPE_EXIT(setLodSizeInBytesScopeExit) {
				validatedImageInfo.SetLodSizeInBytes(i, 0);
			};

			const RawBufferEx &RAW_BUFFER = rawBuffers[i];

			if (!RAW_BUFFER.pointer) {
				continue;
			}

			if (RAW_BUFFER.uncompressed) {
				validatedImageInfo.SetLodSizeInBytes(i, uncompressedImageInfo.lodSizesInBytes[i]);
				setLodSizeInBytesScopeExit.dismiss();
			} else {
				try {
					M4Image::getInfo(RAW_BUFFER.pointer, RAW_BUFFER.size, extension, 0, &bits, &textureWidth, &textureHeight);
				} catch (...) {
					return;
				}

				validatedImageInfo.SetLodSizeInBytes(i, LOD_SIZE_IN_BYTES(bits, textureWidth, textureHeight, VOLUME_EXTENT));
				setLodSizeInBytesScopeExit.dismiss();
			}
		}

		validatedImageInfoOptionalScopeExit.dismiss();
	}
}