#pragma once
#include <stdint.h>

#define ARES_BASE_RD_CALL

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
		~Resource();

		public:
		typedef unsigned char FLAGS;

		Resource(Resource const &resource);
		Resource &operator=(Resource const &resource);
		virtual Resource* ARES_BASE_RD_CALL Destroy(FLAGS flags);
		virtual const char* ARES_BASE_RD_CALL GetClassNameA() const;
		virtual Resource* ARES_BASE_RD_CALL Clone(EnumCloneType enumCloneType);
	};

	struct ARES_BASE_RD_API RectU32 {
		uint32_t left = 0;
		uint32_t top = 0;
		uint32_t right = 0;
		uint32_t bottom = 0;
	};
}