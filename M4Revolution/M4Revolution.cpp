#include "M4Revolution.h"
#include "AI.h"
#include "GlobalHandle.h"
#include <chrono>
#include <iostream>
#include <filesystem>

#ifdef D3D9
#include <wrl/client.h>
#include <comdef.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <d3d9caps.h>
#endif

void M4Revolution::destroy() {
	#ifdef MULTITHREADED
	CloseThreadpool(pool);
	#endif
}

void M4Revolution::Log::replacedM4Thor(const std::string &name, bool toggledOn) {
	std::cout << "Replaced M4 Thor" << std::endl;
	std::cout << name << TOGGLE_IS << (toggledOn ? TOGGLE_ON : TOGGLE_OFF) << std::endl << std::endl;
}

void M4Revolution::Log::replacedGfxTools() {
	std::cout << "Replaced Gfx Tools" << std::endl << std::endl;
}

M4Revolution::Log::Log(const char* title, std::istream &inputStream, Ubi::BigFile::File::SIZE inputFileSize, bool fileNames, bool slow)
	: inputStream(inputStream),
	inputFileSize(inputFileSize),
	fileNames(fileNames) {
	if (slow) {
		beginOptional = std::chrono::steady_clock::now();
	}

	std::cout << title << "..." << std::endl;
}

M4Revolution::Log::~Log() {
	if (beginOptional.has_value()) {
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		std::cout << "Elapsed Seconds: " << std::chrono::duration_cast<std::chrono::seconds>(end - beginOptional.value()).count() << std::endl << std::endl;
	}
}

void M4Revolution::Log::step() {
	files++;
}

void M4Revolution::Log::copying() {
	if (!fileNames) {
		// use the position to log the current progress
		// this works because we read the input file stream from beginning to end in order
		int currentProgress = (int)(((double)inputStream.tellg() / inputFileSize) * 100.0);

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

	// just so that there is an extra newline on the end so the menu looks nice
	if (!beginOptional.has_value()) {
		std::cout << std::endl;
	}
}

M4Revolution::CompressionOptions::CompressionOptions() {
	rgba.setFormat(nvtt::Format_RGBA);
	rgba.setQuality(nvtt::Quality_Highest);

	dxt1.setFormat(nvtt::Format_DXT1);
	dxt1.setQuality(nvtt::Quality_Highest);

	dxt5.setFormat(nvtt::Format_DXT5);
	dxt5.setQuality(nvtt::Quality_Highest);
}

const nvtt::CompressionOptions &M4Revolution::CompressionOptions::get(const Ubi::BigFile::File &file, const nvtt::Surface &surface, bool hasAlpha) const {
	// immediately use RGBA if the file forces us to
	if (file.rgba) {
		return rgba;
	}

	// ares assumes all DXT textures are square and power of two sized
	// so if they are not, we must use RGBA instead
	const int DEPTH_SQUARE = 1;

	int width = surface.width();
	int height = surface.height();
	int depth = surface.depth();

	if (width != height || depth != DEPTH_SQUARE) {
		return rgba;
	}

	// only need to check width, because we know it's the same as the height
	if (!isPowerOfTwo((unsigned int)width)) {
		return rgba;
	}
	return hasAlpha ? dxt5 : dxt1;
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
		Work::Data::POINTER pointer(new unsigned char[size]);

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
	consoleLog(nvtt::errorString(error), true, false, true);
	result = false;
}

void M4Revolution::waitFiles(Work::FileTask::POINTER_QUEUE::size_type fileTasks) {
	// this function waits for the output thread to catch up
	// if too many files are queued at once (to prevent running out of memory)
	// this is only done on copying files so that it isn't run too often
	if (fileTasks < maxFileTasks) {
		return;
	}

	// this is just some moderately small amount of time
	const std::chrono::milliseconds MILLISECONDS(25);

	while (tasks.fileLock().get().size() >= maxFileTasks) {
		std::this_thread::sleep_for(MILLISECONDS);
	}
}

void M4Revolution::copyFiles(
	std::istream &inputStream,
	Ubi::BigFile::File::SIZE inputPosition,
	Ubi::BigFile::File::SIZE inputCopyPosition,
	Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
	std::streampos bigFileInputPosition,
	Log &log
) {
	inputStream.seekg((std::streampos)inputCopyPosition + bigFileInputPosition);

	Work::FileTask::POINTER_QUEUE::size_type fileTasks = 0;

	// note: this must get created even if filePointerVectorPointer is empty or the count to copy would be zero
	// so that the bigFileInputPosition is reliably seen by the output thread
	Work::FileTask::POINTER fileTaskPointer = std::make_shared<Work::FileTask>(bigFileInputPosition, filePointerVectorPointer);

	// we grab files in this scope so we won't have to lock this twice unnecessarily
	{
		Work::FileTask::POINTER_QUEUE_LOCK fileLock = tasks.fileLock();
		Work::FileTask::POINTER_QUEUE &queue = fileLock.get();
		queue.push(fileTaskPointer);
		fileTasks = queue.size();
	}

	Work::FileTask &fileTask = *fileTaskPointer;
	fileTask.copy(inputStream, inputPosition - inputCopyPosition);
	fileTask.complete();

	waitFiles(fileTasks);

	filePointerVectorPointer = std::make_shared<Ubi::BigFile::File::POINTER_VECTOR>();

	log.copying();
}

void M4Revolution::convertFile(
	std::istream &inputStream,
	std::streampos ownerBigFileInputPosition,
	Ubi::BigFile::File &file,
	Work::Convert::FileWorkCallback fileWorkCallback
) {
	Work::Convert &convert = *new Work::Convert(configuration, context, file);

	Work::Data::POINTER &dataPointer = convert.dataPointer;
	dataPointer = Work::Data::POINTER(new unsigned char[file.size]);
	readStreamSafe(inputStream, dataPointer.get(), file.size);

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

void M4Revolution::convertFile(
	std::istream &inputStream,
	std::streampos bigFileInputPosition,
	Ubi::BigFile::File &file,
	Log &log
) {
	inputStream.seekg((std::streampos)file.position + bigFileInputPosition);

	// these conversion functions update the file sizes passed in
	switch (file.type) {
		case Ubi::BigFile::File::TYPE::BIG_FILE:
		fixLoading(inputStream, bigFileInputPosition, file, log);
		break;
		case Ubi::BigFile::File::TYPE::IMAGE_STANDARD:
		convertFile(inputStream, bigFileInputPosition, file, convertImageStandardWorkCallback);
		break;
		case Ubi::BigFile::File::TYPE::IMAGE_ZAP:
		convertFile(inputStream, bigFileInputPosition, file, convertImageZAPWorkCallback);
		break;
		default:
		// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
		Work::FileTask::POINTER fileTaskPointer = std::make_shared<Work::FileTask>(bigFileInputPosition, &file);
		tasks.fileLock().get().push(fileTaskPointer);

		Work::FileTask &fileTask = *fileTaskPointer;
		fileTask.copy(inputStream, file.size);
		fileTask.complete();
	}

	log.converting(file);
}

void M4Revolution::stepFile(
	Ubi::BigFile::File::SIZE inputPosition,
	Ubi::BigFile::File::SIZE &inputFilePosition,
	Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
	Ubi::BigFile::File::POINTER filePointer,
	Log &log
) {
	inputFilePosition = inputPosition;
	filePointerVectorPointer->push_back(filePointer);

	log.step();
}

void M4Revolution::fixLoading(std::istream &inputStream, std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file, Log &log) {
	// filePointerSetMap is a map where the keys are the file positions beginning to end, and values are sets of files at that position
	Ubi::BigFile::File::POINTER_SET_MAP filePointerSetMap = {};
	std::streampos bigFileInputPosition = inputStream.tellg();

	tasks.bigFileLock().get().insert(
		{
			bigFileInputPosition,

			std::make_shared<Work::BigFileTask>(
				inputStream,
				ownerBigFileInputPosition,
				file,
				filePointerSetMap
			)
		}
	);

	// inputCopyPosition is the position of the files to copy
	// inputFilePosition is the position of a specific input file (for file.size calculation)
	Ubi::BigFile::File::SIZE inputCopyPosition = (Ubi::BigFile::File::SIZE)(inputStream.tellg() - bigFileInputPosition);
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
					copyFiles(inputStream, filePointerSetMapIterator->first, inputCopyPosition, filePointerVectorPointer, bigFileInputPosition, log);
				}

				// we'll need to convert this file type
				convert = true;
			}

			// if we are converting this or any previous file in the set
			if (convert) {
				convertFile(inputStream, bigFileInputPosition, file, log);
			} else {
				// other identical, copied files at the same position in the input should likewise be at the same position in the output
				file.padding = filePointerSetMapIterator->first - inputFilePosition;
				stepFile(filePointerSetMapIterator->first, inputFilePosition, filePointerVectorPointer, *filePointerSetIterator, log);
			}
		}
	}

	// if we just converted a set of files then there are no remaining files to copy, but otherwise...
	if (!convert) {
		// always copy here even if filePointerVectorPointer is empty
		// (ensure every BigFile has at least one FileTask)
		copyFiles(inputStream, file.size, inputCopyPosition, filePointerVectorPointer, bigFileInputPosition, log);
	}
}

const M4Revolution::CompressionOptions M4Revolution::COMPRESSION_OPTIONS;

void M4Revolution::replaceM4Thor(std::fstream &fileStream, const std::string &name) {
	const size_t COMPUTE_MOVE_VECTOR_SIZE = 13;
	const unsigned char COMPUTE_MOVE_VECTOR_ON[COMPUTE_MOVE_VECTOR_SIZE + 1] = { 0xD9, 0x44, 0x24, 0x08, 0x83, 0xEC, 0x0C, 0xD8, 0x41, 0x50, 0xD9, 0x51, 0x50, 0x00 };
	const unsigned char COMPUTE_MOVE_VECTOR_OFF[COMPUTE_MOVE_VECTOR_SIZE + 1] = { 0xC6, 0x44, 0x24, 0x0B, 0x48, 0x90, 0xD9, 0x44, 0x24, 0x08, 0x83, 0xEC, 0x0C, 0x00 };

	unsigned long computeMoveVectorPosition = 0;

	while (!getComputeMoveVectorPosition(computeMoveVectorPosition)) {
		RETRY_ERR(Work::Output::FILE_RETRY);
	}

	unsigned char computeMoveVector[COMPUTE_MOVE_VECTOR_SIZE] = "";

	Work::Edit edit(fileStream, Work::Output::M4_THOR_PATH);

	fileStream.seekg(computeMoveVectorPosition);
	readStreamSafe(fileStream, computeMoveVector, COMPUTE_MOVE_VECTOR_SIZE);

	std::thread copyThread(Work::Edit::copyThread, std::ref(edit));

	// we must either be on or off
	// if we're neither, something is wrong so give up
	bool toggledOn = memoryEquals(computeMoveVector, COMPUTE_MOVE_VECTOR_ON, COMPUTE_MOVE_VECTOR_SIZE);

	if (!toggledOn) {
		toggledOn = !memoryEquals(computeMoveVector, COMPUTE_MOVE_VECTOR_OFF, COMPUTE_MOVE_VECTOR_SIZE);

		if (toggledOn) {
			throw std::logic_error("Compute Move Vector untoggleable");
		}
	}

	// toggle happens here
	toggledOn = !toggledOn;
	edit.join(copyThread, computeMoveVectorPosition, (const char*)(toggledOn ? COMPUTE_MOVE_VECTOR_ON : COMPUTE_MOVE_VECTOR_OFF));

	Log::replacedM4Thor(name, toggledOn);
}

#ifdef WINDOWS
void M4Revolution::replaceGfxTools() {
	// scope so that we let go of the output before renaming it and free the resource when no longer needed
	{
		HRSRC resourceHandle = FindResource(NULL, MAKEINTRESOURCE(IDR_BIN_GFX_TOOLS), TEXT("BIN"));

		if (!resourceHandle) {
			throw std::runtime_error("Failed to Find Resource");
		}

		GlobalHandleLock<> resourceGlobalHandleLock(NULL, resourceHandle);

		Work::Output output = {};
		writeStreamSafe(output.fileStream, resourceGlobalHandleLock.get(), resourceGlobalHandleLock.size());
	}

	if (Work::Backup::create(Work::Output::GFX_TOOLS_PATH.string().c_str())) {
		Work::Backup::log();
	}

	Log::replacedGfxTools();
}
#endif

Ubi::BigFile::File M4Revolution::createInputFile(std::istream &inputStream) {
	inputStream.seekg(0, std::ios::end);
	Ubi::BigFile::File inputFile((Ubi::BigFile::File::SIZE)inputStream.tellg());
	inputStream.seekg(0, std::ios::beg);
	return inputFile;
}

void M4Revolution::convertSurface(Work::Convert &convert, nvtt::Surface &surface, bool hasAlpha) {
	const Work::Convert::Configuration &CONFIGURATION = convert.CONFIGURATION;

	#ifdef EXTENTS_MAKE_POWER_OF_TWO
	#ifdef TO_NEXT_POWER_OF_TWO
	const nvtt::RoundMode ROUND_MODE = nvtt::RoundMode_ToNextPowerOfTwo;
	#else
	const nvtt::RoundMode ROUND_MODE = nvtt::RoundMode_ToPreviousPowerOfTwo;
	#endif
	#else
	const nvtt::RoundMode ROUND_MODE = nvtt::RoundMode_None;
	#endif

	const nvtt::ResizeFilter RESIZE_FILTER = nvtt::ResizeFilter_Triangle;
	const nvtt::Context &CONTEXT = convert.CONTEXT;
	const int MIPMAP_COUNT = 1;

	Work::Convert::EXTENT width = clamp((Work::Convert::EXTENT)surface.width(), CONFIGURATION.minTextureWidth, CONFIGURATION.maxTextureWidth);
	Work::Convert::EXTENT height = clamp((Work::Convert::EXTENT)surface.height(), CONFIGURATION.minTextureHeight, CONFIGURATION.maxTextureHeight);
	Work::Convert::EXTENT depth = clamp((Work::Convert::EXTENT)surface.depth(), CONFIGURATION.minVolumeExtent, CONFIGURATION.maxVolumeExtent);

	Work::Convert::EXTENT maxExtent = __max(width, height);
	maxExtent = __max(depth, maxExtent);

	#ifdef EXTENTS_MAKE_SQUARE
	surface.resize_make_square(maxExtent, ROUND_MODE, RESIZE_FILTER);
	#else
	surface.resize(maxExtent, ROUND_MODE, RESIZE_FILTER);
	#endif

	Ubi::BigFile::File &file = convert.file;

	/*
	if (file.greyScale) {
		// NTSC Luminance Weights
		surface.toGreyScale(0.299f, 0.587f, 0.114f, 1.0f);
	}
	*/

	// must be called here after we've modified the surface
	const nvtt::CompressionOptions &COMPRESSION_OPTIONS = M4Revolution::COMPRESSION_OPTIONS.get(file, surface, hasAlpha);

	nvtt::OutputOptions outputOptions = {};
	outputOptions.setContainer(nvtt::Container_DDS);

	Work::FileTask &fileTask = *convert.fileTaskPointer;

	OutputHandler outputHandler(fileTask);
	outputOptions.setOutputHandler(&outputHandler);

	ErrorHandler errorHandler;
	outputOptions.setErrorHandler(&errorHandler);

	if (!CONTEXT.outputHeader(surface, MIPMAP_COUNT, COMPRESSION_OPTIONS, outputOptions)) {
		throw std::runtime_error("Failed to Output Context Header");
	}

	for (int i = 0; i < MIPMAP_COUNT; i++) {
		if (!CONTEXT.compress(surface, 0, i, COMPRESSION_OPTIONS, outputOptions) || !errorHandler.result) {
			throw std::runtime_error("Failed to Compress Context");
		}
	}

	file.size = outputHandler.size;

	// this will wake up the output thread to tell it we have no more data to add, and to move on to the next FileTask
	fileTask.complete();
}

void M4Revolution::convertImageStandardWorkCallback(Work::Convert* convertPointer) {
	SCOPE_EXIT {
		delete convertPointer;
	};

	Work::Convert &convert = *convertPointer;
	nvtt::Surface surface = {};
	bool hasAlpha = true;

	if (!surface.loadFromMemory(convert.dataPointer.get(), convert.file.size, &hasAlpha)) {
		throw std::runtime_error("Failed to Load Surface From Memory");
	}

	// when this unlocks one line later, the output thread will begin waiting on data
	convertSurface(convert, surface, hasAlpha);
}

void M4Revolution::convertImageZAPWorkCallback(Work::Convert* convertPointer) {
	SCOPE_EXIT {
		delete convertPointer;
	};

	Work::Convert &convert = *convertPointer;
	nvtt::Surface surface = {};

	{
		zap_byte_t* image = 0;
		zap_size_t size = 0;
		zap_int_t width = 0;
		zap_int_t height = 0;
		zap_size_t stride = 0;

		zap_error_t err = zap_load_memory(convert.dataPointer.get(), ZAP_COLOR_FORMAT_BGRA, &image, &size, &width, &height, &stride);

		if (err != ZAP_ERROR_NONE) {
			throw std::runtime_error("Failed to Load ZAP From Memory");
		}

		SCOPE_EXIT {
			if (!freeZAP(image)) {
				throw std::runtime_error("Failed to Free ZAP");
			}
		};

		const int DEPTH = 1;

		if (!surface.setImage(nvtt::InputFormat::InputFormat_BGRA_8UB, width, height, DEPTH, image)) {
			throw std::runtime_error("Failed to Set Surface Image");
		}
	}

	// when this unlocks one line later, the output thread will begin waiting on data
	convertSurface(convert, surface, true);
}

bool M4Revolution::getComputeMoveVectorPosition(unsigned long &computeMoveVectorPosition) {
	// default value is the position as of the latest Steam version
	MAKE_SCOPE_EXIT(computeMoveVectorPositionScopeExit) {
		computeMoveVectorPosition = 0x000109C0;
	};

	#ifdef WINDOWS
	TCHAR commandLine[] = TEXT("GetDLLExportRVA m4_thor_rd.dll ?ComputeMoveVector@COrientationUpdateManager@thor@@AAE?AVVector3@ubi@@M@Z");

	PROCESS_INFORMATION processInformation = {};
	HANDLE &process = processInformation.hProcess;
	HANDLE &thread = processInformation.hThread;

	SCOPE_EXIT {
		if (!closeProcess(process)) {
			throw std::runtime_error("Failed to Close Process");
		}
	};

	SCOPE_EXIT {
		if (!closeThread(thread)) {
			throw std::runtime_error("Failed to Close Thread");
		}
	};

	const DWORD BUFFER_SIZE = sizeof("0x00000000");
	CHAR buffer[BUFFER_SIZE] = "";

	{
		HANDLE stdoutReadPipe = NULL;

		SCOPE_EXIT {
			if (!closeHandle(stdoutReadPipe)) {
				throw std::runtime_error("Failed to Close Handle");
			}
		};

		{
			HANDLE stdoutWritePipe = NULL;

			SCOPE_EXIT {
				if (!closeHandle(stdoutWritePipe)) {
					throw std::runtime_error("Failed to Close Handle");
				}
			};

			SECURITY_ATTRIBUTES securityAttributes = {};
			securityAttributes.nLength = sizeof(securityAttributes);
			securityAttributes.bInheritHandle = TRUE;

			if (!CreatePipe(&stdoutReadPipe, &stdoutWritePipe, &securityAttributes, BUFFER_SIZE - 1)) {
				throw std::runtime_error("Failed to Create Pipe");
			}

			if (!SetHandleInformation(stdoutReadPipe, HANDLE_FLAG_INHERIT, 0)) {
				throw std::runtime_error("Failed to Set Handle Information");
			}

			STARTUPINFO startupInfo = {};
			startupInfo.cb = sizeof(startupInfo);
			startupInfo.dwFlags = STARTF_USESTDHANDLES;
			startupInfo.hStdOutput = stdoutWritePipe;

			if (!CreateProcess(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, TEXT("bin"), &startupInfo, &processInformation)
				|| !process
				|| !thread) {
				throw std::runtime_error("Failed to Create Process");
			}
		}

		DWORD numberOfBytesRead = 0;
		readPipePartial(stdoutReadPipe, buffer, BUFFER_SIZE - 1, numberOfBytesRead);
	}

	// max so we don't hang forever in the worst case
	const DWORD MILLISECONDS = 10000;

	DWORD wait = WaitForSingleObject(process, MILLISECONDS);

	if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) {
		throw std::runtime_error("Failed to Wait For Single Object");
	}

	size_t size = stringToLongUnsigned(buffer, computeMoveVectorPosition);

	if (!size) {
		DWORD exitCode = 0;

		if (!GetExitCodeProcess(process, &exitCode)) {
			throw std::runtime_error("Failed to Get Process Exit Code");
		}

		SetLastError(exitCode);
	}

	computeMoveVectorPositionScopeExit.dismiss();
	return size;
	#endif
	return true;
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

void M4Revolution::outputData(std::ostream &outputStream, Work::FileTask &fileTask, bool &yield) {
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

			writeStreamSafe(outputStream, data.pointer.get(), data.size);

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

void M4Revolution::outputThread(Work::Tasks &tasks, bool &yield) {
	Work::Output output = {};

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
				throw std::logic_error("queue must not be empty if yield is false");
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

			Work::FileTask::FILE_VARIANT fileVariant = fileTask.getFileVariant();
			outputFiles(output, fileVariant);

			fileTaskPointerQueue.pop();
		}
	}
}

M4Revolution::M4Revolution(
	const std::filesystem::path &path,
	bool logFileNames,
	bool disableHardwareAcceleration,
	uint32_t maxThreads,
	Work::FileTask::POINTER_QUEUE::size_type maxFileTasks,
	std::optional<Work::Convert::Configuration> configurationOptional
)
	: logFileNames(logFileNames) {
	// here we make the path lexically normal just so that it displays nice
	Work::Output::findInstallPath(path.lexically_normal());

	context.enableCudaAcceleration(!disableHardwareAcceleration);

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

	// the number 216 was chosen for being the standard number of tiles in a cube
	const Work::FileTask::POINTER_QUEUE::size_type DEFAULT_MAX_FILE_TASKS = 216;

	this->maxFileTasks = maxFileTasks ? maxFileTasks : DEFAULT_MAX_FILE_TASKS;

	if (configurationOptional.has_value()) {
		configuration = configurationOptional.value();
	}
	#ifdef D3D9
	else {
		// we need to get the max texture size supported by this graphics card
		// and ensure that all textures we convert are resized to less than this size
		// ares uses Direct3D 9 to do this - so I do also
		// it is assumed this tool will be run on the same GPU as the game itself will
		Microsoft::WRL::ComPtr<IDirect3D9> direct3D9InterfacePointer = Direct3DCreate9(D3D_SDK_VERSION);

		if (!direct3D9InterfacePointer) {
			throw std::runtime_error("Failed to Create Direct3D");
		}

		D3DCAPS9 d3dcaps9 = {};

		HRESULT err = direct3D9InterfacePointer->GetDeviceCaps(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			&d3dcaps9
		);

		if (err != D3D_OK) {
			throw _com_error(err);
		}

		configuration.maxTextureWidth = d3dcaps9.MaxTextureWidth;
		configuration.maxTextureHeight = d3dcaps9.MaxTextureHeight;
		configuration.maxVolumeExtent = d3dcaps9.MaxVolumeExtent;
	}
	#endif
}

M4Revolution::~M4Revolution() {
	destroy();
}

void M4Revolution::editTransitionTime() {
	std::fstream fileStream;

	Log log("Editing Transition Time", fileStream);

	OPERATION_EXCEPTION_RETRY_ERR(AI::editTransitionTime(fileStream), StreamFailed, Work::Output::FILE_RETRY);
}

void M4Revolution::toggleSoundFading() {
	std::fstream fileStream;

	Log log("Toggling Sound Fading", fileStream);

	OPERATION_EXCEPTION_RETRY_ERR(AI::toggleSoundFading(fileStream), StreamFailed, Work::Output::FILE_RETRY);
}

void M4Revolution::toggleCameraInertia() {
	std::fstream fileStream;

	Log log("Toggling Camera Inertia", fileStream);

	OPERATION_EXCEPTION_RETRY_ERR(replaceM4Thor(fileStream, "Camera Inertia"), StreamFailed, Work::Output::FILE_RETRY);
}

void M4Revolution::fixLoading() {
	// always delete the temporary file when done
	SCOPE_EXIT {
		std::filesystem::remove(Work::Output::FILE_NAME);
	};

	{
		std::ifstream inputFileStream;

		for (;;) {
			inputFileStream.open(Work::Output::DATA_PATH, std::ios::binary, _SH_DENYWR);

			if (inputFileStream.is_open()) {
				break;
			}

			RETRY_ERR(Work::Output::FILE_RETRY);
		}

		Ubi::BigFile::File inputFile = createInputFile(inputFileStream);

		Log log("Fixing Loading, this may take several minutes", inputFileStream, inputFile.size, logFileNames, true);

		// to avoid a sharing violation this must happen first before creating the output thread
		// as they will both write to the same temporary file
		#ifdef WINDOWS
		OPERATION_EXCEPTION_RETRY_ERR(replaceGfxTools(), StreamFailed, Work::Output::FILE_RETRY);
		#endif

		bool yield = true;
		std::thread outputThread(M4Revolution::outputThread, std::ref(tasks), std::ref(yield));

		fixLoading(inputFileStream, 0, inputFile, log);
		log.finishing();

		// necessary to wake up the output thread one last time at the end
		Work::FileTask::POINTER fileTaskPointer = std::make_shared<Work::FileTask>(-1, &inputFile);
		fileTaskPointer->complete();
		tasks.fileLock().get().push(fileTaskPointer);

		yield = false;
		outputThread.join();
	}

	if (Work::Backup::create(Work::Output::DATA_PATH.string().c_str())) {
		Work::Backup::log();
	}
}

bool M4Revolution::restoreBackup() {
	Work::Output::FILE_PATH filePath = 0;

	{
		std::ifstream inputFileStream;

		for (
			Work::Output::PATH_MAP::const_iterator pathMapIterator = Work::Output::FILE_PATH_MAP.begin();
			pathMapIterator != Work::Output::FILE_PATH_MAP.end();
			pathMapIterator++
		) {
			inputFileStream.open(Work::Backup::getPath(pathMapIterator->second), std::ios::binary, _SH_DENYWR);
			
			if (inputFileStream.is_open()) {
				filePath |= pathMapIterator->first;
			}

			inputFileStream.close();
		}

		Log log("Restoring Backup", inputFileStream);
	}

	if (!filePath) {
		consoleLog("No backup was found. Backups will automatically be created when other operations are performed.", 2);
		return false;
	}

	if (!consoleBool("Restoring the backup will revert any changes made by this tool. Would you like to restore the backup?", false)) {
		return false;
	}

	for (
		Work::Output::PATH_MAP::const_iterator pathMapIterator = Work::Output::FILE_PATH_MAP.begin();
		pathMapIterator != Work::Output::FILE_PATH_MAP.end();
		pathMapIterator++
	) {
		if (filePath & pathMapIterator->first) {
			Work::Backup::restore(pathMapIterator->second);
		}
	}
	return true;
}