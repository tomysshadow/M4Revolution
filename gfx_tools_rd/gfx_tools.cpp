#include "gfx_tools.h"

namespace gfx_tools {
	Color16* convertHeightMapIntoDuDvBumpMapColor(
		unsigned long width,
		unsigned long height,
		Color32* inputPointer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputStride,
		Color16* outputPointer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputStride
	) {
		// this function normally returns a pointer to the output's end
		// if the output pixel format is neither of the supported ones, it returns the same pointer passed in
		bool luminance = outputEnumPixelFormat == EnumPixelFormat::PIXELFORMAT_XLVU_8888;

		if (!luminance && outputEnumPixelFormat != EnumPixelFormat::PIXELFORMAT_VU_88) {
			return outputPointer;
		}

		const size_t INPUT_CHANNEL_ALPHA = 0;
		const size_t INPUT_CHANNEL_LUMINANCE = 3;

		const size_t OUTPUT_CHANNEL_U = 0;
		const size_t OUTPUT_CHANNEL_V = 1;
		const size_t OUTPUT_CHANNEL_LUMINANCE = 2;

		Color32* endPointer = (Color32*)((unsigned char*)inputPointer + (height * inputStride) - inputStride);
		Color32* rowPointer = 0;
		Color32* colorPointer = 0;

		Color32* inputUColorPointer = 0;
		Color32* inputVColorPointer = 0;

		while (inputPointer <= endPointer) {
			rowPointer = inputPointer + width - 1;
			colorPointer = inputPointer;

			// inputVColorPointer should point to the pixel one row below
			inputUColorPointer = inputPointer;
			inputVColorPointer = (inputPointer == endPointer) ? inputPointer : (Color32*)((unsigned char*)inputVColorPointer + inputStride);

			while (colorPointer <= rowPointer) {
				// inputUColorPointer should point to the pixel one column right
				if (inputUColorPointer != rowPointer) {
					inputUColorPointer++;
				}

				Color32 &color = *colorPointer;
				Color32 &inputUColor = *inputUColorPointer;
				Color32 &inputVColor = *inputVColorPointer;
				Color16 &output = *outputPointer;

				output.channels[OUTPUT_CHANNEL_U] = color.channels[INPUT_CHANNEL_ALPHA] - inputUColor.channels[INPUT_CHANNEL_ALPHA];
				output.channels[OUTPUT_CHANNEL_V] = color.channels[INPUT_CHANNEL_ALPHA] - inputVColor.channels[INPUT_CHANNEL_ALPHA];

				if (luminance) {
					((Color32*)outputPointer)->channels[OUTPUT_CHANNEL_LUMINANCE] = color.channels[INPUT_CHANNEL_LUMINANCE];
					(Color32*)outputPointer++;
				} else {
					outputPointer++;
				}

				colorPointer++;
			}

			inputPointer = (Color32*)((unsigned char*)inputPointer + inputStride);
			outputPointer = (Color16*)((unsigned char*)outputPointer + outputStride);
		}
		return outputPointer;
	}

	GFX_TOOLS_RD_API unsigned char* ConvertHeightMapIntoDuDvBumpMap(
		unsigned long width,
		unsigned long height,
		unsigned char* inputPointer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputStride,
		unsigned char* outputPointer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputStride
	) {
		return (unsigned char*)convertHeightMapIntoDuDvBumpMapColor(
			width,
			height,
			(Color32*)inputPointer,
			inputEnumPixelFormat,
			inputStride,
			(Color16*)outputPointer,
			outputEnumPixelFormat,
			outputStride
		);
	}
};