#pragma once

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

	class PixelFormat {
		public:
		using Mask = unsigned __int64;
		using BitsPerPixel = unsigned char;

		GFX_TOOLS_API PixelFormat();

		GFX_TOOLS_API PixelFormat(
			Mask maskRed,
			Mask maskGreen,
			Mask maskBlue,
			Mask maskAlpha,
			Mask maskPalette,
			BitsPerPixel bitsPerPixel,
			Mask maskDepth,
			Mask maskStencil
		);

		GFX_TOOLS_API bool GFX_TOOLS_CALL HasRed();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasGreen();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasBlue();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasAlpha();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasPalette();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasDepth();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasStencil();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasColor();
		GFX_TOOLS_API bool GFX_TOOLS_CALL HasBitsPerPixel();

		GFX_TOOLS_API Mask GFX_TOOLS_CALL GetMaskRed();
		GFX_TOOLS_API Mask GFX_TOOLS_CALL GetMaskGreen();
		GFX_TOOLS_API Mask GFX_TOOLS_CALL GetMaskBlue();
		GFX_TOOLS_API Mask GFX_TOOLS_CALL GetMaskAlpha();
		GFX_TOOLS_API Mask GFX_TOOLS_CALL GetMaskPalette();
		GFX_TOOLS_API BitsPerPixel GFX_TOOLS_CALL GetBitsPerPixel();
		GFX_TOOLS_API Mask GFX_TOOLS_CALL GetMaskDepth();
		GFX_TOOLS_API Mask GFX_TOOLS_CALL GetMaskStencil();

		GFX_TOOLS_API static PixelFormat* GFX_TOOLS_CALL GetPixelFormat(EnumPixelFormat enumPixelFormat);
		GFX_TOOLS_API static EnumPixelFormat GFX_TOOLS_CALL GetEnumPixelFormatWithAlpha(EnumPixelFormat enumPixelFormat);
		GFX_TOOLS_API static EnumPixelFormat GFX_TOOLS_CALL GetEnumPixelFormatWithoutAlpha(EnumPixelFormat enumPixelFormat);
		GFX_TOOLS_API static char* GFX_TOOLS_CALL GetPixelFormatString(EnumPixelFormat enumPixelFormat);

		private:
		Mask maskRed = 0;
		Mask maskGreen = 0;
		Mask maskBlue = 0;
		Mask maskAlpha = 0;
		Mask maskPalette = 0;
		BitsPerPixel bitsPerPixel = 0;
		Mask maskDepth = 0;
		Mask maskStencil = 0;
		bool hasColor = false;
		bool hasBitsPerPixel = false;

		static PixelFormat m_formatDescriptionTable[];
		static char* ms_formatNames[];
	};
}