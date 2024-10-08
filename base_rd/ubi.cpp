#include "ubi.h"

namespace ubi {
	unsigned long InputStream::Read(unsigned char* buffer, unsigned long position, unsigned long size) {
		return 0;
	}

	InputFileStream::InputFileStream(char const* path) {
	}

	InputFileStream::~InputFileStream() {
	}

	unsigned __int64 InputFileStream::GetSize() {
		return 0;
	}
}