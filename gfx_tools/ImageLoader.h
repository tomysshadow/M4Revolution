#pragma once
#include "base.h"
#include "ares_base.h"
#include "RawBuffer.h"
#include "ImageInfo.h"
#include "PixelFormat.h"
#include "FormatHint.h"
#include <optional>

namespace gfx_tools {
	class ImageLoader : public ares::Resource {
		public:
		using Size = unsigned long;
		using QFactor = char;
		using Dimension = unsigned long;
		using Handle = void*;

		GFX_TOOLS_API RawBuffer::Size GFX_TOOLS_CALL GetRawBufferTotalSize();
		GFX_TOOLS_API bool GFX_TOOLS_CALL GetImageInfo(ImageInfo &imageInfo);
		GFX_TOOLS_API void GFX_TOOLS_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		GFX_TOOLS_API virtual GFX_TOOLS_CALL ~ImageLoader();
		GFX_TOOLS_API virtual void GFX_TOOLS_CALL SetHint(FormatHint formatHint);

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL GetLOD(
			Lod lod,
			RawBuffer::Pointer pointer,
			Size stride,
			Size sizeInBytes
		) = 0;

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL ResizeLOD(
			Lod lod,
			RawBuffer::Pointer pointer,
			Size stride,
			Size sizeInBytes,
			QFactor qFactor,
			const ImageInfo &imageInfo,
			Dimension resizeTextureWidth,
			Dimension resizeTextureHeight,
			ares::RectU32* rectU32Pointer
		) = 0;

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL SetLOD(
			Lod lod,
			RawBuffer::Pointer pointer,
			Size stride,
			Size sizeInBytes,
			QFactor qFactor,
			const ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		) = 0;

		GFX_TOOLS_API virtual RawBuffer::Pointer GFX_TOOLS_CALL CreateLODRawBuffer(Lod lod, RawBuffer::Size size) = 0;

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL SetLODRawBuffer(
			Lod lod,
			RawBuffer::Pointer pointer, RawBuffer::Size size,
			ubi::RefCounted* refCountedPointer = nullptr
		) = 0;

		GFX_TOOLS_API virtual RawBuffer::Pointer GFX_TOOLS_CALL GetLODRawBuffer(Lod lod) = 0;

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL GetLODRawBuffer(
			Lod lod, RawBuffer::Pointer &pointer, RawBuffer::Size &size
		) = 0;

		GFX_TOOLS_API virtual bool GFX_TOOLS_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		) = 0;

		protected:
		virtual void GFX_TOOLS_CALL SetLODRawBufferImp(
			Lod lod,
			RawBuffer::Pointer pointer, RawBuffer::Size size,
			bool owner = false,
			ubi::RefCounted* refCountedPointer = nullptr
		) = 0;

		virtual const L_TCHAR* GFX_TOOLS_CALL GetExtension() = 0;
		virtual L_INT GFX_TOOLS_CALL GetFormat() = 0;
		virtual L_INT GFX_TOOLS_CALL CreateBitmapHandle(Lod lod, Handle &bitmapHandlePointer) = 0;

		// these methods do not exist on the original ImageLoader
		// they are my own "how it should've been done" methods
		// which the others are built on top of
		virtual void GFX_TOOLS_CALL GetRawBufferInfo(
			const RawBufferEx &rawBuffer,
			bool* isAlphaPointer, uint32_t* bitsPointer,
			int* textureWidthPointer, int* textureHeightPointer
		) = 0;

		virtual void GFX_TOOLS_CALL LoadRawBuffer(
			const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::Pointer pointer, Size stride
		) = 0;

		virtual void GFX_TOOLS_CALL SaveRawBuffer(
			const RawBufferEx &rawBuffer, RawBuffer::Pointer &pointer, RawBuffer::Size &size
		) = 0;

		virtual void GFX_TOOLS_CALL GetImageInfoImpEx() = 0;

		virtual void GFX_TOOLS_CALL SetLODRawBufferImpEx(
			Lod lod,
			RawBuffer::Pointer pointer, RawBuffer::Size size,
			bool owner = false,
			const std::optional<RawBufferEx::ResizeInfo> &resizeInfoOptional = std::nullopt,
			ubi::RefCounted* refCountedPointer = nullptr
		) = 0;

		RawBuffer::Size rawBufferTotalSize = 0;
		ubi::RefCounted* refCountedPointer = nullptr;
		std::optional<ValidatedImageInfo> validatedImageInfoOptional = std::nullopt;
		FormatHint formatHint = { FormatHint::HINT_NONE };
	};

	class ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		GFX_TOOLS_API Size GetNumberOfRawBuffers();

		GFX_TOOLS_API virtual GFX_TOOLS_CALL ~ImageLoaderMultipleBuffer();

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL GetLOD(
			Lod lod,
			RawBuffer::Pointer pointer,
			Size stride,
			Size sizeInBytes
		) override;

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL ResizeLOD(
			Lod lod,
			RawBuffer::Pointer pointer,
			Size stride,
			Size sizeInBytes,
			QFactor qFactor,
			const ImageInfo &imageInfo,
			Dimension resizeTextureWidth,
			Dimension resizeTextureHeight,
			ares::RectU32* rectU32Pointer
		) override;

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL SetLOD(
			Lod lod,
			RawBuffer::Pointer pointer,
			Size stride,
			Size sizeInBytes,
			QFactor qFactor,
			const ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		) override;

		GFX_TOOLS_API virtual RawBuffer::Pointer GFX_TOOLS_CALL CreateLODRawBuffer(Lod lod, RawBuffer::Size size) override;

		GFX_TOOLS_API virtual void GFX_TOOLS_CALL SetLODRawBuffer(
			Lod lod,
			RawBuffer::Pointer pointer, RawBuffer::Size size,
			ubi::RefCounted* refCountedPointer = nullptr
		) override;

		GFX_TOOLS_API virtual RawBuffer::Pointer GFX_TOOLS_CALL GetLODRawBuffer(Lod lod) override;
		GFX_TOOLS_API virtual void GFX_TOOLS_CALL GetLODRawBuffer(Lod lod, RawBuffer::Pointer &pointer, RawBuffer::Size &size) override;

		GFX_TOOLS_API virtual bool GFX_TOOLS_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		) override;

		protected:
		virtual void GFX_TOOLS_CALL SetLODRawBufferImp(
			Lod lod,
			RawBuffer::Pointer pointer, RawBuffer::Size size,
			bool owner = false,
			ubi::RefCounted* refCountedPointer = nullptr
		) override;

		virtual const L_TCHAR* GFX_TOOLS_CALL GetExtension() override;
		virtual L_INT GFX_TOOLS_CALL GetFormat() override;
		virtual L_INT GFX_TOOLS_CALL CreateBitmapHandle(Lod lod, Handle &bitmapHandlePointer) override;

		virtual void GFX_TOOLS_CALL GetRawBufferInfo(
			const RawBufferEx &rawBuffer,
			bool* isAlphaPointer, uint32_t* bitsPointer,
			int* textureWidthPointer, int* textureHeightPointer
		) override;

		virtual void GFX_TOOLS_CALL LoadRawBuffer(
			const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::Pointer pointer, Size stride
		) override;

		virtual void GFX_TOOLS_CALL SaveRawBuffer(
			const RawBufferEx &rawBuffer, RawBuffer::Pointer &pointer, RawBuffer::Size &size
		) override;

		virtual void GFX_TOOLS_CALL GetImageInfoImpEx() override;

		virtual void GFX_TOOLS_CALL SetLODRawBufferImpEx(
			Lod lod,
			RawBuffer::Pointer pointer, RawBuffer::Size size,
			bool owner = false,
			const std::optional<RawBufferEx::ResizeInfo> &resizeInfoOptional = std::nullopt,
			ubi::RefCounted* refCountedPointer = nullptr
		) override;

		Size numberOfRawBuffers = 0;
		std::optional<RawBufferEx> rawBufferOptionals[NUMBER_OF_LOD_MAX] = {};
		ImageInfo resizeImageInfo;
	};

	class ImageLoaderMultipleBufferZAP : public ImageLoaderMultipleBuffer {
		protected:
		virtual const L_TCHAR* GFX_TOOLS_CALL GetExtension() override;
		virtual L_INT GFX_TOOLS_CALL GetFormat() override;

		virtual void GFX_TOOLS_CALL GetRawBufferInfo(
			const RawBufferEx &rawBuffer,
			bool* isAlphaPointer, uint32_t* bitsPointer,
			int* textureWidthPointer, int* textureHeightPointer
		) override;

		virtual void GFX_TOOLS_CALL LoadRawBuffer(
			const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::Pointer pointer, Size stride
		) override;

		virtual void GFX_TOOLS_CALL SaveRawBuffer(
			const RawBufferEx &rawBuffer, RawBuffer::Pointer &pointer, Size &size
		) override;
	};

	class ImageLoaderMultipleBufferTGA : public ImageLoaderMultipleBuffer {
		protected:
		static constexpr L_INT FILE_TGA = 4;

		virtual const L_TCHAR* GFX_TOOLS_CALL GetExtension() override;
		virtual L_INT GFX_TOOLS_CALL GetFormat() override;
	};

	class ImageLoaderMultipleBufferPNG : public ImageLoaderMultipleBuffer {
		protected:
		static constexpr L_INT FILE_PNG = 75;

		virtual const L_TCHAR* GFX_TOOLS_CALL GetExtension() override;
		virtual L_INT GFX_TOOLS_CALL GetFormat() override;
	};

	class ImageLoaderMultipleBufferJPEG : public ImageLoaderMultipleBuffer {
		protected:
		static constexpr L_INT FILE_JPEG = 10;

		virtual const L_TCHAR* GFX_TOOLS_CALL GetExtension() override;
		virtual L_INT GFX_TOOLS_CALL GetFormat() override;
	};

	class ImageLoaderMultipleBufferBMP : public ImageLoaderMultipleBuffer {
		protected:
		static constexpr L_INT FILE_BMP = 6;

		virtual const L_TCHAR* GFX_TOOLS_CALL GetExtension() override;
		virtual L_INT GFX_TOOLS_CALL GetFormat() override;
	};
}