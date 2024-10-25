#pragma once
#include "shared.h"

namespace gfx_tools {
	struct BaseBuffer {
		typedef unsigned char* POINTER;
		typedef unsigned long SIZE;

		POINTER buffer = 0;
		SIZE size = 0;
		bool owner = true;
	};

	struct RawBuffer : public BaseBuffer {
	};

	struct Buffer : public BaseBuffer {
	};
}