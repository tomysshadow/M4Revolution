#pragma once
#include "M4Image/shared.h"
#include <string>
#include <stdint.h>

namespace M4Image {
    typedef void* (*MallocProc)(size_t size);
    typedef void (*FreeProc)(void* block);
    typedef void* (*ReallocProc)(void* block, size_t size);

    struct Color16 {
        unsigned char channels[2] = {};
    };

    struct Color32 {
        unsigned char channels[4] = {};
    };

    enum struct COLOR_FORMAT {
        RGBA32 = 0,
        RGBX32,
        BGRA32,
        BGRX32,
        RGB24,
        BGR24,
        AL16,
        A8,
        L8,

        // these colour formats are mostly for internal use (you're free to use them, though)
        XXXL32 = 16000,
        XXLA32
    };

    // note: extension is a string but we export it as const char* because
    // you're not supposed to export STL classes across DLL boundaries
    M4IMAGE_API unsigned char* M4IMAGE_CALL blit(
        const void* image,
        COLOR_FORMAT inputColorFormat,
        int inputWidth,
        int inputHeight,
        size_t inputStride,
        COLOR_FORMAT outputColorFormat,
        int outputWidth,
        int outputHeight,
        size_t &outputStride,
        bool linear = false
    );

    M4IMAGE_API unsigned char* M4IMAGE_CALL blit(
        const void* image,
        COLOR_FORMAT inputColorFormat,
        int inputWidth,
        int inputHeight,
        size_t inputStride,
        COLOR_FORMAT outputColorFormat,
        int outputWidth,
        int outputHeight
    );

    M4IMAGE_API unsigned char* M4IMAGE_CALL load(
        const char* extension,
        const unsigned char* address,
        size_t size,
        COLOR_FORMAT colorFormat,
        int width,
        int height,
        size_t &stride,
        bool &linear
    );

    M4IMAGE_API unsigned char* M4IMAGE_CALL load(
        const char* extension,
        const unsigned char* address,
        size_t size,
        COLOR_FORMAT colorFormat,
        int width,
        int height,
        size_t &stride
    );

    M4IMAGE_API unsigned char* M4IMAGE_CALL load(
        const char* extension,
        const unsigned char* address,
        size_t size,
        COLOR_FORMAT colorFormat,
        int width,
        int height
    );

    M4IMAGE_API unsigned char* M4IMAGE_CALL save(
        const char* extension,
        const void* image,
        size_t &size,
        COLOR_FORMAT colorFormat,
        int width,
        int height,
        size_t stride = 0,
        float quality = 0.90f
    );

    M4IMAGE_API void* M4IMAGE_CALL malloc(size_t size);
    M4IMAGE_API void M4IMAGE_CALL free(void* block);
    M4IMAGE_API void* M4IMAGE_CALL realloc(void* block, size_t size);

    M4IMAGE_API bool M4IMAGE_CALL getInfo(
        const char* extension,
        const unsigned char* address,
        size_t size,
        uint32_t* bitsPointer,
        bool* alphaPointer,
        int* widthPointer,
        int* heightPointer
    );

    M4IMAGE_API void M4IMAGE_CALL setAllocator(MallocProc mallocProc, FreeProc freeProc, ReallocProc reallocProc);
};