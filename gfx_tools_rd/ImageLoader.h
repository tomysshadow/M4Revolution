#pragma once
#include "shared.h"
#include "base_rd.h"
#include "ares_base_rd.h"
#include "ImageInfo.h"
#include "PixelFormat.h"

namespace gfx_tools {
	class GFX_TOOLS_RD_API ImageLoader : public ares::Resource {
		public:
		size_t GFX_TOOLS_RD_CALL GetRawBufferTotalSize();
		bool GFX_TOOLS_RD_CALL GetImageInfo(ImageInfo &imageInfo);
		EnumPixelFormat GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		virtual FormatHint GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint) = 0;

		virtual unsigned char* GFX_TOOLS_RD_CALL GetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t rows
		) = 0;

		virtual unsigned char* GFX_TOOLS_RD_CALL ResizeLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t rows,
			char requestedQFactor,
			ImageInfo imageInfo,
			size_t requestedWidth,
			size_t requestedHeight,
			size_t requestedExtent
		) = 0;

		virtual unsigned char* GFX_TOOLS_RD_CALL SetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t rows,
			char requestedQFactor,
			ImageInfo &imageInfo,
			size_t requestedExtent
		) = 0;

		virtual unsigned char* GFX_TOOLS_RD_CALL CreateLODRawBuffer(unsigned int lod,size_t size) = 0;
		virtual unsigned char* GFX_TOOLS_RD_CALL SetLODRawBuffer(unsigned int lod, unsigned char* address, size_t size, ubi::RefCounted* refCountedPointer) = 0;
		virtual void GFX_TOOLS_RD_CALL GetLODRawBuffer(unsigned int lod, unsigned char* address, size_t* sizePointer) = 0;
		virtual unsigned char* GFX_TOOLS_RD_CALL GetLODRawBuffer(unsigned int lod) = 0;

		virtual unsigned char* GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		) = 0;

		virtual unsigned char* GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			unsigned int lod,
			unsigned char* address,
			size_t size,
			bool unknown,
			ubi::RefCounted &refCounted
		) = 0;
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		virtual GFX_TOOLS_RD_CALL ~ImageLoaderMultipleBuffer();
		FormatHint GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);

		unsigned char* GFX_TOOLS_RD_CALL GetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t rows
		);

		unsigned char* GFX_TOOLS_RD_CALL ResizeLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t rows,
			char requestedQFactor,
			ImageInfo imageInfo,
			size_t requestedWidth,
			size_t requestedHeight,
			ares::RectU32* rectU32Pointer
		);

		unsigned char* GFX_TOOLS_RD_CALL SetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t rows,
			char requestedQFactor,
			ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		);

		unsigned char* GFX_TOOLS_RD_CALL CreateLODRawBuffer(unsigned int lod, size_t size);
		unsigned char* GFX_TOOLS_RD_CALL SetLODRawBuffer(unsigned int lod, unsigned char* address, size_t size, ubi::RefCounted* refCountedPointer);
		void GFX_TOOLS_RD_CALL GetLODRawBuffer(unsigned int lod, unsigned char* address, size_t* sizePointer);
		unsigned char* GFX_TOOLS_RD_CALL GetLODRawBuffer(unsigned int lod);

		unsigned char* GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		);

		unsigned char* GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			unsigned int lod,
			unsigned char* address,
			size_t size,
			bool owner,
			ubi::RefCounted &refCounted
		);
	};

	class GFX_TOOLS_RD_API ImageLoaderBitmap : public ImageLoaderMultipleBuffer {
		public:
		virtual const char* GFX_TOOLS_RD_CALL GetExtension() = 0;
		virtual int GFX_TOOLS_RD_CALL GetFormat() = 0;
		virtual int GFX_TOOLS_RD_CALL CreateBitmapHandle(unsigned int lod, void* bitmapHandlePointer);
	};
}