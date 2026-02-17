#pragma once

namespace gfx_tools {
	class Configuration {
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
		bool toNext = true;

		GFX_TOOLS_API static Configuration const GFX_TOOLS_CALL &Get();
		GFX_TOOLS_API static void GFX_TOOLS_CALL Set(Configuration const &configuration);

		GFX_TOOLS_API Configuration();
	};
}