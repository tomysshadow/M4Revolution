#include "M4Revolution.h"
#include <iostream>
#include <chrono>

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

M4Revolution::OutputHandler::OutputHandler(Work::FileTask &fileTask)
	: fileTask(fileTask) {
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

const char* M4Revolution::OUTPUT_FILE_NAME = "data.m4b.tmp";

const Ubi::BigFile::Path::VECTOR M4Revolution::AI_TRANSITION_FADE_PATH_VECTOR = {
		{{"gamedata", "common"}, "common.m4b"},
		{{"common", "ai", "aitransitionfade"}, "ai_transition_fade.ai"}
};

const Ubi::BigFile::Path::VECTOR M4Revolution::AI_USER_CONTROLS_PATH_VECTOR = {
		{{"gamedata", "common"}, "common.m4b"},
		{{"common", "ai", "aiusercontrols"}, "user_controls.ai"}
};

void M4Revolution::waitFiles(Work::FileTask::POINTER_QUEUE::size_type fileTasks) {
	// this function waits for the output thread to catch up
	// if too many files are queued at once (to prevent running out of memory)
	// this is only done on copying files so that it isn't run too often
	if (fileTasks < maxFileTasks) {
		return;
	}

	// this is just some moderately small amount of time
	const std::chrono::milliseconds MILLISECONDS = std::chrono::milliseconds(25);

	while (tasks.fileLock().get().size() >= maxFileTasks) {
		std::this_thread::sleep_for(MILLISECONDS);
	}
}

void M4Revolution::copyFiles(
	Ubi::BigFile::File::SIZE inputPosition,
	Ubi::BigFile::File::SIZE inputCopyPosition,
	Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
	std::streampos bigFileInputPosition,
	Log &log
) {
	inputFileStream.seekg((std::streampos)inputCopyPosition + bigFileInputPosition);

	Work::FileTask::POINTER_QUEUE::size_type fileTasks = 0;

	// note: this must get created even if filePointerVectorPointer is empty or the count to copy would be zero
	// so that the bigFileInputPosition is reliably seen by the output thread
	Work::FileTask::POINTER fileTaskPointer = std::make_shared<Work::FileTask>(bigFileInputPosition, filePointerVectorPointer);

	// we grab files in this scope so we won't have to lock this twice unnecessarily
	{
		Work::FileTask::POINTER_QUEUE_LOCK &fileLock = tasks.fileLock();
		Work::FileTask::POINTER_QUEUE &queue = fileLock.get();
		queue.push(fileTaskPointer);
		fileTasks = queue.size();
	}

	Work::FileTask &fileTask = *fileTaskPointer;
	fileTask.copy(inputFileStream, inputPosition - inputCopyPosition);
	fileTask.complete();

	waitFiles(fileTasks);

	filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

	log.copying();
}

void M4Revolution::convertFile(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file, Work::Convert::FileWorkCallback fileWorkCallback) {
	// TODO: compressionOptionsWater here if necessary
	Work::Convert* convertPointer = new Work::Convert(file, context, compressionOptions);
	Work::Convert &convert = *convertPointer;

	Work::Data::POINTER &dataPointer = convert.dataPointer;
	dataPointer = Work::Data::POINTER(new unsigned char[file.size]);
	readFileStreamSafe(inputFileStream, dataPointer.get(), file.size);

	Work::FileTask::POINTER &fileTaskPointer = convert.fileTaskPointer;
	fileTaskPointer = std::make_shared<Work::FileTask>(ownerBigFileInputPosition, &file);
	tasks.fileLock().get().push(fileTaskPointer);

	convert.fileWorkCallback = fileWorkCallback;

	#ifdef MULTITHREADED
	PTP_WORK work = CreateThreadpoolWork(convertFileProc, &convert, NULL);

	if (!work) {
		throw std::runtime_error("Failed to Create Threadpool Work");
	}

	SubmitThreadpoolWork(work);
	CloseThreadpoolWork(work);
	#endif
	#ifdef SINGLETHREADED
	convert.fileWorkCallback(&convert);
	#endif
}

void M4Revolution::convertFile(std::streampos bigFileInputPosition, Ubi::BigFile::File &file, Log &log) {
	inputFileStream.seekg((std::streampos)file.position + bigFileInputPosition);

	// these conversion functions update the file sizes passed in
	switch (file.type) {
		case Ubi::BigFile::File::TYPE::BIG_FILE:
		fixLoading(bigFileInputPosition, file, log);
		break;
		case Ubi::BigFile::File::TYPE::JPEG:
		convertFile(bigFileInputPosition, file, convertJPEGWorkCallback);
		break;
		case Ubi::BigFile::File::TYPE::ZAP:
		convertFile(bigFileInputPosition, file, convertZAPWorkCallback);
		break;
		default:
		// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
		Work::FileTask::POINTER fileTaskPointer = std::make_shared<Work::FileTask>(bigFileInputPosition, &file);
		tasks.fileLock().get().push(fileTaskPointer);

		Work::FileTask &fileTask = *fileTaskPointer;
		fileTask.copy(inputFileStream, file.size);
		fileTask.complete();
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
	tasks.bigFileLock().get().insert({bigFileInputPosition, std::make_shared<Work::BigFileTask>(inputFileStream, ownerBigFileInputPosition, file, filePointerSetMap)});

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
			if (!convert && file.type != Ubi::BigFile::File::TYPE::NONE && file.type != Ubi::BigFile::File::TYPE::BINARY) {
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

void M4Revolution::convertSurface(Work::Convert &convert, nvtt::Surface &surface) {
	nvtt::OutputOptions outputOptions = {};
	outputOptions.setContainer(nvtt::Container_DDS);

	Work::FileTask &fileTask = *convert.fileTaskPointer;

	OutputHandler outputHandler(fileTask);
	outputOptions.setOutputHandler(&outputHandler);

	ErrorHandler errorHandler;
	outputOptions.setErrorHandler(&errorHandler);

	nvtt::Context &context = convert.context;
	nvtt::CompressionOptions &compressionOptions = convert.compressionOptions;

	if (!context.outputHeader(surface, 1, compressionOptions, outputOptions)) {
		throw std::runtime_error("Failed to Output Context Header");
	}

	if (!context.compress(surface, 0, 0, compressionOptions, outputOptions) || !errorHandler.result) {
		throw std::runtime_error("Failed to Compress Context");
	}

	convert.file.size = outputHandler.size;

	// this will wake up the output thread to tell it we have no more data to add, and to move on to the next FileTask
	fileTask.complete();
}

void M4Revolution::convertJPEGWorkCallback(Work::Convert* convertPointer) {
	SCOPE_EXIT {
		delete convertPointer;
	};

	Work::Convert &convert = *convertPointer;
	nvtt::Surface surface = {};

	if (!surface.loadFromMemory(convert.dataPointer.get(), convert.file.size)) {
		throw std::runtime_error("Failed to Load Surface From Memory");
	}

	// when this unlocks one line later, the output thread will begin waiting on data
	convertSurface(convert, surface);
}

void M4Revolution::convertZAPWorkCallback(Work::Convert* convertPointer) {
	SCOPE_EXIT {
		delete convertPointer;
	};

	Work::Convert &convert = *convertPointer;
	nvtt::Surface surface = {};

	{
		zap_byte_t* out = 0;
		zap_size_t outSize = 0;
		zap_int_t outWidth = 0;
		zap_int_t outHeight = 0;

		if (zap_load_memory(convert.dataPointer.get(), ZAP_COLOR_FORMAT_RGBA32, &out, &outSize, &outWidth, &outHeight) != ZAP_ERROR_NONE) {
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
			throw std::runtime_error("Failed to Set Surface Image");
		}
	}

	// when this unlocks one line later, the output thread will begin waiting on data
	convertSurface(convert, surface);
}

#ifdef MULTITHREADED
VOID CALLBACK M4Revolution::convertFileProc(PTP_CALLBACK_INSTANCE instance, PVOID parameter, PTP_WORK work) {
	Work::Convert* convertPointer = (Work::Convert*)parameter;
	convertPointer->fileWorkCallback(convertPointer);
}
#endif

bool M4Revolution::outputBigFiles(Work::Output &output, std::streampos bigFileInputPosition, Work::Tasks &tasks) {
	std::streampos &currentBigFileInputPosition = output.currentBigFileInputPosition;

	// if this is true we haven't moved on to another BigFile, so just return immediately
	if (bigFileInputPosition == currentBigFileInputPosition) {
		return true;
	}

	std::ofstream &fileStream = output.fileStream;
	Work::BigFileTask::POINTER &bigFileTaskPointer = output.bigFileTaskPointer;
	Ubi::BigFile::File::SIZE &filePosition = output.filePosition;
	Ubi::BigFile::File::POINTER_VECTOR::size_type &filesWritten = output.filesWritten;

	Ubi::BigFile::File::POINTER_VECTOR::size_type files = 0;

	Work::BigFileTask::POINTER eraseBigFileTaskPointer = 0;
	Work::BigFileTask::POINTER currentBigFileTaskPointer = 0;

	std::streampos eraseBigFileInputPosition = -1;
	std::streampos currentOutputPosition = -1;

	// may be zero if this is the first BigFile so this hasn't been set before
	if (bigFileTaskPointer) {
		Work::BigFileTask &bigFileTask = *bigFileTaskPointer;
		files = bigFileTask.getFiles();

		// checks if divisible, in case same BigFile referenced from multiple locations
		if (files && (!filesWritten || filesWritten % files)) {
			// we might be coming back to this BigFile later
			// so set the number of files we've written
			bigFileTask.filesWritten = filesWritten;
		} else {
			do {
				// we're done with this BigFile, so
				// write it, then erase it
				eraseBigFileTaskPointer = bigFileTaskPointer;
				Work::BigFileTask &eraseBigFileTask = *eraseBigFileTaskPointer;

				// jump to the beginning where the filesystem is meant to be
				// then jump to the end again
				currentOutputPosition = fileStream.tellp();

				std::streampos &eraseOutputPosition = eraseBigFileTask.outputPosition;
				fileStream.seekp(eraseOutputPosition);

				eraseBigFileTask.getBigFilePointer()->write(fileStream);

				fileStream.seekp(currentOutputPosition);

				eraseBigFileInputPosition = currentBigFileInputPosition;
				currentBigFileInputPosition = eraseBigFileTask.getOwnerBigFileInputPosition();

				{
					Work::BigFileTask::POINTER_MAP_LOCK bigFileLock = tasks.bigFileLock();
					Work::BigFileTask::POINTER_MAP &bigFileTaskPointerMap = bigFileLock.get();

					bigFileTaskPointerMap.erase(eraseBigFileInputPosition);

					// if the BigFile owns itself that means it's the top, and we're done
					if (eraseBigFileInputPosition == currentBigFileInputPosition) {
						return false;
					}

					// since we're already holding the lock, might as well get this now
					currentBigFileTaskPointer = bigFileTaskPointerMap.at(bigFileInputPosition);
					bigFileTaskPointer = bigFileTaskPointerMap.at(currentBigFileInputPosition);
				}

				// we now need to check the owner in case we are the last file in the owner and now all its files are written
				Work::BigFileTask &ownerBigFileTask = *bigFileTaskPointer;

				// update the size and position in the owner's filesystem
				Ubi::BigFile::File &file = eraseBigFileTask.getFile();
				file.size = (Ubi::BigFile::File::SIZE)(currentOutputPosition - eraseOutputPosition);
				file.position = (Ubi::BigFile::File::SIZE)(eraseOutputPosition - ownerBigFileTask.outputPosition);
				filePosition = file.size + file.position;
				ownerBigFileTask.filesWritten++;

				files = bigFileTaskPointer->getFiles();
			} while (!files || (bigFileTaskPointer->filesWritten && !(bigFileTaskPointer->filesWritten % files)));
		}
	}

	// get the current BigFile now, if we didn't before already
	currentBigFileInputPosition = bigFileInputPosition;
	bigFileTaskPointer = currentBigFileTaskPointer ? currentBigFileTaskPointer : tasks.bigFileLock().get().at(currentBigFileInputPosition);
	Work::BigFileTask &currentBigFileTask = *bigFileTaskPointer;

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

void M4Revolution::outputData(std::ofstream &fileStream, Work::FileTask &fileTask, bool &yield) {
	Work::Data::QUEUE dataQueue = {};

	for (;;) {
		// copy out the queue
		// this is fast since data is just a struct with a number and pointer
		// (this is several times faster than holding the lock while writing)
		{
			Work::Data::QUEUE_LOCK lock = fileTask.lock(yield);
			Work::Data::QUEUE &queue = lock.get();

			if (queue.empty()) {
				continue;
			}

			dataQueue = queue;
			queue = {};
		}

		while (!dataQueue.empty()) {
			Work::Data &data = dataQueue.front();

			// a null pointer signals that the file is complete
			if (!data.pointer) {
				return;
			}

			writeFileStreamSafe(fileStream, data.pointer.get(), data.size);

			dataQueue.pop();
		}
	}
}

void M4Revolution::outputFiles(Work::Output &output, Work::FileTask::FILE_VARIANT &fileVariant) {
	Ubi::BigFile::File::SIZE &filePosition = output.filePosition;
	Ubi::BigFile::File::POINTER_VECTOR::size_type &filesWritten = output.filesWritten;

	// depending on if the files was copied or converted
	// we will either have a vector or a singular dataPointer
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

void M4Revolution::outputThread(const char* outputFileName, Work::Tasks &tasks, bool &yield) {
	Work::Output output(outputFileName);

	Work::FileTask::POINTER_QUEUE fileTaskPointerQueue = {};

	for (;;) {
		// copy out the queue
		// (this is fast because it's just a queue of pointers, much faster than holding the lock while writing)
		{
			Work::FileTask::POINTER_QUEUE_LOCK fileLock = tasks.fileLock(yield);
			Work::FileTask::POINTER_QUEUE &queue = fileLock.get();

			if (queue.empty()) {
				if (yield) {
					continue;
				}

				// this would mean we made it to the end, but didn't write all the filesystems somehow
				throw std::logic_error("fileTaskPointerQueue must not be empty if yield is false");
			}

			fileTaskPointerQueue = queue;
			queue = {};
		}

		while (!fileTaskPointerQueue.empty()) {
			Work::FileTask &fileTask = *fileTaskPointerQueue.front();

			// if this returns false it means we're done
			if (!outputBigFiles(output, fileTask.getOwnerBigFileInputPosition(), tasks)) {
				return;
			}

			outputData(output.fileStream, fileTask, yield);
			outputFiles(output, fileTask.getFileVariant());

			fileTaskPointerQueue.pop();
		}
	}
}

M4Revolution::M4Revolution(
	const char* inputFileName,
	bool logFileNames,
	bool disableHardwareAcceleration,
	uint32_t maxThreads,
	Work::FileTask::POINTER_QUEUE::size_type maxFileTasks
)
	: inputFileStream(inputFileName, std::ios::binary),
	logFileNames(logFileNames) {
	// the number 216 was chosen for being the standard number of tiles in a cube
	const Work::FileTask::POINTER_QUEUE::size_type DEFAULT_MAX_FILE_TASKS = 216;

	this->maxFileTasks = maxFileTasks ? maxFileTasks : DEFAULT_MAX_FILE_TASKS;

	context.enableCudaAcceleration(!disableHardwareAcceleration);

	compressionOptions.setFormat(nvtt::Format_DXT5);
	compressionOptions.setQuality(nvtt::Quality_Highest);

	compressionOptionsWater.setFormat(nvtt::Format_RGBA);
	compressionOptionsWater.setQuality(nvtt::Quality_Highest);

	#ifdef MULTITHREADED
	pool = CreateThreadpool(NULL);

	if (!pool) {
		throw std::runtime_error("Failed to Create Threadpool");
	}

	if (!maxThreads) {
		// chosen so that if you have a quad core there will still be at least two threads for other system stuff
		// (meanwhile, barely affecting even more powerful processors)
		const DWORD RESERVED_THREADS = 2;

		SYSTEM_INFO systemInfo = {};
		GetSystemInfo(&systemInfo);

		// can't use max because this is unsigned
		maxThreads = systemInfo.dwNumberOfProcessors > RESERVED_THREADS ? systemInfo.dwNumberOfProcessors - RESERVED_THREADS : 1;
	}

	SetThreadpoolThreadMaximum(pool, maxThreads);

	if (!SetThreadpoolThreadMinimum(pool, 1)) {
		throw std::runtime_error("Failed to Set Threadpool Thread Minimum");
	}
	#endif
}

M4Revolution::~M4Revolution() {
	destroy();
}

void M4Revolution::fixLoading() {
	inputFileStream.seekg(0, std::ios::end);
	Ubi::BigFile::File inputFile((Ubi::BigFile::File::SIZE)inputFileStream.tellg());
	inputFileStream.seekg(0, std::ios::beg);

	bool yield = true;
	std::thread outputThread(M4Revolution::outputThread, OUTPUT_FILE_NAME, std::ref(tasks), std::ref(yield));

	Log log("Fixing Loading, this may take several minutes", inputFileStream, inputFile.size, logFileNames);
	fixLoading(0, inputFile, log);
	log.finishing();

	// necessary to wake up the output thread one last time at the end
	Work::FileTask::POINTER fileTaskPointer = std::make_shared<Work::FileTask>(-1, &inputFile);
	fileTaskPointer->complete();
	tasks.fileLock().get().push(fileTaskPointer);

	yield = false;
	outputThread.join();
}