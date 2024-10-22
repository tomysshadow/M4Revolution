#pragma once
#include "shared.h"
#include "base_rd.h"
#include "ares_base_rd.h"
#include "PixelFormat.h"
#include "ImageInfo.h"
#include "ValidatedImageInfo.h"

namespace gfx_tools {
	typedef int FormatHint;

	class GFX_TOOLS_RD_API ImageLoader : public ares::Resource {
		public:
		size_t GetRawBufferTotalSize();
		bool GetImageInfo(ImageInfo &imageInfo);
		EnumPixelFormat SetPixelFormat(EnumPixelFormat enumPixelFormat);

		virtual FormatHint SetHint(FormatHint formatHint) = 0;

		virtual unsigned char* GetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t height
		) = 0;

		virtual unsigned char* ResizeLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t height,
			char requestedQFactor,
			ImageInfo imageInfo,
			size_t requestedWidth,
			size_t requestedHeight,
			size_t requestedExtent
		) = 0;

		virtual unsigned char* SetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t height,
			char requestedQFactor,
			ImageInfo &imageInfo,
			size_t requestedExtent
		) = 0;

		virtual unsigned char* CreateLODRawBuffer(unsigned int lod,size_t size) = 0;
		virtual unsigned char* SetLODRawBuffer(unsigned int lod, unsigned char* address, size_t size, ubi::RefCounted* refCountedPointer) = 0;
		virtual void GetLODRawBuffer(unsigned int lod, unsigned char* address, size_t* sizePointer) = 0;
		virtual unsigned char* GetLODRawBuffer(unsigned int lod) = 0;

		virtual unsigned char* GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		) = 0;

		virtual unsigned char* SetLODRawBufferImp(
			unsigned int lod,
			unsigned char* address,
			size_t size,
			bool unknown,
			ubi::RefCounted &refCounted
		) = 0;
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		FormatHint SetHint(FormatHint formatHint);

		unsigned char* GetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t height
		);

		unsigned char* ResizeLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t height,
			char requestedQFactor,
			ImageInfo imageInfo,
			size_t requestedWidth,
			size_t requestedHeight,
			size_t requestedExtent
		);

		unsigned char* SetLODBuffer(
			unsigned int lod,
			unsigned char* address,
			size_t stride,
			size_t height,
			char requestedQFactor,
			ImageInfo &imageInfo,
			size_t requestedExtent
		);

		unsigned char* CreateLODRawBuffer(unsigned int lod, size_t size);
		unsigned char* SetLODRawBuffer(unsigned int lod, unsigned char* address, size_t size, ubi::RefCounted* refCountedPointer);
		void GetLODRawBuffer(unsigned int lod, unsigned char* address, size_t* sizePointer);
		unsigned char* GetLODRawBuffer(unsigned int lod);

		unsigned char* GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		);

		unsigned char* SetLODRawBufferImp(
			unsigned int lod,
			unsigned char* address,
			size_t size,
			bool unknown,
			ubi::RefCounted &refCounted
		);
	};

	class GFX_TOOLS_RD_API ImageLoaderBitmap : public ImageLoaderMultipleBuffer {
		public:
		virtual const char* GetExtension() = 0;
		virtual int GetFormat() = 0;
		virtual int CreateBitmapHandle(unsigned int lod, void* bitmapHandlePointer);
	};
}