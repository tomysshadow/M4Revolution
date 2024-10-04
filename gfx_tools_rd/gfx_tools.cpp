#include "gfx_tools.h"
#include <math.h>
#include <M4Image/M4Image.h>

namespace gfx_tools {
	M4Image::Color16* convertHeightMapIntoDuDvBumpMapColor(
		unsigned long width,
		unsigned long height,
		M4Image::Color32* inputPointer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputStride,
		M4Image::Color16* outputPointer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputStride
	) {
		// this function normally returns a pointer to the output's end
		// if the output pixel format is neither of the supported ones, it returns the same pointer passed in
		bool luminance = outputEnumPixelFormat == EnumPixelFormat::PIXELFORMAT_XLVU_8888;

		if (!luminance && outputEnumPixelFormat != EnumPixelFormat::PIXELFORMAT_VU_88) {
			return outputPointer;
		}

		const size_t INPUT_CHANNEL_UV = 0;
		const size_t INPUT_CHANNEL_LUMINANCE = 3;

		const size_t OUTPUT_CHANNEL_U = 0;
		const size_t OUTPUT_CHANNEL_V = 1;
		const size_t OUTPUT_CHANNEL_LUMINANCE = 2;

		M4Image::Color32* endPointer = (M4Image::Color32*)((unsigned char*)inputPointer + (height * inputStride) - inputStride);
		M4Image::Color32* rowPointer = 0;
		M4Image::Color32* colorPointer = 0;

		M4Image::Color32* inputUColorPointer = 0;
		M4Image::Color32* inputVColorPointer = 0;

		while (inputPointer <= endPointer) {
			rowPointer = inputPointer + width - 1;
			colorPointer = inputPointer;

			// inputVColorPointer should point to the pixel one row below
			inputUColorPointer = inputPointer;
			inputVColorPointer = (inputPointer == endPointer) ? inputPointer : (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);

			while (colorPointer <= rowPointer) {
				// inputUColorPointer should point to the pixel one column right
				if (inputUColorPointer != rowPointer) {
					inputUColorPointer++;
				}

				M4Image::Color32 &color = *colorPointer;
				M4Image::Color16 &output = *outputPointer;

				output.channels[OUTPUT_CHANNEL_U] = color.channels[INPUT_CHANNEL_UV] - inputUColorPointer->channels[INPUT_CHANNEL_UV];
				output.channels[OUTPUT_CHANNEL_V] = color.channels[INPUT_CHANNEL_UV] - inputVColorPointer->channels[INPUT_CHANNEL_UV];

				if (luminance) {
					((M4Image::Color32*)outputPointer)->channels[OUTPUT_CHANNEL_LUMINANCE] = color.channels[INPUT_CHANNEL_LUMINANCE];
					outputPointer = (M4Image::Color16*)((M4Image::Color32*)outputPointer + 1);
				} else {
					outputPointer++;
				}

				colorPointer++;
			}

			inputPointer = (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);
			outputPointer = (M4Image::Color16*)((unsigned char*)outputPointer + outputStride);
		}
		return outputPointer;
	}

	M4Image::Color32* convertHeightMapIntoNormalMap(
		unsigned long width,
		unsigned long height,
		M4Image::Color32* inputPointer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputStride,
		M4Image::Color32* outputPointer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputStride,
		double strength
	) {
		const size_t OUTPUT_CHANNEL_B = 0;
		const size_t OUTPUT_CHANNEL_G = 1;
		const size_t OUTPUT_CHANNEL_R = 2;
		const size_t OUTPUT_CHANNEL_A = 3;

		const double MULTIPLIER = 127.0;
		const unsigned char COLOR_GRAY = 128;
		const unsigned char ALPHA_OPAQUE = 255;

		M4Image::Color32* endPointer = (M4Image::Color32*)((unsigned char*)inputPointer + (height * inputStride) - inputStride);
		M4Image::Color32* rowPointer = 0;
		M4Image::Color32* colorPointer = 0;

		M4Image::Color32* inputXColorPointer = 0;
		M4Image::Color32* inputYColorPointer = 0;

		double x = 0.0;
		double y = 0.0;
		double z = 0.0;

		while (inputPointer <= endPointer) {
			rowPointer = inputPointer + width - 1;
			colorPointer = inputPointer;

			// inputYColorPointer should point to the pixel one row below
			inputXColorPointer = inputPointer;
			inputYColorPointer = (inputPointer == endPointer) ? inputPointer : (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);

			while (colorPointer <= rowPointer) {
				// inputXColorPointer should point to the pixel one column right
				if (inputXColorPointer != rowPointer) {
					inputXColorPointer++;
				}

				M4Image::Color32 &color = *colorPointer;
				M4Image::Color32 &output = *outputPointer;

				x = strength * (inputXColorPointer->channels[0] - color.channels[0]);
				y = strength * (inputYColorPointer->channels[0] - color.channels[0]);
				z = 1.0 / sqrt(x * x + y * y + 1.0) * MULTIPLIER;

				output.channels[OUTPUT_CHANNEL_B] = COLOR_GRAY + (unsigned char)z;
				output.channels[OUTPUT_CHANNEL_G] = COLOR_GRAY - (unsigned char)(y * z);
				output.channels[OUTPUT_CHANNEL_R] = COLOR_GRAY - (unsigned char)(x * z);
				output.channels[OUTPUT_CHANNEL_A] = ALPHA_OPAQUE;

				outputPointer++;
				colorPointer++;
			}

			inputPointer = (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);
			outputPointer = (M4Image::Color32*)((unsigned char*)outputPointer + outputStride);
		}
		return outputPointer;
	}

	unsigned char* ConvertHeightMapIntoDuDvBumpMap(
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
			(M4Image::Color32*)inputPointer,
			inputEnumPixelFormat,
			inputStride,
			(M4Image::Color16*)outputPointer,
			outputEnumPixelFormat,
			outputStride
		);
	}

	unsigned char* ConvertHeightMapIntoNormalMap(
		unsigned long width,
		unsigned long height,
		unsigned char* inputPointer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputStride,
		unsigned char* outputPointer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputStride,
		float strength
	) {
		return (unsigned char*)convertHeightMapIntoNormalMap(
			width,
			height,
			(M4Image::Color32*)inputPointer,
			inputEnumPixelFormat,
			inputStride,
			(M4Image::Color32*)outputPointer,
			outputEnumPixelFormat,
			outputStride,
			strength
		);
	}
};