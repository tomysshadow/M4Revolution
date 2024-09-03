#include "Ubi.h"

std::optional<std::string> Ubi::String::readOptional(std::ifstream &inputFileStream, bool &nullTerminator) {
	MAKE_SCOPE_EXIT(nullTerminatorScopeExit) {
		nullTerminator = true;
	};

	SIZE size = 0;
	readFileStreamSafe(inputFileStream, &size, SIZE_SIZE);

	if (!size) {
		return std::nullopt;
	}

	std::unique_ptr<char> strPointer = std::unique_ptr<char>(new char[(size_t)size + 1]);
	char* str = strPointer.get();
	readFileStreamSafe(inputFileStream, str, size);
	nullTerminator = !str[size - 1];
	str[size] = 0;
	nullTerminatorScopeExit.dismiss();
	return str;
}

std::optional<std::string> Ubi::String::readOptional(std::ifstream &inputFileStream) {
	bool nullTerminator = true;
	return readOptional(inputFileStream, nullTerminator);
}

void Ubi::String::writeOptional(std::ofstream &outputFileStream, const std::optional<std::string> &strOptional, bool nullTerminator) {
	SIZE size = strOptional.has_value() ? (SIZE)strOptional.value().size() + nullTerminator : 0;
	writeFileStreamSafe(outputFileStream, &size, SIZE_SIZE);

	if (!size) {
		return;
	}

	writeFileStreamSafe(outputFileStream, strOptional.value().c_str(), size);
}

std::string &Ubi::String::swizzle(std::string &encryptedString) {
	const char MASK = 85;

	for (std::string::iterator encryptedStringIterator = encryptedString.begin(); encryptedStringIterator != encryptedString.end(); encryptedStringIterator++) {
		char &encryptedChar = *encryptedStringIterator;
		char encryptedCharLeft = encryptedChar << 1;
		char encryptedCharRight = encryptedChar >> 1;

		encryptedChar = (encryptedCharLeft ^ encryptedCharRight) & MASK ^ encryptedCharLeft;
	}
	return encryptedString;
}

Ubi::Binary::RLE::SLICE_MAP Ubi::Binary::RLE::readFile(std::ifstream &inputFileStream, std::streamsize size) {
	Header header(inputFileStream, size);

	uint32_t waterSlices = 0;

	ROW sliceRow = 0;
	COL sliceCol = 0;

	SLICE_MAP sliceMap = {};
	SLICE_MAP::iterator sliceMapIterator = {};

	uint32_t waterRLERegions = 0;

	uint32_t groups = 0;
	uint32_t subGroups = 0;

	uint32_t pixels = 0;
	std::streamsize pixelsSize = 0;

	const size_t WATER_FACE_FIELDS_SIZE = 20; // Type, Width, Height, SliceWidth, SliceHeight
	const size_t WATER_SLICES_SIZE = sizeof(waterSlices);
	const size_t SLICE_ROW_SIZE = sizeof(sliceRow);
	const size_t SLICE_COL_SIZE = sizeof(sliceCol);
	const size_t WATER_SLICE_FIELDS_SIZE = 8; // Width, Height
	const size_t WATER_RLE_REGIONS_SIZE = sizeof(waterRLERegions);
	const size_t WATER_RLE_REGION_FIELDS_SIZE = 20; // TextureCoordsInFace (X, Y,) TextureCoordsInSlice (X, Y,) RegionSize
	const size_t GROUPS_SIZE = sizeof(groups);
	const size_t WATER_RLE_REGION_GROUP_FIELDS_SIZE = 4; // Unknown
	const size_t SUB_GROUPS_SIZE = sizeof(subGroups);
	const size_t PIXELS_SIZE = sizeof(pixels);

	inputFileStream.seekg(WATER_FACE_FIELDS_SIZE, std::ios::cur);
	readFileStreamSafe(inputFileStream, &waterSlices, WATER_SLICES_SIZE);

	for (uint32_t i = 0; i < waterSlices; i++) {
		readFileStreamSafe(inputFileStream, &sliceRow, SLICE_ROW_SIZE);
		sliceMapIterator = sliceMap.find(sliceRow);

		if (sliceMapIterator == sliceMap.end()) {
			sliceMapIterator = sliceMap.insert({ sliceRow, {} }).first;
		}

		readFileStreamSafe(inputFileStream, &sliceCol, SLICE_COL_SIZE);
		sliceMapIterator->second.insert(sliceCol);

		inputFileStream.seekg(WATER_SLICE_FIELDS_SIZE, std::ios::cur);
		readFileStreamSafe(inputFileStream, &waterRLERegions, WATER_RLE_REGIONS_SIZE);

		for (uint32_t j = 0; j < waterRLERegions; j++) {
			inputFileStream.seekg(WATER_RLE_REGION_FIELDS_SIZE, std::ios::cur);
			readFileStreamSafe(inputFileStream, &groups, GROUPS_SIZE);

			for (uint32_t l = 0; l < groups; l++) {
				inputFileStream.seekg(WATER_RLE_REGION_GROUP_FIELDS_SIZE, std::ios::cur);
				readFileStreamSafe(inputFileStream, &subGroups, SUB_GROUPS_SIZE);

				for (uint32_t m = 0; m < subGroups; m++) {
					readFileStreamSafe(inputFileStream, &pixels, PIXELS_SIZE);

					pixelsSize = pixels;
					inputFileStream.seekg(pixelsSize + pixelsSize, std::ios::cur);
				}
			}
		}
	}
	return sliceMap;
}

Ubi::Binary::Resource::Loader::Loader(std::ifstream &inputFileStream) {
	const size_t ID_SIZE = sizeof(id);
	const size_t VERSION_SIZE = sizeof(version);

	readFileStreamSafe(inputFileStream, &id, ID_SIZE);
	readFileStreamSafe(inputFileStream, &version, VERSION_SIZE);
	name = String::readOptional(inputFileStream);

	if (name.has_value()) {
		String::swizzle(name.value());
	}
}

Ubi::Binary::Resource::Resource(Loader::POINTER loaderPointer)
	: loaderPointer(loaderPointer) {
}

Ubi::Binary::InteractiveOffsetProvider::InteractiveOffsetProvider(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer) {
	const size_t FIELDS_SIZE = 33;
	inputFileStream.seekg(FIELDS_SIZE, std::ios::cur);
}

Ubi::Binary::TextureAlignedOffsetProvider::TextureAlignedOffsetProvider(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer) {
	const size_t FIELDS_SIZE = 65;
	inputFileStream.seekg(FIELDS_SIZE, std::ios::cur);
}

void Ubi::Binary::StateData::create(std::ifstream &inputFileStream, RLE::MASK_NAME_SET &maskNameSet) {
	uint32_t nbrAliases = 0;
	const size_t NBR_ALIASES_SIZE = sizeof(nbrAliases);

	readFileStreamSafe(inputFileStream, &nbrAliases, NBR_ALIASES_SIZE);

	for (uint32_t i = 0; i < nbrAliases; i++) {
		String::readOptional(inputFileStream);
	}

	const size_t REFRESH_RATE_SIZE = 4;
	inputFileStream.seekg(REFRESH_RATE_SIZE, std::ios::cur);

	maskFileOptional = String::readOptional(inputFileStream);

	if (maskFileOptional.has_value()) {
		maskNameSet.insert(String::swizzle(maskFileOptional.value()));
	}

	uint32_t resources = 0;
	const size_t RESOURCES_SIZE = sizeof(resources);

	readFileStreamSafe(inputFileStream, &resources, RESOURCES_SIZE);

	for (uint32_t i = 0; i < resources; i++) {
		createResourcePointer(inputFileStream);
	}

	const size_t WATER_FACE_BILERP_FIELDS_SIZE = 6;
	inputFileStream.seekg(WATER_FACE_BILERP_FIELDS_SIZE, std::ios::cur);
}

Ubi::Binary::StateData::StateData(Loader::POINTER loaderPointer, std::ifstream &inputFileStream, RLE::MASK_NAME_SET &maskNameSet)
	: Resource(loaderPointer) {
	create(inputFileStream, maskNameSet);
}

Ubi::Binary::StateData::StateData(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer) {
	RLE::MASK_NAME_SET maskNameSet = {};
	create(inputFileStream, maskNameSet);
}

void Ubi::Binary::Water::create(std::ifstream &inputFileStream, RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap) {
	textureBoxNameOptional = String::readOptional(inputFileStream);

	if (!textureBoxNameOptional.has_value()) {
		return;
	}

	const size_t WATER_FIELDS_SIZE = 9; // AssignReflectionAlpha, ReflectionAlphaAtEdge, ReflectionAlphaAtHorizon
	inputFileStream.seekg(WATER_FIELDS_SIZE, std::ios::cur);

	uint32_t resources = 0;
	const size_t RESOURCES_SIZE = sizeof(resources);

	readFileStreamSafe(inputFileStream, &resources, RESOURCES_SIZE);

	RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP::iterator textureBoxNameMaskVariantMapIterator = {};

	std::string &textureBoxName = textureBoxNameOptional.value();
	String::swizzle(textureBoxName);

	for (uint32_t i = 0; i < resources; i++) {
		textureBoxNameMaskVariantMapIterator = textureBoxNameMaskVariantMap.find(textureBoxName);

		if (textureBoxNameMaskVariantMapIterator == textureBoxNameMaskVariantMap.end()) {
			textureBoxNameMaskVariantMapIterator = textureBoxNameMaskVariantMap.insert({ textureBoxName, {} }).first;
		}

		getMaskNameSet(inputFileStream, std::get<RLE::MASK_NAME_SET>(textureBoxNameMaskVariantMapIterator->second));
	}
}

Ubi::Binary::Water::Water(Loader::POINTER loaderPointer, std::ifstream &inputFileStream, RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap)
	: Resource(loaderPointer) {
	create(inputFileStream, textureBoxNameMaskVariantMap);
}

Ubi::Binary::Water::Water(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer) {
	RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP textureBoxNameMaskVariantMap = {};
	create(inputFileStream, textureBoxNameMaskVariantMap);
}

void Ubi::Binary::Header::testReadPastEnd() {
	if (fileSize < inputFileStream.tellg() - filePosition) {
		throw ReadPastEnd();
	}
}

Ubi::Binary::Header::Header(std::ifstream &inputFileStream, std::streamsize fileSize)
	: inputFileStream(inputFileStream),
	fileSize(fileSize),
	filePosition(inputFileStream.tellg()) {
	ID id = 0;
	const size_t ID_SIZE = sizeof(id);

	readFileStreamSafe(inputFileStream, &id, ID_SIZE);
	testReadPastEnd();

	if (id != UBI_B0_L) {
		throw Invalid();
	}
}

Ubi::Binary::Header::~Header() {
	testReadPastEnd();
}

Ubi::Binary::Resource::POINTER Ubi::Binary::createResourcePointer(std::ifstream &inputFileStream) {
	Resource::Loader::POINTER loaderPointer = std::make_shared<Resource::Loader>(inputFileStream);

	switch (loaderPointer->id) {
		//case TextureBox::ID:
		//return std::make_shared<TextureBox>(loaderPointer, inputFileStream);
		case Water::ID:
		return std::make_shared<Water>(loaderPointer, inputFileStream);
		case StateData::ID:
		return std::make_shared<StateData>(loaderPointer, inputFileStream);
		case InteractiveOffsetProvider::ID:
		return std::make_shared<InteractiveOffsetProvider>(loaderPointer, inputFileStream);
		case TextureAlignedOffsetProvider::ID:
		return std::make_shared<TextureAlignedOffsetProvider>(loaderPointer, inputFileStream);
	}
	return 0;
}

void Ubi::Binary::getMaskNameSet(std::ifstream &inputFileStream, RLE::MASK_NAME_SET &maskNameSet) {
	Resource::Loader::POINTER loaderPointer = std::make_shared<Resource::Loader>(inputFileStream);

	switch (loaderPointer->id) {
		case StateData::ID:
		StateData stateData(loaderPointer, inputFileStream, maskNameSet);
		return;
	}

	throw NotImplemented();
}

void Ubi::Binary::readFile(std::ifstream &inputFileStream, std::streamsize size, RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap) {
	Header header(inputFileStream, size);

	Resource::Loader::POINTER loaderPointer = std::make_shared<Resource::Loader>(inputFileStream);

	switch (loaderPointer->id) {
		case Water::ID:
		Water water(loaderPointer, inputFileStream, textureBoxNameMaskVariantMap);
		return;
	}

	throw NotImplemented();
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

			if (texture && type == TYPE::BINARY) {
				type = TYPE::BINARY_RESOURCE_IMAGE_DATA;
			} else {
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

void Ubi::BigFile::File::appendToTextureBoxNameMaskVariantMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap) {
	std::streampos position = inputFileStream.tellg();
	
	try {
		inputFileStream.seekg(fileSystemPosition + (std::streampos)this->position);
		Binary::readFile(inputFileStream, this->size, textureBoxNameMaskVariantMap);
	} catch (...) {
		// fail silently
	}

	inputFileStream.seekg(position);
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
	{"bin", {TYPE::BINARY, "bin"}},
	{"jpg", {TYPE::JPEG, "dds"}},
	{"zap", {TYPE::ZAP, "dds"}}
};

Ubi::BigFile::Directory::Directory(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::SIZE &fileSystemPosition, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &filePointerSetMap)
	: nameOptional(String::readOptional(inputFileStream)) {
	DIRECTORY_VECTOR_SIZE directoryVectorSize = 0;
	readFileStreamSafe(inputFileStream, &directoryVectorSize, DIRECTORY_VECTOR_SIZE_SIZE);

	FILE_POINTER_VECTOR_SIZE_VECTOR cubeIndices = {};
	FILE_POINTER_VECTOR_SIZE_VECTOR waterIndices = {};

	for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
		Directory &directory = directoryVector.emplace_back(
			inputFileStream,
			fileSystemSize,
			fileSystemPosition,
			files,
			filePointerSetMap
		);

		const std::optional<std::string> &NAME_OPTIONAL = directory.nameOptional;

		if (NAME_OPTIONAL.has_value()) {
			const std::string &NAME = NAME_OPTIONAL.value();

			if (NAME == "cube") {
				cubeIndices.push_back(i);
			} else if (NAME == "water") {
				waterIndices.push_back(i);
			}
		}
	}

	if (!cubeIndices.empty()) {
		Binary::RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP textureBoxNameMaskVariantMap = {};

		for (
			FILE_POINTER_VECTOR_SIZE_VECTOR::iterator waterIndicesIterator = waterIndices.begin();
			waterIndicesIterator != waterIndices.end();
			waterIndicesIterator++
		) {
			directoryVector[*waterIndicesIterator].appendToTextureBoxNameMaskVariantMap(inputFileStream, fileSystemPosition, textureBoxNameMaskVariantMap);

			// TODO: we now need to navigate down textureBoxNameMaskVariantMap
			// find the texture box (from cube directory) with the matching name to each key
			// as well as find each RLE file to read
			// then assign all this information onto the cube
		}
	}

	FILE_POINTER_VECTOR_SIZE filePointerVectorSize = 0;
	readFileStreamSafe(inputFileStream, &filePointerVectorSize, FILE_POINTER_VECTOR_SIZE_SIZE);

	File::POINTER filePointer = 0;
	File::POINTER_SET_MAP::iterator filePointerSetMapIterator = {};

	// the NAME_TEXTURE comparison is so we only convert textures in the texture folder specifically, even if the extension matches
	for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
		filePointer = std::make_shared<File>(
			inputFileStream,
			fileSystemSize,

			nameOptional.has_value()
			? nameOptional.value() == NAME_TEXTURE
			: false
		);

		File &file = *filePointer;

		if (file.type == File::TYPE::BINARY) {
			binaryFilePointerVector.push_back(filePointer);
		} else {
			filePointerVector.push_back(filePointer);
		}

		// are there any other files at this position?
		filePointerSetMapIterator = filePointerSetMap.find(file.position);

		// if not, then create a new set
		if (filePointerSetMapIterator == filePointerSetMap.end()) {
			filePointerSetMapIterator = filePointerSetMap.insert({ file.position, {} }).first;
		}

		// add this file to the set
		filePointerSetMapIterator->second.insert(filePointer);
	}

	files += filePointerVectorSize;

	fileSystemSize += (File::SIZE)(
		String::SIZE_SIZE

		+ (
			nameOptional.has_value()
			? nameOptional.value().size() + 1
			: 0
		)

		+ DIRECTORY_VECTOR_SIZE_SIZE
		+ FILE_POINTER_VECTOR_SIZE_SIZE
	);
}

Ubi::BigFile::Directory::Directory(std::ifstream &inputFileStream, const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, std::optional<File> &fileOptional) {
	MAKE_SCOPE_EXIT(fileOptionalScopeExit) {
		fileOptional = std::nullopt;
	};

	const Path::NAME_VECTOR &DIRECTORY_NAME_VECTOR = path.directoryNameVector;

	nameOptional = String::readOptional(inputFileStream);

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

	FILE_POINTER_VECTOR_SIZE filePointerVectorSize = 0;
	readFileStreamSafe(inputFileStream, &filePointerVectorSize, FILE_POINTER_VECTOR_SIZE_SIZE);

	if (match) {
		File::POINTER filePointer = 0;

		for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
			filePointer = std::make_shared<File>(inputFileStream);

			filePointerVector.push_back(filePointer);
			File &file = *filePointer;

			// is this the file we are looking for?
			const std::optional<std::string> &NAME_OPTIONAL = file.nameOptional;

			if (NAME_OPTIONAL.has_value() && NAME_OPTIONAL.value() == path.fileName) {
				// erase all but the last element
				// (there should always be at least one element in the vector at this point)
				filePointerVector.erase(filePointerVector.begin(), filePointerVector.end() - 1);
				fileOptional = file;
				fileOptionalScopeExit.dismiss();
				return;
			}
		}

		filePointerVector = {};
	} else {
		for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
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

	FILE_POINTER_VECTOR_SIZE fileVectorSize = (FILE_POINTER_VECTOR_SIZE)filePointerVector.size();
	writeFileStreamSafe(outputFileStream, &fileVectorSize, FILE_POINTER_VECTOR_SIZE_SIZE);

	for (
		File::POINTER_VECTOR::const_iterator filePointerVectorIterator = filePointerVector.begin();
		filePointerVectorIterator != filePointerVector.end();
		filePointerVectorIterator++
	) {
		(*filePointerVectorIterator)->write(outputFileStream);
	}
}

void Ubi::BigFile::Directory::appendToTextureBoxNameMaskVariantMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap) {
	appendToTextureBoxNameMaskVariantMap(inputFileStream, fileSystemPosition, textureBoxNameMaskVariantMap, binaryFilePointerVector);

	for (
		VECTOR::iterator directoryVectorIterator = directoryVector.begin();
		directoryVectorIterator != directoryVector.end();
		directoryVectorIterator++
	) {
		appendToTextureBoxNameMaskVariantMap(inputFileStream, fileSystemPosition, textureBoxNameMaskVariantMap, directoryVectorIterator->binaryFilePointerVector);
	}
}

void Ubi::BigFile::Directory::appendToTextureBoxNameMaskVariantMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap, File::POINTER_VECTOR &binaryFilePointerVector) {
	for (
		File::POINTER_VECTOR::iterator binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
		binaryFilePointerVectorIterator != binaryFilePointerVector.end();
		binaryFilePointerVectorIterator++
	) {
		(*binaryFilePointerVectorIterator)->appendToTextureBoxNameMaskVariantMap(inputFileStream, fileSystemPosition, textureBoxNameMaskVariantMap);
	}
}

const std::string Ubi::BigFile::Directory::NAME_TEXTURE = "texture";
const std::string Ubi::BigFile::Directory::NAME_WATER = "water";

Ubi::BigFile::Header::Header(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::SIZE &fileSystemPosition) {
	fileSystemPosition = (File::SIZE)inputFileStream.tellg();
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
	: header(inputFileStream, fileSystemSize, fileSystemPosition),
	directory(inputFileStream, fileSystemSize, fileSystemPosition, files, filePointerSetMap) {
}

Ubi::BigFile::BigFile(std::ifstream &inputFileStream, const Path &path, std::optional<File> &fileOptional)
	: header(inputFileStream, fileOptional),
	directory(inputFileStream, path, path.directoryNameVector.begin(), fileOptional) {
}

void Ubi::BigFile::write(std::ofstream &outputFileStream) const {
	header.write(outputFileStream);
	directory.write(outputFileStream);
}