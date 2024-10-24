#pragma once
#include "shared.h"
#include "base_rd.h"
#include "ares_base_rd.h"
#include "ImageInfo.h"
#include "PixelFormat.h"

namespace gfx_tools {
	class GFX_TOOLS_RD_API ImageLoader : public ares::Resource {
		public:
		typedef unsigned char* BUFFER;
		typedef unsigned long BUFFER_SIZE;
		typedef unsigned int LOD;
		typedef char Q_FACTOR;

		BUFFER_SIZE GFX_TOOLS_RD_CALL GetRawBufferTotalSize();
		bool GFX_TOOLS_RD_CALL GetImageInfo(ImageInfo &imageInfo);
		EnumPixelFormat GFX_TOOLS_RD_CALL SetPixelFormat(EnumPixelFormat enumPixelFormat);

		virtual FormatHint GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint) = 0;

		virtual BUFFER GFX_TOOLS_RD_CALL GetLODBuffer(
			LOD lod,
			BUFFER buffer,
			size_t stride,
			size_t rows
		) = 0;

		virtual BUFFER GFX_TOOLS_RD_CALL ResizeLODBuffer(
			LOD lod,
			BUFFER buffer,
			size_t stride,
			size_t rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			size_t requestedWidth,
			size_t requestedHeight,
			size_t requestedExtent
		) = 0;

		virtual BUFFER GFX_TOOLS_RD_CALL SetLODBuffer(
			LOD lod,
			BUFFER buffer,
			size_t stride,
			size_t rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			size_t requestedExtent
		) = 0;

		virtual BUFFER GFX_TOOLS_RD_CALL CreateLODRawBuffer(LOD lod, BUFFER_SIZE size) = 0;
		virtual BUFFER GFX_TOOLS_RD_CALL SetLODRawBuffer(LOD lod, BUFFER buffer, BUFFER_SIZE size, ubi::RefCounted* refCountedPointer) = 0;
		virtual void GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod, BUFFER buffer, BUFFER_SIZE &size) = 0;
		virtual BUFFER GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod) = 0;

		virtual bool GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		) = 0;

		virtual BUFFER GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			BUFFER buffer,
			BUFFER_SIZE size,
			bool owner,
			ubi::RefCounted* refCountedPointer
		) = 0;
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBuffer : public ImageLoader {
		public:
		virtual GFX_TOOLS_RD_CALL ~ImageLoaderMultipleBuffer();

		unsigned long GetNumberOfRawBuffers();

		FormatHint GFX_TOOLS_RD_CALL SetHint(FormatHint formatHint);

		BUFFER GFX_TOOLS_RD_CALL GetLODBuffer(
			LOD lod,
			BUFFER buffer,
			size_t stride,
			size_t rows
		);

		BUFFER GFX_TOOLS_RD_CALL ResizeLODBuffer(
			LOD lod,
			BUFFER buffer,
			size_t stride,
			size_t rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			size_t requestedWidth,
			size_t requestedHeight,
			ares::RectU32* rectU32Pointer
		);

		BUFFER GFX_TOOLS_RD_CALL SetLODBuffer(
			LOD lod,
			BUFFER buffer,
			size_t stride,
			size_t rows,
			Q_FACTOR requestedQFactor,
			ImageInfo &imageInfo,
			ares::RectU32* rectU32Pointer
		);

		BUFFER GFX_TOOLS_RD_CALL CreateLODRawBuffer(LOD lod, BUFFER_SIZE size);
		BUFFER GFX_TOOLS_RD_CALL SetLODRawBuffer(LOD lod, BUFFER buffer, BUFFER_SIZE size, ubi::RefCounted* refCountedPointer);
		void GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod, BUFFER buffer, BUFFER_SIZE &size);
		BUFFER GFX_TOOLS_RD_CALL GetLODRawBuffer(LOD lod);

		bool GFX_TOOLS_RD_CALL GetImageInfoImp(
			ValidatedImageInfo &validatedImageInfo
		);

		BUFFER GFX_TOOLS_RD_CALL SetLODRawBufferImp(
			LOD lod,
			BUFFER buffer,
			BUFFER_SIZE size,
			bool owner,
			ubi::RefCounted* refCountedPointer
		);
	};

	class GFX_TOOLS_RD_API ImageLoaderMultipleBufferBitmap : public ImageLoaderMultipleBuffer {
		public:
		virtual const L_TCHAR* GFX_TOOLS_RD_CALL GetExtension() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL GetFormat() = 0;
		virtual L_INT GFX_TOOLS_RD_CALL CreateBitmapHandle(LOD lod, void* bitmapHandlePointer);
	};
}