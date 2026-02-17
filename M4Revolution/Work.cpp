#include "pch.h"
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

		static const size_t BUFFER_SIZE = 0x10000;

		std::streamsize countRead = BUFFER_SIZE;
		std::streamsize gcountRead = 0;

		do {
			countRead = (std::streamsize)__min((size_t)count, (size_t)countRead);

			{
				Data::POINTER pointer = makeSharedArray<unsigned char>((size_t)countRead);

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

	const Output::INFO_MAP Output::FILE_PATH_INFO_MAP = {
		{FILE_PATH_DATA, {GAMEDATABINDIR "/data.m4b", true}},
		{FILE_PATH_USER_PREFERENCE, {EXEDIR "/user.dsc", false}},
		{FILE_PATH_M4_THOR, {EXEDIR "/m4_thor_rd.dll", true}},
		{FILE_PATH_M4_AI_GLOBAL, {EXEDIR "/m4_ai_global_rd.dll", true}},
		{FILE_PATH_GFX_TOOLS, {EXEDIR "/gfx_tools_rd.dll", true}}
	};

	const std::filesystem::path Output::DATA_PATH = FILE_PATH_INFO_MAP.at(FILE_PATH_DATA).path;
	const std::filesystem::path Output::USER_PREFERENCE_PATH = FILE_PATH_INFO_MAP.at(FILE_PATH_USER_PREFERENCE).path;
	const std::filesystem::path Output::M4_THOR_PATH = FILE_PATH_INFO_MAP.at(FILE_PATH_M4_THOR).path;
	const std::filesystem::path Output::M4_AI_GLOBAL_PATH = FILE_PATH_INFO_MAP.at(FILE_PATH_M4_AI_GLOBAL).path;
	const std::filesystem::path Output::GFX_TOOLS_PATH = FILE_PATH_INFO_MAP.at(FILE_PATH_GFX_TOOLS).path;

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
		try {
			std::filesystem::current_path(path);
		} catch (const std::filesystem::filesystem_error&) {
			return false;
		}

		for (INFO_MAP::const_iterator infoMapIterator = FILE_PATH_INFO_MAP.begin(); infoMapIterator != FILE_PATH_INFO_MAP.end(); infoMapIterator++) {
			const Info &INFO = infoMapIterator->second;

			// this check is just to prevent the user from being dumb
			// we need proper checks upon actually opening these as well of course
			if (INFO.required && !std::filesystem::is_regular_file(INFO.path)) {
				return false;
			}
		}
		return true;
	}

	Output::Output(bool binary) {
		// without this remove first it may crash trying to open a hidden file
		// (I mean, this isn't atomic so that can happen anyway but at least it's not our fault then)
		// this is just a temp file so deleting it should be fine
		std::filesystem::remove(FILE_NAME);

		fileStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		fileStream.open(FILE_NAME, std::ofstream::trunc | (std::ofstream::binary * binary), _SH_DENYRW);

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
		bool rename(const char* oldFileName, const char* newFileName) {
			bool result = false;

			for (;;) {
				result = !::rename(oldFileName, newFileName);

				// the error is EEXIST if the file exists and is not empty, but ENOENT if it exists and is empty
				if (result || errno == EEXIST || errno == ENOENT) {
					break;
				}

				RETRY_ERR(Output::FILE_RETRY);
			}
			return result;
		}

		void log() {
			consoleLog("A backup has been created.", 2);
		}

		void create(const char* fileName) {
			bool createdNew = rename(fileName, getPath(fileName).string().c_str());

			// here I use std::filesystem::rename because I do want to overwrite the file if it exists
			OPERATION_EXCEPTION_RETRY_ERR(std::filesystem::rename(Output::FILE_NAME, fileName), std::filesystem::filesystem_error, Output::FILE_RETRY);
			
			if (createdNew) {
				log();
			}
		}

		void createOutput(const char* fileName) {
			if (rename(Output::FILE_NAME, getPath(fileName).string().c_str())) {
				log();
			}
		}

		void createEmpty(const std::filesystem::path &path) {
			const std::filesystem::path PATH = getPath(path);

			// we only do this check because we want to know
			// if the file exists before already (just to log if we created it or not)
			if (std::filesystem::is_regular_file(PATH)) {
				return;
			}

			std::ofstream outputFileStream;
			outputFileStream.exceptions(std::ofstream::failbit);

			OPERATION_EXCEPTION_RETRY_ERR(outputFileStream.open(PATH), std::ofstream::failure, Output::FILE_RETRY);

			log();
		}

		void restore(const std::filesystem::path &path) {
			OPERATION_EXCEPTION_RETRY_ERR(std::filesystem::rename(getPath(path), path), std::filesystem::filesystem_error, Output::FILE_RETRY);
		}

		std::filesystem::path getPath(std::filesystem::path path) {
			return path.replace_extension("bak");
		}
	}

	void Edit::copyThread(Edit &edit) {
		std::fstream &fileStream = edit.fileStream;
		std::optional<Output> outputOptional = std::nullopt;

		if (!edit.copied) {
			// check if the file exists, if it doesn't create a backup
			// note this is not a TOCTOU bug, fileStream already has an exclusive lock
			// on this file, unless it is inaccessible
			// in which case we'll error out on copyStream (as we should)
			if (!std::filesystem::is_regular_file(Backup::getPath(edit.path))) {
				outputOptional.emplace();

				fileStream.seekg(0);
				copyStream(fileStream, outputOptional.value().fileStream);
			}

			edit.copied = true;
		}

		edit.event.wait(true);

		// this happens after the wait because createOutput logs stuff
		// we don't want it to interfere with the logging on the main thread
		if (outputOptional.has_value()) {
			outputOptional = std::nullopt;

			Backup::createOutput(edit.path.string().c_str());
		}

		CODE_VECTOR &codeVector = edit.codeVector;

		for (CODE_VECTOR::iterator codeVectorIterator = codeVector.begin(); codeVectorIterator != codeVector.end(); codeVectorIterator++) {
			fileStream.seekp(codeVectorIterator->position);

			std::string &str = codeVectorIterator->str;
			writeStream(fileStream, str.c_str(), (std::streamsize)str.length());
		}
	}

	Edit::Edit(std::fstream &fileStream, const std::filesystem::path &path)
		: fileStream(fileStream),
		path(path) {
		fileStream.clear();
		fileStream.exceptions(std::fstream::failbit | std::fstream::badbit);

		if (fileStream.is_open()) {
			fileStream.close();
		}

		fileStream.open(path, std::fstream::binary | std::fstream::in | std::fstream::out, _SH_DENYRW);
	}

	void Edit::apply(std::thread &copyThread, const CODE_VECTOR &codeVector) {
		this->codeVector = codeVector;

		if (!copied) {
			consoleLog("Please wait...", 2);
		}

		event.set();

		copyThread.join();
	}
}