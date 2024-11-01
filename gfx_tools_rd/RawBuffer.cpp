#include "RawBuffer.h"

namespace gfx_tools {
	RawBuffer::RawBuffer() {
	}

	RawBuffer::RawBuffer(POINTER pointer, SIZE size, bool owner)
		: pointer(pointer),
		size(size),
		owner(owner) {
	}

	RawBufferEx::LoadedInfo::LoadedInfo(int width, int height, size_t stride, L_INT qFactor)
		: width(width),
		height(height),
		stride(stride),
		quality(getQuality(qFactor)) {
	}

	RawBufferEx::RawBufferEx() {
	}

	RawBufferEx::RawBufferEx(POINTER pointer, SIZE size, bool owner, const std::optional<LoadedInfo> &loadedInfoOptional)
		: RawBuffer(pointer, size, owner),
		loadedInfoOptional(loadedInfoOptional) {
	}

	RawBufferEx::RawBufferEx(const RawBuffer &rawBuffer, const std::optional<LoadedInfo> &loadedInfoOptional)
		: RawBuffer(rawBuffer),
		loadedInfoOptional(loadedInfoOptional) {
	}
}