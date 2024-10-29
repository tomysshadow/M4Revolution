#include "ImageLoader.h"
#include <M4Image/M4Image.h>

namespace gfx_tools {
	RawBuffer::SIZE GFX_TOOLS_RD_CALL ImageLoader::GetRawBufferTotalSize() {
		return rawBufferTotalSize;
	}

	// TODO: this should just call GetImageInfoImp to do the work!
	bool GFX_TOOLS_RD_CALL ImageLoader::GetImageInfo(ImageInfo &imageInfo) {
		MAKE_SCOPE_EXIT(imageInfoScopeExit) {
			imageInfo = ImageInfo();
		};

		// because we need to pass an instance of validatedImageInfo for the interface to remain the same
		// we can't use std::optional without it being a big pain, so we just use a bool instead
		if (!validatedImageInfoSet) {
			validatedImageInfoSet = GetImageInfoImp(validatedImageInfo);

			if (!validatedImageInfoSet) {
				return false;
			}
		}

		imageInfo = validatedImageInfo.Get();
		imageInfoScopeExit.dismiss();
		return true;
	}

	void ImageLoader::SetPixelFormat(EnumPixelFormat enumPixelFormat) {
		this->enumPixelFormat = enumPixelFormat;
	}

	bool ImageLoader::GetImageInfoImp(
		ValidatedImageInfo &validatedImageInfo
	) {
		MAKE_SCOPE_EXIT(validatedImageInfoScopeExit) {
			validatedImageInfo = ValidatedImageInfo();
		};

		const RawBufferEx &RAW_BUFFER = rawBuffers[0];

		// the initial LOD is required
		if (!RAW_BUFFER.pointer) {
			return false;
		}

		// TODO: getInfo will only work on image formats, so we'll need to store this data on the raw buffer when uncompressed
		if (!RAW_BUFFER.compressed) {
			return false;
		}

		uint32_t bits = 0;
		bool hasAlpha = false;
		int width = 0;
		int height = 0;

		try {
			M4Image::getInfo(RAW_BUFFER.pointer, RAW_BUFFER.size, 0, &bits, &hasAlpha, &width, &height);
		} catch (...) {
			return false;
		}

		bool result = true;

		const size_t BYTES = 3;

		#define LOD_SIZE_IN_BYTES(bits, width, height) (((bits) >> BYTES) * (width) * (height))

		// TODO: format hint to pixel format conversion
		validatedImageInfo = ValidatedImageInfo(width, height, 1, GetEnumPixelFormatFromFormatHint(bits, hasAlpha), formatHint);
		validatedImageInfo.SetNumberOfLOD(numberOfLod);
		bits = validatedImageInfo.GetBitsPerPixel();
		validatedImageInfo.SetLodSizeInBytes(0, LOD_SIZE_IN_BYTES(bits, width, height));

		// the following should fail only if we fail to get info
		// we are allowed to have buffers with null pointers, with zero sized images
		// it is only a failure if there is an invalid image
		LOD i = 1;

		while (i < numberOfLod) {
			MAKE_SCOPE_EXIT(setLodSizeInBytesScopeExit) {
				validatedImageInfo.SetLodSizeInBytes(i++, 0);
			};

			const RawBufferEx &RAW_BUFFER = rawBuffers[i];

			if (!RAW_BUFFER.pointer) {
				continue;
			}

			// TODO
			if (!RAW_BUFFER.compressed) {
				continue;
			}

			try {
				M4Image::getInfo(RAW_BUFFER.pointer, RAW_BUFFER.size, 0, &bits, 0, &width, &height);
			} catch (...) {
				result = false;
				continue;
			}

			setLodSizeInBytesScopeExit.dismiss();
			validatedImageInfo.SetLodSizeInBytes(i++, LOD_SIZE_IN_BYTES(bits, width, height));
		}

		if (result) {
			validatedImageInfoScopeExit.dismiss();
		}
		return result;
	}
}