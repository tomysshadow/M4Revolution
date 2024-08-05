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
	class OutputFile {
		private:
		std::mutex mutex = {};
		std::ofstream stream = {};

		public:
		class StreamLock {
			private:
			std::lock_guard<std::mutex> lock;
			std::ofstream &stream;

			public:
			StreamLock(std::mutex &mutex, std::ofstream &stream, std::streampos position);
			std::ofstream &get();
		};

		OutputFile(const char* name);
		StreamLock lock(std::streampos position);
	};

	struct Media {
		typedef Ubi::BigFile::File::SIZE SIZE;
		typedef std::unique_ptr<unsigned char> DATA_POINTER;

		SIZE size = 0;
		DATA_POINTER dataPointer = 0;

		struct Pool {
			typedef std::vector<Media> VECTOR;

			static std::atomic<VECTOR::size_type> index;
			static VECTOR vector;

			nvtt::Context* contextPointer = 0;
			nvtt::Surface* surfacePointer = 0;
			nvtt::CompressionOptions* compressionOptionsPointer = 0;

			OutputFile* outputFilePointer = 0;
			std::streampos outputPosition = 0;

			Ubi::BigFile::File::SIZE &size;

			Pool(SIZE &size);
			~Pool();
		};
	};
};