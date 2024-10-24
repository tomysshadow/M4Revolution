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
		public:
		typedef unsigned __int64 MASK;
		typedef unsigned char BITS_PER_PIXEL;

		PixelFormat();

		PixelFormat(
			MASK maskRed,
			MASK maskGreen,
			MASK maskBlue,
			MASK maskAlpha,
			MASK maskPalette,
			BITS_PER_PIXEL bitsPerPixel,
			MASK maskDepth,
			MASK maskStencil
		);

		bool GFX_TOOLS_RD_CALL HasRed();
		bool GFX_TOOLS_RD_CALL HasGreen();
		bool GFX_TOOLS_RD_CALL HasBlue();
		bool GFX_TOOLS_RD_CALL HasAlpha();
		bool GFX_TOOLS_RD_CALL HasPalette();
		bool GFX_TOOLS_RD_CALL HasDepth();
		bool GFX_TOOLS_RD_CALL HasStencil();
		bool GFX_TOOLS_RD_CALL HasColor();
		bool GFX_TOOLS_RD_CALL HasBitsPerPixel();

		MASK GFX_TOOLS_RD_CALL GetMaskRed();
		MASK GFX_TOOLS_RD_CALL GetMaskGreen();
		MASK GFX_TOOLS_RD_CALL GetMaskBlue();
		MASK GFX_TOOLS_RD_CALL GetMaskAlpha();
		MASK GFX_TOOLS_RD_CALL GetMaskPalette();
		BITS_PER_PIXEL GFX_TOOLS_RD_CALL GetBitsPerPixel();
		MASK GFX_TOOLS_RD_CALL GetMaskDepth();
		MASK GFX_TOOLS_RD_CALL GetMaskStencil();

		static PixelFormat* GFX_TOOLS_RD_CALL GetPixelFormat(EnumPixelFormat enumPixelFormat);
		static EnumPixelFormat GFX_TOOLS_RD_CALL GetPixelFormatWithAlpha(EnumPixelFormat enumPixelFormat);
		static EnumPixelFormat GFX_TOOLS_RD_CALL GetPixelFormatWithoutAlpha(EnumPixelFormat enumPixelFormat);
		static char* GFX_TOOLS_RD_CALL GetPixelFormatString(EnumPixelFormat enumPixelFormat);

		private:
		MASK maskRed = 0;
		MASK maskGreen = 0;
		MASK maskBlue = 0;
		MASK maskAlpha = 0;
		MASK maskPalette = 0;
		BITS_PER_PIXEL bitsPerPixel = 0;
		MASK maskDepth = 0;
		MASK maskStencil = 0;
		bool hasColor = false;
		bool hasBitsPerPixel = false;

		static PixelFormat m_formatDescriptionTable[];
		static char* ms_formatNames[];
	};
}