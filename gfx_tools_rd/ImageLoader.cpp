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
		if (validatedImageInfoOptional.has_value()) {
			validatedImageInfoOptional.value().OverwritePixelFormat(enumPixelFormat);
		}
	}

	ImageLoader::~ImageLoader() {
		if (refCountedPointer) {
			refCountedPointer->Release();
		}
	}

	void ImageLoader::SetHint(FormatHint formatHint) {
		this->formatHint = formatHint;
	}

	ImageLoaderMultipleBuffer::SIZE ImageLoaderMultipleBuffer::GetNumberOfRawBuffers() {
		return numberOfRawBuffers;
	}

	ImageLoaderMultipleBuffer::~ImageLoaderMultipleBuffer() {
	}

	void ImageLoaderMultipleBuffer::GetLOD(LOD lod, RawBuffer::DATA data, SIZE stride, SIZE size) {
		if (!data) {
			throw std::invalid_argument("data must not be zero");
		}

		if (lod > numberOfRawBuffers) {
			throw std::invalid_argument("lod must not be greater than numberOfRawBuffers");
		}

		if (!validatedImageInfoOptional.has_value()) {
			GetImageInfoImpEx();
		}

		const ImageInfo &IMAGE_INFO = validatedImageInfoOptional.value().Get();

		if (size < IMAGE_INFO.textureHeight * stride) {
			throw std::invalid_argument("size is too small");
		}

		const RawBufferEx::POINTER &RAW_BUFFER_POINTER = rawBufferPointers[lod];

		if (!RAW_BUFFER_POINTER) {
			throw std::logic_error("Raw Buffer must be set");
		}

		const RawBufferEx &RAW_BUFFER = *RAW_BUFFER_POINTER;

		if (RAW_BUFFER.resizeInfoOptional.has_value()) {
			const RawBufferEx::ResizeInfo &RESIZE_INFO = RAW_BUFFER.resizeInfoOptional.value();

			size_t m4ImageStride = RESIZE_INFO.stride;

			const M4Image RAW_BUFFER_M4_IMAGE(
				RESIZE_INFO.width,
				RESIZE_INFO.height,
				m4ImageStride,
				resizeImageInfo.GetColorFormat(),
				RAW_BUFFER.data
			);

			m4ImageStride = stride;

			M4Image m4Image(
				IMAGE_INFO.textureWidth,
				IMAGE_INFO.textureHeight,
				m4ImageStride,
				IMAGE_INFO.GetColorFormat(),
				data
			);

			m4Image.blit(RAW_BUFFER_M4_IMAGE);
			return;
		}

		LoadRawBuffer(RAW_BUFFER, IMAGE_INFO, data, stride);
	}

	void ImageLoaderMultipleBuffer::ResizeLOD(
		LOD lod,
		RawBuffer::DATA data,
		SIZE stride,
		SIZE size,
		Q_FACTOR qFactor,
		const ImageInfo &imageInfo,
		DIMENSION resizeTextureWidth,
		DIMENSION resizeTextureHeight,
		ares::RectU32* rectU32Pointer
	) {
		if (!data) {
			throw std::invalid_argument("data must not be zero");
		}

		if (size < imageInfo.textureHeight * stride) {
			throw std::invalid_argument("size is too small");
		}

		size_t m4ImageStride = stride;

		const M4Image M4IMAGE(
			imageInfo.textureWidth,
			imageInfo.textureHeight,
			m4ImageStride,
			imageInfo.GetColorFormat(),
			data
		);

		m4ImageStride = 0;

		M4Image resizeM4Image(
			resizeTextureWidth,
			resizeTextureHeight,
			m4ImageStride,
			imageInfo.GetColorFormat()
		);

		resizeM4Image.blit(M4IMAGE);

		RawBufferEx::ResizeInfo resizeInfo(resizeTextureWidth, resizeTextureHeight, m4ImageStride, qFactor);
		SetLODRawBufferImpEx(lod, resizeM4Image.acquire(), (RawBuffer::SIZE)(resizeTextureHeight * m4ImageStride), true, resizeInfo, 0);
		resizeImageInfo = imageInfo;
	}

	void ImageLoaderMultipleBuffer::SetLOD(
		LOD lod,
		RawBuffer::DATA data,
		SIZE stride,
		SIZE size,
		Q_FACTOR qFactor,
		const ImageInfo &imageInfo,
		ares::RectU32* rectU32Pointer
	) {
		ResizeLOD(lod, data, stride, size, qFactor, imageInfo, imageInfo.textureWidth, imageInfo.textureHeight, rectU32Pointer);
	}

	RawBuffer::DATA ImageLoaderMultipleBuffer::CreateLODRawBuffer(LOD lod, RawBuffer::SIZE size) {
		RawBuffer::DATA data = (RawBuffer::DATA)M4Image::allocator.mallocSafe(size);
		SetLODRawBufferImp(lod, data, size, true, 0);
		return data;
	}

	void ImageLoaderMultipleBuffer::SetLODRawBuffer(LOD lod, RawBuffer::DATA data, RawBuffer::SIZE size, ubi::RefCounted* refCountedPointer) {
		SetLODRawBufferImp(lod, data, size, false, refCountedPointer);
	}

	RawBuffer::DATA ImageLoaderMultipleBuffer::GetLODRawBuffer(LOD lod) {
		RawBuffer::DATA data = 0;
		RawBuffer::SIZE size = 0;
		GetLODRawBuffer(lod, data, size);
		return data;
	}

	void ImageLoaderMultipleBuffer::GetLODRawBuffer(LOD lod, RawBuffer::DATA &data, RawBuffer::SIZE &size) {
		data = 0;

		MAKE_SCOPE_EXIT(dataScopeExit) {
			M4Image::allocator.freeSafe(data);
		};

		MAKE_SCOPE_EXIT(sizeScopeExit) {
			size = 0;
		};

		if (lod > numberOfRawBuffers) {
			throw std::invalid_argument("lod must not be greater than numberOfRawBuffers");
		}

		const RawBufferEx::POINTER &RAW_BUFFER_POINTER = rawBufferPointers[lod];

		if (!RAW_BUFFER_POINTER) {
			data = 0;
			size = 0;
			sizeScopeExit.dismiss();
			dataScopeExit.dismiss();
			return;
		}

		const RawBufferEx &RAW_BUFFER = *RAW_BUFFER_POINTER;

		if (RAW_BUFFER.resizeInfoOptional.has_value()) {
			SaveRawBuffer(RAW_BUFFER, data, size);
			sizeScopeExit.dismiss();
			dataScopeExit.dismiss();
			return;
		}

		data = RAW_BUFFER.data;
		size = RAW_BUFFER.size;
		sizeScopeExit.dismiss();
		dataScopeExit.dismiss();
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

	void ImageLoaderMultipleBuffer::SetLODRawBufferImp(
		LOD lod,
		RawBuffer::DATA data,
		RawBuffer::SIZE size,
		bool owner,
		ubi::RefCounted* refCountedPointer
	) {
		SetLODRawBufferImpEx(lod, data, size, owner, std::nullopt, refCountedPointer);
	}

	const L_TCHAR* ImageLoaderMultipleBuffer::GetExtension() {
		return 0;
	}

	L_INT ImageLoaderMultipleBuffer::GetFormat() {
		return 0;
	}

	L_INT ImageLoaderMultipleBuffer::CreateBitmapHandle(LOD lod, HANDLE &bitmapHandlePointer) {
		// in this implementation we have no concept of a bitmap handle
		bitmapHandlePointer = 0;
		return SUCCESS;
	}

	void ImageLoaderMultipleBuffer::GetRawBufferInfo(
		const RawBufferEx &rawBuffer,
		bool* isAlphaPointer,
		uint32_t* bitsPointer,
		int* textureWidthPointer,
		int* textureHeightPointer
	) {
		M4Image::getInfo(rawBuffer.data, rawBuffer.size, GetExtension(), isAlphaPointer, bitsPointer, textureWidthPointer, textureHeightPointer);
	}

	void ImageLoaderMultipleBuffer::LoadRawBuffer(const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::DATA data, SIZE stride) {
		size_t m4ImageStride = stride;

		M4Image m4Image(
			imageInfo.textureWidth,
			imageInfo.textureHeight,
			m4ImageStride,
			imageInfo.GetColorFormat(),
			data
		);

		m4Image.load(rawBuffer.data, rawBuffer.size, GetExtension());
	}

	void ImageLoaderMultipleBuffer::SaveRawBuffer(const RawBufferEx &rawBuffer, RawBuffer::DATA &data, SIZE &size) {
		const RawBufferEx::ResizeInfo &RESIZE_INFO = rawBuffer.resizeInfoOptional.value();

		size_t m4ImageStride = RESIZE_INFO.stride;

		const M4Image M4_IMAGE(
			RESIZE_INFO.width,
			RESIZE_INFO.height,
			m4ImageStride,
			resizeImageInfo.GetColorFormat(),
			rawBuffer.data
		);

		size_t m4ImageSize = 0;
		data = M4_IMAGE.save(m4ImageSize, GetExtension(), RESIZE_INFO.quality);
		size = (SIZE)m4ImageSize;
	}

	void ImageLoaderMultipleBuffer::GetImageInfoImpEx() {
		MAKE_SCOPE_EXIT(validatedImageInfoOptionalScopeExit) {
			validatedImageInfoOptional = std::nullopt;
		};

		const LOD MAIN_LOD = 0;

		const RawBufferEx::POINTER &RAW_BUFFER_POINTER = rawBufferPointers[MAIN_LOD];

		if (!RAW_BUFFER_POINTER) {
			return;
		}

		const RawBufferEx &MAIN_RAW_BUFFER = *RAW_BUFFER_POINTER;

		// the main raw buffer's data is required
		if (!MAIN_RAW_BUFFER.data) {
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

		if (MAIN_RAW_BUFFER.resizeInfoOptional.has_value()) {
			validatedImageInfoOptional.emplace(resizeImageInfo);
			sizeInBytes = resizeImageInfo.lodSizesInBytes[MAIN_LOD];
		} else {
			bool isAlpha = false;

			try {
				GetRawBufferInfo(MAIN_RAW_BUFFER, &isAlpha, &bits, &textureWidth, &textureHeight);
			} catch (...) {
				return;
			}

			validatedImageInfoOptional.emplace(textureWidth, textureHeight, VOLUME_EXTENT, formatHint.GetEnumPixelFormat(isAlpha, bits), formatHint);
			bits = validatedImageInfoOptional.value().GetBitsPerPixel();
			sizeInBytes = LOD_SIZE_IN_BYTES(bits, textureWidth, textureHeight, VOLUME_EXTENT);
		}

		ValidatedImageInfo &validatedImageInfo = validatedImageInfoOptional.value();
		validatedImageInfo.SetNumberOfLOD((LOD)numberOfRawBuffers);
		validatedImageInfo.SetLodSizeInBytes(MAIN_LOD, sizeInBytes);

		// the following should fail only if we fail to get info
		// we are allowed to have buffers with null pointers, with zero sized images
		// it is only a failure if there is an invalid image
		for (LOD i = MAIN_LOD + 1; i < numberOfRawBuffers; i++) {
			MAKE_SCOPE_EXIT(setLodSizeInBytesScopeExit) {
				validatedImageInfo.SetLodSizeInBytes(i, 0);
			};

			const RawBufferEx::POINTER &RAW_BUFFER_POINTER = rawBufferPointers[i];

			if (!RAW_BUFFER_POINTER) {
				continue;
			}

			const RawBufferEx &RAW_BUFFER = *RAW_BUFFER_POINTER;

			if (!RAW_BUFFER.data) {
				continue;
			}

			if (RAW_BUFFER.resizeInfoOptional.has_value()) {
				validatedImageInfo.SetLodSizeInBytes(i, resizeImageInfo.lodSizesInBytes[i]);
				setLodSizeInBytesScopeExit.dismiss();
			} else {
				try {
					GetRawBufferInfo(RAW_BUFFER, 0, &bits, &textureWidth, &textureHeight);
				} catch (...) {
					return;
				}

				validatedImageInfo.SetLodSizeInBytes(i, LOD_SIZE_IN_BYTES(bits, textureWidth, textureHeight, VOLUME_EXTENT));
				setLodSizeInBytesScopeExit.dismiss();
			}
		}

		validatedImageInfoOptionalScopeExit.dismiss();
	}

	void ImageLoaderMultipleBuffer::SetLODRawBufferImpEx(
		LOD lod,
		RawBuffer::DATA data,
		RawBuffer::SIZE size,
		bool owner,
		std::optional<RawBufferEx::ResizeInfo> resizeInfoOptional,
		ubi::RefCounted* refCountedPointer
	) {
		if (lod >= NUMBER_OF_LOD_MAX) {
			throw std::invalid_argument("lod must not be greater than or equal to NUMBER_OF_LOD_MAX");
		}

		if (numberOfRawBuffers <= lod) {
			numberOfRawBuffers = lod + 1;
		}

		RawBufferEx::POINTER &rawBufferPointer = rawBufferPointers[lod];
		RawBuffer::SIZE sizeOld = rawBufferPointer ? rawBufferPointer->size : 0;
		rawBufferPointer = std::make_unique<RawBufferEx>(data, size, owner, resizeInfoOptional);
		rawBufferTotalSize += size - sizeOld;

		if (this->refCountedPointer != refCountedPointer) {
			if (this->refCountedPointer) {
				this->refCountedPointer->Release();
			}

			this->refCountedPointer = refCountedPointer;

			if (this->refCountedPointer) {
				this->refCountedPointer->AddRef();
			}
		}
	}

	const L_TCHAR* ImageLoaderMultipleBufferZAP::GetExtension() {
		return "ZAP";
	}

	L_INT ImageLoaderMultipleBufferZAP::GetFormat() {
		return 0;
	}

	void ImageLoaderMultipleBufferZAP::GetRawBufferInfo(
		const RawBufferEx &rawBuffer,
		bool* isAlphaPointer,
		uint32_t* bitsPointer,
		int* textureWidthPointer,
		int* textureHeightPointer
	) {
		if (isAlphaPointer) {
			const bool ZAP_IS_ALPHA = true;

			*isAlphaPointer = ZAP_IS_ALPHA;
		}

		if (bitsPointer) {
			const uint32_t ZAP_BITS = 32;

			*bitsPointer = ZAP_BITS;
		}

		zap_error_t err = zap_get_info(rawBuffer.data, textureWidthPointer, textureHeightPointer);

		if (err != ZAP_ERROR_NONE) {
			throw std::runtime_error("Failed to Get ZAP Info");
		}
	}

	void ImageLoaderMultipleBufferZAP::LoadRawBuffer(const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::DATA data, SIZE stride) {
		zap_size_t zapStride = stride;
		zap_size_t zapSize = 0;

		zap_error_t err = zap_resize_memory(
			rawBuffer.data,
			(zap_uint_t)imageInfo.GetColorFormat(),
			&data,
			&zapSize,
			imageInfo.textureWidth,
			imageInfo.textureHeight,
			&zapStride
		);

		if (err != ZAP_ERROR_NONE) {
			throw std::runtime_error("Failed to Resize ZAP Memory");
		}
	}

	void ImageLoaderMultipleBufferZAP::SaveRawBuffer(const RawBufferEx &rawBuffer, RawBuffer::DATA &data, SIZE &size) {
		const RawBufferEx::ResizeInfo &RESIZE_INFO = rawBuffer.resizeInfoOptional.value();

		zap_size_t zapSize = 0;
		size_t zapStride = RESIZE_INFO.stride;

		zap_error_t err = zap_save_memory(
			&data,
			&zapSize,
			RESIZE_INFO.width,
			RESIZE_INFO.height,
			zapStride,
			(zap_uint_t)resizeImageInfo.GetColorFormat(),
			ZAP_IMAGE_FORMAT_JPG,
			ZAP_IMAGE_FORMAT_PNG
		);

		if (err != ZAP_ERROR_NONE) {
			throw std::runtime_error("Failed to Save ZAP Memory");
		}

		size = (SIZE)zapSize;
	}

	const L_TCHAR* ImageLoaderMultipleBufferTGA::GetExtension() {
		return "TGA";
	}

	L_INT ImageLoaderMultipleBufferTGA::GetFormat() {
		return FILE_TGA;
	}

	const L_TCHAR* ImageLoaderMultipleBufferPNG::GetExtension() {
		return "PNG";
	}

	L_INT ImageLoaderMultipleBufferPNG::GetFormat() {
		return FILE_PNG;
	}

	const L_TCHAR* ImageLoaderMultipleBufferJPEG::GetExtension() {
		return "JPEG"; // normally "JFIF" but must be "JPEG" for M4Image to recognize it
	}

	L_INT ImageLoaderMultipleBufferJPEG::GetFormat() {
		return FILE_JPEG;
	}

	const L_TCHAR* ImageLoaderMultipleBufferBMP::GetExtension() {
		return "BMP";
	}

	L_INT ImageLoaderMultipleBufferBMP::GetFormat() {
		return FILE_BMP;
	}
}