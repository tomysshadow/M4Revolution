#pragma once
#include "M4Image\shared.h"
#include <string>

#include <mango/core/configure.hpp>

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
        RGB24,
        RGBA32,
        RGBX32,
        BGR24,
        BGRA32,
        BGRX32,
        A8,
        L8,
        AL16,
        XXXL32,
        XXLA32
    };

    M4IMAGE_API unsigned char* M4IMAGE_CALL load(
        const std::string &extension,
        unsigned char* address,
        size_t size,
        int &width,
        int &height,
        COLOR_FORMAT colorFormat
    );

    M4IMAGE_API unsigned char* M4IMAGE_CALL resize(
        const std::string &extension,
        unsigned char* address,
        size_t size,
        int width,
        int height,
        COLOR_FORMAT colorFormat
    );

    M4IMAGE_API void M4IMAGE_CALL setAllocator(MAllocProc mallocProc, FreeProc freeProc);
};