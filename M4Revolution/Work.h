#include "shared.h"
#include "Ubi.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>

#if defined(_WIN32)
#define MULTITHREADED
#else
#define SINGLETHREADED
#endif

namespace Work {
	// a "signal the other thread to wake up and do stuff" class (similar to SetEvent)
	class Event {
		private:
		// threadIDOptional has a thread ID if the event is set (notified, time to wake up, lock is unlocked...)
		// and is std::nullopt if the event is not set (locked, currently in use, etc.)
		// the thread ID is used to yield to other threads instead of busy looping
		std::mutex mutex = {};
		std::condition_variable conditionVariable = {};
		std::optional<std::thread::id> threadIDOptional = std::nullopt;

		void setPredicate(bool value);

		public:
		Event(bool set = false);
		Event(const Event &event) = delete;
		Event &operator=(const Event &event) = delete;
		void wait(bool &yield, bool reset);
		void wait(bool reset = false);
		void set();
		void reset();
	};

	// generic "lock a thing for the duration of this scope, and only then allow me to get the thing" class (like a GlobalLock type deal)
	template <typename T> class Lock {
		private:
		Event &event;
		T &value;

		public:
		Lock(Event &event, T &value, bool &yield)
			: event(event),
			value(value) {
			event.wait(yield, true);
		}

		Lock(const Lock &lock) = delete;
		Lock &operator=(const Lock &lock) = delete;

		~Lock() {
			// always notify that we are done with this lock
			// (SOMETHING should always happen when it is released, of course)
			event.set();
		}

		T &get() const {
			return value;
		}
	};

	// a "packet" type structure representing some data (not necessarily an entire file)
	struct Data {
		typedef std::shared_ptr<unsigned char> POINTER;
		typedef std::vector<Data> VECTOR;
		typedef std::queue<Data> QUEUE;
		typedef Lock<QUEUE> QUEUE_LOCK;
		typedef std::shared_ptr<QUEUE_LOCK> QUEUE_LOCK_POINTER;

		size_t allocationSize = 0;
		size_t size = 0;

		POINTER pointer = 0;

		Data();
		Data(size_t size, POINTER pointer);
	};

	class Memory {
		private:
		#ifdef MULTITHREADED
		std::atomic<Data::VECTOR::size_type> dataVectorIndex = 0;
		Data::VECTOR dataVector = {};
		#endif
		#ifdef SINGLETHREADED
		Data data = {};
		#endif

		public:
		class Allocation {
			private:
			#ifdef MULTITHREADED
			std::atomic<Data::VECTOR::size_type> &dataVectorIndex;
			#endif

			Data &data;

			void create(size_t size);

			#ifdef MULTITHREADED
			Data &from(Data::VECTOR &dataVector);
			#endif

			public:
			#ifdef MULTITHREADED
			Allocation(std::atomic<Data::VECTOR::size_type> &dataVectorIndex, Data::VECTOR &dataVector, size_t size);
			#endif
			#ifdef SINGLETHREADED
			Allocation(Data &data, size_t size);
			#endif
			Allocation(const Allocation &allocation) = delete;
			Allocation &operator=(const Allocation &allocation) = delete;
			#ifdef MULTITHREADED
			~Allocation();
			#endif
			Data &get();
		};

		Memory();
		Memory(const Memory &memory) = delete;
		Memory &operator=(const Memory &memory) = delete;
		Allocation allocate(size_t size);
	};

	// BigFileTask (must seek over them, then come back later)
	class BigFileTask {
		private:
		// fileSystemSize MUST be defined before bigFile
		// (otherwise the constructor will be all messed up)
		// it can't be const because it's passed to BigFile's constructor by reference
		// so it has a getter instead
		// file is the associated file (so the size can be set on it later)
		std::streampos ownerBigFileInputPosition = -1;
		Ubi::BigFile::File &file;
		Ubi::BigFile::File::SIZE fileSystemSize = 0;
		Ubi::BigFile::File::POINTER_VECTOR::size_type files = 0;
		Ubi::BigFile::POINTER bigFilePointer = 0;

		public:
		typedef std::map<std::streampos, BigFileTask> MAP;
		typedef Lock<MAP> MAP_LOCK;
		typedef std::shared_ptr<MAP_LOCK> MAP_LOCK_POINTER;

		// outputPosition is set by the output thread, and later used by it so it knows where to jump back
		std::streampos outputPosition = -1;
		Ubi::BigFile::File::POINTER_VECTOR::size_type filesWritten = 0;

		BigFileTask(
			std::ifstream &inputFileStream,
			std::streampos ownerBigFileInputPosition,
			Ubi::BigFile::File &file,
			Ubi::BigFile::File::POINTER_SET_MAP &fileVectorIteratorSetMap
		);

		std::streampos getOwnerBigFileInputPosition() const;
		Ubi::BigFile::File &getFile() const;
		Ubi::BigFile::File::SIZE getFileSystemSize() const;
		Ubi::BigFile::File::POINTER_VECTOR::size_type getFiles() const;
		Ubi::BigFile::POINTER getBigFilePointer() const;
	};

	// FileTask (must be written in order)
	class FileTask {
		public:
		typedef std::queue<FileTask> QUEUE;
		typedef Lock<FileTask::QUEUE> QUEUE_LOCK;
		typedef std::shared_ptr<QUEUE_LOCK> QUEUE_LOCK_POINTER;
		typedef std::variant<Ubi::BigFile::File::POINTER_VECTOR_POINTER, Ubi::BigFile::File*> FILE_VARIANT;

		private:
		// this needs its own queue, because
		// different files will be converted at the same time, each with their own FileTask
		// (in the FileTask queue)
		// but they need to be written in order
		// so other FileTasks will be having their queues populated
		// but the output thread must not progress until the first FileTask in queue is completed
		// (because it can't know what its final size will be, and therefore the next position to go to)
		// once at the end of the data queue, the output thread will check if completed is true
		// if it's false, it'll wait on more data again, otherwise it'll move to the next FileTask
		// the output thread will check if the next file in the queue has a lesser value for bigFileInputPosition
		// and if so, the corresponding BigFile(s) in the task vector are considered completed and are written
		std::streampos ownerBigFileInputPosition = -1;
		FILE_VARIANT fileVariant = {};
		Event event;
		Data::QUEUE queue = {};

		public:
		FileTask(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File* filePointer);
		FileTask(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer);
		Data::QUEUE_LOCK lock(bool &yield);
		Data::QUEUE_LOCK lock();
		void copy(std::ifstream &inputFileStream, std::streamsize count, Memory &memory);
		void complete();
		std::streampos getOwnerBigFileInputPosition();
		FILE_VARIANT getFileVariant();
	};

	// Tasks (to be performed by the output thread)
	class Tasks {
		private:
		// the list of BigFileTasks must be a vector, because
		// they can't be handled in FIFO order
		// (otherwise, the first BigFile would block for the entire duration)
		Event bigFileEvent;
		BigFileTask::MAP bigFileTaskMap = {};

		// the list of FileTasks must be a queue, because
		// they must be written in order, start to finish
		// regardless of the order the data becomes available in
		Event fileEvent;
		FileTask::QUEUE fileTaskQueue = {};

		public:
		Tasks();
		BigFileTask::MAP_LOCK bigFileLock(bool &yield);
		BigFileTask::MAP_LOCK bigFileLock();
		FileTask::QUEUE_LOCK fileLock(bool &yield);
		FileTask::QUEUE_LOCK fileLock();
	};

	struct Output {
		std::ofstream fileStream = {};

		std::streampos currentBigFileInputPosition = -1;
		BigFileTask* bigFileTaskPointer = 0;

		Ubi::BigFile::File::SIZE filePosition = 0;
		Ubi::BigFile::File::POINTER_VECTOR::size_type filesWritten = 0;

		Output(const char* fileName);
	};
};