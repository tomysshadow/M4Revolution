#pragma once
#include "shared.h"
#include "base_rd.h"
#include "ares_base_rd.h"
#include "RawBuffer.h"
#include "ImageInfo.h"
#include "PixelFormat.h"
#include <optional>
#include <stdint.h>

namespace gfx_tools {
	class GFX_TOOLS_RD_API ImageLoader : public ares::Resource {
		public:
		typedef unsigned int LOD;
		typedef unsigned long SIZE;
		typedef char Q_FACTOR;
		typedef unsigned long DIMENSION;

		RawBuffer::SIZE rawBufferTotalSize = 0;
		LOD numberOfLod = 0;
		RawBufferEx rawBuffers[NUMBER_OF_LOD_MAX] = {};
		std::optional<ValidatedImageInfo> validatedImageInfoOptional = std::nullopt;
		EnumPixelFormat enumPixelFormat = PIXELFORMAT_UNKNOWN;
		FormatHint formatHint = { false };

		RawBuffer::SIZE GFX_TOOLS_RD_CALL GetRawBufferTotalSize();
		bool GFX_TOOLS_RD_CALL GetImageInfo(ImageInfo &imageInfo);
		void GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		virtual GFX_TOOLS_RD_CALL ~ImageLoader();
		virtual void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint) = 0;

		virtual void GFX_TOOLS_RD_CALL GetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows
		) = 0;

		virtual void GFX_TOOLS_RD_CALL ResizeLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			DIMENSION requestedWidth,
			DIMENSION requestedHeight,
			ares::RectU32* rectU32Pointer
		) = 0;

		virtual void GFX_TOOLS_RD_CALL SetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		) = 0;

		virtual RawBuffer::POINTER GFX_TOOLS_RD_CALL CreateLODRawBuffer(LOD lod, RawBuffer::SIZE size) = 0;
		virtual void GFX_TOOLS_RD_CALL SetLODRawBuffer(LOD lod, RawBuffer::POINTER pointer, RawBuffer::SIZE size, ubi::RefCounted* refCountedPointer) = 0;
		virtual void GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod, RawBuffer::POINTER &pointer, RawBuffer::SIZE &size) = 0;
		virtual RawBuffer::POINTER GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod) = 0;

		virtual bool GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		) = 0;

		virtual void GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			RawBuffer::POINTER pointer,
			RawBuffer::SIZE size,
			bool owner,
			ubi::RefCounted* refCountedPointer
		) = 0;

		// these methods are normally only on the ImageLoaderMultipleBufferBitmap interface
		// but have been put here as pure virtual so I can safely add additional virtual methods
		virtual const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL GetFormat() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, HANDLE bitmapHandlePointer) = 0;

		protected:
		// these methods do not exist on the original ImageLoader
		// they are my own "how it should've been done" methods
		// which the others are built on top of
		virtual void GFX_TOOLS_RD_CALL GetImageInfoImpEx(const char* extension = 0) = 0;

		EnumPixelFormat GetEnumPixelFormatFromFormatHint(uint32_t bits, bool hasAlpha) const;
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		typedef void* HANDLE;

		SIZE numberOfRawBuffers = 0;

		SIZE GetNumberOfRawBuffers();

		virtual GFX_TOOLS_RD_CALL ~ImageLoaderMultipleBuffer();
		void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);

		void GFX_TOOLS_RD_CALL GetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows
		);

		void GFX_TOOLS_RD_CALL ResizeLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			DIMENSION requestedWidth,
			DIMENSION requestedHeight,
			ares::RectU32* rectU32Pointer
		);

		void GFX_TOOLS_RD_CALL SetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		);

		RawBuffer::POINTER GFX_TOOLS_RD_CALL CreateLODRawBuffer(LOD lod, RawBuffer::SIZE size);
		void GFX_TOOLS_RD_CALL SetLODRawBuffer(LOD lod, RawBuffer::POINTER pointer, RawBuffer::SIZE size, ubi::RefCounted* refCountedPointer);
		void GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod, RawBuffer::POINTER &pointer, RawBuffer::SIZE &size);
		RawBuffer::POINTER GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod);

		bool GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		);

		void GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			RawBuffer::POINTER pointer,
			RawBuffer::SIZE size,
			bool owner,
			ubi::RefCounted* refCountedPointer
		);

		protected:
		// TODO: the Bitmap interfaces will call ImageLoaderMultipleBuffer::GetImageInfoImpEx with the extension from their GetExtension
		void GFX_TOOLS_RD_CALL GetImageInfoImpEx(const char* extension = 0);
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBufferBitmap : public ImageLoaderMultipleBuffer {
		public:
		L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, HANDLE bitmapHandlePointer);
	};
}