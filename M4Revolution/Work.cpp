#include "Work.h"
#include <stdio.h>

namespace Work {
	// acquire lock to prevent data race on predicate
	void Event::setPredicate(bool value) {
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

	Event::Event(bool set) {
		setPredicate(set);
	}

	// the point of this method is to make it so that
	// one thread will wait until another is doing work
	// then, continue waiting while it does that work
	// and then, finally, immediately wake up only once it is done that work
	// that way we do not have to sleep in a loop
	// however, in order to prevent busy waiting
	// the waiting thread must "yield" to other threads
	// meaning, it won't wake up more than once in a row
	// if no work is currently happening
	void Event::wait(bool &yield, bool reset) {
		std::unique_lock<std::mutex> lock(mutex);

		conditionVariable.wait(lock, [&] {
			// prevent spurious wakeup
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

	void Event::wait(bool reset) {
		bool yield = false;
		wait(yield, reset);
	}

	// wake up the next thread
	void Event::set() {
		setPredicate(true);
		conditionVariable.notify_one();
	}

	void Event::reset() {
		setPredicate(false);
	}

	Data::Data() {
	}

	Data::Data(size_t size, POINTER pointer)
		: size(size),
		pointer(pointer) {
	}

	BigFileTask::BigFileTask(
		std::istream &inputStream,
		std::streampos ownerBigFileInputPosition,
		Ubi::BigFile::File &file,
		Ubi::BigFile::File::POINTER_SET_MAP &fileVectorIteratorSetMap
	)
		: ownerBigFileInputPosition(ownerBigFileInputPosition),
		file(file),
		bigFilePointer(std::make_shared<Ubi::BigFile>(inputStream, fileSystemSize, files, fileVectorIteratorSetMap, file)) {
	}

	std::streampos BigFileTask::getOwnerBigFileInputPosition() const {
		return ownerBigFileInputPosition;
	}

	Ubi::BigFile::File &BigFileTask::getFile() const {
		return file;
	}

	Ubi::BigFile::File::SIZE BigFileTask::getFileSystemSize() const {
		return fileSystemSize;
	}

	Ubi::BigFile::File::POINTER_VECTOR::size_type BigFileTask::getFiles() const {
		return files;
	}

	Ubi::BigFile::POINTER BigFileTask::getBigFilePointer() const {
		return bigFilePointer;
	}

	FileTask::FileTask(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File* filePointer)
		: ownerBigFileInputPosition(ownerBigFileInputPosition),
		fileVariant(filePointer),
		event(true) {
	}

	FileTask::FileTask(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer)
		: ownerBigFileInputPosition(ownerBigFileInputPosition),
		fileVariant(filePointerVectorPointer),
		event(true) {
	}

	// called in order to lock the data queue so we can add new data
	// the Lock class ensures the output thread will automatically wake up to write it after we add the new data
	Data::QUEUE_LOCK FileTask::lock(bool &yield) {
		return Data::QUEUE_LOCK(event, queue, yield);
	}

	Data::QUEUE_LOCK FileTask::lock() {
		bool yield = false;
		return lock(yield);
	}

	void FileTask::copy(std::istream &inputStream, std::streamsize count) {
		if (!count) {
			return;
		}

		const size_t BUFFER_SIZE = 0x10000;

		std::streamsize countRead = BUFFER_SIZE;
		std::streamsize gcountRead = 0;

		do {
			countRead = (std::streamsize)__min((size_t)count, (size_t)countRead);

			{
				Data::POINTER pointer(new unsigned char[(size_t)countRead]);

				readStreamPartial(inputStream, pointer.get(), countRead, gcountRead);

				if (!gcountRead) {
					break;
				}

				lock().get().emplace((size_t)gcountRead, pointer);
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
	void FileTask::complete() {
		lock().get().emplace();
	}

	std::streampos FileTask::getOwnerBigFileInputPosition() {
		return ownerBigFileInputPosition;
	}

	FileTask::FILE_VARIANT FileTask::getFileVariant() {
		return fileVariant;
	}

	Tasks::Tasks()
		: bigFileEvent(true),
		fileEvent(true) {
	}

	BigFileTask::POINTER_MAP_LOCK Tasks::bigFileLock(bool &yield) {
		return BigFileTask::POINTER_MAP_LOCK(bigFileEvent, bigFileTaskPointerMap, yield);
	}

	BigFileTask::POINTER_MAP_LOCK Tasks::bigFileLock() {
		bool yield = false;
		return bigFileLock(yield);
	}

	FileTask::POINTER_QUEUE_LOCK Tasks::fileLock(bool &yield) {
		return FileTask::POINTER_QUEUE_LOCK(fileEvent, fileTaskPointerQueue, yield);
	}

	FileTask::POINTER_QUEUE_LOCK Tasks::fileLock() {
		bool yield = false;
		return fileLock(yield);
	}

	Convert::Convert(
		const Configuration &configuration,
		const nvtt::Context &context,
		Ubi::BigFile::File &file
	)
		: CONFIGURATION(configuration),
		CONTEXT(context),
		file(file) {
	}

	const char* Output::FILE_NAME = "~M4R.tmp"; // must be an 8.3 filename
	const char* Output::FILE_RETRY = "The game files could not be accessed. Please ensure the game is not open while using this tool. If this error is occuring and the game is not open, you may be out of disk space, or you may need to run this tool as admin.";

	const Output::PATH_MAP Output::FILE_PATH_MAP = {
		{FILE_PATH_DATA, "data/data.m4b"},
		{FILE_PATH_M4_THOR, "bin/m4_thor_rd.dll"},
		{FILE_PATH_GFX_TOOLS, "bin/gfx_tools_rd.dll"}
	};

	const std::filesystem::path Output::DATA_PATH = FILE_PATH_MAP.at(FILE_PATH_DATA);
	const std::filesystem::path Output::M4_THOR_PATH = FILE_PATH_MAP.at(FILE_PATH_M4_THOR);
	const std::filesystem::path Output::GFX_TOOLS_PATH = FILE_PATH_MAP.at(FILE_PATH_GFX_TOOLS);

	void Output::findInstallPath(const std::filesystem::path &path) {
		bool foundInstalled = setPath(path);

		if (foundInstalled) {
			consoleLog("An install of Myst IV: Revelation was found at this path:");
			consoleLog(path.string().c_str(), 2);

			foundInstalled = consoleBool("Is this the path to the install you would like to modify?", true);
		} else {
			consoleLog("No install of Myst IV: Relevation was found. ", false);
		}

		if (!foundInstalled) {
			while (!setPath(consoleString("Please enter the path to Myst IV: Relevation."))) {
				consoleLog("No install of Myst IV: Relevation was found at this path. ", false);
			}
		}
	}

	bool Output::setPath(const std::filesystem::path &path) {
		// this check is just to prevent the user from being dumb
		// we need proper checks upon actually opening these as well of course
		try {
			std::filesystem::current_path(path);
		} catch (std::filesystem::filesystem_error) {
			return false;
		}

		for (PATH_MAP::const_iterator pathMapIterator = FILE_PATH_MAP.begin(); pathMapIterator != FILE_PATH_MAP.end(); pathMapIterator++) {
			if (!std::filesystem::is_regular_file(pathMapIterator->second)) {
				return false;
			}
		}
		return true;
	}

	Output::Output() {
		// without this remove first it may crash trying to open a hidden file
		// (I mean, this isn't atomic so that can happen anyway but at least it's not our fault then)
		// this is just a temp file so deleting it should be fine
		std::filesystem::remove(FILE_NAME);

		fileStream.open(FILE_NAME, std::ios::binary | std::ios::trunc, _SH_DENYRW);

		#ifdef WINDOWS
		setFileAttributeHidden(true, FILE_NAME);
		#endif
	}

	Output::~Output() {
		#ifdef WINDOWS
		setFileAttributeHidden(false, FILE_NAME);
		#endif
	}

	namespace Backup {
		bool create(const char* fileName) {
			// here I'm using CRT rename because I don't want to be able
			// to overwrite the backup file (which std::filesystem::rename has no option to disallow)
			bool result = !rename(fileName, getPath(fileName).string().c_str());

			if (!result && errno != EEXIST) {
				throw std::runtime_error("Failed to Rename");
			}

			// here I use std::filesystem::rename because I do want to overwrite the file if it exists
			OPERATION_EXCEPTION_RETRY_ERR(std::filesystem::rename(Output::FILE_NAME, fileName), std::filesystem::filesystem_error, Output::FILE_RETRY);
			return result;
		}

		bool createFromOutput(const char* fileName) {
			bool result = !rename(Output::FILE_NAME, getPath(fileName).string().c_str());

			if (!result && errno != EEXIST) {
				throw std::runtime_error("Failed to Rename");
			}
			return result;
		}

		void restore(const std::filesystem::path &path) {
			OPERATION_EXCEPTION_RETRY_ERR(std::filesystem::rename(getPath(path), path), std::filesystem::filesystem_error, Output::FILE_RETRY);
		}

		void log() {
			consoleLog("A backup has been created.", 2);
		}

		std::filesystem::path getPath(std::filesystem::path path) {
			return path.replace_extension("bak");
		}
	}

	void Edit::copyThread(Edit &edit) {
		std::fstream &fileStream = edit.fileStream;
		bool backup = false;

		if (!edit.copied) {
			// check if the file exists, if it doesn't create a backup
			std::ifstream backupInputFileStream(Backup::getPath(edit.path), std::ios::binary, _SH_DENYWR);

			if (!backupInputFileStream.is_open()) {
				// always delete the temporary file when done
				SCOPE_EXIT {
					std::filesystem::remove(Output::FILE_NAME);
				};

				{
					Output output = {};

					fileStream.seekg(0);
					copyStream(fileStream, output.fileStream);
				}

				backup = Backup::createFromOutput(edit.path.string().c_str());
			}

			edit.copied = true;
		}

		edit.event.wait(true);

		if (backup) {
			Backup::log();
		}

		fileStream.seekp(edit.position);
		writeStreamSafe(fileStream, edit.str.c_str(), edit.str.length());
	}

	Edit::Edit(std::fstream &fileStream, const std::filesystem::path &path)
		: fileStream(fileStream),
		path(path) {
		if (!fileStream.is_open()) {
			fileStream.open(path, std::ios::binary | std::ios::in | std::ios::out, _SH_DENYRW);
		}
	}

	void Edit::join(std::thread &copyThread, std::streampos position, const std::string &str) {
		this->position = position;
		this->str = str;

		if (!copied) {
			consoleLog("Please wait...", 2);
		}

		event.set();

		copyThread.join();
	}
}