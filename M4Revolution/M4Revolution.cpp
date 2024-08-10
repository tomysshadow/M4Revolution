#include "M4Revolution.h"
#include <iostream>

#define M4REVOLUTION_OUT true, 1
#define M4REVOLUTION_ERR true, 1, true, __FILE__, __LINE__

M4Revolution::Log::Log(const char* title, std::ifstream &inputFileStream, Ubi::BigFile::File::SIZE inputFileSize, bool fileNames)
	: inputFileStream(inputFileStream),
	inputFileSize(inputFileSize),
	fileNames(fileNames) {
	std::cout << title << "..." << std::endl;
}

void M4Revolution::Log::step() {
	files++;
}

void M4Revolution::Log::copied() {
	if (!fileNames) {
		// use the position to log the current progress
		// this works because we read the input file stream from beginning to end in order
		int currentProgress = (int)(((double)inputFileStream.tellg() / inputFileSize) * 100.0);

		while (progress < currentProgress) {
			std::cout << ++progress << "% complete" << std::endl;
		}
		return;
	}

	// log the number of files copied without any conversion performed
	copiedFiles = files - copiedFiles;

	if (copiedFiles) {
		std::cout << "Copied " << copiedFiles << " file(s)" << std::endl;
	}

	copiedFiles = files;
}

void M4Revolution::Log::converted(const Ubi::BigFile::File &file) {
	if (!fileNames) {
		return;
	}

	// newline used here instead of endl for performance (buffer is flushed on copies instead)
	if (file.nameOptional.has_value()) {
		std::cout << "Converted \"" << file.nameOptional.value() << "\"\n";
	} else {
		std::cout << "Converted file at position " << file.position << ", size " << file.size << "\n";
	}
}

M4Revolution::OutputHandler::OutputHandler(Work::FileTask &fileTask) : fileTask(fileTask) {
}

void M4Revolution::OutputHandler::beginImage(int size, int width, int height, int depth, int face, int miplevel) {
	// never called
}

void M4Revolution::OutputHandler::endImage() {
	// never called
}

bool M4Revolution::OutputHandler::writeData(const void* data, int size) {
	if (!data) {
		return false;
	}

	try {
		Work::Data::POINTER pointer = Work::Data::POINTER(new unsigned char[size]);

		if (memcpy_s(pointer.get(), size, data, size)) {
			return false;
		}

		// this locks the FileTask for a single line
		// when it unlocks, the writer thread will wake up to write the data
		// then it will wait on more data again
		fileTask.lock().get().emplace(size, pointer);
	} catch (...) {
		return false;
	}

	this->size += size;
	return true;
}

void M4Revolution::ErrorHandler::error(nvtt::Error error) {
	consoleLog(nvtt::errorString(error), M4REVOLUTION_ERR);
	result = false;
}

const Ubi::BigFile::Path::VECTOR M4Revolution::AI_TRANSITION_FADE_PATH_VECTOR = {
		{{"common"}, "common.m4b"},
		{{"ai", "aitransitionfade"}, "ai_transition_fade.ai"}
};

void M4Revolution::convertZAP(Work::Tasks &tasks, Ubi::BigFile::File &file, std::streampos inputPosition) {
	#ifdef MULTITHREADED
	while (dataVectorIndex >= dataVector.size()) {
		dataVector.emplace_back();
	}

	Work::Data &data = dataVector[dataVectorIndex];
	#endif

	if (file.size > data.size || !data.pointer) {
		data.size = file.size;
		data.pointer = Work::Data::POINTER(new unsigned char[data.size]);
	}

	// note: not data.size here, that would be bad
	readFileStreamSafe(inputFileStream, data.pointer.get(), file.size);

	{
		zap_byte_t* out = 0;
		zap_size_t outSize = 0;
		zap_int_t outWidth = 0;
		zap_int_t outHeight = 0;

		if (zap_load_memory(data.pointer.get(), ZAP_COLOR_FORMAT_RGBA32, &out, &outSize, &outWidth, &outHeight) != ZAP_ERROR_NONE) {
			throw std::runtime_error("Failed to Load ZAP From Memory");
		}

		SCOPE_EXIT {
			if (!freeZAP(out)) {
				throw std::runtime_error("Failed to Free ZAP");
			}
		};

		if (!surface.setImage(nvtt::InputFormat::InputFormat_BGRA_8UB, outWidth, outHeight, 1, out)) {
			throw std::runtime_error("Failed to Set Image");
		}
	}

	#ifdef MULTITHREADED
	dataVectorIndex--;
	#endif

	// when this unlocks one line later, the writer thread will begin waiting on data
	Work::FileTask &fileTask = tasks.fileLock(false).get().emplace(inputPosition);

	OutputHandler outputHandler(fileTask);
	outputOptions.setOutputHandler(&outputHandler);

	ErrorHandler errorHandler;
	outputOptions.setErrorHandler(&errorHandler);

	if (!context.outputHeader(surface, 1, compressionOptions, outputOptions)) {
		throw std::runtime_error("Failed to Process Context");
	}

	if (!context.compress(surface, 0, 0, compressionOptions, outputOptions) || !errorHandler.result) {
		throw std::runtime_error("Failed to Process Context");
	}

	file.size = outputHandler.size;

	// this will wake up the writer thread to tell it we have no more data to add, and to move on to the next FileTask
	fileTask.complete();
}

/*
void M4Revolution::fixLoading(Work::Tasks &tasks, Ubi::BigFile::File* filePointer, Log &log) {
	Ubi::BigFile::File::SIZE fileSize = filePointer->size;

	// filePointerSetMap is a map where the keys are the file positions beginning to end, and values are sets of files at that position
	Ubi::BigFile::File::POINTER_SET_MAP filePointerSetMap = {};
	std::streampos inputPosition = tasks.bigFileLock(false).get().emplace_back(inputFileStream, filePointer, filePointerSetMap).INPUT_POSITION;

	// inputCopyPosition is the position of the files to copy
	// inputFilePosition is the position of a specific input file (for file.size calculation)
	Ubi::BigFile::File::SIZE inputCopyPosition = (Ubi::BigFile::File::SIZE)(inputFileStream.tellg() - inputPosition);
	Ubi::BigFile::File::SIZE inputFilePosition = inputCopyPosition;

	// convert keeps track of if we just converted any files within the inner, set loop
	// (in which case, inputCopyPosition is advanced)
	// countCopy is the count of the bytes to copy when copying files
	bool convert = false;
	std::streampos countCopy = 0;
	Ubi::BigFile::File::POINTER_VECTOR filePointerVector = {};

	for (
		Ubi::BigFile::File::POINTER_SET_MAP::iterator filePointerSetMapIterator = filePointerSetMap.begin();
		filePointerSetMapIterator != filePointerSetMap.end();
		filePointerSetMapIterator++
	) {
		if (convert) {
			inputCopyPosition = filePointerSetMapIterator->first;
			inputFilePosition = inputCopyPosition;
			convert = false;
		}

		// we need an inner loop with a set here in case there are identical files with different paths, at the same position
		Ubi::BigFile::File::POINTER_SET &filePointerSet = filePointerSetMapIterator->second;

		for (
			Ubi::BigFile::File::POINTER_SET::iterator filePointerSetIterator = filePointerSet.begin();
			filePointerSetIterator != filePointerSet.end();
			filePointerSetIterator++
		) {
			Ubi::BigFile::File &file = **filePointerSetIterator;

			// if we encounter a file we need to convert for the first time, then first copy the files before it
			if (!convert && file.type != Ubi::BigFile::File::TYPE::NONE) {
				countCopy = filePointerSetMapIterator->first - inputCopyPosition;

				// only do this if there are files to copy and we have not yet converted any files in the set
				if (countCopy) {
					// copy the files before this one, if any
					// filePointerVector is moved, clear returns it to a valid state
					inputFileStream.seekg((std::streampos)inputCopyPosition + inputPosition);
					tasks.fileLock(false).get().emplace(inputPosition, inputFileStream, countCopy, filePointerVector).complete();
					filePointerVector.clear();

					log.copied();
				}

				// we'll need to convert this file type
				convert = true;
			}

			// if we are converting this or any previous file in the set
			if (convert) {
				// handle this file specifically
				inputFileStream.seekg((std::streampos)file.position + inputPosition);

				// these conversion functions update the file sizes passed in
				switch (file.type) {
					case Ubi::BigFile::File::TYPE::RECURSIVE:
					fixLoading(tasks, *filePointerSetIterator, log);
					break;
					case Ubi::BigFile::File::TYPE::ZAP:
					convertZAP(tasks, *filePointerSetIterator, inputPosition);
					break;
					default:
					// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
					tasks.fileLock(false).get().emplace(inputPosition, inputFileStream, file.size).complete();
				}

				log.converted(file);
			} else {
				// other identical, copied files at the same position in the input should likewise be at the same position in the output
				file.padding = filePointerSetMapIterator->first - inputFilePosition;
				inputFilePosition = filePointerSetMapIterator->first;
				filePointerVector.push_back(*filePointerSetIterator);

				log.step();
			}
		}
	}

	// if we just converted a set of files then there are no remaining files to copy, but otherwise...
	if (!convert) {
		countCopy = fileSize - inputCopyPosition;

		// copy any remaining files
		if (countCopy) {
			inputFileStream.seekg((std::streampos)inputCopyPosition + inputPosition);
			tasks.fileLock(false).get().emplace(inputPosition, inputFileStream, countCopy, filePointerVector).complete();
			filePointerVector.clear();

			log.copied();
		}
	}

	// TODO: this will need to happen elsewhere
	// give the caller the new file size
	//size = outputFilePosition;
}
*/

void M4Revolution::fixLoading(Work::Tasks &tasks, Ubi::BigFile::File &file, Log &log) {
	Ubi::BigFile::File::SIZE fileSize = file.size;

	// filePointerSetMap is a map where the keys are the file positions beginning to end, and values are sets of files at that position
	Ubi::BigFile::File::POINTER_SET_MAP filePointerSetMap = {};
	std::streampos inputPosition = inputFileStream.tellg();
	tasks.bigFileLock().get().insert({inputPosition, Work::BigFileTask(inputFileStream, file, filePointerSetMap)});

	// inputCopyPosition is the position of the files to copy
	// inputFilePosition is the position of a specific input file (for file.size calculation)
	Ubi::BigFile::File::SIZE inputCopyPosition = (Ubi::BigFile::File::SIZE)(inputFileStream.tellg() - inputPosition);
	Ubi::BigFile::File::SIZE inputFilePosition = inputCopyPosition;

	// convert keeps track of if we just converted any files within the inner, set loop
	// (in which case, inputCopyPosition is advanced)
	// countCopy is the count of the bytes to copy when copying files
	bool convert = false;
	std::streampos countCopy = 0;
	Ubi::BigFile::File::POINTER_VECTOR_POINTER filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

	for (
		Ubi::BigFile::File::POINTER_SET_MAP::iterator filePointerSetMapIterator = filePointerSetMap.begin();
		filePointerSetMapIterator != filePointerSetMap.end();
		filePointerSetMapIterator++
		) {
		if (convert) {
			inputCopyPosition = filePointerSetMapIterator->first;
			inputFilePosition = inputCopyPosition;
			convert = false;
		}

		// we need an inner loop with a set here in case there are identical files with different paths, at the same position
		Ubi::BigFile::File::POINTER_SET &filePointerSet = filePointerSetMapIterator->second;

		for (
			Ubi::BigFile::File::POINTER_SET::iterator filePointerSetIterator = filePointerSet.begin();
			filePointerSetIterator != filePointerSet.end();
			filePointerSetIterator++
			) {
			Ubi::BigFile::File &file = **filePointerSetIterator;

			// if we encounter a file we need to convert for the first time, then first copy the files before it
			if (!convert && file.type != Ubi::BigFile::File::TYPE::NONE) {
				countCopy = filePointerSetMapIterator->first - inputCopyPosition;

				// only do this if there are files to copy and we have not yet converted any files in the set
				if (countCopy) {
					// copy the files before this one, if any
					// filePointerVector is moved, clear returns it to a valid state
					inputFileStream.seekg((std::streampos)inputCopyPosition + inputPosition);
					tasks.fileLock().get().emplace(inputPosition, inputFileStream, countCopy, filePointerVectorPointer).complete();
					filePointerVectorPointer->clear();

					log.copied();
				}

				// we'll need to convert this file type
				convert = true;
			}

			// if we are converting this or any previous file in the set
			if (convert) {
				// handle this file specifically
				inputFileStream.seekg((std::streampos)file.position + inputPosition);

				// these conversion functions update the file sizes passed in
				switch (file.type) {
					case Ubi::BigFile::File::TYPE::RECURSIVE:
					fixLoading(tasks, file, log);
					break;
					case Ubi::BigFile::File::TYPE::ZAP:
					convertZAP(tasks, file, inputPosition);
					break;
					default:
					// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
					tasks.fileLock().get().emplace(inputPosition, inputFileStream, file.size).complete();
				}

				log.converted(file);
			} else {
				// other identical, copied files at the same position in the input should likewise be at the same position in the output
				file.padding = filePointerSetMapIterator->first - inputFilePosition;
				inputFilePosition = filePointerSetMapIterator->first;
				filePointerVectorPointer->push_back(&file);

				log.step();
			}
		}
	}

	// if we just converted a set of files then there are no remaining files to copy, but otherwise...
	if (!convert) {
		countCopy = fileSize - inputCopyPosition;

		// copy any remaining files
		if (countCopy) {
			inputFileStream.seekg((std::streampos)inputCopyPosition + inputPosition);
			tasks.fileLock().get().emplace(inputPosition, inputFileStream, countCopy, filePointerVectorPointer).complete();
			filePointerVectorPointer->clear();

			log.copied();
		}
	}

	// TODO: this will need to happen elsewhere
	// give the caller the new file size
	//size = outputFilePosition;
}

void M4Revolution::outputThread(const char* outputFileName, Work::Tasks &tasks) {
	std::ofstream outputFileStream(outputFileName, std::ios::binary);

	std::streampos bigFileInputPosition = 0;
	std::streampos fileInputPosition = -1;
	std::streampos outputPosition = 0;
	Work::FileTask::QUEUE_LOCK_POINTER queueLockPointer = 0;

	for (;;) {
		queueLockPointer = tasks.fileLockPointer(true);

		Work::FileTask::QUEUE &fileTaskQueue = queueLockPointer->get();

		if (fileTaskQueue.empty()) {
			queueLockPointer = 0;
			continue;
		}

		Work::FileTask &fileTask = fileTaskQueue.front();
		queueLockPointer = 0;

		bigFileInputPosition = fileTask.getBigFileInputPosition();

		if (bigFileInputPosition != fileInputPosition) {
			Work::BigFileTask::MAP_LOCK_POINTER bigFileLockPointer = tasks.bigFileLockPointer(true);

			Work::BigFileTask::MAP &bigFileTaskMap = bigFileLockPointer->get();

			if (bigFileTaskMap.empty()) {
				return;
			}

			Work::BigFileTask &bigFileTask = bigFileTaskMap.at(bigFileInputPosition);
			bigFileTask.outputPosition = outputFileStream.tellp();
			outputFileStream.seekp(bigFileTask.getFileSystemSize(), std::ios::cur);

			if (bigFileInputPosition < fileInputPosition) {
				outputPosition = outputFileStream.tellp();

				outputFileStream.seekp(bigFileTask.outputPosition);
				bigFileTask.getBigFilePointer()->write(outputFileStream);

				outputFileStream.seekp(outputPosition);

				bigFileTaskMap.erase(bigFileInputPosition);
			}

			fileInputPosition = bigFileInputPosition;
		}

		{
			Work::Lock<Work::Data::QUEUE> lock = fileTask.lock();
			Work::Data::QUEUE &dataQueue = lock.get();

			while (!dataQueue.empty()) {
				Work::Data &data = dataQueue.front();
				writeFileStreamSafe(outputFileStream, data.pointer.get(), data.size);
				dataQueue.pop();
			}
		}

		if (fileTask.getCompleted()) {
			tasks.fileLock(true).get().pop();
		}
	}
}

M4Revolution::M4Revolution(const char* inputFileName, bool logFileNames, bool disableHardwareAcceleration)
	: inputFileStream(inputFileName, std::ios::binary),
	logFileNames(logFileNames) {
	context.enableCudaAcceleration(!disableHardwareAcceleration);

	compressionOptions.setFormat(nvtt::Format_DXT5);
	compressionOptions.setQuality(nvtt::Quality_Highest);

	outputOptions.setContainer(nvtt::Container_DDS);
}

void M4Revolution::fixLoading(const char* outputFileName) {
	Work::Tasks tasks = {};

	inputFileStream.seekg(0, std::ios::end);
	Ubi::BigFile::File inputFile((Ubi::BigFile::File::SIZE)inputFileStream.tellg());
	inputFileStream.seekg(0, std::ios::beg);

	std::thread outputThread(M4Revolution::outputThread, outputFileName, std::ref(tasks));

	Log log("Fixing Loading, this may take several minutes", inputFileStream, inputFile.size, logFileNames);
	fixLoading(tasks, inputFile, log);

	outputThread.join();
}