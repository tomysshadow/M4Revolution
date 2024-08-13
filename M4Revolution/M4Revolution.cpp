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

void M4Revolution::Log::copying() {
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
		std::cout << "Copying " << copiedFiles << " file(s)" << std::endl;
	}

	copiedFiles = files;
}

void M4Revolution::Log::converting(const Ubi::BigFile::File &file) {
	if (!fileNames) {
		return;
	}

	// newline used here instead of endl for performance (buffer is flushed on copies instead)
	if (file.nameOptional.has_value()) {
		std::cout << "Converting \"" << file.nameOptional.value() << "\"\n";
	} else {
		std::cout << "Converting file at position " << file.position << ", size " << file.size << "\n";
	}
}

void M4Revolution::Log::finishing() {
	std::cout << "Finishing, please wait..." << std::endl;
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

void M4Revolution::convertZAP(Work::Tasks &tasks, std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file) {
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
		
		const size_t COLOR32_SIZE = sizeof(COLOR32);

		COLOR32* color32Pointer = (COLOR32*)out;

		// need to flip the endianness, NVTT expects BGR instead of RGB
		color32X(color32Pointer, outWidth * COLOR32_SIZE, outSize);

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

void M4Revolution::fixLoading(Work::Tasks &tasks, std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file, Log &log) {
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
	// filePointerVectorPointer is to communicate file sizes/positions to the output thread
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
				file.padding = filePointerSetMapIterator->first - inputFilePosition;

				// only do this if there are files to copy and we have not yet converted any files in the set
				if (countCopy) {
					// copy the files before this one, if any
					inputFileStream.seekg((std::streampos)inputCopyPosition + bigFileInputPosition);
					tasks.fileLock().get().emplace(bigFileInputPosition, inputFileStream, countCopy, filePointerVectorPointer).complete();
					filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

					log.copying();
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
					fixLoading(tasks, bigFileInputPosition, file, log);
					break;
					case Ubi::BigFile::File::TYPE::ZAP:
					convertZAP(tasks, bigFileInputPosition, file);
					break;
					default:
					// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
					tasks.fileLock().get().emplace(bigFileInputPosition, inputFileStream, &file).complete();
				}

				log.converting(file);
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
		countCopy = file.size - inputCopyPosition;

		// copy any remaining files
		if (countCopy) {
			inputFileStream.seekg((std::streampos)inputCopyPosition + bigFileInputPosition);
			tasks.fileLock().get().emplace(bigFileInputPosition, inputFileStream, countCopy, filePointerVectorPointer).complete();
			filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

			log.copying();
		}
	}
}

void M4Revolution::color32X(COLOR32* color32Pointer, size_t stride, size_t size) {
	const COLOR32 R = 0x000000FF;
	const COLOR32 G = 0x0000FF00;
	const COLOR32 B = 0x00FF0000;
	const COLOR32 A = 0xFF000000;
	const int SWAP = 16;

	COLOR32* endPointer = (COLOR32*)((uint8_t*)color32Pointer + size) - 1;
	COLOR32* rowPointer = 0;

	COLOR32 color32 = 0;

	while (color32Pointer <= endPointer) {
		rowPointer = (COLOR32*)((uint8_t*)color32Pointer + stride) - 1;

		while (color32Pointer <= rowPointer) {
			color32 = *color32Pointer;

			*color32Pointer = (color32 & (G | A))
			| (color32 >> SWAP & R)
			| (color32 << SWAP & B);

			color32Pointer = color32Pointer + 1;
		}

		color32Pointer = rowPointer + 1;
	}
}

// TODO: this function is very spaghetti and should ideally be split into smaller functions
void M4Revolution::outputThread(const char* outputFileName, Work::Tasks &tasks, bool &yield) {
	std::ofstream outputFileStream(outputFileName, std::ios::binary);

	std::streampos bigFileInputPosition = -1;
	std::streampos currentBigFileInputPosition = -1;
	std::streampos eraseBigFileInputPosition = -1;

	Work::BigFileTask* bigFileTaskPointer = 0;
	std::streampos currentOutputPosition = -1;
	std::streampos eraseOutputPosition = -1;

	Ubi::BigFile::File::SIZE outputFilePosition = 0;
	Ubi::BigFile::File::POINTER_VECTOR::size_type filesWritten = 0;

	Work::FileTask::FILE_VARIANT fileVariant = {};
	Ubi::BigFile::File::POINTER_VECTOR_POINTER filePointerVectorPointer = 0;

	for (;;) {
		Work::FileTask::QUEUE_LOCK queueLock = tasks.fileLock(yield);
		Work::FileTask::QUEUE &fileTaskQueue = queueLock.get();

		while (!fileTaskQueue.empty()) {
			Work::FileTask &fileTask = fileTaskQueue.front();
			bigFileInputPosition = fileTask.getOwnerBigFileInputPosition();

			if (bigFileInputPosition != currentBigFileInputPosition) {
				// if we passed this BigFile and have moved onto another one (might be owned, might be immediately after...)
				Work::BigFileTask::MAP_LOCK bigFileLock = tasks.bigFileLock();
				Work::BigFileTask::MAP &bigFileTaskMap = bigFileLock.get();

				if (bigFileTaskPointer) {
					Work::BigFileTask &bigFileTask = *bigFileTaskPointer;

					if (filesWritten < bigFileTask.getFiles()) {
						bigFileTask.filesWritten = filesWritten;
					} else {
						do {
							Work::BigFileTask &eraseBigFileTask = *bigFileTaskPointer;

							currentOutputPosition = outputFileStream.tellp();
							eraseOutputPosition = eraseBigFileTask.outputPosition;

							outputFileStream.seekp(eraseOutputPosition);
							eraseBigFileTask.getBigFilePointer()->write(outputFileStream);

							outputFileStream.seekp(currentOutputPosition);

							Ubi::BigFile::File &file = eraseBigFileTask.getFile();

							eraseBigFileInputPosition = currentBigFileInputPosition;
							currentBigFileInputPosition = eraseBigFileTask.getOwnerBigFileInputPosition();

							bigFileTaskMap.erase(eraseBigFileInputPosition);

							if (eraseBigFileInputPosition == currentBigFileInputPosition) {
								return;
							}

							Work::BigFileTask &ownerBigFileTask = bigFileTaskMap.at(currentBigFileInputPosition);
							bigFileTaskPointer = &ownerBigFileTask;

							file.size = (Ubi::BigFile::File::SIZE)(currentOutputPosition - eraseOutputPosition);
							file.position = (Ubi::BigFile::File::SIZE)(eraseOutputPosition - ownerBigFileTask.outputPosition);
							outputFilePosition = file.size + file.position;
							ownerBigFileTask.filesWritten++;
							// we now need to check the owner in case we are the last file in the owner and now all its files are written
						} while (bigFileTaskPointer->filesWritten >= bigFileTaskPointer->getFiles());
					}
				}

				currentBigFileInputPosition = bigFileInputPosition;
				Work::BigFileTask &currentBigFileTask = bigFileTaskMap.at(currentBigFileInputPosition);
				bigFileTaskPointer = &currentBigFileTask;

				filesWritten = currentBigFileTask.filesWritten;

				if (!filesWritten) {
					currentBigFileTask.outputPosition = outputFileStream.tellp();

					outputFilePosition = currentBigFileTask.getFileSystemSize();
					outputFileStream.seekp(outputFilePosition, std::ios::cur);
				}
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

			if (!fileTask.getCompleted()) {
				break;
			}

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
				outputFilePosition += file.padding;
				file.position = outputFilePosition;
				outputFilePosition += file.size;
				filesWritten++;
			}

			fileTaskQueue.pop();

			if (!yield && fileTaskQueue.empty()) {
				throw std::logic_error("fileTaskQueue must not be empty if yield is false");
			}
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
	fixLoading(tasks, 0, inputFile, log);
	log.finishing();

	tasks.fileLock().get().emplace(-1, &inputFile).complete();
	yield = false;
	outputThread.join();
}