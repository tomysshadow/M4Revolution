#include "base_rd.h"

namespace ubi {
	unsigned long ErrorManager::RegisterCategory(unsigned long reserved, char const* name) {
		return 0;
	}

	void ErrorManager::SetSystemFlag(unsigned long category, unsigned long mask, bool value) {
	}

	ErrorManager& BASE_RD_CALL ErrorManager::GetSingletonInstance() {
		return *(new ErrorManager());
	}

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