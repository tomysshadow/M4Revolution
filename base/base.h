#pragma once
#include <stddef.h>

#define BASE_CALL

#ifdef _WIN32
	#ifdef BASE_LIBRARY
		#define BASE_API __declspec(dllexport)
	#else
		#define BASE_API __declspec(dllimport)
	#endif
#else
	#define BASE_API
#endif

namespace ubi {
	/*
	class SystemBase {
		private:
		static unsigned char ms_UpperCase[];

		public:
		BASE_API static void BASE_CALL ToUpperCase(char* str);
	};
	*/

	class RefCounted {
		public:
		using RefCount = unsigned int;

		inline RefCount BASE_CALL AddRef() {
			return ++refCount;
		}

		inline RefCount BASE_CALL Release() {
			if (refCount > 1) {
				return --refCount;
			}

			delete this;
			return 0;
		}

		private:
		// this actually starts at zero, not one
		// that is, it defaults to not currently in use
		RefCount refCount = 0;
	};

	class CriticalSection {
		public:
		BASE_API CriticalSection();
		BASE_API ~CriticalSection();
		BASE_API void BASE_CALL Lock();
		BASE_API void BASE_CALL Unlock();
	};

	class InstanceManager {
		public:
		BASE_API static CriticalSection& BASE_CALL GetCriticalSection();
	};

	class ErrorManager {
		public:
		using Category = unsigned long;
		using Mask = unsigned long;

		BASE_API unsigned long BASE_CALL RegisterCategory(unsigned long reserved, const char* name);
		BASE_API void BASE_CALL SetSystemFlag(Category category, Mask mask, bool value);

		BASE_API static ErrorManager& BASE_CALL GetSingletonInstance();
	};

	class Stream {
		public:
		using Size = unsigned long;
	};

	class InputStream : public Stream {
		public:
		BASE_API Size BASE_CALL Read(unsigned char* buffer, Size offset, Size size);
	};

	/*
	class OutputStream : public Stream {
	};
	*/

	class InputFileStream : public InputStream {
		public:
		using Size = unsigned __int64;

		BASE_API InputFileStream(const char* path);
		BASE_API virtual ~InputFileStream();
		BASE_API Size BASE_CALL GetSize();
	};

	/*
	class AllocatorStatMarker {
	};

	class MemoryTagsManager {
	};
	*/

	class Allocator {
		public:
		BASE_API static Allocator* BASE_CALL GetOwner(void const* pointer);

		BASE_API void* BASE_CALL Malloc(size_t size);
		BASE_API void* BASE_CALL ReAlloc(void* pointer, size_t size);

		BASE_API virtual ~Allocator();
		BASE_API virtual void* BASE_CALL MallocNoThrow(size_t size) = 0;
		BASE_API virtual void BASE_CALL Free(void const* pointer) = 0;
		BASE_API virtual void BASE_CALL FreeAll() = 0;
		BASE_API virtual void* BASE_CALL ReAllocNoThrow(void* pointer, size_t size) = 0;
		/*
		BASE_API virtual void BASE_CALL DumpStatistics(unsigned long, AllocatorStatMarker*, AllocatorStatMarker*, OutputStream*, char*) = 0;
		BASE_API virtual void BASE_CALL FillStatMarker(AllocatorStatMarker &allocatorStatMarker) = 0;
		BASE_API virtual bool BASE_CALL ValidateMemory() = 0;
		BASE_API virtual bool BASE_CALL IsValidPointer(void const* pointer) = 0;
		BASE_API virtual size_t BASE_CALL GetMaxSupportedAllocSize() const = 0;
		BASE_API virtual ErrorManager::Category BASE_CALL GetErrorCategory() const = 0;
		BASE_API virtual void* BASE_CALL MallocDebugNoThrow(size_t size, const char*, const char*, ErrorManager::Category category) = 0;
		BASE_API virtual void* BASE_CALL ReAllocDebugNoThrow(void* pointer, size_t size, const char*, const char*, ErrorManager::Category category) = 0;
		BASE_API virtual void BASE_CALL FreeDebug(void const* pointer, const char*, ErrorManager::Category category) = 0;
		BASE_API virtual void* BASE_CALL Transfert(Allocator* allocatorPointer, void* pointer, size_t size) = 0;
		BASE_API virtual void* BASE_CALL TransfertDebug(Allocator* allocatorPointer, void* pointer, size_t size, const char*, const char*, ErrorManager::Category category) = 0;
		BASE_API virtual void BASE_CALL AddContext(const char*) = 0;
		BASE_API virtual void BASE_CALL RemoveContext() = 0;
		BASE_API virtual void BASE_CALL FillTagManagerForHTMLStatistics(MemoryTagsManager &memoryTagsManager, char*) = 0;
		*/
	};

	class Mem {
		public:
		BASE_API static Allocator* BASE_CALL GetGeneralAlloc();
	};
}