#include "shared.h"
#include "Ubi.h"
#include <vector>
#include <atomic>

#if defined(WIN32)
#define MULTITHREADED
#else
#define SINGLETHREADED
#endif

namespace Work {
	struct Media {
		typedef Ubi::BigFile::File::SIZE SIZE;
		typedef std::unique_ptr<unsigned char> DATA_POINTER;

		SIZE size = 0;
		DATA_POINTER dataPointer = 0;

		class Pool {
			public:
			typedef std::vector<Media> VECTOR;

			static std::atomic<VECTOR::size_type> index;
			static VECTOR vector;

			Pool();
			~Pool();
		};
	};
};