#include "M4Revolution.h"
#include <iostream>

#define M4REVOLUTION_OUT true, 1
#define M4REVOLUTION_ERR true, 1, true, __FILE__, __LINE__

void M4Revolution::destroy() {
	#ifdef MULTITHREADED
	CloseThreadpool(pool);
	#endif
};

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
	filesCopying = files - filesCopying;

	if (filesCopying) {
		std::cout << "Copying " << filesCopying << " file(s)" << std::endl;
	}

	filesCopying = files;
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

M4Revolution::OutputHandler::OutputHandler(Work::ConvertFileTask &convertFileTask)
	: convertFileTask(convertFileTask) {
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
		convertFileTask.queue.emplace(size, pointer);

		this->size += size;
	} catch (...) {
		return false;
	}
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

void M4Revolution::convertZAP(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file) {
	Work::Convert* convertPointer = new Work::Convert(ownerBigFileInputPosition, file, tasks, context, compressionOptions);

	Work::Data::POINTER &pointer = convertPointer->pointer;
	pointer = Work::Data::POINTER(new unsigned char[file.size]);
	readFileStreamSafe(inputFileStream, pointer.get(), file.size);

	#ifdef MULTITHREADED
	PTP_WORK work = CreateThreadpoolWork(convertZAPProc, convertPointer, NULL);

	if (!work) {
		throw std::runtime_error("Failed to Create Threadpool Work");
	}

	SubmitThreadpoolWork(work);
	CloseThreadpoolWork(work);
	#endif
	#ifdef SINGLETHREADED
	convertZAPWorkCallback(convertPointer);
	#endif
}

void M4Revolution::copyFiles(
	Ubi::BigFile::File::SIZE inputPosition,
	Ubi::BigFile::File::SIZE inputCopyPosition,
	Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
	std::streampos bigFileInputPosition,
	Log &log
) {
	inputFileStream.seekg((std::streampos)inputCopyPosition + bigFileInputPosition);

	// note: this must get created even if filePointerVectorPointer is empty or the count to copy would be zero
	// so that the bigFileInputPosition is reliably seen by the output thread
	Work::Tasks::FILE_TASK_VARIANT &fileTaskVariant = tasks.fileLock().get().emplace(std::make_shared<Work::CopyFileTask>(bigFileInputPosition, filePointerVectorPointer));
	Work::CopyFileTask &copyFileTask = *std::get<Work::CopyFileTask::POINTER>(fileTaskVariant);
	copyFileTask.copy(inputFileStream, inputPosition - inputCopyPosition);
	copyFileTask.complete();

	filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

	log.copying();
}

void M4Revolution::convertFile(std::streampos bigFileInputPosition, Ubi::BigFile::File &file, Log &log) {
	inputFileStream.seekg((std::streampos)file.position + bigFileInputPosition);

	// these conversion functions update the file sizes passed in
	switch (file.type) {
		case Ubi::BigFile::File::TYPE::BIG_FILE:
		fixLoading(bigFileInputPosition, file, log);
		break;
		case Ubi::BigFile::File::TYPE::ZAP:
		convertZAP(bigFileInputPosition, file);
		break;
		default:
		// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
		Work::Tasks::FILE_TASK_VARIANT &fileTaskVariant = tasks.fileLock().get().emplace(std::make_shared<Work::CopyFileTask>(bigFileInputPosition, &file));
		Work::CopyFileTask &copyFileTask = *std::get<Work::CopyFileTask::POINTER>(fileTaskVariant);
		copyFileTask.copy(inputFileStream, file.size);
		copyFileTask.complete();
	}

	log.converting(file);
}

void M4Revolution::stepFile(
	Ubi::BigFile::File::SIZE inputPosition,
	Ubi::BigFile::File::SIZE &inputFilePosition,
	Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
	Ubi::BigFile::File &file,
	Log &log
) {
	inputFilePosition = inputPosition;
	filePointerVectorPointer->push_back(&file);

	log.step();
}

void M4Revolution::fixLoading(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file, Log &log) {
	// filePointerSetMap is a map where the keys are the file positions beginning to end, and values are sets of files at that position
	Ubi::BigFile::File::POINTER_SET_MAP filePointerSetMap = {};
	std::streampos bigFileInputPosition = inputFileStream.tellg();
	tasks.bigFileLock().get().try_emplace(bigFileInputPosition, inputFileStream, ownerBigFileInputPosition, file, filePointerSetMap);

	// inputCopyPosition is the position of the files to copy
	// inputFilePosition is the position of a specific input file (for file.size calculation)
	Ubi::BigFile::File::SIZE inputCopyPosition = (Ubi::BigFile::File::SIZE)(inputFileStream.tellg() - bigFileInputPosition);
	Ubi::BigFile::File::SIZE inputFilePosition = inputCopyPosition;

	// convert keeps track of if we just converted any files within the inner, set loop
	// (in which case, inputCopyPosition is advanced)
	// countCopy is the count of the bytes to copy when copying files
	// filePointerVectorPointer is to communicate file sizes/positions to the output thread
	bool convert = false;
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
				file.padding = filePointerSetMapIterator->first - inputFilePosition;

				// prevent copying if there are no files (this is safe in this scenario only)
				if (!filePointerVectorPointer->empty()) {
					copyFiles(filePointerSetMapIterator->first, inputCopyPosition, filePointerVectorPointer, bigFileInputPosition, log);
				}

				// we'll need to convert this file type
				convert = true;
			}

			// if we are converting this or any previous file in the set
			if (convert) {
				convertFile(bigFileInputPosition, file, log);
			} else {
				// other identical, copied files at the same position in the input should likewise be at the same position in the output
				file.padding = filePointerSetMapIterator->first - inputFilePosition;
				stepFile(filePointerSetMapIterator->first, inputFilePosition, filePointerVectorPointer, file, log);
			}
		}
	}

	// if we just converted a set of files then there are no remaining files to copy, but otherwise...
	if (!convert) {
		// always copy here even if filePointerVectorPointer is empty
		// (ensure every BigFile has at least one FileTask)
		copyFiles(file.size, inputCopyPosition, filePointerVectorPointer, bigFileInputPosition, log);
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

void M4Revolution::convertZAPWorkCallback(Work::Convert* convertPointer) {
	SCOPE_EXIT {
		delete convertPointer;
	};

	nvtt::Surface surface = {};

	{
		zap_byte_t* out = 0;
		zap_size_t outSize = 0;
		zap_int_t outWidth = 0;
		zap_int_t outHeight = 0;

		if (zap_load_memory(convertPointer->pointer.get(), ZAP_COLOR_FORMAT_RGBA32, &out, &outSize, &outWidth, &outHeight) != ZAP_ERROR_NONE) {
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

	// when this unlocks one line later, the output thread will begin waiting on data
	Ubi::BigFile::File &file = convertPointer->file;

	Work::ConvertFileTask convertFileTask(convertPointer->ownerBigFileInputPosition, &file);

	nvtt::OutputOptions outputOptions = {};
	outputOptions.setContainer(nvtt::Container_DDS);

	OutputHandler outputHandler(convertFileTask);
	outputOptions.setOutputHandler(&outputHandler);

	ErrorHandler errorHandler;
	outputOptions.setErrorHandler(&errorHandler);

	nvtt::Context &context = convertPointer->context;
	nvtt::CompressionOptions &compressionOptions = convertPointer->compressionOptions;

	if (!context.outputHeader(surface, 1, compressionOptions, outputOptions)) {
		throw std::runtime_error("Failed to Output Context Header");
	}

	if (!context.compress(surface, 0, 0, compressionOptions, outputOptions) || !errorHandler.result) {
		throw std::runtime_error("Failed to Compress Context");
	}

	file.size = outputHandler.size;

	// this will wake up the output thread to tell it we have no more data to add, and to move on to the next FileTask
	convertFileTask.complete();

	convertPointer->tasks.fileLock().get().push(convertFileTask);
}

VOID CALLBACK M4Revolution::convertZAPProc(PTP_CALLBACK_INSTANCE instance, PVOID parameter, PTP_WORK work) {
	Work::Convert* convertPointer = (Work::Convert*)parameter;
	convertZAPWorkCallback(convertPointer);
}

bool M4Revolution::outputBigFiles(Work::Output &output, std::streampos bigFileInputPosition, Work::Tasks &tasks) {
	std::streampos &currentBigFileInputPosition = output.currentBigFileInputPosition;

	// if this is true we haven't moved on to another BigFile, so just return immediately
	if (bigFileInputPosition == currentBigFileInputPosition) {
		return true;
	}

	std::ofstream &fileStream = output.fileStream;
	Work::BigFileTask* &bigFileTaskPointer = output.bigFileTaskPointer;
	Ubi::BigFile::File::SIZE &filePosition = output.filePosition;
	Ubi::BigFile::File::POINTER_VECTOR::size_type &filesWritten = output.filesWritten;

	Ubi::BigFile::File::POINTER_VECTOR::size_type files = 0;

	std::streampos eraseBigFileInputPosition = -1;

	std::streampos currentOutputPosition = -1;
	std::streampos eraseOutputPosition = -1;

	Work::BigFileTask::MAP_LOCK bigFileLock = tasks.bigFileLock();
	Work::BigFileTask::MAP &bigFileTaskMap = bigFileLock.get();

	// may be zero if this is the first BigFile so this hasn't been set before
	if (bigFileTaskPointer) {
		Work::BigFileTask &bigFileTask = *bigFileTaskPointer;
		files = bigFileTask.getFiles();

		// checks if divisible, in case same BigFile referenced from multiple locations
		if (files && filesWritten % files) {
			// we might be coming back to this BigFile later
			// so set the number of files we've written
			bigFileTask.filesWritten = filesWritten;
		} else {
			do {
				// we're done with this BigFile, so
				// write it, then erase it
				Work::BigFileTask &eraseBigFileTask = *bigFileTaskPointer;

				// jump to the beginning where the filesystem is meant to be
				// then jump to the end again
				currentOutputPosition = fileStream.tellp();
				eraseOutputPosition = eraseBigFileTask.outputPosition;

				fileStream.seekp(eraseOutputPosition);
				eraseBigFileTask.getBigFilePointer()->write(fileStream);

				fileStream.seekp(currentOutputPosition);

				// need to get these now, before the BigFile is erased and we can't anymore
				// (this is fine because they don't live on the BigFile object)
				Ubi::BigFile::File &file = eraseBigFileTask.getFile();

				eraseBigFileInputPosition = currentBigFileInputPosition;
				currentBigFileInputPosition = eraseBigFileTask.getOwnerBigFileInputPosition();

				bigFileTaskMap.erase(eraseBigFileInputPosition);

				// if the BigFile owns itself that means it's the top, and we're done
				if (eraseBigFileInputPosition == currentBigFileInputPosition) {
					return false;
				}

				// we now need to check the owner in case we are the last file in the owner and now all its files are written
				Work::BigFileTask &ownerBigFileTask = bigFileTaskMap.at(currentBigFileInputPosition);
				bigFileTaskPointer = &ownerBigFileTask;

				// update the size and position in the owner's filesystem
				file.size = (Ubi::BigFile::File::SIZE)(currentOutputPosition - eraseOutputPosition);
				file.position = (Ubi::BigFile::File::SIZE)(eraseOutputPosition - ownerBigFileTask.outputPosition);
				filePosition = file.size + file.position;
				ownerBigFileTask.filesWritten++;

				files = bigFileTaskPointer->getFiles();
			} while (!files || !(bigFileTaskPointer->filesWritten % files));
		}
	}

	// get the current BigFile now
	currentBigFileInputPosition = bigFileInputPosition;
	Work::BigFileTask &currentBigFileTask = bigFileTaskMap.at(currentBigFileInputPosition);
	bigFileTaskPointer = &currentBigFileTask;

	// get files written in case we are coming back to this BigFile from before
	filesWritten = currentBigFileTask.filesWritten;

	if (!filesWritten) {
		// if we've not written any files for this BigFile yet
		// then we are at the beginning of it, so seek ahead
		// so that there is space for the filesystem later
		currentBigFileTask.outputPosition = fileStream.tellp();

		filePosition = currentBigFileTask.getFileSystemSize();
		fileStream.seekp(filePosition, std::ios::cur);
	}
	return true;
}

bool M4Revolution::outputDataQueue(Work::Output &output, Work::Data::QUEUE &queue) {
	while (!queue.empty()) {
		Work::Data &data = queue.front();

		// a NULL pointer signifies the file is completed
		if (!data.pointer) {
			return false;
		}

		writeFileStreamSafe(output.fileStream, data.pointer.get(), data.size);
		queue.pop();
	}
	return true;
}

void M4Revolution::outputData(Work::Output &output, Work::Tasks::FILE_TASK_VARIANT &fileTaskVariant, bool &yield) {
	for (;;) {
		if (std::holds_alternative<Work::CopyFileTask::POINTER>(fileTaskVariant)) {
			Work::Data::QUEUE_LOCK lock = std::get<Work::CopyFileTask::POINTER>(fileTaskVariant)->lock(yield);
			Work::Data::QUEUE &queue = lock.get();
			
			if (!outputDataQueue(output, queue)) {
				return;
			}
		} else {
			if (!outputDataQueue(output, std::get<Work::ConvertFileTask>(fileTaskVariant).queue)) {
				return;
			}
		}
	}
}

void M4Revolution::outputFileVariant(Work::Output &output, Work::FileTask::FILE_VARIANT &fileVariant) {
	Ubi::BigFile::File::SIZE &filePosition = output.filePosition;
	Ubi::BigFile::File::POINTER_VECTOR::size_type &filesWritten = output.filesWritten;

	if (std::holds_alternative<Ubi::BigFile::File::POINTER_VECTOR_POINTER>(fileVariant)) {
		Ubi::BigFile::File::POINTER_VECTOR_POINTER filePointerVectorPointer = std::get<Ubi::BigFile::File::POINTER_VECTOR_POINTER>(fileVariant);

		for (
			Ubi::BigFile::File::POINTER_VECTOR::iterator filePointerVectorIterator = filePointerVectorPointer->begin();
			filePointerVectorIterator != filePointerVectorPointer->end();
			filePointerVectorIterator++
		) {
			// in this case, we intentionally disregard the file's size
			// the padding is all that matters
			Ubi::BigFile::File &file = **filePointerVectorIterator;
			filePosition += file.padding;
			file.position = filePosition;
		}

		filesWritten += filePointerVectorPointer->size();
	} else {
		// in this case we know for sure we should use the size
		Ubi::BigFile::File &file = *std::get<Ubi::BigFile::File*>(fileVariant);
		filePosition += file.padding;
		file.position = filePosition;
		filePosition += file.size;
		filesWritten++;
	}
}

void M4Revolution::outputFiles(Work::Output &output, Work::Tasks::FILE_TASK_VARIANT_QUEUE &fileTaskVariantQueue) {
	// depending on if the files was copied or converted
	// we will either have a vector or a singular pointer
	Work::Tasks::FILE_TASK_VARIANT &fileTaskVariant = fileTaskVariantQueue.front();

	if (std::holds_alternative<Work::CopyFileTask::POINTER>(fileTaskVariant)) {
		outputFileVariant(output, std::get<Work::CopyFileTask::POINTER>(fileTaskVariant)->getFileVariant());
	} else {
		outputFileVariant(output, std::get<Work::ConvertFileTask>(fileTaskVariant).getFileVariant());
	}

	// note that we do not continue looping here - need to check on the BigFile next after doing this
	fileTaskVariantQueue.pop();
}

void M4Revolution::outputThread(const char* outputFileName, Work::Tasks &tasks, bool &yield) {
	Work::Output output(outputFileName);

	for (;;) {
		Work::Tasks::FILE_TASK_VARIANT_QUEUE_LOCK queueLock = tasks.fileLock(yield);
		Work::Tasks::FILE_TASK_VARIANT_QUEUE &fileTaskVariantQueue = queueLock.get();

		// this would mean we made it to the end, but didn't write all the filesystems somehow
		if (!yield && fileTaskVariantQueue.empty()) {
			throw std::logic_error("fileTaskQueue must not be empty if yield is false");
		}

		while (!fileTaskVariantQueue.empty()) {
			Work::Tasks::FILE_TASK_VARIANT &fileTaskVariant = fileTaskVariantQueue.front();

			// if this returns false it means we're done
			if (!outputBigFiles(
					output,

					std::holds_alternative<Work::CopyFileTask::POINTER>(fileTaskVariant)
					? std::get<Work::CopyFileTask::POINTER>(fileTaskVariant)->getOwnerBigFileInputPosition()
					: std::get<Work::ConvertFileTask>(fileTaskVariant).getOwnerBigFileInputPosition(),

					tasks
				)
			) {
				return;
			}

			outputData(output, fileTaskVariant, yield);
			outputFiles(output, fileTaskVariantQueue);
		}
	}
}

M4Revolution::M4Revolution(const char* inputFileName, bool logFileNames, bool disableHardwareAcceleration)
	: inputFileStream(inputFileName, std::ios::binary),
	logFileNames(logFileNames) {
	context.enableCudaAcceleration(!disableHardwareAcceleration);

	compressionOptions.setFormat(nvtt::Format_DXT5);
	compressionOptions.setQuality(nvtt::Quality_Highest);

	#ifdef MULTITHREADED
	pool = CreateThreadpool(NULL);

	if (!pool) {
		throw std::runtime_error("Failed to Create Threadpool");
	}

	const DWORD RESERVED_THREADS = 2;

	SYSTEM_INFO systemInfo = {};
	GetSystemInfo(&systemInfo);

	DWORD maxThreads = systemInfo.dwNumberOfProcessors - RESERVED_THREADS;
	SetThreadpoolThreadMaximum(pool, max(maxThreads, 1));

	if (!SetThreadpoolThreadMinimum(pool, 1)) {
		throw std::runtime_error("Failed to Set Threadpool Thread Minimum");
	}
	#endif
}

M4Revolution::~M4Revolution() {
	destroy();
}

void M4Revolution::fixLoading(const char* outputFileName) {
	inputFileStream.seekg(0, std::ios::end);
	Ubi::BigFile::File inputFile((Ubi::BigFile::File::SIZE)inputFileStream.tellg());
	inputFileStream.seekg(0, std::ios::beg);

	bool yield = true;
	std::thread outputThread(M4Revolution::outputThread, outputFileName, std::ref(tasks), std::ref(yield));

	Log log("Fixing Loading, this may take several minutes", inputFileStream, inputFile.size, logFileNames);
	fixLoading(0, inputFile, log);
	log.finishing();

	// necessary to wake up the output thread one last time at the end
	std::get<Work::CopyFileTask::POINTER>(tasks.fileLock().get().emplace(std::make_shared<Work::CopyFileTask>(-1, &inputFile)))->complete();
	yield = false;
	outputThread.join();
}