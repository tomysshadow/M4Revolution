#pragma once
#include "shared.h"

namespace gfx_tools {
	class GFX_TOOLS_RD_API Configuration {
		private:
		static Configuration ms_currentConfiguration;

		public:
		unsigned long maxTextureWidth = 1024;
		unsigned long minTextureWidth = 1;
		unsigned long maxTextureHeight = 1024;
		unsigned long minTextureHeight = 1;
		unsigned long maxVolumeExtent = 1024;
		unsigned long minVolumeExtent = 1;
		bool dimensionsMakeSquare = false;
		bool dimensionsMakePowerOfTwo = false;
		bool upscale = true;

		static Configuration* Get();
		static void Set(Configuration const &configuration);

		Configuration();
	};
}