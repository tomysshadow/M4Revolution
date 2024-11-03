#pragma once
#include "shared.h"
#include "ImageLoader.h"
#include "IgnoreCaseComparer.h"
#include <map>
#include <string>

namespace gfx_tools {
	class ImageCreator {
		public:
		typedef ImageLoader* (GFX_TOOLS_RD_CALL *ImageSerializerProc)();

		GFX_TOOLS_RD_API static ImageLoader* GFX_TOOLS_RD_CALL CreateLoader(char* extension);
		GFX_TOOLS_RD_API static ImageLoader* GFX_TOOLS_RD_CALL CreateLoaderFromFileName(char const* fileName);
		GFX_TOOLS_RD_API static ImageLoader* GFX_TOOLS_RD_CALL CreateAndFillLoaderForFile(char* fileName);
		GFX_TOOLS_RD_API static void GFX_TOOLS_RD_CALL RegisterImageSerializer(char* extension, ImageSerializerProc imageSerializerProc);
		GFX_TOOLS_RD_API static ImageCreator* GFX_TOOLS_RD_CALL CreateSingletonInstance();
		GFX_TOOLS_RD_API static ImageCreator& GFX_TOOLS_RD_CALL GetSingletonInstance();
		GFX_TOOLS_RD_API static ImageCreator& GFX_TOOLS_RD_CALL GetSingletonInstanceFast();

		private:
		typedef std::map<std::string, ImageSerializerProc, IgnoreCaseComparer> IMAGE_SERIALIZER_PROC_MAP;

		GFX_TOOLS_RD_API static IMAGE_SERIALIZER_PROC_MAP EXTENSION_IMAGE_SERIALIZER_PROC_MAP;

		GFX_TOOLS_RD_API static ImageCreator* ms_SingletonInstance;

		static ImageLoader* GFX_TOOLS_RD_CALL ImageSerializerZAP();
		static ImageLoader* GFX_TOOLS_RD_CALL ImageSerializerTGA();
		static ImageLoader* GFX_TOOLS_RD_CALL ImageSerializerPNG();
		static ImageLoader* GFX_TOOLS_RD_CALL ImageSerializerJPEG();
		static ImageLoader* GFX_TOOLS_RD_CALL ImageSerializerBMP();

		static void DestroySingletonInstance();

		GFX_TOOLS_RD_API ImageLoader* GFX_TOOLS_RD_CALL CreateLoaderImp(char const* extension);
		GFX_TOOLS_RD_API ImageLoader* GFX_TOOLS_RD_CALL CreateLoaderFromFileNameImp(char const* fileName);
		GFX_TOOLS_RD_API ImageLoader* GFX_TOOLS_RD_CALL CreateAndFillLoaderForFileImp(char* fileName);
		GFX_TOOLS_RD_API void GFX_TOOLS_RD_CALL RegisterImageSerializerImp(char* fileName, ImageSerializerProc imageSerializerProc);
	};
}