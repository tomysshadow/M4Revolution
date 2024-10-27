#pragma once
#include "M4Image/shared.h"
#include <stdint.h>

class M4IMAGE_API M4Image {
    public:
    class M4IMAGE_API Allocator {
        public:
        typedef void* (*MallocProc)(size_t size);
        typedef void (*FreeProc)(void* block);
        typedef void* (*ReallocProc)(void* block, size_t size);

        Allocator();
        Allocator(MallocProc mallocProc, FreeProc freeProc, ReallocProc reallocProc);
        void* M4IMAGE_CALL malloc(size_t size) const;
        void M4IMAGE_CALL free(void* block) const;
        void* M4IMAGE_CALL realloc(void* block, size_t size) const;

        private:
        MallocProc mallocProc = ::malloc;
        FreeProc freeProc = ::free;
        ReallocProc reallocProc = ::realloc;
    };

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

    static Allocator allocator;

    static void M4IMAGE_CALL getInfo(
        const unsigned char* address,
        size_t size,
        const char* extension,
        uint32_t* bitsPointer,
        bool* alphaPointer,
        int* widthPointer,
        int* heightPointer,
        bool* linearPointer,
        bool* premultipliedPointer
    );

    M4Image(int width, int height, size_t &stride, COLOR_FORMAT colorFormat = COLOR_FORMAT::RGBA32, unsigned char* image = 0);
    M4Image(int width, int height);
    ~M4Image();
    void M4IMAGE_CALL blit(const M4Image &m4Image, bool linear = false, bool premultiplied = false);
    void M4IMAGE_CALL load(const unsigned char* address, size_t size, const char* extension, bool &linear, bool &premultiplied);
    void M4IMAGE_CALL load(const unsigned char* address, size_t size, const char* extension, bool &linear);
    void M4IMAGE_CALL load(const unsigned char* address, size_t size, const char* extension);
    unsigned char* M4IMAGE_CALL save(size_t &size, const char* extension, float quality = 0.90f) const;
    unsigned char* M4IMAGE_CALL acquire();

    private:
    void create(int width, int height, size_t &stride, COLOR_FORMAT colorFormat = COLOR_FORMAT::RGBA32, unsigned char* image = 0);
    void destroy();

    int width = 0;
    int height = 0;
    size_t stride = 0;
    COLOR_FORMAT colorFormat = COLOR_FORMAT::RGBA32;
    unsigned char* image = 0;
    bool owner = false;
};