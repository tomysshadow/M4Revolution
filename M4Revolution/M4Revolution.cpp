#include "M4Revolution.h"
#include <iostream>

#include <nvimage/Image.h>
#include <nvcore/Utils.h>

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

M4Revolution::OutputHandler::OutputHandler(std::ofstream &outputFileStream) : outputFileStream(outputFileStream) {
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
		writeFileStreamSafe(outputFileStream, data, size);
	} catch (...) {
		return false;
	}

	this->size += size;
	return true;
}

void M4Revolution::ErrorHandler::error(nvtt::Error e) {
	consoleLog(nvtt::errorString(e), M4REVOLUTION_ERR);
	result = false;
}

void M4Revolution::convertZAP(std::ofstream &outputFileStream, Ubi::BigFile::File::SIZE &size) {
	nvtt::Context context;
	context.enableCudaAcceleration(!disableHardwareAcceleration);

	nvtt::InputOptions inputOptions;
	inputOptions.setMipmapGeneration(false); // not needed by game, waste of file size
	inputOptions.setAlphaMode(nvtt::AlphaMode_Transparency); // game requires 32-bit depth for all images

	{
		std::unique_ptr<unsigned char> data = std::unique_ptr<unsigned char>(new unsigned char[size]);
		readFileStreamSafe(inputFileStream, data.get(), size);

		zap_byte_t* out = 0;
		zap_size_t outSize = 0;
		zap_int_t outWidth = 0;
		zap_int_t outHeight = 0;

		SCOPE_EXIT {
			if (!freeZAP(out)) {
				throw std::runtime_error("Failed to Free ZAP");
			}
		};

		if (zap_load_memory(data.get(), ZAP_COLOR_FORMAT_RGBA32, &out, &outSize, &outWidth, &outHeight) != ZAP_ERROR_NONE) {
			throw std::runtime_error("Failed to Load ZAP From Memory");
		}

		const size_t COLOR32_SIZE = sizeof(nv::Color32);

		nv::Color32* color32Pointer = (nv::Color32*)out;

		// need to flip the endianness, NVTT expects BGR instead of RGB
		color32X(color32Pointer, outWidth * COLOR32_SIZE, outSize);

		inputOptions.setTextureLayout(nvtt::TextureType_2D, outWidth, outHeight);
		inputOptions.setMipmapData(color32Pointer, outWidth, outHeight);
	}

	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setFormat(nvtt::Format_BC3); // a.k.a. DXT5
	compressionOptions.setQuality(nvtt::Quality_Highest);

	nvtt::OutputOptions outputOptions;
	outputOptions.setContainer(nvtt::Container_DDS);

	OutputHandler outputHandler(outputFileStream);
	outputOptions.setOutputHandler(&outputHandler);

	ErrorHandler errorHandler;
	outputOptions.setErrorHandler(&errorHandler);

	if (!context.process(inputOptions, compressionOptions, outputOptions) || !errorHandler.result) {
		throw std::runtime_error("Failed to Process Context");
	}

	size = outputHandler.size;
}

void M4Revolution::fixLoading(std::ofstream &outputFileStream, Ubi::BigFile::File::SIZE &size, Log &log) {
	// positions into the streams for later reference
	std::streampos inputPosition = inputFileStream.tellg();
	std::streampos outputPosition = outputFileStream.tellp();

	// outputFilePosition is the position of a specific output file (for file.position assignment)
	// filePointerSetMap is a map where the keys are the file positions beginning to end, and values are sets of files at that position
	Ubi::BigFile::File::SIZE outputFilePosition = 0;
	Ubi::BigFile::File::POINTER_SET_MAP filePointerSetMap = {};
	Ubi::BigFile bigFile(inputFileStream, outputFilePosition, filePointerSetMap);

	// insert padding for the filesystem for now, we'll go back and write it later
	padFileStream(outputFileStream, outputFilePosition);

	// inputCopyPosition is the position of the files to copy
	// inputFilePosition is the position of a specific input file (for file.size calculation)
	Ubi::BigFile::File::SIZE inputCopyPosition = (Ubi::BigFile::File::SIZE)(inputFileStream.tellg() - inputPosition);
	Ubi::BigFile::File::SIZE inputFilePosition = inputCopyPosition;

	// convert keeps track of if we just converted any files within the inner, set loop
	// (in which case, inputCopyPosition is advanced)
	// countCopy is the count of the bytes to copy when copying files
	bool convert = false;
	std::streampos countCopy = 0;

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
					inputFileStream.seekg((std::streampos)inputCopyPosition + inputPosition);
					copyFileStream(inputFileStream, outputFileStream, countCopy);

					log.copied();
				}

				// we'll need to convert this file type
				convert = true;
			}

			// add the position of this file minus the last (includes any potential padding)
			// we can't just add the file size in case there is dead, padding space between files
			outputFilePosition += filePointerSetMapIterator->first - inputFilePosition;

			// if we are converting this or any previous file in the set
			if (convert) {
				// handle this file specifically
				inputFileStream.seekg((std::streampos)file.position + inputPosition);
				file.position = outputFilePosition;

				// these conversion functions update the file sizes passed in
				switch (file.type) {
					case Ubi::BigFile::File::TYPE::RECURSIVE:
					fixLoading(outputFileStream, file.size, log);
					break;
					case Ubi::BigFile::File::TYPE::ZAP:
					convertZAP(outputFileStream, file.size);
					break;
					default:
					// either a file we need to copy at the same position as ones we need to convert, or is a type not yet implemented
					copyFileStream(inputFileStream, outputFileStream, file.size);
				}
				
				// here this is safe because this is the exact amount written to the stream, so there is guaranteed to be no padding between
				outputFilePosition += file.size;

				log.converted(file);
			} else {
				// other identical, copied files at the same position in the input should likewise be at the same position in the output
				inputFilePosition = filePointerSetMapIterator->first;
				file.position = outputFilePosition;

				log.step();
			}
		}
	}

	// if we just converted a set of files then there are no remaining files to copy, but otherwise...
	if (!convert) {
		countCopy = size - inputCopyPosition;

		// copy any remaining files
		if (countCopy) {
			inputFileStream.seekg((std::streampos)inputCopyPosition + inputPosition);
			copyFileStream(inputFileStream, outputFileStream, countCopy);

			log.copied();
		}

		// account for this in outputFilePosition so that the file size will be correct
		outputFilePosition += size - inputFilePosition;
	}

	// go back and write the filesystem now that all the positions and sizes are correct
	outputFileStream.seekp(outputPosition);
	bigFile.write(outputFileStream);

	// seek to the end again and give the caller the new file size
	outputFileStream.seekp((std::streampos)outputFilePosition + outputPosition);
	size = outputFilePosition;
}

void M4Revolution::color32X(nv::Color32* color32Pointer, size_t stride, size_t size) {
	nv::Color32* endPointer = (nv::Color32*)((uint8*)color32Pointer + size) - 1;
	nv::Color32* rowPointer = 0;

	while (color32Pointer <= endPointer) {
		rowPointer = (nv::Color32*)((uint8*)color32Pointer + stride) - 1;

		while (color32Pointer <= rowPointer) {
			nv::swap(color32Pointer->r, color32Pointer->b);

			color32Pointer = color32Pointer + 1;
		}

		color32Pointer = rowPointer + 1;
	}
}

const Ubi::BigFile::Path::VECTOR M4Revolution::AI_TRANSITION_FADE_PATH_VECTOR = {
		{{"common"}, "common.m4b"},
		{{"ai", "aitransitionfade"}, "ai_transition_fade.ai"}
};

M4Revolution::M4Revolution(const std::string &inputFileName, bool disableHardwareAcceleration, bool logFileNames)
: inputFileStream(inputFileName, std::ios::binary),
disableHardwareAcceleration(disableHardwareAcceleration),
logFileNames(logFileNames) {
}

void M4Revolution::fixLoading(const std::string &outputFileName) {
	inputFileStream.seekg(0, std::ios::end);
	Ubi::BigFile::File::SIZE inputFileSize = (Ubi::BigFile::File::SIZE)inputFileStream.tellg();

	inputFileStream.seekg(0, std::ios::beg);
	std::ofstream outputFileStream(outputFileName, std::ios::binary);

	Log log("Fixing Loading", inputFileStream, inputFileSize, logFileNames);
	fixLoading(outputFileStream, inputFileSize, log);
}