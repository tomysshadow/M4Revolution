#include "M4Image/M4Image.h"
#include <map>
#include <stdint.h>

#include <mango/image/surface.hpp>
#include <mango/image/decoder.hpp>
#include <pixman.h>

static M4Image::MAllocProc mallocProc = malloc;
static M4Image::FreeProc freeProc = free;

// normally the free function will work even if it is passed a null pointer
// but since we allow custom allocators to be used, this may not always be a guarantee
// since I rely on this, I wrap the free procedure here to check
void freeBits(unsigned char* &bits) {
    if (bits) {
        freeProc(bits);
    }

    bits = 0;
}

void setFormat(mango::image::Format &format, M4Image::COLOR_FORMAT value) {
    switch (value) {
        case M4Image::COLOR_FORMAT::RGBA32:
        format = mango::image::Format(32, mango::image::Format::UNORM, mango::image::Format::RGBA, 8, 8, 8, 8);
        return;
        case M4Image::COLOR_FORMAT::RGBX32:
        format = mango::image::Format(32, mango::image::Format::UNORM, mango::image::Format::RGBA, 8, 8, 8, 0);
        return;
        case M4Image::COLOR_FORMAT::BGRA32:
        format = mango::image::Format(32, mango::image::Format::UNORM, mango::image::Format::BGRA, 8, 8, 8, 8);
        return;
        case M4Image::COLOR_FORMAT::BGRX32:
        format = mango::image::Format(32, mango::image::Format::UNORM, mango::image::Format::BGRA, 8, 8, 8, 0);
        return;
        case M4Image::COLOR_FORMAT::RGB24:
        format = mango::image::Format(24, mango::image::Format::UNORM, mango::image::Format::RGB, 8, 8, 8, 0);
        return;
        case M4Image::COLOR_FORMAT::BGR24:
        format = mango::image::Format(24, mango::image::Format::UNORM, mango::image::Format::BGR, 8, 8, 8, 0);
        return;
        case M4Image::COLOR_FORMAT::AL16:
        format = mango::image::LuminanceFormat(16, mango::image::Format::UNORM, 8, 8);
        return;
        case M4Image::COLOR_FORMAT::A8:
        format = mango::image::Format(8, mango::image::Format::UNORM, mango::image::Format::A, 8, 0, 0, 0);
        return;
        case M4Image::COLOR_FORMAT::L8:
        format = mango::image::LuminanceFormat(8, mango::image::Format::UNORM, 8, 0);
        return;
        case M4Image::COLOR_FORMAT::XXXL32:
        format = mango::image::LuminanceFormat(32, 0xFF000000, 0x00000000);
        return;
        case M4Image::COLOR_FORMAT::XXLA32:
        format = mango::image::LuminanceFormat(32, 0x00FF0000, 0xFF000000);
    }
}

void setSurfaceImageHeader(mango::image::Surface &surface, const mango::image::ImageHeader &imageHeader) {
    surface.stride = (size_t)imageHeader.width * (size_t)surface.format.bytes();
    surface.width = imageHeader.width;
    surface.height = imageHeader.height;
}

void decodeSurfaceImage(mango::image::Surface &surface, mango::image::ImageDecoder &imageDecoder) {
    // allocate memory for the image
    surface.image = (mango::u8*)mallocProc(surface.stride * (size_t)surface.height);

    if (!surface.image) {
        return;
    }

    MAKE_SCOPE_EXIT(surfaceImageScopeExit) {
        freeBits(surface.image);
    };

    mango::image::ImageDecodeStatus status = imageDecoder.decode(surface);

    // status is false if decoding the image failed
    if (!status) {
        MANGO_EXCEPTION("[WARNING] {}", status.info);
    }

    surfaceImageScopeExit.dismiss();
}

static const mango::image::Format IMAGE_HEADER_FORMAT_RGBA = mango::image::Format(32, mango::image::Format::UNORM, mango::image::Format::RGBA, 8, 8, 8, 8);

typedef std::map<M4Image::COLOR_FORMAT, pixman_format_code_t> PIXMAN_FORMAT_CODE_MAP;

// no, these are not backwards (read comments for getResizeFormat function below)
static const PIXMAN_FORMAT_CODE_MAP RGBA_PIXMAN_FORMAT_CODE_MAP = {
    {M4Image::COLOR_FORMAT::RGBA32, PIXMAN_a8r8g8b8},
    {M4Image::COLOR_FORMAT::RGBX32, PIXMAN_x8r8g8b8},
    {M4Image::COLOR_FORMAT::BGRA32, PIXMAN_a8b8g8r8},
    {M4Image::COLOR_FORMAT::BGRX32, PIXMAN_x8b8g8r8},
    {M4Image::COLOR_FORMAT::RGB24, PIXMAN_r8g8b8},
    {M4Image::COLOR_FORMAT::BGR24, PIXMAN_b8g8r8},
    {M4Image::COLOR_FORMAT::A8, PIXMAN_a8}
};

static const PIXMAN_FORMAT_CODE_MAP BGRA_PIXMAN_FORMAT_CODE_MAP = {
    {M4Image::COLOR_FORMAT::RGBA32, PIXMAN_a8b8g8r8},
    {M4Image::COLOR_FORMAT::RGBX32, PIXMAN_x8b8g8r8},
    {M4Image::COLOR_FORMAT::BGRA32, PIXMAN_a8r8g8b8},
    {M4Image::COLOR_FORMAT::BGRX32, PIXMAN_x8r8g8b8},
    {M4Image::COLOR_FORMAT::RGB24, PIXMAN_b8g8r8},
    {M4Image::COLOR_FORMAT::BGR24, PIXMAN_r8g8b8},
    {M4Image::COLOR_FORMAT::A8, PIXMAN_a8}
};

// the COLOR_FORMAT enum uses little endian, like mango
// note that Pixman expresses colours in big endian (the opposite of mango)
// so Pixman's "ARGB" is equal to mango's BGRA
// this makes the constants a little confusing
// from here on out, I'll be using little endian (like mango)
M4Image::COLOR_FORMAT getResizeColorFormat(
    const mango::image::ImageHeader &imageHeader,
    M4Image::COLOR_FORMAT colorFormat,
    pixman_format_code_t &sourceFormat,
    pixman_format_code_t &destinationFormat
) {
    switch (colorFormat) {
        case M4Image::COLOR_FORMAT::A8:
        // for COLOR_FORMAT::A8, the colours do not need to be premultiplied
        // because we toss them anyway, only keeping the alpha
        // we still use a 32-bit source format to keep things fast
        // only converting to an 8-bit format at the end, as is typical
        // note the break - we don't return here, because
        // the input format is interchangeable between RGBA/BGRA
        sourceFormat = PIXMAN_a8r8g8b8;
        break;
        case M4Image::COLOR_FORMAT::L8:
        // for COLOR_FORMAT::L8, the image is loaded in XXXL format
        // so Pixman will think the luminance is "alpha"
        // the destination is set to PIXMAN_a8, tossing the reserved channels
        // this once again allows us to keep the source format 32-bit
        sourceFormat = PIXMAN_a8r8g8b8;
        destinationFormat = PIXMAN_a8;
        return M4Image::COLOR_FORMAT::XXXL32;
        case M4Image::COLOR_FORMAT::XXXL32:
        // here we just keep it 32-bit the whole way, easy peasy
        // we do the same trick as COLOR_FORMAT::L8 where luminance = Pixman's alpha
        sourceFormat = PIXMAN_a8r8g8b8;
        destinationFormat = PIXMAN_a8r8g8b8;
        return M4Image::COLOR_FORMAT::XXXL32;
        case M4Image::COLOR_FORMAT::AL16:
        case M4Image::COLOR_FORMAT::XXLA32:
        // for COLOR_FORMAT::AL16, mango reads the image in XXLA format
        // the luminance can't be in the alpha channel now
        // because we need to premultiply in this case
        // so we just shove it in the blue channel now
        // Pixman does not support any 16-bit formats with
        // two 8-bit channels, so
        // we manually convert down to 16-bit during the
        // unpremultiplication step, since we need to do a whole
        // pass over the image for that anyway, so might as well
        // kill two birds with one stone
        sourceFormat = PIXMAN_x8r8g8b8;
        destinationFormat = PIXMAN_a8r8g8b8;
        return M4Image::COLOR_FORMAT::XXLA32;
        default:
        // sourceFormat is only ever PIXMAN_x8r8g8b8 or PIXMAN_a8r8g8b8
        // it is the former if the colours will need to be premultiplied
        // and the latter if they do not need to be
        // it is mango's job to decode the image into a 32-bit format before
        // ever being passed to Pixman, which is only fast with 32-bit colour
        sourceFormat = PIXMAN_x8r8g8b8;
    }

    // as an optimization, we allow mango to import in RGBA
    // RGBA and BGRA are the only import formats allowed
    // (A must come last, and these are the only formats Pixman supports where A is last)
    if (imageHeader.format == IMAGE_HEADER_FORMAT_RGBA) {
        // for all other colour formats, we start with 32-bit colour, converting down as necessary
        // Pixman operates the fastest in BGRA mode, but since both operations
        // that we are doing apply the same to all colour channels (premultiplying and interpolating)
        // the only thing that matters is the position of the alpha channel, so
        // we can just "lie" to Pixman and say our RGBA image is BGRA
        // then, flip it to "RGBA" at the end if BGRA was requested
        destinationFormat = RGBA_PIXMAN_FORMAT_CODE_MAP.at(colorFormat);
        return M4Image::COLOR_FORMAT::RGBA32;
    }

    // once again, these are not wrong. Setting ARGB as the destination
    // means to "flip" the colour, as Pixman always thinks the image is ABGR
    destinationFormat = BGRA_PIXMAN_FORMAT_CODE_MAP.at(colorFormat);
    return M4Image::COLOR_FORMAT::BGRA32;
}

// if we need to premultiply the colours
// then we use a mask (from which only the alpha channel is used)
// on the surface (which we mark as RGBX, so its alpha is ignored)
// this has the effect of multiplying RGB * Alpha, which
// is the exact operation we need
// (doing it this way is significantly more performant than manually doing it ourselves)
// both use the same underlying data, so no memory is allocated
// they are just seperate views into the same memory
// note that we need the alpha channel for the actual resize
// so we use maskImage as the destination, rather than writing over sourceImage
// therefore the sourceImage (which is RGBX) is only used for this premultiply step
// and the maskImage is what is what we actually transform later on
// if you're totally lost on why this is needed for resizing, then
// see: https://www.realtimerendering.com/blog/gpus-prefer-premultiplication/
pixman_image_t* premultiplyMaskImage(const mango::image::Surface &surface, pixman_image_t* sourceImage) {
    pixman_image_t* maskImage = pixman_image_create_bits(
        PIXMAN_a8r8g8b8,
        surface.width, surface.height,
        (uint32_t*)surface.image,
        (int)surface.stride
    );

    if (!maskImage) {
        return 0;
    }

    MAKE_SCOPE_EXIT(maskImageScopeExit) {
        unrefImage(maskImage);
    };

    pixman_image_composite(
        PIXMAN_OP_SRC,
        sourceImage, maskImage, maskImage,
        0, 0, 0, 0, 0, 0,
        surface.width, surface.height
    );

    maskImageScopeExit.dismiss();
    return maskImage;
}

// Pixman wants a scale (like a percentage to resize by,) not a pixel size
// so here we create that
bool setTransform(pixman_image_t* maskImage, const mango::image::Surface &surface, int width, int height) {
    // this is initialized by pixman_transform_init_identity
    pixman_transform_t transform;
    pixman_transform_init_identity(&transform);

    double sx = (double)surface.width / width;
    double sy = (double)surface.height / height;

    if (!pixman_transform_scale(&transform, NULL, pixman_double_to_fixed(sx), pixman_double_to_fixed(sy))) {
        return false;
    }

    if (!pixman_image_set_transform(maskImage, &transform)) {
        return false;
    }
    return true;
}

static const size_t CHANNEL_SIZE = 8;

// aligned to nearest 64 bytes so it is on cache lines
static bool channelUnpremultiplierCreated = false;
__declspec(align(64)) static unsigned char CHANNEL_UNPREMULTIPLIER[65536] = {};

void createChannelUnpremultiplier() {
    if (channelUnpremultiplierCreated) {
        return;
    }

    // note: the alpha, divided by two, is added to the channel
    // so the channel is scaled instead of stripped (it works out to rounding the number, instead of flooring)
    // alpha starts at one, since if it's zero the colour is invisible anyway (and thus would be a divide by zero)
    const size_t DIVIDE_BY_TWO = 1;

    for (int channel = 0; channel <= UCHAR_MAX; channel++) {
        for (int alpha = 1; alpha <= UCHAR_MAX; alpha++) {
            CHANNEL_UNPREMULTIPLIER[(channel << CHANNEL_SIZE) | alpha] = clampUCHAR(((channel * UCHAR_MAX) + (alpha >> DIVIDE_BY_TWO)) / alpha);
        }
    }

    channelUnpremultiplierCreated = true;
}

#define UNPREMULTIPLY_CHANNEL(channel, alpha) (CHANNEL_UNPREMULTIPLIER[((channel) << CHANNEL_SIZE) | (alpha)])

unsigned char* convertImage(M4Image::Color32* colorPointer, M4Image::Color32* endPointer, bool unpremultiply) {
    createChannelUnpremultiplier();

    const size_t DIVIDE_BY_TWO = 1;
    const size_t CHANNEL_LUMINANCE = 2;
    const size_t CHANNEL_ALPHA = 3;

    unsigned char* bits = (unsigned char*)mallocProc(((size_t)endPointer - (size_t)colorPointer) >> DIVIDE_BY_TWO);

    MAKE_SCOPE_EXIT(bitsScopeExit) {
        freeBits(bits);
    };

    M4Image::Color16* luminancePointer = (M4Image::Color16*)bits;

    if (unpremultiply) {
        while (colorPointer != endPointer) {
            unsigned char &alpha = luminancePointer->channels[1];
            alpha = colorPointer->channels[CHANNEL_ALPHA];

            if (alpha) {
                luminancePointer->channels[0] = UNPREMULTIPLY_CHANNEL(colorPointer->channels[CHANNEL_LUMINANCE], alpha);
            }

            colorPointer++;
            luminancePointer++;
        }
    } else {
        while (colorPointer != endPointer) {
            *luminancePointer++ = *(M4Image::Color16*)&colorPointer++->channels[CHANNEL_LUMINANCE];
        }
    }

    bitsScopeExit.dismiss();
    return bits;
}

void unpremultiplyColors(M4Image::Color32* colorPointer, M4Image::Color32* endPointer) {
    createChannelUnpremultiplier();

    const size_t CHANNEL_ALPHA = 3;

    while (colorPointer != endPointer) {
        const unsigned char &ALPHA = colorPointer->channels[CHANNEL_ALPHA];

        if (ALPHA) {
            colorPointer->channels[0] = UNPREMULTIPLY_CHANNEL(colorPointer->channels[0], ALPHA);
            colorPointer->channels[1] = UNPREMULTIPLY_CHANNEL(colorPointer->channels[1], ALPHA);
            colorPointer->channels[2] = UNPREMULTIPLY_CHANNEL(colorPointer->channels[2], ALPHA);
        }

        colorPointer++;
    }
}

namespace M4Image {
    unsigned char* load(
        const std::string &extension,
        unsigned char* address,
        size_t size,
        int &width,
        int &height,
        size_t &stride,
        COLOR_FORMAT colorFormat
    ) {
        if (!address) {
            return 0;
        }

        mango::image::ImageDecoder imageDecoder(mango::ConstMemory(address, size), extension);

        if (!imageDecoder.isDecoder()) {
            return 0;
        }

        mango::image::Surface surface;

        try {
            setFormat(surface.format, colorFormat);
            setSurfaceImageHeader(surface, imageDecoder.header());
            decodeSurfaceImage(surface, imageDecoder);

            width = surface.width;
            height = surface.height;
            stride = surface.stride;
        } catch (...) {
            freeBits(surface.image);
            return 0;
        }
        return surface.image;
    }

    unsigned char* resize(
        const std::string &extension,
        unsigned char* address,
        size_t size,
        int width,
        int height,
        size_t &stride,
        COLOR_FORMAT colorFormat
    ) {
        if (!address) {
            return 0;
        }

        if (!width || !height) {
            return 0;
        }

        mango::image::ImageDecoder imageDecoder(mango::ConstMemory(address, size), extension);

        if (!imageDecoder.isDecoder()) {
            return 0;
        }

        mango::image::ImageHeader imageHeader = imageDecoder.header();
        bool resize = width != imageHeader.width || height != imageHeader.height;
        bool convert = colorFormat == COLOR_FORMAT::AL16;

        pixman_format_code_t sourceFormat = PIXMAN_x8r8g8b8;
        pixman_format_code_t destinationFormat = PIXMAN_a8r8g8b8;

        if (resize) {
            colorFormat = getResizeColorFormat(imageHeader, colorFormat, sourceFormat, destinationFormat);
        }

        mango::image::Surface surface;

        try {
            setFormat(surface.format, colorFormat);
            setSurfaceImageHeader(surface, imageHeader);
            decodeSurfaceImage(surface, imageDecoder);
        } catch (...) {
            freeBits(surface.image);
            return 0;
        }

        if (!surface.image) {
            return 0;
        }

        // if we don't need to resize the image (width and height matches) then job done
        if (!resize) {
            stride = surface.stride;
            return surface.image;
        }

        // otherwise we have to do a bunch of boilerplate setup stuff for the resize operation
        SCOPE_EXIT {
            freeBits(surface.image);
        };

        // allocate the memory that will get the resized image
        const size_t BYTES = 3;

        size_t bitsStride = (PIXMAN_FORMAT_BPP(destinationFormat) >> BYTES) * (size_t)width;
        size_t bitsSize = bitsStride * (size_t)height;
        unsigned char* bits = (unsigned char*)mallocProc(bitsSize);

        if (!bits) {
            return 0;
        }

        MAKE_SCOPE_EXIT(bitsScopeExit) {
            freeBits(bits);
        };

        // create the destination image in the user's desired format
        // (unless we need to convert to 16-bit after, then we still make it 32-bit)
        pixman_image_t* destinationImage = pixman_image_create_bits(
            destinationFormat,
            width, height,
            (uint32_t*)bits,
            (int)bitsStride
        );

        if (!destinationImage) {
            return 0;
        }

        SCOPE_EXIT {
            if (!unrefImage(destinationImage)) {
                freeBits(bits);
            }
        };

        pixman_image_t* sourceImage = pixman_image_create_bits(
            sourceFormat,
            surface.width, surface.height,
            (uint32_t*)surface.image,
            (int)surface.stride
        );

        if (!sourceImage) {
            return 0;
        }

        SCOPE_EXIT {
            if (!unrefImage(sourceImage)) {
                freeBits(bits);
            }
        };

        // we should only care about premultiplying if:
        // -the source format is PIXMAN_x8r8g8b8 (indicating we are meant to use it with maskImage)
        // -the destination format has RGB channels (because otherwise the colour data will be thrown out anyway)
        // -the destination format has alpha (because otherwise the colours will be unaffected by alpha)
        // -the image has alpha (that is, we aren't creating a new alpha channel for an actually opaque image)
        // we don't care about if the surface has alpha here, because it always should if its relevant
        bool premultiply = sourceFormat == PIXMAN_x8r8g8b8
            && PIXMAN_FORMAT_COLOR(destinationFormat)
            && PIXMAN_FORMAT_A(destinationFormat)
            && imageHeader.format.isAlpha();

        // we also don't premultiply if the original image was already premultiplied
        // (this is checked seperately, though, so we know whether to unpremultiply later)
        pixman_image_t* maskImage = (premultiply && !imageHeader.premultiplied)
            ? premultiplyMaskImage(surface, sourceImage)
            : sourceImage;

        SCOPE_EXIT {
            if (maskImage && maskImage != sourceImage) {
                if (!unrefImage(maskImage)) {
                    freeBits(bits);
                }
            }
        };

        if (!setTransform(maskImage, surface, width, height)) {
            return 0;
        }

        if (!pixman_image_set_filter(maskImage, PIXMAN_FILTER_BILINEAR, NULL, 0)) {
            return 0;
        }

        // setting the repeat mode to pad prevents some semi-transparent lines at the edge of the image
        // (because the image is interpreted as being in the middle of a transparent void of pixels otherwise)
        pixman_image_set_repeat(maskImage, PIXMAN_REPEAT_PAD);

        // the actual resize happens here
        pixman_image_composite(
            PIXMAN_OP_SRC,
            maskImage, NULL, destinationImage,
            0, 0, 0, 0, 0, 0,
            width, height
        );

        // as a final step we need to unpremultiply
        // as also convert down to 16-bit colour as necessary
        Color32* colorPointer = (Color32*)bits;
        Color32* endPointer = (Color32*)(bits + bitsSize);
        bool unpremultiply = premultiply || imageHeader.premultiplied;

        if (convert) {
            unsigned char* convertedBits = convertImage(colorPointer, endPointer, unpremultiply);

            if (!convertedBits) {
                return 0;
            }

            const size_t DIVIDE_BY_TWO = 1;

            // this is necessary so that, if an error occurs with unreffing the images
            // that we return zero, because bits will become set to zero
            freeBits(bits);
            bits = convertedBits;

            stride = bitsStride >> DIVIDE_BY_TWO;
            bitsScopeExit.dismiss();
            return bits;
        } else if (unpremultiply) {
            unpremultiplyColors(colorPointer, endPointer);
        }

        stride = bitsStride;
        bitsScopeExit.dismiss();
        return bits;
    }

    void setAllocator(MAllocProc _mallocProc, FreeProc _freeProc) {
        mallocProc = _mallocProc;
        freeProc = _freeProc;
    }
};