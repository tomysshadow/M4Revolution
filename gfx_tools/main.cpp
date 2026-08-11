#include "pch.h"
#include "main.h"
#include "base.h"
#include "Validate.h"
#include <math.h>
#include <M4Image.h>

namespace gfx_tools {
	template<bool luminance>
	void convertHeightMapIntoDuDvBumpMapColor(
		Dimension width, Dimension height,
		M4Image::Color32* inputPointer, Stride inputStride,
		M4Image::Color16* outputPointer, Stride outputStride
	) {
		static constexpr size_t INPUT_CHANNEL_UV = 0;
		static constexpr size_t INPUT_CHANNEL_LUMINANCE = 3;

		static constexpr size_t OUTPUT_CHANNEL_DU = 0;
		static constexpr size_t OUTPUT_CHANNEL_DV = 1;
		static constexpr size_t OUTPUT_CHANNEL_LUMINANCE = 2;

		M4Image::Color32* endPointer = (M4Image::Color32*)((unsigned char*)inputPointer + ((size_t)height * (size_t)inputStride) - inputStride);
		M4Image::Color32* rowPointer = nullptr;

		M4Image::Color32* inputColorPointer = nullptr;
		M4Image::Color32* inputUColorPointer = nullptr;
		M4Image::Color32* inputVColorPointer = nullptr;

		M4Image::Color16* outputColorPointer = nullptr;

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

				// DU is done last so input and output buffer may be the same
				outputColor.channels[OUTPUT_CHANNEL_DV] = (unsigned char)(inputColor.channels[INPUT_CHANNEL_UV]
					- inputVColorPointer++->channels[INPUT_CHANNEL_UV]);

				outputColor.channels[OUTPUT_CHANNEL_DU] = (unsigned char)(inputColor.channels[INPUT_CHANNEL_UV]
					- inputUColorPointer->channels[INPUT_CHANNEL_UV]);

				if constexpr (luminance) {
					((M4Image::Color32*)outputColorPointer)->channels[OUTPUT_CHANNEL_LUMINANCE] =
						inputColorPointer->channels[INPUT_CHANNEL_LUMINANCE];
					
					outputColorPointer = (M4Image::Color16*)((M4Image::Color32*)outputColorPointer + 1);
				} else {
					outputColorPointer++;
				}

				inputColorPointer++;
			}

			inputPointer = (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);
			outputPointer = (M4Image::Color16*)((unsigned char*)outputPointer + outputStride);
		}
	}

	void convertHeightMapIntoNormalMapColor(
		Dimension width, Dimension height,
		M4Image::Color32* inputPointer, Stride inputStride,
		M4Image::Color32* outputPointer, Stride outputStride,
		double strength
	) {
		static constexpr size_t INPUT_CHANNEL_XY = 0;

		static constexpr size_t OUTPUT_CHANNEL_B = 0;
		static constexpr size_t OUTPUT_CHANNEL_G = 1;
		static constexpr size_t OUTPUT_CHANNEL_R = 2;
		static constexpr size_t OUTPUT_CHANNEL_A = 3;

		static constexpr double MULTIPLIER = 127.0;
		static constexpr unsigned char BGR_GRAY = 128;
		static constexpr unsigned char ALPHA_OPAQUE = 255;

		M4Image::Color32* endPointer = (M4Image::Color32*)((unsigned char*)inputPointer + ((size_t)height * (size_t)inputStride) - inputStride);
		M4Image::Color32* rowPointer = nullptr;

		M4Image::Color32* inputColorPointer = nullptr;
		M4Image::Color32* inputXColorPointer = nullptr;
		M4Image::Color32* inputYColorPointer = nullptr;

		M4Image::Color32* outputColorPointer = nullptr;

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

				// these specifically need to be signed, we want them to either be negative or positive
				// (but we only cast to signed char after doing the subtraction, as it's entirely expected this could underflow)
				x = strength * (inputXColorPointer->channels[INPUT_CHANNEL_XY] - inputColor.channels[INPUT_CHANNEL_XY]);
				y = strength * (inputYColorPointer++->channels[INPUT_CHANNEL_XY] - inputColor.channels[INPUT_CHANNEL_XY]);
				z = 1.0 / sqrt(x * x + y * y + 1.0) * MULTIPLIER;

				outputColor.channels[OUTPUT_CHANNEL_B] = (unsigned char)(BGR_GRAY + (unsigned char)z);
				outputColor.channels[OUTPUT_CHANNEL_G] = (unsigned char)(BGR_GRAY - (unsigned char)(y * z));
				outputColor.channels[OUTPUT_CHANNEL_R] = (unsigned char)(BGR_GRAY - (unsigned char)(x * z));
				outputColor.channels[OUTPUT_CHANNEL_A] = ALPHA_OPAQUE;

				outputColorPointer++;
				inputColorPointer++;
			}

			inputPointer = (M4Image::Color32*)((unsigned char*)inputPointer + inputStride);
			outputPointer = (M4Image::Color32*)((unsigned char*)outputPointer + outputStride);
		}
	}

	using RefCount = unsigned long;

	static RefCount refCount = 0;
	static bool initialized = false;

	void Init() {
		if (refCount++) {
			return;
		}

		if (initialized) {
			return;
		}

		static constexpr ubi::ErrorManager::Mask INITIALIZED = 0x00000040;

		ubi::ErrorManager &errorManager = ubi::ErrorManager::GetSingletonInstance();
		errorManager.SetSystemFlag(errorManager.RegisterCategory(0, "Gfx_Tools"), INITIALIZED, true);
		initialized = true;
	}

	void Shutdown() {
		if (refCount) {
			refCount--;
		}
	}

	void ConvertHeightMapIntoDuDvBumpMap(
		Dimension width, Dimension height,
		unsigned char* inputPointer, EnumPixelFormat inputEnumPixelFormat, Stride inputStride,
		unsigned char* outputPointer, EnumPixelFormat outputEnumPixelFormat, Stride outputStride
	) {
		bool luminance = outputEnumPixelFormat == EnumPixelFormat::PIXELFORMAT_XLVU_8888;

		if (!luminance && outputEnumPixelFormat != EnumPixelFormat::PIXELFORMAT_VU_88) {
			return;
		}

		if (luminance) {
			convertHeightMapIntoDuDvBumpMapColor<true>(
				width, height,
				(M4Image::Color32*)inputPointer, inputStride,
				(M4Image::Color16*)outputPointer, outputStride
			);
		} else {
			convertHeightMapIntoDuDvBumpMapColor<false>(
				width, height,
				(M4Image::Color32*)inputPointer, inputStride,
				(M4Image::Color16*)outputPointer, outputStride
			);
		}
	}

	void ConvertHeightMapIntoNormalMap(
		Dimension width, Dimension height,
		unsigned char* inputPointer, EnumPixelFormat inputEnumPixelFormat, Stride inputStride,
		unsigned char* outputPointer, EnumPixelFormat outputEnumPixelFormat, Stride outputStride,
		float strength
	) {
		convertHeightMapIntoNormalMapColor(
			width, height,
			(M4Image::Color32*)inputPointer, inputStride,
			(M4Image::Color32*)outputPointer, outputStride,
			strength
		);
	}
};

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(
	size_t _Size
	) {
	ubi::Allocator &generalAlloc = *ubi::Mem::GetGeneralAlloc();
	return generalAlloc.Malloc(_Size);
}

void __CRTDECL operator delete(
	void* _Block
	) {
	ubi::Allocator &allocator = *ubi::Allocator::GetOwner(_Block);
	allocator.Free(_Block);
}

_Ret_notnull_ _Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR void* __CRTDECL operator new(
	size_t           _Size,
	std::align_val_t _Al
	) {
	size_t padding = (size_t)_Al - 1;
	size_t offset = padding + sizeof(void*);

	try {
		Validate::overflow(offset, _Size);
	} catch (const std::invalid_argument&) {
		throw std::bad_alloc();
	}

	ubi::Allocator &generalAlloc = *ubi::Mem::GetGeneralAlloc();
	void* block = generalAlloc.Malloc(offset + _Size);
	void** aligned = (void**)(((uintptr_t)block + offset) & ~padding);

	aligned[-1] = block;
	return aligned;
}

void __CRTDECL operator delete(
	void*            _Block,
	std::align_val_t _Al
	) noexcept {
	if (!_Block) {
		return;
	}

	ubi::Allocator &allocator = *ubi::Allocator::GetOwner(_Block);
	allocator.Free(((void**)_Block)[-1]);
}

void* mallocProc(size_t size) {
	ubi::Allocator &generalAlloc = *ubi::Mem::GetGeneralAlloc();
	return generalAlloc.MallocNoThrow(size);
}

void freeProc(void* block) {
	ubi::Allocator &allocator = *ubi::Allocator::GetOwner(block);
	allocator.Free(block);
}

void* reAllocProc(void* block, size_t size) {
	ubi::Allocator &generalAlloc = *ubi::Mem::GetGeneralAlloc();
	return generalAlloc.ReAllocNoThrow(block, size);
}

static constexpr M4Image::Allocator ubiAllocator = M4Image::Allocator(mallocProc, freeProc, reAllocProc);

#ifdef _WIN32
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);

		M4Image::allocator = ubiAllocator;
	}
	return TRUE;
}
#endif