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
	class ErrorManager {
		public:
		typedef unsigned long CATEGORY;
		typedef unsigned long MASK;

		BASE_RD_API unsigned long BASE_RD_CALL RegisterCategory(unsigned long reserved, char const* name);
		BASE_RD_API void BASE_RD_CALL SetSystemFlag(CATEGORY category, MASK mask, bool value);

		BASE_RD_API static ErrorManager& BASE_RD_CALL GetSingletonInstance();
	};

	class BASE_RD_API RefCounted {
		public:
		typedef unsigned int REF_COUNT;
		
		inline REF_COUNT BASE_RD_CALL AddRef() {
			return ++refCount;
		}
		
		inline REF_COUNT BASE_RD_CALL Release() {
			if (--refCount) {
				return refCount;
			}

			delete this;
			return 0;
		}

		private:
		REF_COUNT refCount = 1;
	};

	class InputStream {
		public:
		typedef unsigned long SIZE;

		BASE_RD_API SIZE BASE_RD_CALL Read(unsigned char* buffer, SIZE position, SIZE size);
	};

	class InputFileStream {
		public:
		typedef unsigned __int64 SIZE;

		BASE_RD_API InputFileStream(char const* path);
		BASE_RD_API virtual ~InputFileStream();
		BASE_RD_API SIZE BASE_RD_CALL GetSize();
	};
}