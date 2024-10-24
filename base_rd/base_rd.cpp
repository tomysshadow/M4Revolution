#include "base_rd.h"

namespace ubi {
	unsigned long ErrorManager::RegisterCategory(unsigned long reserved, char const* name) {
		return 0;
	}

	void ErrorManager::SetSystemFlag(CATEGORY category, MASK mask, bool value) {
	}

	ErrorManager& ErrorManager::GetSingletonInstance() {
		return *(new ErrorManager());
	}


	RefCounted::~RefCounted() {
	}

	InputStream::SIZE InputStream::Read(unsigned char* buffer, SIZE position, SIZE size) {
		return 0;
	}

	InputFileStream::InputFileStream(char const* path) {
	}

	InputFileStream::~InputFileStream() {
	}

	InputFileStream::SIZE InputFileStream::GetSize() {
		return 0;
	}
}