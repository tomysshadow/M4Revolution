#include "RawBuffer.h"

namespace gfx_tools {
	RawBuffer::RawBuffer() {
	}

	RawBuffer::RawBuffer(POINTER pointer, SIZE size, bool owner)
		: pointer(pointer),
		size(size),
		owner(owner) {
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

	RawBufferEx::RawBufferEx(POINTER pointer, SIZE size, bool owner, const std::optional<ResizeInfo> &resizeInfoOptional)
		: RawBuffer(pointer, size, owner),
		resizeInfoOptional(resizeInfoOptional) {
	}

	RawBufferEx::RawBufferEx(const RawBuffer &rawBuffer, const std::optional<ResizeInfo> &resizeInfoOptional)
		: RawBuffer(rawBuffer),
		resizeInfoOptional(resizeInfoOptional) {
	}
}