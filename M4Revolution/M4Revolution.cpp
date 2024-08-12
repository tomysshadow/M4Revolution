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
		// when it unlocks, the output thread will wake up to write the data
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

void M4Revolution::convertZAP(Work::Tasks &tasks, Ubi::BigFile::File &file, std::streampos ownerBigFileInputPosition) {
	#ifdef MULTITHREADED
	while (dataVectorIndex >= dataVector.size()) {
		dataVector.emplace_back();
	}

	Work::Data &data = dataVector[dataVectorIndex++];
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

	// when this unlocks one line later, the output thread will begin waiting on data
	Work::FileTask &fileTask = tasks.fileLock().get().emplace(ownerBigFileInputPosition, &file);

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

	// this will wake up the output thread to tell it we have no more data to add, and to move on to the next FileTask
	fileTask.complete();
}

void M4Revolution::fixLoading(Work::Tasks &tasks, Ubi::BigFile::File &file, std::streampos ownerBigFileInputPosition, Log &log) {
	Ubi::BigFile::File::SIZE fileSize = file.size;

	// filePointerSetMap is a map where the keys are the file positions beginning to end, and values are sets of files at that position
	Ubi::BigFile::File::POINTER_SET_MAP filePointerSetMap = {};
	std::streampos bigFileInputPosition = inputFileStream.tellg();
	tasks.bigFileLock().get().insert({bigFileInputPosition, Work::BigFileTask(inputFileStream, ownerBigFileInputPosition, file, filePointerSetMap)});

	// inputCopyPosition is the position of the files to copy
	// inputFilePosition is the position of a specific input file (for file.size calculation)
	Ubi::BigFile::File::SIZE inputCopyPosition = (Ubi::BigFile::File::SIZE)(inputFileStream.tellg() - bigFileInputPosition);
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
					inputFileStream.seekg((std::streampos)inputCopyPosition + bigFileInputPosition);
					tasks.fileLock().get().emplace(bigFileInputPosition, inputFileStream, countCopy, filePointerVectorPointer).complete();
					filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

					log.copied();
				}

				// we'll need to convert this file type
				convert = true;
			}

			// if we are converting this or any previous file in the set
			if (convert) {
				// handle this file specifically
				inputFileStream.seekg((std::streampos)file.position + bigFileInputPosition);

				// these conversion functions update the file sizes passed in
				switch (file.type) {
					case Ubi::BigFile::File::TYPE::RECURSIVE:
					fixLoading(tasks, file, bigFileInputPosition, log);
					break;
					case Ubi::BigFile::File::TYPE::ZAP:
					convertZAP(tasks, file, bigFileInputPosition);
					break;
					default:
					// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
					tasks.fileLock().get().emplace(bigFileInputPosition, inputFileStream, &file).complete();
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
			inputFileStream.seekg((std::streampos)inputCopyPosition + bigFileInputPosition);
			tasks.fileLock().get().emplace(bigFileInputPosition, inputFileStream, countCopy, filePointerVectorPointer).complete();
			filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

			log.copied();
		}
	}
}

// TODO: this function is very spaghetti and should ideally be split into smaller functions
void M4Revolution::outputThread(const char* outputFileName, Work::Tasks &tasks, bool &yield) {
	std::ofstream outputFileStream(outputFileName, std::ios::binary);

	// ownerBigFileInputPositionOptional is an optional for the case of the outermost BigFile
	std::streampos bigFileInputPosition = 0;
	std::optional<std::streampos> ownerBigFileInputPositionOptional = std::nullopt;

	Ubi::BigFile::File::SIZE outputFilePosition = 0;
	Ubi::BigFile::File::POINTER_SET_MAP::size_type filesWritten = 0;

	// must be set to the end of the map initially
	Work::BigFileTask::MAP::iterator bigFileTaskMapFileIterator = {};

	{
		Work::BigFileTask::MAP_LOCK bigFileLock = tasks.bigFileLock();
		Work::BigFileTask::MAP &bigFileTaskMap = bigFileLock.get();

		bigFileTaskMapFileIterator = bigFileTaskMap.end();
	}

	Work::BigFileTask::MAP::iterator ownerBigFileTaskMapFileIterator = {};
	std::streampos outputPosition = 0;

	Work::FileTask::FILE_VARIANT fileVariant = {};
	Ubi::BigFile::File::POINTER_VECTOR_POINTER filePointerVectorPointer = 0;

	for (;;) {
		Work::FileTask::QUEUE_LOCK queueLock = tasks.fileLock(yield);
		Work::FileTask::QUEUE &fileTaskQueue = queueLock.get();

		if (fileTaskQueue.empty()) {
			continue;
		}

		Work::FileTask &fileTask = fileTaskQueue.front();
		bigFileInputPosition = fileTask.getOwnerBigFileInputPosition();

		if (bigFileInputPosition != ownerBigFileInputPositionOptional) {
			Work::BigFileTask::MAP_LOCK bigFileLock = tasks.bigFileLock();
			Work::BigFileTask::MAP &bigFileTaskMap = bigFileLock.get();

			if (bigFileTaskMap.empty()) {
				return;
			}

			if (!ownerBigFileInputPositionOptional.has_value() || bigFileInputPosition > ownerBigFileInputPositionOptional) {
				if (bigFileTaskMapFileIterator != bigFileTaskMap.end()) {
					Work::BigFileTask &bigFileTask = bigFileTaskMapFileIterator->second;

					if (filesWritten < bigFileTask.getFiles()) {
						bigFileTask.filesWritten = filesWritten;
					} else {
						while (bigFileTaskMapFileIterator != bigFileTaskMap.begin()) {
							Work::BigFileTask &bigFileTask = bigFileTaskMapFileIterator->second;

							outputPosition = outputFileStream.tellp();

							outputFileStream.seekp(bigFileTask.outputPosition);
							bigFileTask.getBigFilePointer()->write(outputFileStream);

							outputFileStream.seekp(outputPosition);

							ownerBigFileInputPositionOptional = bigFileTask.getOwnerBigFileInputPosition();
							ownerBigFileTaskMapFileIterator = bigFileTaskMap.find(ownerBigFileInputPositionOptional.value());

							if (ownerBigFileTaskMapFileIterator == bigFileTaskMap.end() || ownerBigFileTaskMapFileIterator == bigFileTaskMapFileIterator) {
								return;
							}

							Work::BigFileTask &ownerBigFileTask = ownerBigFileTaskMapFileIterator->second;

							Ubi::BigFile::File &file = bigFileTask.getFile();
							file.size = outputFilePosition;
							file.position = (Ubi::BigFile::File::SIZE)(bigFileTask.outputPosition - ownerBigFileTask.outputPosition);
							outputFilePosition = file.position + file.size;
							ownerBigFileTask.filesWritten++;

							bigFileTaskMapFileIterator = bigFileTaskMap.erase(bigFileTaskMapFileIterator);

							if ((--bigFileTaskMapFileIterator)->first == ownerBigFileInputPositionOptional) {
								break;
							}
						}
					}
				}

				bigFileTaskMapFileIterator = bigFileTaskMap.find(bigFileInputPosition);

				if (bigFileTaskMapFileIterator == bigFileTaskMap.end()) {
					return;
				}

				Work::BigFileTask &bigFileTask = bigFileTaskMapFileIterator->second;
				filesWritten = bigFileTask.filesWritten;

				if (!filesWritten) {
					bigFileTask.outputPosition = outputFileStream.tellp();

					outputFilePosition = bigFileTask.getFileSystemSize();
					outputFileStream.seekp(outputFilePosition, std::ios::cur);
				}
			}

			ownerBigFileInputPositionOptional = bigFileInputPosition;
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
			fileVariant = fileTask.getFileVariant();

			if (std::holds_alternative<Ubi::BigFile::File::POINTER_VECTOR_POINTER>(fileVariant)) {
				filePointerVectorPointer = std::get<Ubi::BigFile::File::POINTER_VECTOR_POINTER>(fileVariant);

				for (
					Ubi::BigFile::File::POINTER_VECTOR::iterator filePointerVectorIterator = filePointerVectorPointer->begin();
					filePointerVectorIterator != filePointerVectorPointer->end();
					filePointerVectorIterator++
				) {
					Ubi::BigFile::File &file = **filePointerVectorIterator;
					outputFilePosition += file.padding;
					file.position = outputFilePosition;
				}

				filesWritten += filePointerVectorPointer->size();
			} else {
				Ubi::BigFile::File &file = *std::get<Ubi::BigFile::File*>(fileVariant);
				file.position = outputFilePosition;
				outputFilePosition += file.size;
				filesWritten++;
			}

			fileTaskQueue.pop();
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

	bool yield = true;
	std::thread outputThread(M4Revolution::outputThread, outputFileName, std::ref(tasks), std::ref(yield));

	Log log("Fixing Loading, this may take several minutes", inputFileStream, inputFile.size, logFileNames);
	fixLoading(tasks, inputFile, 0, log);

	yield = false;
	tasks.fileLock();
	outputThread.join();
}