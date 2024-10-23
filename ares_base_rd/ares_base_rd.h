#pragma once
#include <stdint.h>

#define ARES_BASE_RD_CALL __cdecl

#ifdef _WIN32
	#ifdef ARES_BASE_RD_LIBRARY
		#define ARES_BASE_RD_API __declspec(dllexport)
	#else
		#define ARES_BASE_RD_API __declspec(dllimport)
	#endif
#else
	#define ARES_BASE_RD_API
#endif

namespace ares {
	enum EnumCloneType {
		CLONETYPE_UNKNOWN = 0
	};

	class ARES_BASE_RD_API Resource {
		protected:
		Resource();
		virtual ~Resource();

		public:
		Resource(Resource const &resource);
		Resource &operator=(Resource const &resource);
		virtual Resource* Destroy(unsigned char flags);
		virtual const char* GetClassNameA() const;
		virtual Resource* Clone(EnumCloneType enumCloneType);
	};

	struct ARES_BASE_RD_API RectU32 {
		uint32_t left = 0;
		uint32_t top = 0;
		uint32_t right = 0;
		uint32_t bottom = 0;
	};
}