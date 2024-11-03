#pragma once
#include "shared.h"
#include "base_rd.h"
#include "ares_base_rd.h"
#include "RawBuffer.h"
#include "ImageInfo.h"
#include "PixelFormat.h"
#include "FormatHint.h"
#include <optional>
#include <stdint.h>

namespace gfx_tools {
	class ImageLoader : public ares::Resource {
		public:
		typedef unsigned long SIZE;
		typedef char Q_FACTOR;
		typedef unsigned long DIMENSION;
		typedef void* HANDLE;

		GFX_TOOLS_RD_API RawBuffer::SIZE GFX_TOOLS_RD_CALL GetRawBufferTotalSize();
		GFX_TOOLS_RD_API bool GFX_TOOLS_RD_CALL GetImageInfo(ImageInfo &imageInfo);
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		GFX_TOOLS_RD_API virtual GFX_TOOLS_RD_CALL ~ImageLoader();
		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);

		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL GetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE size
		) = 0;

		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL ResizeLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE size,
			Q_FACTOR qFactor,
			const ImageInfo &imageInfo,
			DIMENSION resizeTextureWidth,
			DIMENSION resizeTextureHeight,
			ares::RectU32* rectU32Pointer
		) = 0;

		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL SetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE size,
			Q_FACTOR requestedQFactor,
			const ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		) = 0;

		GFX_TOOLS_RD_API virtual RawBuffer::POINTER GFX_TOOLS_RD_CALL CreateLODRawBuffer(LOD lod, RawBuffer::SIZE size) = 0;
		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL SetLODRawBuffer(LOD lod, RawBuffer::POINTER pointer, RawBuffer::SIZE size, ubi::RefCounted* refCountedPointer) = 0;
		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod, RawBuffer::POINTER &pointer, RawBuffer::SIZE &size) = 0;
		GFX_TOOLS_RD_API virtual RawBuffer::POINTER GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod) = 0;

		GFX_TOOLS_RD_API virtual bool GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		) = 0;

		protected:
		virtual void GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			RawBuffer::POINTER pointer,
			RawBuffer::SIZE size,
			bool owner,
			ubi::RefCounted* refCountedPointer
		) = 0;

		virtual const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL GetFormat() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, HANDLE &bitmapHandlePointer) = 0;

		// these methods do not exist on the original ImageLoader
		// they are my own "how it should've been done" methods
		// which the others are built on top of
		virtual void GFX_TOOLS_RD_CALL GetRawBufferInfo(
			const RawBufferEx &rawBuffer,
			bool* isAlphaPointer,
			uint32_t* bitsPointer,
			int* widthPointer,
			int* heightPointer
		) = 0;

		virtual void GFX_TOOLS_RD_CALL LoadRawBuffer(const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::POINTER pointer, SIZE stride) = 0;
		virtual void GFX_TOOLS_RD_CALL SaveRawBuffer(const RawBufferEx &rawBuffer, RawBuffer::POINTER &pointer, SIZE &size) = 0;
		virtual void GFX_TOOLS_RD_CALL GetImageInfoImpEx() = 0;

		virtual void GFX_TOOLS_RD_CALL SetLODRawBufferImpEx(
			LOD lod,
			const RawBufferEx &value,
			ubi::RefCounted* refCountedPointer
		) = 0;

		RawBuffer::SIZE rawBufferTotalSize = 0;
		ubi::RefCounted* refCountedPointer = 0;
		std::optional<ValidatedImageInfo> validatedImageInfoOptional = std::nullopt;
		FormatHint formatHint = { FormatHint::HINT_NONE };
	};

	class ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		GFX_TOOLS_RD_API SIZE GetNumberOfRawBuffers();

		GFX_TOOLS_RD_API virtual GFX_TOOLS_RD_CALL ~ImageLoaderMultipleBuffer();

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL GetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE size
		);

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL ResizeLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE size,
			Q_FACTOR qFactor,
			const ImageInfo &imageInfo,
			DIMENSION resizeTextureWidth,
			DIMENSION resizeTextureHeight,
			ares::RectU32* rectU32Pointer
		);

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE size,
			Q_FACTOR requestedQFactor,
			const ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		);

		GFX_TOOLS_RD_API RawBuffer::POINTER GFX_TOOLS_RD_CALL CreateLODRawBuffer(LOD lod, RawBuffer::SIZE size);
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetLODRawBuffer(LOD lod, RawBuffer::POINTER pointer, RawBuffer::SIZE size, ubi::RefCounted* refCountedPointer);
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod, RawBuffer::POINTER &pointer, RawBuffer::SIZE &size);
		GFX_TOOLS_RD_API RawBuffer::POINTER GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod);

		GFX_TOOLS_RD_API bool GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		);

		protected:
		void GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			RawBuffer::POINTER pointer,
			RawBuffer::SIZE size,
			bool owner,
			ubi::RefCounted* refCountedPointer
		);

		const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		L_INT GFX_TOOLS_RD_CALL GetFormat();
		L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, HANDLE &bitmapHandlePointer);

		void GFX_TOOLS_RD_CALL GetRawBufferInfo(
			const RawBufferEx &rawBuffer,
			bool* isAlphaPointer,
			uint32_t* bitsPointer,
			int* textureWidthPointer,
			int* textureHeightPointer
		);

		void GFX_TOOLS_RD_CALL LoadRawBuffer(const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::POINTER pointer, SIZE stride);
		void GFX_TOOLS_RD_CALL SaveRawBuffer(const RawBufferEx &rawBuffer, RawBuffer::POINTER &pointer, SIZE &size);
		void GFX_TOOLS_RD_CALL GetImageInfoImpEx();

		void GFX_TOOLS_RD_CALL SetLODRawBufferImpEx(
			LOD lod,
			const RawBufferEx &value,
			ubi::RefCounted* refCountedPointer
		);

		SIZE numberOfRawBuffers = 0;
		RawBufferEx rawBuffers[NUMBER_OF_LOD_MAX] = {};
		ImageInfo resizeImageInfo;
	};

	class ImageLoaderMultipleBufferZAP : public ImageLoaderMultipleBuffer {
		protected:
		const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		L_INT GFX_TOOLS_RD_CALL GetFormat();

		void GFX_TOOLS_RD_CALL GetRawBufferInfo(
			const RawBufferEx &rawBuffer,
			bool* isAlphaPointer,
			uint32_t* bitsPointer,
			int* textureWidthPointer,
			int* textureHeightPointer
		);

		void GFX_TOOLS_RD_CALL LoadRawBuffer(const RawBufferEx &rawBuffer, const ImageInfo &imageInfo, RawBuffer::POINTER pointer, SIZE stride);
		void GFX_TOOLS_RD_CALL SaveRawBuffer(const RawBufferEx &rawBuffer, RawBuffer::POINTER &pointer, SIZE &size);
	};

	class ImageLoaderMultipleBufferTGA : public ImageLoaderMultipleBuffer {
		protected:
		static const L_INT FILE_TGA = 4;

		const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		L_INT GFX_TOOLS_RD_CALL GetFormat();
	};

	class ImageLoaderMultipleBufferPNG : public ImageLoaderMultipleBuffer {
		protected:
		static const L_INT FILE_PNG = 75;

		const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		L_INT GFX_TOOLS_RD_CALL GetFormat();
	};

	class ImageLoaderMultipleBufferJPEG : public ImageLoaderMultipleBuffer {
		protected:
		static const L_INT FILE_JPEG = 10;

		const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		L_INT GFX_TOOLS_RD_CALL GetFormat();
	};

	class ImageLoaderMultipleBufferBMP : public ImageLoaderMultipleBuffer {
		protected:
		static const L_INT FILE_BMP = 6;

		const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		L_INT GFX_TOOLS_RD_CALL GetFormat();
	};
}