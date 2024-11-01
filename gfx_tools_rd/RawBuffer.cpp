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

	RawBufferEx::RawBufferEx(POINTER pointer, SIZE size, bool owner, size_t stride)
	: RawBuffer(pointer, size, owner),
	stride(stride) {
	}

	RawBufferEx::RawBufferEx(const RawBuffer &rawBuffer, size_t stride)
	: RawBuffer(rawBuffer),
	stride(stride) {
	}
}