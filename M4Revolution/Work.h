#pragma once
#include "Ubi.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <filesystem>
#include <nvtt/nvtt.h>

#define GAMEDATABINDIR "data"
#define EXEDIR "bin"

#ifdef WINDOWS
	#define MULTITHREADED
#else
	#define SINGLETHREADED
#endif

namespace Work {
	// a "signal the other thread to wake up and do stuff" class (similar to SetEvent)
	class Event : NonCopyable {
		private:
		// threadIdOptional has a thread ID if the event is set (notified, time to wake up, lock is unlocked...)
		// and is std::nullopt if the event is not set (locked, currently in use, etc.)
		// the thread ID is used to yield to other threads instead of busy looping
		std::mutex mutex = {};
		std::condition_variable conditionVariable = {};
		std::optional<std::thread::id> threadIdOptional = std::nullopt;

		void setPredicate(bool value);

		public:
		Event(bool set = false);
		void wait(bool &yield, bool reset);
		void wait(bool reset = false);
		void set();
		void reset();
	};

	// generic "lock a thing for the duration of this scope, and only then allow me to get the thing" class
	// (like a GlobalLock type deal)
	template <typename T> class Lock : NonCopyable {
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

		T &get() const {
			return value;
		}
	};

	// a "packet" type structure representing some data (not necessarily an entire file)
	struct Data {
		using Pointer = std::shared_ptr<unsigned char[]>;
		using Queue = std::queue<Data>;
		using QueueLock = Lock<Queue>;

		size_t size = 0;
		Pointer pointer = nullptr;

		Data() = default;
		Data(size_t size, Pointer pointer);
	};

	// BigFileTask (must seek over them, then come back later)
	class BigFileTask {
		private:
		// fileSystemSize MUST be defined before bigFile
		// (otherwise the constructor will be all messed up)
		// it can't be const because it's passed to BigFile's constructor by reference
		// so it has a getter instead
		// file is the associated file (so the size can be set on it later)
		std::streamoff ownerBigFileInputOffset = -1;
		Ubi::BigFile::File &file;
		Ubi::BigFile::File::Size fileSystemSize = 0;
		Ubi::BigFile::File::PointerVector::size_type files = 0;
		Ubi::BigFile::Pointer bigFilePointer = nullptr;

		public:
		using Pointer = std::shared_ptr<BigFileTask>;
		using PointerMap = std::unordered_map<std::streamoff, Pointer>;
		using PointerMapLock = Lock<PointerMap>;

		// outputOffset is set by the output thread, and later used by it so it knows where to jump back
		std::streamoff outputOffset = -1;
		Ubi::BigFile::File::PointerVector::size_type filesWritten = 0;

		BigFileTask(
			std::istream &inputStream,
			std::streamoff ownerBigFileInputOffset,
			Ubi::BigFile::File &file,
			Ubi::BigFile::File::PointerSetMap &fileVectorIteratorSetMap
		);

		std::streamoff getOwnerBigFileInputOffset() const;
		Ubi::BigFile::File &getFile() const;
		Ubi::BigFile::File::Size getFileSystemSize() const;
		Ubi::BigFile::File::PointerVector::size_type getFiles() const;
		Ubi::BigFile::Pointer getBigFilePointer() const;
	};

	// FileTask (must be written in order)
	class FileTask {
		public:
		using Pointer = std::shared_ptr<FileTask>;
		using PointerQueue = std::queue<Pointer>;
		using PointerQueueLock = Lock<PointerQueue>;
		using FileVariant = std::variant<Ubi::BigFile::File::PointerVectorPointer, Ubi::BigFile::File*>;

		private:
		// this needs its own queue, because
		// different files will be converted at the same time, each with their own FileTask
		// (in the FileTask queue)
		// but they need to be written in order
		// so other FileTasks will be having their queues populated
		// but the output thread must not progress until the first FileTask in queue is completed
		// (because it can't know what its final size will be, and therefore the next offset to go to)
		// once at the end of the data queue, the output thread will check if completed is true
		// if it's false, it'll wait on more data again, otherwise it'll move to the next FileTask
		// the output thread will check if the next file in the queue has a lesser value for bigFileInputPosition
		// and if so, the corresponding BigFile(s) in the task vector are considered completed and are written
		std::streamoff ownerBigFileInputOffset = -1;
		FileVariant fileVariant = {};
		Event event;
		Data::Queue queue = {};

		public:
		FileTask(std::streamoff ownerBigFileInputOffset, Ubi::BigFile::File* filePointer);
		FileTask(std::streamoff ownerBigFileInputOffset, Ubi::BigFile::File::PointerVectorPointer &filePointerVectorPointer);
		Data::QueueLock lock(bool &yield);
		Data::QueueLock lock();
		void copy(std::istream &inputStream, std::streamsize count);
		void complete();
		std::streamoff getOwnerBigFileInputOffset();
		FileVariant getFileVariant();
	};

	// Tasks (to be performed by the output thread)
	class Tasks {
		private:
		// the list of BigFileTasks must be a vector, because
		// they can't be handled in FIFO order
		// (otherwise, the first BigFile would block for the entire duration)
		Event bigFileEvent;
		BigFileTask::PointerMap bigFileTaskPointerMap = {};

		// the list of FileTasks must be a queue, because
		// they must be written in order, start to finish
		// regardless of the order the data becomes available in
		Event fileEvent;
		FileTask::PointerQueue fileTaskPointerQueue = {};

		public:
		Tasks();
		BigFileTask::PointerMapLock bigFileLock(bool &yield);
		BigFileTask::PointerMapLock bigFileLock();
		FileTask::PointerQueueLock fileLock(bool &yield);
		FileTask::PointerQueueLock fileLock();
	};

	struct Convert {
		using Extent = unsigned long;
		using FileWorkCallback = void(*)(Work::Convert* convertPointer);

		struct Configuration {
			Extent minTextureWidth = 1;
			Extent maxTextureWidth = 1024;
			Extent minTextureHeight = 1;
			Extent maxTextureHeight = 1024;
			Extent minVolumeExtent = 1;
			Extent maxVolumeExtent = 1024;
		};

		FileWorkCallback fileWorkCallback = 0;

		const Configuration &configuration;
		const nvtt::Context &context;

		Ubi::BigFile::File &file;

		FileTask::Pointer fileTaskPointer = nullptr;
		Data::Pointer dataPointer = nullptr;

		Convert(
			const Configuration &configuration,
			const nvtt::Context &context,
			Ubi::BigFile::File &file
		);
	};

	struct Output {
		std::ofstream fileStream = {};

		std::streamoff currentBigFileInputOffset = -1;
		BigFileTask::Pointer bigFileTaskPointer = nullptr;

		Ubi::BigFile::File::Size fileOffset = 0;
		Ubi::BigFile::File::PointerVector::size_type filesWritten = 0;

		struct Info {
			std::filesystem::path path = {};
			bool required = false;
		};

		using FilePath = unsigned int;
		using InfoMap = std::unordered_map<FilePath, Info>;

		static const char* FILE_NAME;
		static const char* FILE_RETRY;

		static constexpr FilePath FILE_PATH_DATA = 0x00000001;
		static constexpr FilePath FILE_PATH_USER_PREFERENCE = 0x00000002;
		static constexpr FilePath FILE_PATH_M4_THOR = 0x00000004;
		static constexpr FilePath FILE_PATH_M4_AI_GLOBAL = 0x00000008;
		static constexpr FilePath FILE_PATH_GFX_TOOLS = 0x00000010;

		static const InfoMap FILE_PATH_INFO_MAP;

		static const std::filesystem::path DATA_PATH;
		static const std::filesystem::path USER_PREFERENCE_PATH;
		static const std::filesystem::path M4_THOR_PATH;
		static const std::filesystem::path M4_AI_GLOBAL_PATH;
		static const std::filesystem::path GFX_TOOLS_PATH;

		static void findInstallPath(const std::filesystem::path &path);
		static bool setPath(const std::filesystem::path &path);

		Output(bool binary = true);
		~Output();
	};

	namespace Backup {
		void create(const char* fileName);
		void createOutput(const char* fileName);
		void createEmpty(const std::filesystem::path &path);
		void deleteEmpty(const std::filesystem::path &path);
		void restore(const std::filesystem::path &path);
		std::filesystem::path getPath(std::filesystem::path path);
	}

	class Edit {
		public:
		static void copyThread(Edit &edit);

		template <typename Value>
		static void outputCurrent(std::ostream &outputStream, const std::string &name, const Value &value) {
			outputStream << "The current " << name << " is: " << value << ".";
		}

		static void outputNew(std::ostream &outputStream, const std::string &name) {
			outputStream << "Please enter the new " << name << ".";
		}

		struct Code {
			std::streamoff offset = 0;
			std::string str = "";
		};

		using CodeVector = std::vector<Code>;

		std::fstream &fileStream;

		Edit(std::fstream &fileStream, const std::filesystem::path &path);
		void apply(std::thread &copyThread, const CodeVector &codeVector);

		private:
		std::filesystem::path path = {};
		CodeVector codeVector = {};
		Event event;
		bool copied = false;
	};
};