#include "Work.h"

// acquire lock to prevent data race on predicate
void Work::Event::setPredicate(bool value) {
	std::lock_guard<std::mutex> lock(mutex);

	if (value) {
		// if the event is set
		// then the wait should immediately go through
		// (as long as the thread ID does not match this)
		threadIDOptional = std::this_thread::get_id();
	} else {
		// if the event is not set
		// then the event should wait no matter what
		// (should return false)
		threadIDOptional = std::nullopt;
	}
}

Work::Event::Event(bool set) {
	setPredicate(set);
}

// prevent spurious wakeup
void Work::Event::wait(bool reset) {
	std::unique_lock<std::mutex> lock(mutex);

	conditionVariable.wait(lock, [&] {
		if (threadIDOptional != std::this_thread::get_id()) {
			// reset the event if desired (this is run while the lock is held, so is safe)
			if (reset) {
				// if the event is reset
				// then that should cause this to wait (return false)
				threadIDOptional = std::nullopt;
			}
			return true;
		}
		return false;
	});
}

// wake up the next thread
void Work::Event::set() {
	setPredicate(true);
	conditionVariable.notify_one();
}

void Work::Event::reset() {
	setPredicate(false);
}

Work::Data::Data() {
}

Work::Data::Data(size_t size, POINTER pointer)
	: size(size),
	pointer(pointer) {
}

Work::BigFileTask::BigFileTask(std::ifstream &inputFileStream, Ubi::BigFile::File &file, Ubi::BigFile::File::POINTER_SET_MAP &fileVectorIteratorSetMap)
	: INPUT_POSITION(inputFileStream.tellg()),
	file(file),
	BIG_FILE(Ubi::BigFile(inputFileStream, fileSystemSize, fileVectorIteratorSetMap)) {
}

Ubi::BigFile::File::SIZE Work::BigFileTask::getFileSystemSize() const {
	return fileSystemSize;
}

Ubi::BigFile::File &Work::BigFileTask::getFile() const {
	return file;
}

// TODO: this feels sketch, make sure it works
void Work::FileTask::create(std::ifstream &inputFileStream, std::streamsize count) {
	if (!count) {
		return;
	}

	const size_t BUFFER_SIZE = 0x10000;

	Work::Data::POINTER pointer = 0;
	std::streamsize countRead = BUFFER_SIZE;
	std::streamsize gcountRead = 0;

	do {
		countRead = (std::streamsize)min((size_t)count, (size_t)countRead);
		pointer = Work::Data::POINTER(new unsigned char[countRead]);

		readFileStreamPartial(inputFileStream, pointer.get(), countRead, gcountRead);

		if (!gcountRead) {
			break;
		}

		queue.emplace(gcountRead, pointer);

		if (count != -1) {
			count -= gcountRead;

			if (!count) {
				break;
			}
		}
	} while (countRead == gcountRead);

	if (count != -1) {
		if (count) {
			throw std::runtime_error("count must not be greater than file size");
		}
	}
}

Work::FileTask::FileTask(std::streampos bigFileInputPosition)
	: BIG_FILE_INPUT_POSITION(bigFileInputPosition),
	FILE_POINTER_VECTOR_OPTIONAL(std::nullopt),
	event(false) {
}

Work::FileTask::FileTask(std::streampos bigFileInputPosition, std::ifstream &inputFileStream, std::streamsize count, Ubi::BigFile::File::POINTER_VECTOR &filePointerVector)
	: BIG_FILE_INPUT_POSITION(bigFileInputPosition),
	FILE_POINTER_VECTOR_OPTIONAL(std::move(filePointerVector)),
	event(false) {
	create(inputFileStream, count);
}

Work::FileTask::FileTask(std::streampos bigFileInputPosition, std::ifstream &inputFileStream, std::streamsize count)
	: BIG_FILE_INPUT_POSITION(bigFileInputPosition),
	FILE_POINTER_VECTOR_OPTIONAL(std::nullopt),
	event(false) {
	create(inputFileStream, count);
}

// called in order to lock the data queue so we can add new data
// the Lock class ensures the writer thread will automatically wake up to write it after we add the new data
Work::Data::QUEUE_LOCK Work::FileTask::lock(bool sync) {
	return Data::QUEUE_LOCK(event, queue, sync);
}

void Work::FileTask::lock(Data::QUEUE_LOCK_POINTER &queueLockPointer, bool sync) {
	queueLockPointer = std::make_unique<Data::QUEUE_LOCK>(event, queue, sync);
}

// called to signal to the writer thread that we are done adding new data
// the event is set in order to make sure it gets this message
void Work::FileTask::complete() {
	completed = true;
	event.set();
}

bool Work::FileTask::getCompleted() const {
	return completed;
}

Work::Tasks::Tasks()
	: bigFileEvent(false),
	fileEvent(false) {
}

Work::BigFileTask::VECTOR_LOCK Work::Tasks::bigFileLock(bool sync) {
	return BigFileTask::VECTOR_LOCK(bigFileEvent, bigFileTaskVector, sync);
}

void Work::Tasks::bigFileLock(BigFileTask::VECTOR_LOCK_POINTER &vectorLockPointer, bool sync) {
	vectorLockPointer = std::make_unique<BigFileTask::VECTOR_LOCK>(bigFileEvent, bigFileTaskVector, sync);
}

Work::FileTask::QUEUE_LOCK Work::Tasks::fileLock(bool sync) {
	return FileTask::QUEUE_LOCK(fileEvent, fileTaskQueue, sync);
}

void Work::Tasks::fileLock(FileTask::QUEUE_LOCK_POINTER &queueLockPointer, bool sync) {
	queueLockPointer = std::make_unique<FileTask::QUEUE_LOCK>(fileEvent, fileTaskQueue, sync);
}