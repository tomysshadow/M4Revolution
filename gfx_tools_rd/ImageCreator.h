#pragma once
#include "ImageLoader.h"

namespace gfx_tools {
	class ImageCreator {
		public:
		typedef ImageLoader* (GFX_TOOLS_RD_CALL *ImageSerializerProc)();

		GFX_TOOLS_RD_API static ImageLoader* GFX_TOOLS_RD_CALL CreateLoader(char* extension);
		GFX_TOOLS_RD_API static ImageLoader* GFX_TOOLS_RD_CALL CreateLoaderFromFileName(char const* fileName);
		GFX_TOOLS_RD_API static ImageLoader* GFX_TOOLS_RD_CALL CreateAndFillLoaderForFile(char* fileName);
		GFX_TOOLS_RD_API static ImageCreator* GFX_TOOLS_RD_CALL CreateSingletonInstance();
		GFX_TOOLS_RD_API static ImageCreator& GFX_TOOLS_RD_CALL GetSingletonInstance();
		GFX_TOOLS_RD_API static ImageCreator& GFX_TOOLS_RD_CALL GetSingletonInstanceFast();
		GFX_TOOLS_RD_API static void GFX_TOOLS_RD_CALL RegisterImageSerializer(char* extension);

		private:
		ImageLoader* GFX_TOOLS_RD_CALL CreateLoaderImp(char const* extension);
		ImageLoader* GFX_TOOLS_RD_CALL CreateLoaderFromFileNameImp(char const* fileName);
		ImageLoader* GFX_TOOLS_RD_CALL CreateAndFillLoaderForFile(char* fileName);
	};
}