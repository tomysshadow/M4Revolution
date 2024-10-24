#pragma once
#include "shared.h"

namespace gfx_tools {
	class GFX_TOOLS_RD_API Configuration {
		private:
		static Configuration ms_currentConfiguration;

		public:
		typedef unsigned long DIMENSION;

		DIMENSION maxTextureWidth = 1024;
		DIMENSION minTextureWidth = 1;
		DIMENSION maxTextureHeight = 1024;
		DIMENSION minTextureHeight = 1;
		DIMENSION maxVolumeExtent = 1024;
		DIMENSION minVolumeExtent = 1;
		bool dimensionsMakeSquare = false;
		bool dimensionsMakePowerOfTwo = false;
		bool upscale = true;

		static Configuration const GFX_TOOLS_RD_CALL &Get();
		static void GFX_TOOLS_RD_CALL Set(Configuration const &configuration);

		Configuration();
	};
}