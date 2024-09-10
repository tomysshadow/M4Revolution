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
void Work::Event::wait(bool &yield, bool reset) {
	std::unique_lock<std::mutex> lock(mutex);

	conditionVariable.wait(lock, [&] {
		// if we are yielding, we don't allow waking up until some other thread than us is the one that has set the event
		// otherwise, all that matters is that the event was in fact set (i.e., the owning lock isn't currently in use by anyone)
		if (threadIDOptional.has_value() && (!yield || threadIDOptional != std::this_thread::get_id())) {
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

void Work::Event::wait(bool reset) {
	bool yield = false;
	wait(yield, reset);
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

Work::BigFileTask::BigFileTask(
	std::ifstream &inputFileStream,
	std::streampos ownerBigFileInputPosition,
	Ubi::BigFile::File &file,
	Ubi::BigFile::File::POINTER_SET_MAP &fileVectorIteratorSetMap
)
	: ownerBigFileInputPosition(ownerBigFileInputPosition),
	file(file),
	bigFilePointer(std::make_shared<Ubi::BigFile>(inputFileStream, fileSystemSize, files, fileVectorIteratorSetMap, file)) {
}

std::streampos Work::BigFileTask::getOwnerBigFileInputPosition() const {
	return ownerBigFileInputPosition;
}

Ubi::BigFile::File &Work::BigFileTask::getFile() const {
	return file;
}

Ubi::BigFile::File::SIZE Work::BigFileTask::getFileSystemSize() const {
	return fileSystemSize;
}

Ubi::BigFile::File::POINTER_VECTOR::size_type Work::BigFileTask::getFiles() const {
	return files;
}

Ubi::BigFile::POINTER Work::BigFileTask::getBigFilePointer() const {
	return bigFilePointer;
}

Work::FileTask::FileTask(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File* filePointer)
	: ownerBigFileInputPosition(ownerBigFileInputPosition),
	fileVariant(filePointer),
	event(true) {
}

Work::FileTask::FileTask(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer)
	: ownerBigFileInputPosition(ownerBigFileInputPosition),
	fileVariant(filePointerVectorPointer),
	event(true) {
}

// called in order to lock the data queue so we can add new data
// the Lock class ensures the output thread will automatically wake up to write it after we add the new data
Work::Data::QUEUE_LOCK Work::FileTask::lock(bool &yield) {
	return Data::QUEUE_LOCK(event, queue, yield);
}

Work::Data::QUEUE_LOCK Work::FileTask::lock() {
	bool yield = false;
	return lock(yield);
}

void Work::FileTask::copy(std::ifstream &inputFileStream, std::streamsize count) {
	if (!count) {
		return;
	}

	const size_t BUFFER_SIZE = 0x10000;

	std::streamsize countRead = BUFFER_SIZE;
	std::streamsize gcountRead = 0;

	do {
		countRead = (std::streamsize)min((size_t)count, (size_t)countRead);

		{
			Data::POINTER pointer = Data::POINTER(new unsigned char[countRead]);

			readFileStreamPartial(inputFileStream, pointer.get(), countRead, gcountRead);

			if (!gcountRead) {
				break;
			}

			lock().get().emplace(gcountRead, pointer);
		}

		if (count != -1) {
			count -= gcountRead;

			if (!count) {
				break;
			}
		}
	} while (countRead == gcountRead);

	if (count != -1) {
		if (count) {
			throw std::logic_error("count must not be greater than file size");
		}
	}
}

// called to signal to the output thread that we are done adding new data
void Work::FileTask::complete() {
	lock().get().emplace();
}

std::streampos Work::FileTask::getOwnerBigFileInputPosition() {
	return ownerBigFileInputPosition;
}

Work::FileTask::FILE_VARIANT Work::FileTask::getFileVariant() {
	return fileVariant;
}

Work::Tasks::Tasks()
	: bigFileEvent(true),
	fileEvent(true) {
}

Work::BigFileTask::POINTER_MAP_LOCK Work::Tasks::bigFileLock(bool &yield) {
	return BigFileTask::POINTER_MAP_LOCK(bigFileEvent, bigFileTaskPointerMap, yield);
}

Work::BigFileTask::POINTER_MAP_LOCK Work::Tasks::bigFileLock() {
	bool yield = false;
	return bigFileLock(yield);
}

Work::FileTask::POINTER_QUEUE_LOCK Work::Tasks::fileLock(bool &yield) {
	return FileTask::POINTER_QUEUE_LOCK(fileEvent, fileTaskPointerQueue, yield);
}

Work::FileTask::POINTER_QUEUE_LOCK Work::Tasks::fileLock() {
	bool yield = false;
	return fileLock(yield);
}

Work::Convert::Convert(
	Ubi::BigFile::File &file,
	nvtt::Context &context,
	nvtt::CompressionOptions &compressionOptionsDXT5
)
	: file(file),
	context(context),
	compressionOptions(compressionOptions) {
}

Work::Output::Output(const char* fileName)
	: fileStream(fileName, std::ios::binary) {
}