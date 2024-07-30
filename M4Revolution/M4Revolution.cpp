#include "M4Revolution.h"
#include <iostream>

void M4Revolution::convertZAP(std::ofstream &outputFileStream, Ubi::BigFile::File::SIZE &size) {
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

	size = (Ubi::BigFile::File::SIZE)outSize;
	writeFileStreamSafe(outputFileStream, out, size);
}

void M4Revolution::fixLoading(std::ofstream &outputFileStream, Ubi::BigFile::File::SIZE &size) {
	std::streampos beg = inputFileStream.tellg();
	std::streampos end = beg + (std::streampos)size;

	Ubi::BigFile::File::POINTER_SET_MAP filePointerSetMap = {};
	Ubi::BigFile::File::SIZE filesPosition = 0;
	Ubi::BigFile bigFile(inputFileStream, filePointerSetMap, filesPosition);
	bigFile.write(outputFileStream);

	Ubi::BigFile::File::POINTER_SET::iterator filePointerSetIterator = {};
	bool convert = false;
	int filesCopied = files;

	for (
		Ubi::BigFile::File::POINTER_SET_MAP::iterator filePointerSetMapIterator = filePointerSetMap.begin();
		filePointerSetMapIterator != filePointerSetMap.end();
		filePointerSetMapIterator++
		) {
		if (convert) {
			filesPosition = filePointerSetMapIterator->first;
			convert = false;
		}

		Ubi::BigFile::File::POINTER_SET &filePointerSet = filePointerSetMapIterator->second;

		for (filePointerSetIterator = filePointerSet.begin(); filePointerSetIterator != filePointerSet.end(); filePointerSetIterator++) {
			Ubi::BigFile::File &file = **filePointerSetIterator;

			if (file.type == Ubi::BigFile::File::TYPE::NONE) {
				++files;
			} else {
				filesCopied = files - filesCopied;

				if (filesCopied) {
					std::cout << "Copied " << filesCopied << " file(s)\n";
				}

				filesCopied = ++files;

				convert = true;
				inputFileStream.seekg(beg + (std::streampos)filesPosition);
				copyFileStream(inputFileStream, outputFileStream, filePointerSetMapIterator->first - filesPosition);

				inputFileStream.seekg(beg + (std::streampos)file.position);
				file.position = (Ubi::BigFile::File::SIZE)outputFileStream.tellp();

				switch (file.type) {
					case Ubi::BigFile::File::TYPE::RECURSIVE:
					fixLoading(outputFileStream, file.size);
					break;
					case Ubi::BigFile::File::TYPE::ZAP:
					convertZAP(outputFileStream, file.size);
					break;
					default:
					// not yet implemented
					copyFileStream(inputFileStream, outputFileStream, file.size);
				}

				if (file.nameOptional.has_value()) {
					std::cout << "Converted \"" << file.nameOptional.value() << "\"\n";
				} else {
					std::cout << "Converted file at position " << file.position << ", size " << file.size << "\n";
				}
			}
		}
	}

	inputFileStream.seekg(beg + (std::streampos)filesPosition);
	copyFileStream(inputFileStream, outputFileStream, end - inputFileStream.tellg());
	size = (Ubi::BigFile::File::SIZE)(outputFileStream.tellp() - beg);
}

const Ubi::BigFile::Path::VECTOR M4Revolution::AI_TRANSITION_FADE_PATH_VECTOR = {
		{{"common"}, "common.m4b"},
		{{"ai", "aitransitionfade"}, "ai_transition_fade.ai"}
};

M4Revolution::M4Revolution(const std::string &inputFileName) : inputFileStream(inputFileName, std::ios::binary) {
}

void M4Revolution::fixLoading(const std::string &outputFileName) {
	files = 0;

	inputFileStream.seekg(0, std::ios::end);
	Ubi::BigFile::File::SIZE size = (Ubi::BigFile::File::SIZE)(inputFileStream.tellg());

	inputFileStream.seekg(0, std::ios::beg);
	std::ofstream outputFileStream(outputFileName, std::ios::binary);

	fixLoading(outputFileStream, size);
}