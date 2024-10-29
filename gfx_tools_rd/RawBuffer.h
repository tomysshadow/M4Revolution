#pragma once
#include "shared.h"

namespace gfx_tools {
	struct RawBuffer {
		typedef unsigned char* POINTER;
		typedef unsigned long SIZE;

		POINTER pointer = 0;
		SIZE size = 0;
		bool owner = true;
	};

	struct RawBufferEx : public RawBuffer {
		bool compressed = true;
	};
}