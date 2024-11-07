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

	// prevent spurious wakeup
	void Event::wait(bool &yield, bool reset) {
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
			countRead = (std::streamsize)min((size_t)count, (size_t)countRead);

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
		Ubi::BigFile::File &file,
		Configuration &configuration,
		nvtt::Context &context,
		nvtt::CompressionOptions &compressionOptions,
		nvtt::CompressionOptions &compressionOptionsAlpha
	)
		: file(file),
		configuration(configuration),
		context(context),
		compressionOptions(compressionOptions),
		compressionOptionsAlpha(compressionOptionsAlpha) {
	}

	const char* Output::FILE_NAME = "~M4R.tmp";

	const std::filesystem::path Output::DATA_PATH("data/data.m4b");
	const std::filesystem::path Output::GFX_TOOLS_PATH("bin/gfx_tools_rd.dll");

	void Output::validatePath(const std::filesystem::path &path) {
		bool valid = setPath(path);

		if (valid) {
			consoleLog("Myst IV: Revelation was found installed at the following path:");
			consoleLog(path.string().c_str(), 2);

			if (!consoleBool("Is this the path to the install you would like to modify?", true)) {
				valid = false;
			}
		} else {
			consoleLog("No install of Myst IV: Relevation was found.");
		}

		if (!valid) {
			while (!setPath(consoleString("Please enter the path to Myst IV: Relevation."))) {
				consoleLog("No install of Myst IV: Relevation was found at this path.");
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
		return std::filesystem::exists(DATA_PATH) && std::filesystem::exists(GFX_TOOLS_PATH);
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

			// here I use std::filesystem::rename because I do want to overwrite the file if it exists
			//try {
			std::filesystem::rename(Output::FILE_NAME, fileName);
			//} catch (std::filesystem::filesystem_error) {
			// TODO: game is running, or not admin
			//}
			return result;
		}

		bool createFromOutput(const char* fileName) {
			return !rename(Output::FILE_NAME, getPath(fileName).string().c_str());
		}

		void restore(const std::filesystem::path &path) {
			std::filesystem::rename(getPath(path), path);
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
			std::fstream backupFileStream(Backup::getPath(Output::DATA_PATH), std::ios::binary | std::ios::in, _SH_DENYWR);

			if (!backupFileStream.is_open()) {
				// always delete the temporary file when done
				SCOPE_EXIT {
					std::filesystem::remove(Output::FILE_NAME);
				};

				{
					Output output = {};

					fileStream.seekg(0);
					copyStream(fileStream, output.fileStream);
				}

				backup = Backup::createFromOutput(Output::DATA_PATH.string().c_str());
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

	Edit::Edit(std::fstream &fileStream)
		: fileStream(fileStream) {
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