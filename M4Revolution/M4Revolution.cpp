#include "M4Revolution.h"
#include <iostream>

const Ubi::BigFile::Path::VECTOR M4Revolution::AI_TRANSITION_FADE_PATH_VECTOR = {
		{{"common"}, "common.m4b"},
		{{"ai", "aitransitionfade"}, "ai_transition_fade.ai"}
};

M4Revolution::M4Revolution(const std::string &inputFileName) : inputFileStream(inputFileName, std::ios::binary) {
}

void M4Revolution::fixLoading(const std::string &outputFileName) {
	std::ofstream outputFileStream(outputFileName, std::ios::binary);

	Ubi::BigFile::File::VECTOR_ITERATOR_SET_MAP vectorIteratorSetMap = {};
	Ubi::BigFile::File::SIZE filesPosition = 0;
	Ubi::BigFile bigFile(inputFileStream, vectorIteratorSetMap, filesPosition);
	bigFile.write(outputFileStream);

	Ubi::BigFile::File::VECTOR_ITERATOR_SET::iterator vectorIteratorSetIterator = {};
	bool convert = false;

	for (
		Ubi::BigFile::File::VECTOR_ITERATOR_SET_MAP::iterator vectorIteratorSetMapIterator = vectorIteratorSetMap.begin();
		vectorIteratorSetMapIterator != vectorIteratorSetMap.end();
		vectorIteratorSetMapIterator++
	) {
		if (convert) {
			filesPosition = vectorIteratorSetMapIterator->first;
			convert = false;
		}

		Ubi::BigFile::File::VECTOR_ITERATOR_SET &vectorIteratorSet = vectorIteratorSetMapIterator->second;

		for (vectorIteratorSetIterator = vectorIteratorSet.begin(); vectorIteratorSetIterator != vectorIteratorSet.end(); vectorIteratorSetIterator++) {
			const Ubi::BigFile::File::VECTOR::iterator &fileVectorIterator = *vectorIteratorSetIterator;

			if (fileVectorIterator->type != Ubi::BigFile::File::TYPE::NONE) {
				convert = true;
				inputFileStream.seekg(filesPosition);
				copyFileStream(inputFileStream, outputFileStream, vectorIteratorSetMapIterator->first - filesPosition);

				// TODO: add conversion
				inputFileStream.seekg(fileVectorIterator->position);
				copyFileStream(inputFileStream, outputFileStream, fileVectorIterator->size);
			}
		}
	}

	inputFileStream.seekg(filesPosition);
	copyFileStream(inputFileStream, outputFileStream);
}