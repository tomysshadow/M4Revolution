#pragma once
#include "shared.h"
#include "base_rd.h"
#include "ares_base_rd.h"
#include "RawBuffer.h"
#include "ImageInfo.h"
#include "PixelFormat.h"
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

		private:
		EnumPixelFormat GetEnumPixelFormatFromFormatHint(uint32_t bits, bool hasAlpha) const;
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		SIZE numberOfRawBuffers = 0;

		SIZE GetNumberOfRawBuffers();

		virtual GFX_TOOLS_RD_CALL ~ImageLoaderMultipleBuffer();
		virtual void GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);

		virtual void GFX_TOOLS_RD_CALL GetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows
		);

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
		);

		virtual void GFX_TOOLS_RD_CALL SetLOD(
			LOD lod,
			RawBuffer::POINTER pointer,
			SIZE stride,
			SIZE rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		);

		virtual RawBuffer::POINTER GFX_TOOLS_RD_CALL CreateLODRawBuffer(LOD lod, RawBuffer::SIZE size);
		virtual void GFX_TOOLS_RD_CALL SetLODRawBuffer(LOD lod, RawBuffer::POINTER pointer, RawBuffer::SIZE size, ubi::RefCounted* refCountedPointer);
		virtual void GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod, RawBuffer::POINTER &pointer, RawBuffer::SIZE &size);
		virtual RawBuffer::POINTER GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod);

		virtual bool GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		);

		virtual void GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			RawBuffer::POINTER pointer,
			RawBuffer::SIZE size,
			bool owner,
			ubi::RefCounted* refCountedPointer
		);
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBufferBitmap : public ImageLoaderMultipleBuffer {
		public:
		typedef void* HANDLE;

		virtual const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL GetFormat() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, HANDLE bitmapHandlePointer);
	};
}