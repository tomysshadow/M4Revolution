#pragma once
#include "base.h"
#include <stdint.h>

#define ARES_BASE_CALL

#ifdef _WIN32
	#ifdef ARES_BASE_LIBRARY
		#define ARES_BASE_API __declspec(dllexport)
	#else
		#define ARES_BASE_API __declspec(dllimport)
	#endif
#else
	#define ARES_BASE_API
#endif

namespace ares {
	enum EnumCloneType {
		CLONETYPE_UNKNOWN = 0
	};

	class Resource : public ubi::RefCounted {
		protected:
		ARES_BASE_API Resource();
		ARES_BASE_API virtual ~Resource();

		char* instanceName = 0;

		public:
		ARES_BASE_API Resource(Resource const &resource);
		ARES_BASE_API Resource &operator=(Resource const &resource);
		ARES_BASE_API virtual const char* ARES_BASE_CALL GetClassNameA() const;
		ARES_BASE_API virtual Resource* ARES_BASE_CALL Clone(EnumCloneType enumCloneType);
	};

	struct RectU32 {
		uint32_t left = 0;
		uint32_t top = 0;
		uint32_t right = 0;
		uint32_t bottom = 0;
	};
}