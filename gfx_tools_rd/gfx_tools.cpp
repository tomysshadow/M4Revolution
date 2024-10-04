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

		M4Image::Color32* inputColorPointer = 0;
		M4Image::Color32* inputUColorPointer = 0;
		M4Image::Color32* inputVColorPointer = 0;

		M4Image::Color16* outputColorPointer = 0;

		while (inputPointer <= endPointer) {
			rowPointer = inputPointer + width - 1;

			// inputVColorPointer should point to the pixel one row below
			inputColorPointer = inputPointer;
			inputUColorPointer = inputPointer;
			inputVColorPointer = (inputPointer == endPointer) ? inputPointer : (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);

			outputColorPointer = outputPointer;

			while (inputColorPointer <= rowPointer) {
				// inputUColorPointer should point to the pixel one column right
				if (inputUColorPointer != rowPointer) {
					inputUColorPointer++;
				}

				M4Image::Color32 &inputColor = *inputColorPointer;
				M4Image::Color16 &outputColor = *outputColorPointer;

				outputColor.channels[OUTPUT_CHANNEL_U] = inputColor.channels[INPUT_CHANNEL_UV] - inputUColorPointer->channels[INPUT_CHANNEL_UV];
				outputColor.channels[OUTPUT_CHANNEL_V] = inputColor.channels[INPUT_CHANNEL_UV] - inputVColorPointer->channels[INPUT_CHANNEL_UV];

				if (luminance) {
					((M4Image::Color32*)outputColorPointer)->channels[OUTPUT_CHANNEL_LUMINANCE] = inputColor.channels[INPUT_CHANNEL_LUMINANCE];
					outputColorPointer = (M4Image::Color16*)((M4Image::Color32*)outputColorPointer + 1);
				} else {
					outputColorPointer++;
				}

				inputColorPointer++;
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
		const size_t INPUT_CHANNEL_XY = 0;

		const size_t OUTPUT_CHANNEL_B = 0;
		const size_t OUTPUT_CHANNEL_G = 1;
		const size_t OUTPUT_CHANNEL_R = 2;
		const size_t OUTPUT_CHANNEL_A = 3;

		const double MULTIPLIER = 127.0;
		const unsigned char BGR_GRAY = 128;
		const unsigned char ALPHA_OPAQUE = 255;

		M4Image::Color32* endPointer = (M4Image::Color32*)((unsigned char*)inputPointer + (height * inputStride) - inputStride);
		M4Image::Color32* rowPointer = 0;

		M4Image::Color32* inputColorPointer = 0;
		M4Image::Color32* inputXColorPointer = 0;
		M4Image::Color32* inputYColorPointer = 0;

		M4Image::Color32* outputColorPointer = 0;

		double x = 0.0;
		double y = 0.0;
		double z = 0.0;

		while (inputPointer <= endPointer) {
			rowPointer = inputPointer + width - 1;

			// inputYColorPointer should point to the pixel one row below
			inputColorPointer = inputPointer;
			inputXColorPointer = inputPointer;
			inputYColorPointer = (inputPointer == endPointer) ? inputPointer : (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);
			
			outputColorPointer = outputPointer;

			while (inputColorPointer <= rowPointer) {
				// inputXColorPointer should point to the pixel one column right
				if (inputXColorPointer != rowPointer) {
					inputXColorPointer++;
				}

				M4Image::Color32 &inputColor = *inputColorPointer;
				M4Image::Color32 &outputColor = *outputColorPointer;

				x = strength * (signed char)(inputXColorPointer->channels[INPUT_CHANNEL_XY] - inputColor.channels[INPUT_CHANNEL_XY]);
				y = strength * (signed char)(inputYColorPointer->channels[INPUT_CHANNEL_XY] - inputColor.channels[INPUT_CHANNEL_XY]);
				z = 1.0 / sqrt(x * x + y * y + 1.0) * MULTIPLIER;

				outputColor.channels[OUTPUT_CHANNEL_B] = BGR_GRAY + (unsigned char)z;
				outputColor.channels[OUTPUT_CHANNEL_G] = BGR_GRAY - (unsigned char)(y * z);
				outputColor.channels[OUTPUT_CHANNEL_R] = BGR_GRAY - (unsigned char)(x * z);
				outputColor.channels[OUTPUT_CHANNEL_A] = ALPHA_OPAQUE;

				outputColorPointer++;
				inputColorPointer++;
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