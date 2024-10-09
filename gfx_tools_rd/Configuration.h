#pragma once
#include "shared.h"

namespace gfx_tools {
	class GFX_TOOLS_RD_API Configuration {
		private:
		unsigned long maxTextureWidth = 1024;
		unsigned long minTextureWidth = 1;
		unsigned long maxTextureHeight = 1024;
		unsigned long minTextureHeight = 1;
		unsigned long maxVolumeExtent = 1024;
		unsigned long minVolumeExtent = 1;
		bool dimensionsSquare = false;
		bool dimensionsPowerOfTwo = false;
		bool dimensionsUpscale = true;

		static Configuration ms_currentConfiguration;

		public:
		Configuration();
		void Set(Configuration const &configuration);
	};
}