#include "RawBuffer.h"
#include <M4Image/M4Image.h>

namespace gfx_tools {
	RawBuffer::RawBuffer() {
	}

	RawBuffer::RawBuffer(DATA data, SIZE size, bool owner)
		: data(data),
		size(size),
		owner(owner) {
	}

	RawBuffer::~RawBuffer() {
		if (owner) {
			M4Image::allocator.freeSafe(data);
		}
	}
	
	float RawBufferEx::ResizeInfo::GetQuality(L_INT qFactor) {
		const L_INT Q_FACTOR_HIGHEST_QUALITY = 2;
		const L_INT Q_FACTOR_MOST_COMPRESSION = 255;

		if (qFactor < Q_FACTOR_HIGHEST_QUALITY) {
			return 1.0f;
		}
		return 1.0f - ((float)(qFactor - Q_FACTOR_HIGHEST_QUALITY) / (Q_FACTOR_MOST_COMPRESSION - Q_FACTOR_HIGHEST_QUALITY));
	}

	RawBufferEx::ResizeInfo::ResizeInfo() {
	}

	RawBufferEx::ResizeInfo::ResizeInfo(int width, int height, size_t stride, L_INT qFactor)
		: width(width),
		height(height),
		stride(stride),
		quality(GetQuality(qFactor)) {
	}

	RawBufferEx::RawBufferEx() {
	}

	RawBufferEx::RawBufferEx(DATA data, SIZE size, bool owner, const std::optional<ResizeInfo> &resizeInfoOptional)
		: RawBuffer(data, size, owner),
		resizeInfoOptional(resizeInfoOptional) {
	}
}