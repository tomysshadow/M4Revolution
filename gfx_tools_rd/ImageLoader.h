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
		typedef unsigned int LOD;
		typedef unsigned long SIZE;
		typedef char Q_FACTOR;
		typedef unsigned long DIMENSION;
		typedef void* HANDLE;

		GFX_TOOLS_RD_API RawBuffer::SIZE GFX_TOOLS_RD_CALL GetRawBufferTotalSize();
		GFX_TOOLS_RD_API bool GFX_TOOLS_RD_CALL GetImageInfo(ImageInfo &imageInfo);
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		GFX_TOOLS_RD_API virtual GFX_TOOLS_RD_CALL ~ImageLoader();
		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint) = 0;

		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL GetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows
		) = 0;

		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL ResizeLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
			Q_FACTOR qFactor,
			const ImageInfo &imageInfo,
			DIMENSION textureWidth,
			DIMENSION textureHeight,
			ares::RectU32* rectU32Pointer
		) = 0;

		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL SetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
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

		GFX_TOOLS_RD_API virtual void GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			RawBuffer value,
			ubi::RefCounted* refCountedPointer
		) = 0;

		GFX_TOOLS_RD_API virtual const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension() = 0;
		GFX_TOOLS_RD_API virtual L_INT GFX_TOOLS_RD_CALL GetFormat() = 0;
		GFX_TOOLS_RD_API virtual L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, HANDLE &bitmapHandlePointer) = 0;

		protected:
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
	};

	class ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		GFX_TOOLS_RD_API SIZE GetNumberOfRawBuffers();

		GFX_TOOLS_RD_API virtual GFX_TOOLS_RD_CALL ~ImageLoaderMultipleBuffer();
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL GetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows
		);

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL ResizeLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
			Q_FACTOR qFactor,
			const ImageInfo &imageInfo,
			DIMENSION textureWidth,
			DIMENSION textureHeight,
			ares::RectU32* rectU32Pointer
		);

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
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

		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			RawBuffer value,
			ubi::RefCounted* refCountedPointer
		);

		GFX_TOOLS_RD_API const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		GFX_TOOLS_RD_API L_INT GFX_TOOLS_RD_CALL GetFormat();
		GFX_TOOLS_RD_API L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, HANDLE &bitmapHandlePointer);

		protected:
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

		virtual void GFX_TOOLS_RD_CALL SetLODRawBufferImpEx(
			LOD lod,
			const RawBufferEx &value,
			ubi::RefCounted* refCountedPointer
		);

		FormatHint formatHint = { FormatHint::HINT_NONE };
		SIZE numberOfRawBuffers = 0;
		RawBufferEx rawBuffers[NUMBER_OF_LOD_MAX] = {};
		ImageInfo uncompressedImageInfo;
	};

	class ImageLoaderMultipleBufferZAP : public ImageLoaderMultipleBuffer {
		public:
		GFX_TOOLS_RD_API const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		GFX_TOOLS_RD_API L_INT GFX_TOOLS_RD_CALL GetFormat();

		protected:
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
		public:
		static const L_INT FILE_TGA = 4;

		GFX_TOOLS_RD_API const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		GFX_TOOLS_RD_API L_INT GFX_TOOLS_RD_CALL GetFormat();
	};

	class ImageLoaderMultipleBufferPNG : public ImageLoaderMultipleBuffer {
		public:
		static const L_INT FILE_PNG = 75;

		GFX_TOOLS_RD_API const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		GFX_TOOLS_RD_API L_INT GFX_TOOLS_RD_CALL GetFormat();
	};

	class ImageLoaderMultipleBufferJPEG : public ImageLoaderMultipleBuffer {
		public:
		static const L_INT FILE_JPEG = 10;

		GFX_TOOLS_RD_API const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		GFX_TOOLS_RD_API L_INT GFX_TOOLS_RD_CALL GetFormat();
	};

	class ImageLoaderMultipleBufferBMP : public ImageLoaderMultipleBuffer {
		public:
		static const L_INT FILE_BMP = 6;

		GFX_TOOLS_RD_API const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension();
		GFX_TOOLS_RD_API L_INT GFX_TOOLS_RD_CALL GetFormat();
	};
}