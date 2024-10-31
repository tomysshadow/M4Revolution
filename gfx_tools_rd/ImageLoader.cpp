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
		validatedImageInfoOptional = std::nullopt;

		const RawBufferEx &RAW_BUFFER = rawBuffers[0];

		// the initial LOD is required
		if (!RAW_BUFFER.pointer) {
			return;
		}

		const size_t BYTES = 3;
		const int VOLUME_EXTENT = 1;

		#define LOD_SIZE_IN_BYTES(bits, textureWidth, textureHeight, volumeExtent) (((bits) >> BYTES) * (textureWidth) * (textureHeight) * (volumeExtent))

		const char* extension = GetExtension();
		uint32_t bits = 0;
		int width = 0;
		int height = 0;
		ValidatedImageInfo::SIZE_IN_BYTES sizeInBytes = 0;

		if (RAW_BUFFER.uncompressed) {
			validatedImageInfoOptional.emplace(uncompressedImageInfo);
			sizeInBytes = uncompressedImageInfo.lodSizesInBytes[0];
		} else {
			bool isAlpha = false;

			try {
				M4Image::getInfo(RAW_BUFFER.pointer, RAW_BUFFER.size, extension, &isAlpha, &bits, &width, &height);
			} catch (...) {
				return;
			}

			validatedImageInfoOptional.emplace(width, height, VOLUME_EXTENT, formatHint.GetEnumPixelFormat(isAlpha, bits), formatHint);
			bits = validatedImageInfoOptional.value().GetBitsPerPixel();
			sizeInBytes = LOD_SIZE_IN_BYTES(bits, width, height, VOLUME_EXTENT);
		}

		ValidatedImageInfo &validatedImageInfo = validatedImageInfoOptional.value();
		validatedImageInfo.SetNumberOfLOD(numberOfLOD);
		validatedImageInfo.SetLodSizeInBytes(0, sizeInBytes);

		// the following should fail only if we fail to get info
		// we are allowed to have buffers with null pointers, with zero sized images
		// it is only a failure if there is an invalid image
		for (LOD i = 1; i < numberOfLOD; i++) {
			MAKE_SCOPE_EXIT(setLodSizeInBytesScopeExit) {
				validatedImageInfo.SetLodSizeInBytes(i, 0);
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
					M4Image::getInfo(RAW_BUFFER.pointer, RAW_BUFFER.size, extension, 0, &bits, &width, &height);
				} catch (...) {
					validatedImageInfoOptional = std::nullopt;
					return;
				}

				setLodSizeInBytesScopeExit.dismiss();
				validatedImageInfo.SetLodSizeInBytes(i, LOD_SIZE_IN_BYTES(bits, width, height, VOLUME_EXTENT));
			}
		}
	}
}