#pragma once

#define BASE_RD_CALL

#ifdef _WIN32
	#ifdef BASE_RD_LIBRARY
		#define BASE_RD_API __declspec(dllexport)
	#else
		#define BASE_RD_API __declspec(dllimport)
	#endif
#else
	#define BASE_RD_API
#endif

namespace ubi {
	/*
	class SystemBase {
		private:
		static unsigned char ms_UpperCase[];

		public:
		BASE_RD_API static void BASE_RD_CALL ToUpperCase(char* str);
	};
	*/

	class RefCounted {
		public:
		typedef unsigned int REF_COUNT;

		inline REF_COUNT BASE_RD_CALL AddRef() {
			return ++refCount;
		}

		inline REF_COUNT BASE_RD_CALL Release() {
			if (refCount > 1) {
				return --refCount;
			}

			delete this;
			return 0;
		}

		private:
		// this actually starts at zero, not one
		// that is, it defaults to not currently in use
		REF_COUNT refCount = 0;
	};

	class CriticalSection {
		public:
		BASE_RD_API CriticalSection();
		BASE_RD_API ~CriticalSection();
		BASE_RD_API void BASE_RD_CALL Lock();
		BASE_RD_API void BASE_RD_CALL Unlock();
	};

	class InstanceManager {
		public:
		BASE_RD_API static CriticalSection& BASE_RD_CALL GetCriticalSection();
	};

	class ErrorManager {
		public:
		typedef unsigned long CATEGORY;
		typedef unsigned long MASK;

		BASE_RD_API unsigned long BASE_RD_CALL RegisterCategory(unsigned long reserved, char const* name);
		BASE_RD_API void BASE_RD_CALL SetSystemFlag(CATEGORY category, MASK mask, bool value);

		BASE_RD_API static ErrorManager& BASE_RD_CALL GetSingletonInstance();
	};

	class Stream {
		public:
		typedef unsigned long SIZE;
	};

	class InputStream : public Stream {
		public:
		BASE_RD_API SIZE BASE_RD_CALL Read(unsigned char* buffer, SIZE position, SIZE size);
	};

	/*
	class OutputStream : public Stream {
	};
	*/

	class InputFileStream : public InputStream {
		public:
		typedef unsigned __int64 SIZE;

		BASE_RD_API InputFileStream(char const* path);
		BASE_RD_API virtual ~InputFileStream();
		BASE_RD_API SIZE BASE_RD_CALL GetSize();
	};

	/*
	class AllocatorStatMarker {
	};

	class MemoryTagsManager {
	};
	*/

	class Allocator {
		public:
		BASE_RD_API static Allocator* BASE_RD_CALL GetOwner(void const* pointer);

		BASE_RD_API void* BASE_RD_CALL Malloc(size_t size);
		BASE_RD_API void* BASE_RD_CALL ReAlloc(void* pointer, size_t size);

		BASE_RD_API virtual ~Allocator();
		BASE_RD_API virtual void* BASE_RD_CALL MallocNoThrow(size_t size) = 0;
		BASE_RD_API virtual void BASE_RD_CALL Free(void const* pointer) = 0;
		BASE_RD_API virtual void BASE_RD_CALL FreeAll() = 0;
		BASE_RD_API virtual void* BASE_RD_CALL ReAllocNoThrow(void* pointer, size_t size) = 0;
		/*
		BASE_RD_API virtual void BASE_RD_CALL DumpStatistics(unsigned long, AllocatorStatMarker*, AllocatorStatMarker*, OutputStream*, char*) = 0;
		BASE_RD_API virtual void BASE_RD_CALL FillStatMarker(AllocatorStatMarker &allocatorStatMarker) = 0;
		BASE_RD_API virtual bool BASE_RD_CALL ValidateMemory() = 0;
		BASE_RD_API virtual bool BASE_RD_CALL IsValidPointer(void const* pointer) = 0;
		BASE_RD_API virtual size_t BASE_RD_CALL GetMaxSupportedAllocSize() const = 0;
		BASE_RD_API virtual ErrorManager::CATEGORY BASE_RD_CALL GetErrorCategory() const = 0;
		BASE_RD_API virtual void* BASE_RD_CALL MallocDebugNoThrow(size_t size, char const*, char const*, ErrorManager::CATEGORY category) = 0;
		BASE_RD_API virtual void* BASE_RD_CALL ReAllocDebugNoThrow(void* pointer, size_t size, char const*, char const*, ErrorManager::CATEGORY category) = 0;
		BASE_RD_API virtual void BASE_RD_CALL FreeDebug(void const* pointer, char const*, ErrorManager::CATEGORY category) = 0;
		BASE_RD_API virtual void* BASE_RD_CALL Transfert(Allocator* allocatorPointer, void* pointer, size_t size) = 0;
		BASE_RD_API virtual void* BASE_RD_CALL TransfertDebug(Allocator* allocatorPointer, void* pointer, size_t size, char const*, char const*, ErrorManager::CATEGORY category) = 0;
		BASE_RD_API virtual void BASE_RD_CALL AddContext(char const*) = 0;
		BASE_RD_API virtual void BASE_RD_CALL RemoveContext() = 0;
		BASE_RD_API virtual void BASE_RD_CALL FillTagManagerForHTMLStatistics(MemoryTagsManager &memoryTagsManager, char*) = 0;
		*/
	};

	class Mem {
		public:
		BASE_RD_API static Allocator* BASE_RD_CALL GetGeneralAlloc();
	};
}