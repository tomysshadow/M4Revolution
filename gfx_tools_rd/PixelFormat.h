#pragma once
#include "shared.h"
#include <map>

namespace gfx_tools {
	enum EnumPixelFormat {
		PIXELFORMAT_UNKNOWN = 0,
		PIXELFORMAT_RGB_888,
		PIXELFORMAT_ARGB_8888,
		PIXELFORMAT_XRGB_8888,
		PIXELFORMAT_RGB_565,
		PIXELFORMAT_XRGB_1555,
		PIXELFORMAT_ARGB_1555,
		PIXELFORMAT_ARGB_4444,
		PIXELFORMAT_RGB_332,
		PIXELFORMAT_A_8,
		PIXELFORMAT_ARGB_8332,
		PIXELFORMAT_XRGB_4444,
		PIXELFORMAT_ABGR_2_10_10_10,
		PIXELFORMAT_GR_16_16,
		PIXELFORMAT_AP_88,
		PIXELFORMAT_P_8,
		PIXELFORMAT_L_8,
		PIXELFORMAT_AL_88,
		PIXELFORMAT_AL_44,
		PIXELFORMAT_VU_88,
		PIXELFORMAT_LVU_655,
		PIXELFORMAT_XLVU_8888,
		PIXELFORMAT_QWVU_8888,
		PIXELFORMAT_VU_16_16,
		PIXELFORMAT_AWVU_2_10_10_10,
		PIXELFORMAT_UYVY_8888,
		PIXELFORMAT_YUY2_8888,
		PIXELFORMAT_DXT1,
		PIXELFORMAT_DXT2,
		PIXELFORMAT_DXT3,
		PIXELFORMAT_DXT4,
		PIXELFORMAT_DXT5,
		PIXELFORMAT_D_16_LOCKABLE,
		PIXELFORMAT_D_32,
		PIXELFORMAT_DS_15_1,
		PIXELFORMAT_DS_24_8,
		PIXELFORMAT_D_16,
		PIXELFORMAT_DX_24_8,
		PIXELFORMAT_DXS_24_4_4,
		PIXELFORMAT_BGR_888,
		PIXELFORMAT_ABGR_8888,
		PIXELFORMAT_XBGR_8888,
		PIXELFORMAT_BGR_565,
		PIXELFORMAT_XBGR_1555,
		PIXELFORMAT_ABGR_1555,
		PIXELFORMAT_ABGR_4444,
		PIXELFORMAT_BGR_233,
		PIXELFORMAT_ABGR_8233,
		PIXELFORMAT_XBGR_4444
	};

	class GFX_TOOLS_RD_API PixelFormat {
		private:
		unsigned __int64 maskRed = 0;
		unsigned __int64 maskGreen = 0;
		unsigned __int64 maskBlue = 0;
		unsigned __int64 maskAlpha = 0;
		unsigned __int64 maskPalette = 0;
		unsigned char bitsPerPixel = 0;
		unsigned __int64 maskDepth = 0;
		unsigned __int64 maskStencil = 0;
		bool hasColor = false;
		bool hasBitsPerPixel = false;

		static PixelFormat m_formatDescriptionTable[];
		static char* ms_formatNames[];

		public:
		PixelFormat(
			unsigned __int64 maskRed,
			unsigned __int64 maskGreen,
			unsigned __int64 maskBlue,
			unsigned __int64 maskAlpha,
			unsigned __int64 maskPalette,
			unsigned char bitsPerPixel,
			unsigned __int64 maskDepth,
			unsigned __int64 maskStencil
		);

		bool HasRed();
		bool HasGreen();
		bool HasBlue();
		bool HasAlpha();
		bool HasPalette();
		bool HasDepth();
		bool HasStencil();
		bool HasColor();
		bool HasBitsPerPixel();

		unsigned __int64 GetMaskRed();
		unsigned __int64 GetMaskGreen();
		unsigned __int64 GetMaskBlue();
		unsigned __int64 GetMaskAlpha();
		unsigned __int64 GetMaskPalette();
		unsigned char GetBitsPerPixel();
		unsigned __int64 GetMaskDepth();
		unsigned __int64 GetMaskStencil();

		static PixelFormat* GetPixelFormat(EnumPixelFormat enumPixelFormat);
		static EnumPixelFormat GetPixelFormatWithAlpha(EnumPixelFormat enumPixelFormat);
		static EnumPixelFormat GetPixelFormatWithoutAlpha(EnumPixelFormat enumPixelFormat);
		static char** GetPixelFormatString(EnumPixelFormat enumPixelFormat);
	};
}