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
	class BASE_RD_API ErrorManager {
		public:
		typedef unsigned long CATEGORY;
		typedef unsigned long MASK;

		unsigned long BASE_RD_CALL RegisterCategory(unsigned long reserved, char const* name);
		void BASE_RD_CALL SetSystemFlag(CATEGORY category, MASK mask, bool value);

		static ErrorManager& BASE_RD_CALL GetSingletonInstance();
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

	class BASE_RD_API InputStream {
		public:
		typedef unsigned long SIZE;

		SIZE BASE_RD_CALL Read(unsigned char* buffer, SIZE position, SIZE size);
	};

	class BASE_RD_API InputFileStream {
		public:
		typedef unsigned __int64 SIZE;

		InputFileStream(char const* path);
		virtual ~InputFileStream();
		SIZE BASE_RD_CALL GetSize();
	};
}