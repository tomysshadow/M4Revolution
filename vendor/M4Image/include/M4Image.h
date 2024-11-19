#pragma once
#include <stdexcept>
#include <stdint.h>

#define M4IMAGE_API
#define M4IMAGE_CALL __cdecl

// forward declared so Pixman doesn't need to be included here
// (so you only need the static lib to include M4Image in another project)
extern "C" {
    typedef int pixman_bool_t;
    typedef union pixman_image pixman_image_t;
    pixman_bool_t pixman_image_unref(pixman_image_t* image);
}

inline bool unrefImage(pixman_image_t* &image) {
    if (image) {
        if (!pixman_image_unref(image)) {
            return false;
        }
    }

    image = NULL;
    return true;
}

class M4Image {
    public:
    class Allocator {
        public:
        typedef void* (*MallocProc)(size_t size);
        typedef void (*FreeProc)(void* block);
        typedef void* (*ReAllocProc)(void* block, size_t size);

        inline constexpr Allocator() = default;

        inline constexpr Allocator(MallocProc mallocProc, FreeProc freeProc, ReAllocProc reAllocProc)
            : mallocProc(mallocProc),
            freeProc(freeProc),
            reAllocProc(reAllocProc) {
        }

        inline void* M4IMAGE_CALL mallocSafe(size_t size) const {
            void* block = mallocProc(size);

            if (!block) {
                throw std::bad_alloc();
            }
            return block;
        }

        template <typename Block>
        inline void M4IMAGE_CALL freeSafe(Block* &block) const {
            if (block) {
                freeProc(block);
                block = 0;
            }
        }

        template <typename Block>
        inline void M4IMAGE_CALL reAllocSafe(Block* &block, size_t size) const {
            if (!size) {
                throw std::invalid_argument("size must not be zero");
            }

            block = (Block*)reAllocProc(block, size);

            if (!block) {
                throw std::bad_alloc();
            }
        }

        private:
        MallocProc mallocProc = ::malloc;
        FreeProc freeProc = ::free;
        ReAllocProc reAllocProc = ::realloc;
    };

    class Invalid : public std::invalid_argument {
        public:
        M4IMAGE_API Invalid() noexcept : std::invalid_argument("M4Image invalid") {
        }
    };
    
    struct Color16 {
        unsigned char channels[2] = {};
    };

    struct Color32 {
        unsigned char channels[4] = {};
    };

    // note: this is expressed as platform-independent byte arrays, not packed pixels
    enum struct COLOR_FORMAT {
        RGBA = 0,
        RGBX,
        BGRA,
        BGRX,
        RGB,
        BGR,
        LA,
        AL,
        A,
        L,

        // these colour formats are mostly for internal use (you're free to use them, though)
        XXXL = 16000,
        XXLA
    };

    M4IMAGE_API static Allocator allocator;

    M4IMAGE_API static void M4IMAGE_CALL getInfo(
        const unsigned char* pointer,
        size_t size,
        const char* extension = 0,
        bool* isAlphaPointer = 0,
        uint32_t* bitsPointer = 0,
        int* widthPointer = 0,
        int* heightPointer = 0,
        bool* linearPointer = 0,
        bool* premultipliedPointer = 0
    );

    M4IMAGE_API M4Image(int width, int height, size_t &stride, COLOR_FORMAT colorFormat = COLOR_FORMAT::RGBA, unsigned char* imagePointer = 0);
    M4IMAGE_API M4Image(int width, int height);
    M4IMAGE_API ~M4Image();
    M4IMAGE_API void M4IMAGE_CALL blit(const M4Image &m4Image, bool linear = false, bool premultiplied = false);
    M4IMAGE_API void M4IMAGE_CALL load(const unsigned char* pointer, size_t size, const char* extension, bool &linear, bool &premultiplied);
    M4IMAGE_API void M4IMAGE_CALL load(const unsigned char* pointer, size_t size, const char* extension, bool &linear);
    M4IMAGE_API void M4IMAGE_CALL load(const unsigned char* pointer, size_t size, const char* extension = 0);
    M4IMAGE_API unsigned char* M4IMAGE_CALL save(size_t &size, const char* extension = 0, float quality = 0.90f) const;
    M4IMAGE_API unsigned char* M4IMAGE_CALL acquire();

    private:
    void M4IMAGE_CALL create(int width, int height, size_t &stride, COLOR_FORMAT colorFormat = COLOR_FORMAT::RGBA, unsigned char* imagePointer = 0);
    void M4IMAGE_CALL destroy();

    int width = 0;
    int height = 0;
    size_t stride = 0;
    COLOR_FORMAT colorFormat = COLOR_FORMAT::RGBA;
    unsigned char* imagePointer = 0;
    bool owner = false;
};