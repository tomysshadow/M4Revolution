#include "RawBuffer.h"

namespace gfx_tools {
	RawBuffer::RawBuffer() {
	}

	RawBuffer::RawBuffer(POINTER pointer, SIZE size, bool owner)
		: pointer(pointer),
		size(size),
		owner(owner) {
	}

	RawBufferEx::ResizeInfo::ResizeInfo() {
	}

	RawBufferEx::ResizeInfo::ResizeInfo(int width, int height, size_t stride, L_INT qFactor)
		: width(width),
		height(height),
		stride(stride),
		quality(getQuality(qFactor)) {
	}

	RawBufferEx::RawBufferEx() {
	}

	RawBufferEx::RawBufferEx(POINTER pointer, SIZE size, bool owner, const std::optional<ResizeInfo> &resizeInfoOptional)
		: RawBuffer(pointer, size, owner),
		resizeInfoOptional(resizeInfoOptional) {
	}

	RawBufferEx::RawBufferEx(const RawBuffer &rawBuffer, const std::optional<ResizeInfo> &resizeInfoOptional)
		: RawBuffer(rawBuffer),
		resizeInfoOptional(resizeInfoOptional) {
	}
}