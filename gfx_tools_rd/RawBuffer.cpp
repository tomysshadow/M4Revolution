#include "RawBuffer.h"

namespace gfx_tools {
	RawBuffer::RawBuffer() {
	}

	RawBuffer::RawBuffer(POINTER pointer, SIZE size, bool owner)
		: pointer(pointer),
		size(size),
		owner(owner) {
	}

	RawBufferEx::RawBufferEx() {
	}

	RawBufferEx::RawBufferEx(POINTER pointer, SIZE size, bool owner, size_t stride, L_INT qFactor)
		: RawBuffer(pointer, size, owner),
		stride(stride),
		quality(getQuality(qFactor)) {
	}

	RawBufferEx::RawBufferEx(const RawBuffer &rawBuffer, size_t stride, L_INT qFactor)
		: RawBuffer(rawBuffer),
		stride(stride),
		quality(getQuality(qFactor)) {
	}
}