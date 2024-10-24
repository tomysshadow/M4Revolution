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


	RefCounted* RefCounted::Destroy(FLAGS flags) {
		return 0;
	}

	size_t InputStream::Read(unsigned char* buffer, size_t position, size_t size) {
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