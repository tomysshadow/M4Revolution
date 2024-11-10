#pragma once
#include "shared.h"
#include "Ubi.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include <map>
#include <filesystem>

#include <nvtt/nvtt.h>

#ifdef WINDOWS
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

		~Lock() {
			// always notify that we are done with this lock
			// (SOMETHING should always happen when it is released, of course)
			event.set();
		}

		Lock(const Lock &lock) = delete;
		Lock &operator=(const Lock &lock) = delete;

		T &get() const {
			return value;
		}
	};

	// a "packet" type structure representing some data (not necessarily an entire file)
	struct Data {
		typedef std::shared_ptr<unsigned char> POINTER;
		typedef std::queue<Data> QUEUE;
		typedef Lock<QUEUE> QUEUE_LOCK;

		size_t size = 0;
		POINTER pointer = 0;

		Data();
		Data(size_t size, POINTER pointer);
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
		typedef std::shared_ptr<BigFileTask> POINTER;
		typedef std::map<std::streampos, POINTER> POINTER_MAP;
		typedef Lock<POINTER_MAP> POINTER_MAP_LOCK;

		// outputPosition is set by the output thread, and later used by it so it knows where to jump back
		std::streampos outputPosition = -1;
		Ubi::BigFile::File::POINTER_VECTOR::size_type filesWritten = 0;

		BigFileTask(
			std::istream &inputStream,
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
		typedef std::shared_ptr<FileTask> POINTER;
		typedef std::queue<POINTER> POINTER_QUEUE;
		typedef Lock<POINTER_QUEUE> POINTER_QUEUE_LOCK;
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
		void copy(std::istream &inputStream, std::streamsize count);
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
		BigFileTask::POINTER_MAP bigFileTaskPointerMap = {};

		// the list of FileTasks must be a queue, because
		// they must be written in order, start to finish
		// regardless of the order the data becomes available in
		Event fileEvent;
		FileTask::POINTER_QUEUE fileTaskPointerQueue = {};

		public:
		Tasks();
		BigFileTask::POINTER_MAP_LOCK bigFileLock(bool &yield);
		BigFileTask::POINTER_MAP_LOCK bigFileLock();
		FileTask::POINTER_QUEUE_LOCK fileLock(bool &yield);
		FileTask::POINTER_QUEUE_LOCK fileLock();
	};

	struct Convert {
		typedef uint32_t EXTENT;
		typedef void(*FileWorkCallback)(Work::Convert* convertPointer);

		struct Configuration {
			EXTENT minTextureWidth = 1;
			EXTENT maxTextureWidth = 1024;
			EXTENT minTextureHeight = 1;
			EXTENT maxTextureHeight = 1024;
			EXTENT minVolumeExtent = 1;
			EXTENT maxVolumeExtent = 1024;
		};

		FileWorkCallback fileWorkCallback = 0;

		Ubi::BigFile::File &file;

		Configuration &configuration;

		nvtt::Context &context;
		nvtt::CompressionOptions &compressionOptions;
		nvtt::CompressionOptions &compressionOptionsAlpha;

		FileTask::POINTER fileTaskPointer = 0;
		Data::POINTER dataPointer = 0;

		Convert(
			Ubi::BigFile::File &file,
			Configuration &configuration,
			nvtt::Context &context,
			nvtt::CompressionOptions &compressionOptions,
			nvtt::CompressionOptions &compressionOptionsAlpha
		);
	};

	struct Output {
		std::ofstream fileStream = {};

		std::streampos currentBigFileInputPosition = -1;
		BigFileTask::POINTER bigFileTaskPointer = 0;

		Ubi::BigFile::File::SIZE filePosition = 0;
		Ubi::BigFile::File::POINTER_VECTOR::size_type filesWritten = 0;

		typedef int FILE_PATH;
		typedef std::map<FILE_PATH, std::filesystem::path> PATH_MAP;

		static const char* FILE_NAME;
		static const char* FILE_RETRY;

		static const FILE_PATH FILE_PATH_DATA = 1;
		static const FILE_PATH FILE_PATH_M4_THOR = 2;
		static const FILE_PATH FILE_PATH_GFX_TOOLS = 4;

		static const PATH_MAP FILE_PATH_MAP;

		static const std::filesystem::path DATA_PATH;
		static const std::filesystem::path M4_THOR_PATH;
		static const std::filesystem::path GFX_TOOLS_PATH;

		static void findInstallPath(const std::filesystem::path &path);
		static bool setPath(const std::filesystem::path &path);

		Output();
		~Output();
	};

	namespace Backup {
		bool create(const char* fileName);
		bool createFromOutput(const char* fileName);
		void restore(const std::filesystem::path &path);
		void log();
		std::filesystem::path getPath(std::filesystem::path path);
	}

	class Edit {
		private:
		std::filesystem::path path = {};

		std::streampos position = 0;
		std::string str = "";

		Event event;
		bool copied = false;

		public:
		static void copyThread(Edit &edit);

		std::fstream &fileStream;

		Edit(std::fstream &fileStream, const std::filesystem::path &path);
		void join(std::thread &copyThread, std::streampos position, const std::string &str);
	};
};