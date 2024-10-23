#include "base_rd.h"

namespace ubi {unsigned long ErrorManager::RegisterCategory(unsigned long reserved, char const* name) {
		return 0;
	}

	void ErrorManager::SetSystemFlag(unsigned long category, unsigned long mask, bool value) {
	}

	ErrorManager& ErrorManager::GetSingletonInstance() {
		return *(new ErrorManager());
	}


	RefCounted* RefCounted::Destroy(unsigned char flags) {
		return 0;
	}

	unsigned int RefCounted::AddRef() {
		return ++refCount;
	}

	unsigned int RefCounted::Release() {
		if (--refCount) {
			return refCount;
		}

		Destroy(1);
		return 0;
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