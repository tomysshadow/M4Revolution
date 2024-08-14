#include "Ubi.h"

std::optional<std::string> Ubi::String::readOptional(std::ifstream &inputFileStream) {
	SIZE size = 0;
	readFileStreamSafe(inputFileStream, &size, SIZE_SIZE);

	if (!size) {
		return std::nullopt;
	}

	std::unique_ptr<char> str = std::unique_ptr<char>(new char[size]);
	readFileStreamSafe(inputFileStream, str.get(), size);
	return str.get();
}

void Ubi::String::writeOptional(std::ofstream &outputFileStream, const std::optional<std::string> &strOptional) {
	SIZE size = strOptional.has_value() ? (SIZE)strOptional.value().size() + 1 : 0;
	writeFileStreamSafe(outputFileStream, &size, SIZE_SIZE);

	if (!size) {
		return;
	}

	writeFileStreamSafe(outputFileStream, strOptional.value().c_str(), size);
}

Ubi::BigFile::File::File(std::ifstream &inputFileStream, SIZE &fileSystemSize, bool texture) {
	read(inputFileStream);

	#ifdef CONVERSION_ENABLED
	// predetermines what the new name will be after conversion
	// this is necessary so we will know the position of the files before writing them
	if (nameOptional.has_value()) {
		const std::string::size_type PERIOD_SIZE = sizeof(PERIOD);

		std::string nameExtension = getNameExtension();

		// note that these are case sensitive, because Myst 4 also uses case sensitive filenames
		TYPE_EXTENSION_MAP::const_iterator nameTypeExtensionMapIterator = NAME_TYPE_EXTENSION_MAP.find(nameExtension);

		if (nameTypeExtensionMapIterator != NAME_TYPE_EXTENSION_MAP.end()) {
			type = nameTypeExtensionMapIterator->second.type;

			const std::string &NAME = nameOptional.value();
			const std::string &EXTENSION = nameTypeExtensionMapIterator->second.extension;

			nameOptional = NAME.substr(
				0,
				NAME.length() - EXTENSION.length() - PERIOD_SIZE
			)

			+ PERIOD
			+ EXTENSION;
		}
	}
	#endif

	fileSystemSize += (SIZE)(
		String::SIZE_SIZE

		+ (
			nameOptional.has_value()
			? nameOptional.value().size() + 1
			: 0
		)

		+ SIZE_SIZE
		+ POSITION_SIZE
	);
}

Ubi::BigFile::File::File(std::ifstream &inputFileStream) {
	read(inputFileStream);
}

Ubi::BigFile::File::File(SIZE inputFileSize) : size(inputFileSize) {
}

void Ubi::BigFile::File::write(std::ofstream &outputFileStream) const {
	String::writeOptional(outputFileStream, nameOptional);
	writeFileStreamSafe(outputFileStream, &size, SIZE_SIZE);
	writeFileStreamSafe(outputFileStream, &position, POSITION_SIZE);
}

void Ubi::BigFile::File::read(std::ifstream &inputFileStream) {
	nameOptional = String::readOptional(inputFileStream);
	readFileStreamSafe(inputFileStream, &size, SIZE_SIZE);
	readFileStreamSafe(inputFileStream, &position, POSITION_SIZE);
}

std::string Ubi::BigFile::File::getNameExtension() {
	const std::string::size_type PERIOD_SIZE = sizeof(PERIOD);
	const std::string &NAME = nameOptional.value();

	std::string::size_type periodIndex = NAME.rfind(PERIOD);

	return periodIndex == std::string::npos
	? ""

	: NAME.substr(
		periodIndex + PERIOD_SIZE,
		std::string::npos
	);
}

const Ubi::BigFile::File::TYPE_EXTENSION_MAP Ubi::BigFile::File::NAME_TYPE_EXTENSION_MAP = {
	{"m4b", {TYPE::BIG_FILE, "m4b"}},
	//{"jpg", {TYPE::JPEG, "dds"}}, // not yet implemented
	{"zap", {TYPE::ZAP, "dds"}},
	{"bin", {TYPE::TEXTURE, "bin"}}
};

Ubi::BigFile::Directory::Directory(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &filePointerSetMap)
	: nameOptional(String::readOptional(inputFileStream)) {
	DIRECTORY_VECTOR_SIZE directoryVectorSize = 0;
	readFileStreamSafe(inputFileStream, &directoryVectorSize, DIRECTORY_VECTOR_SIZE_SIZE);

	for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
		directoryVector.emplace_back(
			inputFileStream,
			fileSystemSize,
			files,
			filePointerSetMap
		);
	}

	FILE_VECTOR_SIZE fileVectorSize = 0;
	readFileStreamSafe(inputFileStream, &fileVectorSize, FILE_VECTOR_SIZE_SIZE);

	// the NAME_TEXTURE comparison is so we only convert textures in the texture folder specifically, even if the extension matches
	for (FILE_VECTOR_SIZE i = 0; i < fileVectorSize; i++) {
		fileVector.emplace_back(
			inputFileStream,
			fileSystemSize,

			nameOptional.has_value()
			? nameOptional.value() == NAME_TEXTURE
			: false
		);
	}

	files += fileVectorSize;

	// MUST BE DONE SEPERATELY AFTERWARDS!
	// (otherwise iterator invalidation)
	File::POINTER_SET_MAP::iterator filePointerSetMapIterator = {};

	for (File::VECTOR::iterator fileVectorIterator = fileVector.begin(); fileVectorIterator != fileVector.end(); fileVectorIterator++) {
		// are there any other files at this position?
		filePointerSetMapIterator = filePointerSetMap.find(fileVectorIterator->position);

		// if not, then create a new set
		if (filePointerSetMapIterator == filePointerSetMap.end()) {
			filePointerSetMapIterator = filePointerSetMap.insert({fileVectorIterator->position, {}}).first;
		}

		// add this file to the set
		filePointerSetMapIterator->second.insert(&*fileVectorIterator);
	}

	fileSystemSize += (File::SIZE)(
		String::SIZE_SIZE

		+ (
			nameOptional.has_value()
			? nameOptional.value().size() + 1
			: 0
		)

		+ DIRECTORY_VECTOR_SIZE_SIZE
		+ FILE_VECTOR_SIZE_SIZE
	);
}

Ubi::BigFile::Directory::Directory(std::ifstream &inputFileStream, const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, std::optional<File> &fileOptional) {
	MAKE_SCOPE_EXIT(fileOptionalScopeExit) {
		fileOptional = std::nullopt;
	};

	nameOptional = String::readOptional(inputFileStream);

	if (!nameOptional.has_value()) {
		return;
	}

	const Path::NAME_VECTOR &DIRECTORY_NAME_VECTOR = path.directoryNameVector;

	bool match = false;

	// should we care about this directory at all?
	if (directoryNameVectorIterator != DIRECTORY_NAME_VECTOR.end()) {
		// does this directory's name match the one we are trying to find?
		if (!nameOptional.has_value() || nameOptional.value() == *directoryNameVectorIterator) {
			// look for matching subdirectories, or
			// if there are no further subdirectories, look for matching files
			match = ++directoryNameVectorIterator == DIRECTORY_NAME_VECTOR.end();
		} else {
			// don't look for matching subdirectories or files
			directoryNameVectorIterator = DIRECTORY_NAME_VECTOR.end();
		}
	}

	DIRECTORY_VECTOR_SIZE directoryVectorSize = 0;
	readFileStreamSafe(inputFileStream, &directoryVectorSize, DIRECTORY_VECTOR_SIZE_SIZE);

	if (directoryNameVectorIterator == DIRECTORY_NAME_VECTOR.end()) {
		// in this case we just read the directories and don't bother checking fileOptional
		for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
			Directory directory(
				inputFileStream,
				path,
				directoryNameVectorIterator,
				fileOptional
			);
		}
	} else {
		for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
			directoryVector.emplace_back(
				inputFileStream,
				path,
				directoryNameVectorIterator,
				fileOptional
			);

			// if this is true we found the matching file, so exit early
			if (fileOptional.has_value()) {
				// erase all but the last element
				// (there should always be at least one element in the vector at this point)
				directoryVector.erase(directoryVector.begin(), directoryVector.end() - 1);
				return;
			}
		}

		directoryVector = {};
	}

	FILE_VECTOR_SIZE fileVectorSize = 0;
	readFileStreamSafe(inputFileStream, &fileVectorSize, FILE_VECTOR_SIZE_SIZE);

	if (match) {
		for (FILE_VECTOR_SIZE i = 0; i < fileVectorSize; i++) {
			File &file = fileVector.emplace_back(
				inputFileStream
			);

			// is this the file we are looking for?
			const std::optional<std::string> &NAME_OPTIONAL = file.nameOptional;

			if (NAME_OPTIONAL.has_value() && NAME_OPTIONAL.value() == path.fileName) {
				// erase all but the last element
				// (there should always be at least one element in the vector at this point)
				fileVector.erase(fileVector.begin(), fileVector.end() - 1);
				fileOptional = file;
				fileOptionalScopeExit.dismiss();
				return;
			}
		}

		fileVector = {};
	} else {
		for (FILE_VECTOR_SIZE i = 0; i < fileVectorSize; i++) {
			File file(inputFileStream);
		}
	}
}

void Ubi::BigFile::Directory::write(std::ofstream &outputFileStream) const {
	String::writeOptional(outputFileStream, nameOptional);

	DIRECTORY_VECTOR_SIZE directoryVectorSize = (DIRECTORY_VECTOR_SIZE)directoryVector.size();
	writeFileStreamSafe(outputFileStream, &directoryVectorSize, DIRECTORY_VECTOR_SIZE_SIZE);

	for (Directory::VECTOR::const_iterator directoryVectorIterator = directoryVector.begin(); directoryVectorIterator != directoryVector.end(); directoryVectorIterator++) {
		directoryVectorIterator->write(outputFileStream);
	}

	FILE_VECTOR_SIZE fileVectorSize = (FILE_VECTOR_SIZE)fileVector.size();
	writeFileStreamSafe(outputFileStream, &fileVectorSize, FILE_VECTOR_SIZE_SIZE);

	for (File::VECTOR::const_iterator fileVectorIterator = fileVector.begin(); fileVectorIterator != fileVector.end(); fileVectorIterator++) {
		fileVectorIterator->write(outputFileStream);
	}
}

const std::string Ubi::BigFile::Directory::NAME_TEXTURE = "texture";

Ubi::BigFile::Header::Header(std::ifstream &inputFileStream, File::SIZE &fileSystemSize) {
	read(inputFileStream);

	fileSystemSize += (File::SIZE)(
		String::SIZE_SIZE

		+ SIGNATURE.size() + 1
		+ VERSION_SIZE
	);
}

Ubi::BigFile::Header::Header(std::ifstream &inputFileStream, std::optional<File> &fileOptional) {
	// for path vectors
	if (fileOptional.has_value()) {
		inputFileStream.seekg(fileOptional.value().position);
	}

	read(inputFileStream);
}

void Ubi::BigFile::Header::write(std::ofstream &outputFileStream) const {
	String::writeOptional(outputFileStream, SIGNATURE);
	writeFileStreamSafe(outputFileStream, &CURRENT_VERSION, VERSION_SIZE);
}

void Ubi::BigFile::Header::read(std::ifstream &inputFileStream) {
	std::optional<std::string> signatureOptional = String::readOptional(inputFileStream);

	// must exactly match, case sensitively
	if (!signatureOptional.has_value() || signatureOptional.value() != SIGNATURE) {
		throw Invalid();
	}

	VERSION version = 0;
	readFileStreamSafe(inputFileStream, &version, VERSION_SIZE);

	if (version != CURRENT_VERSION) {
		throw Invalid();
	}
}

const std::string Ubi::BigFile::Header::SIGNATURE = "UBI_BF_SIG";

Ubi::BigFile::BigFile(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &filePointerSetMap)
	: header(inputFileStream, fileSystemSize),
	directory(inputFileStream, fileSystemSize, files, filePointerSetMap) {
}

Ubi::BigFile::BigFile(std::ifstream &inputFileStream, const Path &path, std::optional<File> &fileOptional)
	: header(inputFileStream, fileOptional),
	directory(inputFileStream, path, path.directoryNameVector.begin(), fileOptional) {
}

void Ubi::BigFile::write(std::ofstream &outputFileStream) const {
	header.write(outputFileStream);
	directory.write(outputFileStream);
}