#pragma once
#include "ImageLoader.h"
#include "IgnoreCaseComparer.h"
#include <map>
#include <string>

namespace gfx_tools {
	class ImageCreator {
		public:
		typedef ImageLoader* (GFX_TOOLS_CALL *ImageSerializerProc)();

		GFX_TOOLS_API static ImageLoader* GFX_TOOLS_CALL CreateLoader(char* extension);
		GFX_TOOLS_API static ImageLoader* GFX_TOOLS_CALL CreateLoaderFromFileName(const char* fileName);
		GFX_TOOLS_API static ImageLoader* GFX_TOOLS_CALL CreateAndFillLoaderForFile(char* fileName);
		GFX_TOOLS_API static void GFX_TOOLS_CALL RegisterImageSerializer(char* extension, ImageSerializerProc imageSerializerProc);
		GFX_TOOLS_API static ImageCreator* GFX_TOOLS_CALL CreateSingletonInstance();
		GFX_TOOLS_API static ImageCreator& GFX_TOOLS_CALL GetSingletonInstance();
		GFX_TOOLS_API static ImageCreator& GFX_TOOLS_CALL GetSingletonInstanceFast();

		private:
		typedef std::map<std::string, ImageSerializerProc, IgnoreCaseComparer> IMAGE_SERIALIZER_PROC_MAP;

		GFX_TOOLS_API static IMAGE_SERIALIZER_PROC_MAP extensionImageSerializerProcMap;

		GFX_TOOLS_API static ImageCreator* ms_SingletonInstance;

		static ImageLoader* GFX_TOOLS_CALL ImageSerializerZAP();
		static ImageLoader* GFX_TOOLS_CALL ImageSerializerTGA();
		static ImageLoader* GFX_TOOLS_CALL ImageSerializerPNG();
		static ImageLoader* GFX_TOOLS_CALL ImageSerializerJPEG();
		static ImageLoader* GFX_TOOLS_CALL ImageSerializerBMP();

		static void DestroySingletonInstance();

		GFX_TOOLS_API ImageLoader* GFX_TOOLS_CALL CreateLoaderImp(const char* extension);
		GFX_TOOLS_API ImageLoader* GFX_TOOLS_CALL CreateLoaderFromFileNameImp(const char* fileName);
		GFX_TOOLS_API ImageLoader* GFX_TOOLS_CALL CreateAndFillLoaderForFileImp(char* fileName);
		GFX_TOOLS_API void GFX_TOOLS_CALL RegisterImageSerializerImp(char* fileName, ImageSerializerProc imageSerializerProc);
	};
}