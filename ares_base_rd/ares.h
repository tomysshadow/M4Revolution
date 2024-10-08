#pragma once

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
		public:
		virtual Resource* Destroy(unsigned char flags);
		virtual const char* GetClassNameA() const;
		virtual Resource* Clone(EnumCloneType enumCloneType);
	};
}