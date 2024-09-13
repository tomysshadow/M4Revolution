#include "gfx_tools.h"

namespace gfx_tools {
	GFX_TOOLS_RD_API unsigned char* ConvertHeightMapIntoDuDvBumpMap(
		unsigned long width,
		unsigned long height,
		unsigned char* inputPointer,
		EnumPixelFormat inputEnumPixelFormat,
		unsigned long inputRowSize,
		unsigned char* outputPointer,
		EnumPixelFormat outputEnumPixelFormat,
		unsigned long outputRowSize
	) {
		// this function normally returns a pointer to the output's end
		// if the output pixel format is neither of the supported ones, it returns the same pointer passed in
		bool luminance = outputEnumPixelFormat == EnumPixelFormat::PIXELFORMAT_XLVU_8888;

		if (!luminance && outputEnumPixelFormat != EnumPixelFormat::PIXELFORMAT_VU_88) {
			return outputPointer;
		}

		const unsigned long INPUT_PIXEL_SIZE = 4;
		const unsigned long INPUT_LUMINANCE_POSITION = 3;

		const unsigned long OUTPUT_PIXEL_VU_88_SIZE = 2;
		const unsigned long OUTPUT_PIXEL_XLVU_8888_SIZE = 4;
		const unsigned long OUTPUT_U_POSITION = 0;
		const unsigned long OUTPUT_V_POSITION = 1;
		const unsigned long OUTPUT_LUMINANCE_POSITION = 2;

		unsigned long outputPixelSize = luminance ? OUTPUT_PIXEL_XLVU_8888_SIZE : OUTPUT_PIXEL_VU_88_SIZE;

		unsigned char* endPointer = inputPointer + (height * inputRowSize) - inputRowSize;
		unsigned char* rowPointer = 0;
		unsigned char* pixelPointer = 0;

		unsigned char* inputUPointer = 0;
		unsigned char* inputVPointer = 0;
		unsigned char* inputLuminancePointer = 0;

		unsigned char* outputUPointer = 0;
		unsigned char* outputVPointer = 0;
		unsigned char* outputLuminancePointer = 0;

		while (inputPointer <= endPointer) {
			rowPointer = inputPointer + (width * INPUT_PIXEL_SIZE) - INPUT_PIXEL_SIZE;
			pixelPointer = inputPointer;

			// inputVPointer should point to the pixel one row below
			inputUPointer = inputPointer;
			inputVPointer = (inputPointer == endPointer) ? inputPointer : inputPointer + inputRowSize;

			outputUPointer = outputPointer + OUTPUT_U_POSITION;
			outputVPointer = outputPointer + OUTPUT_V_POSITION;

			if (luminance) {
				inputLuminancePointer = inputPointer + INPUT_LUMINANCE_POSITION;
				outputLuminancePointer = outputPointer + OUTPUT_LUMINANCE_POSITION;
			}

			while (pixelPointer <= rowPointer) {
				// inputUPointer should point to the pixel one column right
				if (pixelPointer != rowPointer) {
					inputUPointer += INPUT_PIXEL_SIZE;
				}

				// the difference between the pixel and the one right of it
				*outputUPointer = *pixelPointer - *inputUPointer;
				outputUPointer += outputPixelSize;

				// the difference between the pixel and the one below it
				*outputVPointer = *pixelPointer - *inputVPointer;
				outputVPointer += outputPixelSize;

				if (luminance) {
					// copy the luminance from the input to the output
					*outputLuminancePointer = *inputLuminancePointer;
					outputLuminancePointer += outputPixelSize;

					inputLuminancePointer += INPUT_PIXEL_SIZE;
				}

				inputVPointer += INPUT_PIXEL_SIZE;
				pixelPointer += INPUT_PIXEL_SIZE;
			}

			inputPointer += inputRowSize;
			outputPointer += outputRowSize;
		}
		return outputPointer;
	}
};