#include "shared.h"
#include "Ubi.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>

#include <nvtt/nvtt.h>

#if defined(WIN32)
#define MULTITHREADED
#else
#define SINGLETHREADED
#endif

namespace Work {
	// a "signal the other thread to wake up and do stuff" class (similar to SetEvent)
	class Event {
		private:
		std::mutex mutex = {};
		std::condition_variable conditionVariable = {};
		std::optional<std::thread::id> threadIDOptional = std::nullopt;

		void setPredicate(bool value);

		public:
		Event(bool set = false);
		void wait(bool reset = false);
		void set();
		void reset();
	};

	// generic "lock a thing for the duration of this scope, and only then allow me to get the thing" class (like a GlobalLock type deal)
	template <typename T> class Lock {
		private:
		Event &event;
		T &value;
		bool sync = false;

		public:
		Lock(Event &event, T &value, bool sync = false)
			: event(event),
			value(value),
			sync(sync) {
			event.wait(true);
		}

		~Lock() {
			if (sync) {
				event.set();
			}
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
		typedef std::unique_ptr<QUEUE_LOCK> QUEUE_LOCK_POINTER;

		size_t size = 0;
		POINTER pointer = 0;

		Data();
		Data(size_t size, POINTER pointer);
	};

	// BigFileTask (must seek over them, then come back later)
	class BigFileTask {
		public:
		typedef std::vector<BigFileTask> VECTOR;
		typedef Lock<VECTOR> VECTOR_LOCK;
		typedef std::unique_ptr<VECTOR_LOCK> VECTOR_LOCK_POINTER;

		// INPUT_POSITION is so the writer thread knows when we've passed this BigFile (and can safely write it)
		// since FileTasks in the task queue will have a smaller BIG_FILE_INPUT_POSITION
		// otherwise, the sizes and positions would be wrong
		// essentially, this fills a similar role to the completed variable in FileTask
		const std::streampos INPUT_POSITION;

		// outputPosition is set by the writer thread, and later used by it so it knows where to jump back
		std::streampos outputPosition = 0;

		private:
		// fileSystemSize MUST be defined before bigFile
		// (otherwise the constructor will be all messed up)
		// it can't be const because it's passed to BigFile's constructor by reference
		// so it has a getter instead
		// file is the associated file (so the size can be set on it later)
		Ubi::BigFile::File::SIZE fileSystemSize = 0;
		Ubi::BigFile::File &file;

		public:
		const Ubi::BigFile BIG_FILE;

		BigFileTask(std::ifstream &inputFileStream, Ubi::BigFile::File &file, Ubi::BigFile::File::POINTER_SET_MAP &fileVectorIteratorSetMap);
		Ubi::BigFile::File::SIZE getFileSystemSize() const;
		Ubi::BigFile::File &getFile() const;
	};

	// FileTask (must be written in order)
	class FileTask {
		public:
		typedef std::queue<FileTask> QUEUE;
		typedef Lock<FileTask::QUEUE> QUEUE_LOCK;
		typedef std::unique_ptr<QUEUE_LOCK> QUEUE_LOCK_POINTER;

		// the writer thread will check if the next file in the queue has a lesser value for this
		// and if so, the corresponding BigFile(s) in the task vector are considered completed and are written
		const std::streampos BIG_FILE_INPUT_POSITION;
		const std::optional<Ubi::BigFile::File::POINTER_VECTOR> FILE_POINTER_VECTOR_OPTIONAL;

		private:
		void create(std::ifstream &inputFileStream, std::streamsize count);

		// this needs its own queue, because
		// different files will be converted at the same time, each with their own FileTask
		// (in the FileTask queue)
		// but they need to be written in order
		// so other FileTasks will be having their queues populated
		// but the writer thread must not progress until the first FileTask in queue is completed
		// (because it can't know what its final size will be, and therefore the next position to go to)
		// once at the end of the data queue, the writer thread will check if completed is true
		// if it's false, it'll wait on more data again, otherwise it'll move to the next FileTask
		Event event;
		Data::QUEUE queue = {};
		bool completed = false;

		public:
		FileTask(std::streampos bigFileInputPosition);
		FileTask(std::streampos bigFileInputPosition, std::ifstream &inputFileStream, std::streamsize count, Ubi::BigFile::File::POINTER_VECTOR &filePointerVector);
		FileTask(std::streampos bigFileInputPosition, std::ifstream &inputFileStream, std::streamsize count);
		Data::QUEUE_LOCK lock(bool sync = false);
		void lock(Data::QUEUE_LOCK_POINTER &queueLockPointer, bool sync = false);
		void complete();
		bool getCompleted() const;
	};

	// Tasks (to be performed by the writer thread)
	class Tasks {
		private:
		// the list of BigFileTasks must be a vector, because
		// they can't be handled in FIFO order
		// (otherwise, the first BigFile would block for the entire duration)
		Event bigFileEvent;
		BigFileTask::VECTOR bigFileTaskVector = {};

		// the list of FileTasks must be a queue, because
		// they must be written in order, start to finish
		// regardless of the order the data becomes available in
		Event fileEvent;
		FileTask::QUEUE fileTaskQueue = {};

		public:
		Tasks();
		BigFileTask::VECTOR_LOCK bigFileLock(bool sync = false);
		void bigFileLock(BigFileTask::VECTOR_LOCK_POINTER &vectorLockPointer, bool sync = false);
		FileTask::QUEUE_LOCK fileLock(bool sync = false);
		void fileLock(FileTask::QUEUE_LOCK_POINTER &queueLockPointer, bool sync = false);
	};
};