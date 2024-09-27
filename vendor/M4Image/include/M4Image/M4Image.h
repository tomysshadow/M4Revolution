#pragma once
#include "M4Image/shared.h"
#include <string>

namespace M4Image {
    typedef void* (*MAllocProc)(size_t size);
    typedef void (*FreeProc)(void* block);

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

    M4IMAGE_API unsigned char* M4IMAGE_CALL load(
        const std::string &extension,
        const unsigned char* address,
        size_t size,
        int &width,
        int &height,
        size_t &stride,
        COLOR_FORMAT colorFormat
    );

    M4IMAGE_API unsigned char* M4IMAGE_CALL resize(
        const std::string &extension,
        const unsigned char* address,
        size_t size,
        int width,
        int height,
        size_t &stride,
        COLOR_FORMAT colorFormat
    );

    M4IMAGE_API void M4IMAGE_CALL setAllocator(MAllocProc mallocProc, FreeProc freeProc);
};