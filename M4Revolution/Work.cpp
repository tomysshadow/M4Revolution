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

void Work::Data::destroy() {
	pointer = 0;
}

void Work::Data::duplicate(const Data &data) {
	allocationSize = data.allocationSize;
	size = data.size;

	pointer = POINTER(new unsigned char[allocationSize]);

	errno_t err = memcpy_s(pointer.get(), size, data.pointer.get(), data.size);

	if (err) {
		throw std::system_error(err, std::generic_category());
	}
}

Work::Data::Data() {
}

Work::Data::Data(size_t allocationSize, size_t size, POINTER pointer)
	: allocationSize(allocationSize),
	size(size),
	pointer(pointer) {
}

Work::Data::Data(size_t size, POINTER pointer)
	: allocationSize(size),
	size(size),
	pointer(pointer) {
}

Work::Data::~Data() {
	destroy();
}

Work::Data::Data(const Data &data) {
	duplicate(data);
}

Work::Data &Work::Data::operator=(const Data &data) {
	duplicate(data);
	return *this;
}

void Work::Memory::Allocation::create(size_t size) {
	data.size = size;

	if (size <= data.allocationSize && data.pointer) {
		return;
	}

	data.allocationSize = size;
	data.pointer = Data::POINTER(new unsigned char[size]);
}

#ifdef MULTITHREADED
Work::Data &Work::Memory::Allocation::from(Work::Data::VECTOR &dataVector) {
	while (dataVectorIndex >= dataVector.size()) {
		dataVector.emplace_back();
	}
	return dataVector.at(dataVectorIndex++);
}

Work::Memory::Allocation::Allocation(std::atomic<Work::Data::VECTOR::size_type> &dataVectorIndex, Work::Data::VECTOR &dataVector, size_t size)
	: dataVectorIndex(dataVectorIndex),
	data(from(dataVector)) {
	create(size);
}

Work::Memory::Allocation::~Allocation() {
	dataVectorIndex--;
}
#endif
#ifdef SINGLETHREADED
Work::Memory::Allocation::Allocation(Data &data, size_t size)
	: data(data) {
	create(size);
}
#endif

Work::Data &Work::Memory::Allocation::get() {
	return data;
}

Work::Memory::Memory() {
}

Work::Memory::Allocation Work::Memory::allocate(size_t size) {
	#ifdef MULTITHREADED
	return Allocation(dataVectorIndex, dataVector, size);
	#endif
	#ifdef SINGLETHREADED
	return Allocation(data, size);
	#endif
}

Work::BigFileTask::BigFileTask(
	std::ifstream &inputFileStream,
	std::streampos ownerBigFileInputPosition,
	Ubi::BigFile::File &file,
	Ubi::BigFile::File::POINTER_SET_MAP &fileVectorIteratorSetMap
)
	: ownerBigFileInputPosition(ownerBigFileInputPosition),
	file(file),
	bigFilePointer(std::make_unique<Ubi::BigFile>(inputFileStream, fileSystemSize, files, fileVectorIteratorSetMap)) {
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
			// here, the memory object would provide no speedup (would need to copy memory twice)
			// so just allocate it with new
			Work::Data::POINTER pointer = Work::Data::POINTER(new unsigned char[countRead]);

			readFileStreamPartial(inputFileStream, pointer.get(), countRead, gcountRead);

			if (!gcountRead) {
				break;
			}

			lock().get().emplace(countRead, gcountRead, pointer);
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
			throw std::runtime_error("count must not be greater than file size");
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

Work::BigFileTask::MAP_LOCK Work::Tasks::bigFileLock(bool &yield) {
	return BigFileTask::MAP_LOCK(bigFileEvent, bigFileTaskMap, yield);
}

Work::BigFileTask::MAP_LOCK Work::Tasks::bigFileLock() {
	bool yield = false;
	return bigFileLock(yield);
}

Work::FileTask::QUEUE_LOCK Work::Tasks::fileLock(bool &yield) {
	return FileTask::QUEUE_LOCK(fileEvent, fileTaskQueue, yield);
}

Work::FileTask::QUEUE_LOCK Work::Tasks::fileLock() {
	bool yield = false;
	return fileLock(yield);
}

Work::Output::Output(const char* fileName)
	: fileStream(fileName, std::ios::binary) {
}