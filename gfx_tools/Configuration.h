#pragma once

namespace gfx_tools {
	class Configuration {
		private:
		static Configuration ms_currentConfiguration;

		public:
		using Dimension = unsigned long;

		Dimension maxTextureWidth = 1024;
		Dimension minTextureWidth = 1;
		Dimension maxTextureHeight = 1024;
		Dimension minTextureHeight = 1;
		Dimension maxVolumeExtent = 1024;
		Dimension minVolumeExtent = 1;
		bool dimensionsMakeSquare = false;
		bool dimensionsMakePowerOfTwo = false;
		bool toNext = true;

		GFX_TOOLS_API static Configuration const GFX_TOOLS_CALL &Get();
		GFX_TOOLS_API static void GFX_TOOLS_CALL Set(Configuration const &configuration);

		GFX_TOOLS_API Configuration();
	};
}