#include "shared.h"
#include "Ubi.h"
#include <mutex>
#include <vector>
#include <atomic>

#include <nvtt/nvtt.h>

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
	};
};