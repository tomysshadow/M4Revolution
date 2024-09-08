#include "Ubi.h"
#include <regex>

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

std::optional<std::string> &Ubi::String::swizzle(std::optional<std::string> &encryptedStringOptional) {
	if (!encryptedStringOptional.has_value()) {
		return encryptedStringOptional;
	}

	const char MASK = 85;

	std::string &encryptedString = encryptedStringOptional.value();

	for (std::string::iterator encryptedStringIterator = encryptedString.begin(); encryptedStringIterator != encryptedString.end(); encryptedStringIterator++) {
		char &encryptedChar = *encryptedStringIterator;
		char encryptedCharLeft = encryptedChar << 1;
		char encryptedCharRight = encryptedChar >> 1;

		encryptedChar = (encryptedCharLeft ^ encryptedCharRight) & MASK ^ encryptedCharLeft;
	}
	return encryptedStringOptional;
}

Ubi::Binary::RLE::SLICE_MAP Ubi::Binary::RLE::createSliceMap(std::ifstream &inputFileStream, std::streamsize size) {
	std::optional<Header> headerOptional = std::nullopt;
	readFileHeader(inputFileStream, headerOptional, size);

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
		// sliceRow and sliceCol are incremented by one
		// because they are indexed from zero here, but
		// we want them indexed by one for the face names
		readFileStreamSafe(inputFileStream, &sliceRow, SLICE_ROW_SIZE);
		sliceMapIterator = sliceMap.find(++sliceRow);

		if (sliceMapIterator == sliceMap.end()) {
			sliceMapIterator = sliceMap.insert({ sliceRow, {} }).first;
		}

		readFileStreamSafe(inputFileStream, &sliceCol, SLICE_COL_SIZE);
		sliceMapIterator->second.insert(sliceCol + 1);

		// normally these would be in seperate classes
		// there just isn't much point here because I don't really care about any of this data
		// I only really care about sliceRow/sliceCol and just want to skip the rest of this stuff
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
	nameOptional = String::swizzle(String::readOptional(inputFileStream));
}

Ubi::Binary::Resource::Resource(Loader::POINTER loaderPointer, VERSION version)
	: LOADER_POINTER(loaderPointer) {
	if (version < loaderPointer->version) {
		throw NotImplemented();
	}
}

/*
void Ubi::Binary::Node::create(std::ifstream &inputFileStream, RLE::NAME_SET &nameSet) {
	size_t FIELDS_SIZE = 32;
	inputFileStream.seekg(FIELDS_SIZE, std::ios::cur);

	uint32_t names = 0;
	const size_t NAMES_SIZE = sizeof(names);

	readFileStreamSafe(inputFileStream, &names, NAMES_SIZE);

	std::optional<std::string> nameOptional = std::nullopt;

	for (uint32_t i = 0; i < names; i++) {
		nameOptional = String::swizzle(String::readOptional(inputFileStream));

		if (nameOptional.has_value()) {
			nameSet.insert(nameOptional.value());
		}
	}
}

Ubi::Binary::Node::Node(Loader::POINTER loaderPointer, std::ifstream &inputFileStream, RLE::NAME_SET &nameSet)
	: Resource(loaderPointer, VERSION) {
	create(inputFileStream, nameSet);
}

Ubi::Binary::Node::Node(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer, VERSION) {
	RLE::NAME_SET nameSet = {};
	create(inputFileStream, nameSet);
}
*/

void Ubi::Binary::TextureBox::create(std::ifstream &inputFileStream, Binary::RLE::LAYER_MAP &layerMap) {
	Binary::RLE::LAYER_MAP::iterator layerMapIterator = {};

	std::optional<std::string> layerFileOptional = String::swizzle(String::readOptional(inputFileStream));

	if (layerFileOptional.has_value()) {
		const std::string &LAYER_FILE = layerFileOptional.value();

		layerMapIterator = layerMap.find(LAYER_FILE);

		if (layerMapIterator == layerMap.end()) {
			layerMapIterator = layerMap.insert({ LAYER_FILE, {} }).first;
		}

		layerMapIterator->second.textureBoxNameOptional = LOADER_POINTER->nameOptional;
	}

	const size_t FIELDS_SIZE = 17;
	inputFileStream.seekg(FIELDS_SIZE, std::ios::cur);

	if (layerFileOptional.has_value()) {
		bool &isLayerMask = layerMapIterator->second.isLayerMask;
		const size_t IS_LAYER_MASK_SIZE = sizeof(isLayerMask);

		readFileStreamSafe(inputFileStream, &isLayerMask, IS_LAYER_MASK_SIZE);
	} else {
		const size_t IS_LAYER_MASK_SIZE = 1;
		inputFileStream.seekg(IS_LAYER_MASK_SIZE, std::ios::cur);
	}

	const size_t FIELDS_SIZE2 = 4;
	inputFileStream.seekg(FIELDS_SIZE2, std::ios::cur);

	uint32_t sets = 0;
	const size_t SETS_SIZE = sizeof(sets);

	readFileStreamSafe(inputFileStream, &sets, SETS_SIZE);

	std::optional<std::string> set = std::nullopt;

	for (uint32_t i = 0; i < sets; i++) {
		set = String::swizzle(String::readOptional(inputFileStream));

		if (set.has_value() && layerFileOptional.has_value()) {
			layerMapIterator->second.setsSet.insert(set.value());
		}
	}

	const size_t STATES_FIELDS_SIZE = 4;

	uint32_t states = 0;
	uint32_t stateNames = 0;

	const size_t STATES_SIZE = sizeof(states);
	const size_t STATE_NAMES_SIZE = sizeof(stateNames);

	readFileStreamSafe(inputFileStream, &states, STATES_SIZE);

	for (uint32_t i = 0; i < states; i++) {
		inputFileStream.seekg(STATES_FIELDS_SIZE, std::ios::cur);
		readFileStreamSafe(inputFileStream, &stateNames, STATE_NAMES_SIZE);

		for (uint32_t j = 0; j < stateNames; j++) {
			String::readOptional(inputFileStream);
		}
	}
}

Ubi::Binary::TextureBox::TextureBox(Loader::POINTER loaderPointer, std::ifstream &inputFileStream, Binary::RLE::LAYER_MAP &layerMap)
	: Resource(loaderPointer, VERSION) {
	create(inputFileStream, layerMap);
}

Ubi::Binary::TextureBox::TextureBox(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer, VERSION) {
	Binary::RLE::LAYER_MAP layerMap = {};
	create(inputFileStream, layerMap);
}

Ubi::Binary::InteractiveOffsetProvider::InteractiveOffsetProvider(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer, VERSION) {
	const size_t FIELDS_SIZE = 33;
	inputFileStream.seekg(FIELDS_SIZE, std::ios::cur);
}

Ubi::Binary::TextureAlignedOffsetProvider::TextureAlignedOffsetProvider(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer, VERSION) {
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

	maskFileOptional = String::swizzle(String::readOptional(inputFileStream));

	if (maskFileOptional.has_value()) {
		maskNameSet.insert(maskFileOptional.value());
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
	: Resource(loaderPointer, VERSION) {
	create(inputFileStream, maskNameSet);
}

Ubi::Binary::StateData::StateData(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer, VERSION) {
	RLE::MASK_NAME_SET maskNameSet = {};
	create(inputFileStream, maskNameSet);
}

void Ubi::Binary::Water::create(std::ifstream &inputFileStream, RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP &textureBoxNameMaskNameSetMap) {
	resourceNameOptional = String::swizzle(String::readOptional(inputFileStream));

	if (!resourceNameOptional.has_value()) {
		return;
	}

	const size_t WATER_FIELDS_SIZE = 9; // AssignReflectionAlpha, ReflectionAlphaAtEdge, ReflectionAlphaAtHorizon
	inputFileStream.seekg(WATER_FIELDS_SIZE, std::ios::cur);

	uint32_t resources = 0;
	const size_t RESOURCES_SIZE = sizeof(resources);

	readFileStreamSafe(inputFileStream, &resources, RESOURCES_SIZE);

	const std::optional<std::string> &TEXTURE_BOX_NAME_OPTIONAL = getTextureBoxNameOptional(resourceNameOptional.value());

	if (TEXTURE_BOX_NAME_OPTIONAL.has_value()) {
		const std::string &TEXTURE_BOX_NAME = TEXTURE_BOX_NAME_OPTIONAL.value();

		RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP::iterator textureBoxNameMaskNameSetMapIterator = {};

		for (uint32_t i = 0; i < resources; i++) {
			textureBoxNameMaskNameSetMapIterator = textureBoxNameMaskNameSetMap.find(TEXTURE_BOX_NAME);

			if (textureBoxNameMaskNameSetMapIterator == textureBoxNameMaskNameSetMap.end()) {
				textureBoxNameMaskNameSetMapIterator = textureBoxNameMaskNameSetMap.insert({ TEXTURE_BOX_NAME, RLE::MASK_NAME_SET() }).first;
			}

			createMaskNameSet(inputFileStream, textureBoxNameMaskNameSetMapIterator->second);
		}
	} else {
		for (uint32_t i = 0; i < resources; i++) {
			createResourcePointer(inputFileStream);
		}
	}
}

std::optional<std::string> Ubi::Binary::Water::getTextureBoxNameOptional(const std::string &resourceName) {
	const std::string::size_type PERIOD_SIZE = sizeof(PERIOD);
	std::string::size_type periodIndex = resourceName.find(PERIOD);

	if (periodIndex == std::string::npos) {
		return std::nullopt;
	}

	const std::string CONTEXT_GLOBAL = "global";
	const std::string CONTEXT_SHARED = "shared";

	const std::string &CONTEXT = resourceName.substr(0, periodIndex);

	if (CONTEXT == CONTEXT_GLOBAL || CONTEXT == CONTEXT_SHARED) {
		return std::nullopt;
	}

	return resourceName.substr(
		periodIndex + PERIOD_SIZE,
		std::string::npos
	);
}

Ubi::Binary::Water::Water(Loader::POINTER loaderPointer, std::ifstream &inputFileStream, RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP &textureBoxNameMaskNameSetMap)
	: Resource(loaderPointer, VERSION) {
	create(inputFileStream, textureBoxNameMaskNameSetMap);
}

Ubi::Binary::Water::Water(Loader::POINTER loaderPointer, std::ifstream &inputFileStream)
	: Resource(loaderPointer, VERSION) {
	RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP textureBoxNameMaskNameSetMap = {};
	create(inputFileStream, textureBoxNameMaskNameSetMap);
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

	// we only support the UBI_B0_L ID
	// I obviously understand the actual game uses a serializer that
	// can also read/write text, but this appears totally unused
	// so I'm not implementing it
	if (id != UBI_B0_L) {
		throw Invalid();
	}
}

Ubi::Binary::Header::~Header() {
	testReadPastEnd();
}

void Ubi::Binary::readFileHeader(std::ifstream &inputFileStream, std::optional<Binary::Header> &headerOptional, std::streamsize size) {
	MAKE_SCOPE_EXIT(headerOptionalScopeExit) {
		headerOptional = std::nullopt;
	};

	if (size != -1) {
		headerOptional.emplace(inputFileStream, size);
		headerOptionalScopeExit.dismiss();
	}
}

Ubi::Binary::Resource::Loader::POINTER Ubi::Binary::readFileLoader(std::ifstream &inputFileStream, std::optional<Binary::Header> &headerOptional, std::streamsize size) {
	readFileHeader(inputFileStream, headerOptional, size);
	return std::make_shared<Resource::Loader>(inputFileStream);
}

Ubi::Binary::Resource::POINTER Ubi::Binary::createResourcePointer(std::ifstream &inputFileStream, std::streamsize size) {
	std::optional<Header> headerOptional = std::nullopt;
	Resource::Loader::POINTER loaderPointer = readFileLoader(inputFileStream, headerOptional, size);

	switch (loaderPointer->id) {
		//case Node::ID:
		//return std::make_shared<Node>(loaderPointer, inputFileStream);
		case TextureBox::ID:
		return std::make_shared<TextureBox>(loaderPointer, inputFileStream);
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

Ubi::Binary::Resource::POINTER Ubi::Binary::createLayerMapPointer(std::ifstream &inputFileStream, Binary::RLE::LAYER_MAP_POINTER &layerMapPointer, std::streamsize size) {
	MAKE_SCOPE_EXIT(layerFileOptionalScopeExit) {
		layerMapPointer = 0;
	};

	std::optional<Header> headerOptional = std::nullopt;
	Resource::Loader::POINTER loaderPointer = readFileLoader(inputFileStream, headerOptional, size);
	Resource::POINTER resourcePointer = 0;

	switch (loaderPointer->id) {
		case TextureBox::ID:
		resourcePointer = std::make_shared<TextureBox>(loaderPointer, inputFileStream, *layerMapPointer);
	}

	if (resourcePointer) {
		layerFileOptionalScopeExit.dismiss();
	}
	return resourcePointer;
}

Ubi::Binary::Resource::POINTER Ubi::Binary::createMaskNameSet(std::ifstream &inputFileStream, RLE::MASK_NAME_SET &maskNameSet, std::streamsize size) {
	MAKE_SCOPE_EXIT(maskNameSetScopeExit) {
		maskNameSet = {};
	};

	std::optional<Header> headerOptional = std::nullopt;
	Resource::Loader::POINTER loaderPointer = readFileLoader(inputFileStream, headerOptional, size);
	Resource::POINTER resourcePointer = 0;

	switch (loaderPointer->id) {
		case StateData::ID:
		resourcePointer = std::make_shared<StateData>(loaderPointer, inputFileStream, maskNameSet);
	}

	if (resourcePointer) {
		maskNameSetScopeExit.dismiss();
	}
	return resourcePointer;
}

Ubi::Binary::Resource::POINTER Ubi::Binary::createTextureBoxNameMaskNameSetMap(std::ifstream &inputFileStream, RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP &textureBoxNameMaskNameSetMap, std::streamsize size) {
	MAKE_SCOPE_EXIT(textureBoxNameMaskNameSetMapScopeExit) {
		textureBoxNameMaskNameSetMap = {};
	};

	std::optional<Header> headerOptional = std::nullopt;
	Resource::Loader::POINTER loaderPointer = readFileLoader(inputFileStream, headerOptional, size);
	Resource::POINTER resourcePointer = 0;

	switch (loaderPointer->id) {
		case Water::ID:
		resourcePointer = std::make_shared<Water>(loaderPointer, inputFileStream, textureBoxNameMaskNameSetMap);
	}

	if (resourcePointer) {
		textureBoxNameMaskNameSetMapScopeExit.dismiss();
	}
	return resourcePointer;
}

Ubi::BigFile::Path::Path() {
}

Ubi::BigFile::Path::Path(const NAME_VECTOR &directoryNameVector, const std::string &fileName)
	: directoryNameVector(directoryNameVector),
	fileName(fileName) {
}

Ubi::BigFile::Path::Path(const std::string &copyString) {
	create(copyString);
}

Ubi::BigFile::Path &Ubi::BigFile::Path::operator=(const std::string &assignString) {
	clear();
	return create(assignString);
}

void Ubi::BigFile::Path::clear() {
	directoryNameVector = {};
	fileName = "";
}

Ubi::BigFile::Path& Ubi::BigFile::Path::create(const std::string &file) {
	// split up a string into a Path object
	std::string::size_type begin = 0;
	std::string::size_type end = 0;

	while ((begin = file.find_first_not_of(SEPERATOR, end)) != std::string::npos) {
		end = file.find(SEPERATOR, begin);

		const std::string &NAME = file.substr(begin, end - begin);

		if (end == std::string::npos) {
			fileName = NAME;
		} else {
			directoryNameVector.push_back(NAME);
		}
	}
	return *this;
}

Ubi::BigFile::File::File(std::ifstream &inputFileStream, SIZE &fileSystemSize, const std::optional<File> &layerFileOptional, bool texture) {
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

			if (type == TYPE::BINARY && texture) {
				type = TYPE::BINARY_RESOURCE_IMAGE_DATA;
			} else if ((type == TYPE::JPEG || type == TYPE::ZAP) && isRaw(layerFileOptional)) {
				type = type == TYPE::JPEG ? TYPE::JPEG_RAW : TYPE::ZAP_RAW;
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

Ubi::Binary::Resource::POINTER Ubi::BigFile::File::createLayerMapPointer(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::LAYER_MAP_POINTER &layerMapPointer) const {
	std::streampos position = inputFileStream.tellg();
	Binary::Resource::POINTER resourcePointer = 0;

	try {
		inputFileStream.seekg(fileSystemPosition + (std::streampos)this->position);
		resourcePointer = Binary::createLayerMapPointer(inputFileStream, layerMapPointer, this->size);
	} catch (...) {
		// fail silently
	}

	inputFileStream.seekg(position);
	return resourcePointer;
}

Ubi::Binary::Resource::POINTER Ubi::BigFile::File::appendToTextureBoxNameMaskNameSetMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP &textureBoxNameMaskNameSetMap) const {
	std::streampos position = inputFileStream.tellg();
	Binary::Resource::POINTER resourcePointer = 0;
	
	try {
		inputFileStream.seekg(fileSystemPosition + (std::streampos)this->position);
		resourcePointer = Binary::createTextureBoxNameMaskNameSetMap(inputFileStream, textureBoxNameMaskNameSetMap, this->size);
	} catch (...) {
		// fail silently
	}

	inputFileStream.seekg(position);
	return resourcePointer;
}

void Ubi::BigFile::File::read(std::ifstream &inputFileStream) {
	nameOptional = String::readOptional(inputFileStream);
	readFileStreamSafe(inputFileStream, &size, SIZE_SIZE);
	readFileStreamSafe(inputFileStream, &position, POSITION_SIZE);
}

bool Ubi::BigFile::File::isRaw(const std::optional<File> &layerFileOptional) const {
	if (!layerFileOptional.has_value()) {
		return false;
	}

	if (!nameOptional.has_value()) {
		return false;
	}

	const Binary::RLE::Layer &LAYER = layerFileOptional.value().layerMapIterator->second;

	if (LAYER.isLayerMask) {
		return true;
	}

	if (!LAYER.water) {
		return false;
	}

	const Binary::RLE::MASK_MAP &MASK_MAP = LAYER.maskMap;
	const std::regex FACE_SLICE("^([a-z]+)_(\\d{2})_(\\d{2})\\.");

	std::smatch matches = {};

	if (!std::regex_search(nameOptional.value(), matches, FACE_SLICE)
		|| matches.length() <= 3) {
		return false;
	}

	const std::string &FACE_STR = matches[1];
	const std::string &ROW_STR = matches[2];
	const std::string &COL_STR = matches[3];

	Binary::RLE::FACE_STR_MAP::const_iterator faceStrMapIterator = Binary::RLE::WATER_SLICE_FACE_STR_MAP.find(FACE_STR);

	if (faceStrMapIterator == Binary::RLE::WATER_SLICE_FACE_STR_MAP.end()) {
		return false;
	}

	Binary::RLE::MASK_MAP::const_iterator maskMapIterator = MASK_MAP.find(faceStrMapIterator->second);

	if (maskMapIterator == MASK_MAP.end()) {
		return false;
	}

	// since these have leading zeros, I use base 10 specifically
	// (the ROW/COL should not be misinterpreted as octal)
	const int BASE = 10;

	unsigned long row = 0;

	if (!stringToLongUnsigned(ROW_STR.c_str(), row, BASE)) {
		return false;
	}

	const Binary::RLE::SLICE_MAP &SLICE_MAP = maskMapIterator->second;
	Binary::RLE::SLICE_MAP::const_iterator sliceMapIterator = SLICE_MAP.find(row);

	if (sliceMapIterator == SLICE_MAP.end()) {
		return false;
	}

	unsigned long col = 0;

	if (!stringToLongUnsigned(COL_STR.c_str(), col, BASE)) {
		return false;
	}

	const Binary::RLE::COL_SET &COL_SET = sliceMapIterator->second;
	return COL_SET.find(col) != COL_SET.end();
}

std::string Ubi::BigFile::File::getNameExtension() const {
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

const std::string Ubi::BigFile::Directory::NAME_CUBE = "cube";
const std::string Ubi::BigFile::Directory::NAME_WATER = "water";

Ubi::BigFile::Directory::Directory(Directory* ownerDirectory, std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &filePointerSetMap, const std::optional<File> &layerFileOptional)
	: nameOptional(String::readOptional(inputFileStream)) {
	read(ownerDirectory, inputFileStream, fileSystemSize, files, filePointerSetMap, layerFileOptional);
}

Ubi::BigFile::Directory::Directory(std::ifstream &inputFileStream)
	: nameOptional(String::readOptional(inputFileStream)) {
	// in this case it is the same as not having an owner
	File::SIZE fileSystemSize = 0;
	File::POINTER_VECTOR::size_type files = 0;
	File::POINTER_SET_MAP filePointerSetMap = {};
	read(false, inputFileStream, fileSystemSize, files, filePointerSetMap, std::nullopt);
}

Ubi::BigFile::Directory::Directory(std::ifstream &inputFileStream, const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, File::POINTER &filePointer) {
	MAKE_SCOPE_EXIT(filePointerScopeExit) {
		filePointer = 0;
	};

	const Path::NAME_VECTOR &DIRECTORY_NAME_VECTOR = path.directoryNameVector;

	nameOptional = String::readOptional(inputFileStream);
	bool match = isMatch(DIRECTORY_NAME_VECTOR, directoryNameVectorIterator);

	DIRECTORY_VECTOR_SIZE directoryVectorSize = 0;
	readFileStreamSafe(inputFileStream, &directoryVectorSize, DIRECTORY_VECTOR_SIZE_SIZE);

	if (directoryNameVectorIterator == DIRECTORY_NAME_VECTOR.end()) {
		// in this case we just read the directories and don't bother checking filePointer
		for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
			Directory directory(
				inputFileStream,
				path,
				directoryNameVectorIterator,
				filePointer
			);
		}
	} else {
		for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
			directoryVector.emplace_back(
				inputFileStream,
				path,
				directoryNameVectorIterator,
				filePointer
			);

			// if this is true we found the matching file, so exit early
			if (filePointer) {
				// erase all but the last element
				// (there should always be at least one element in the vector at this point)
				directoryVector.erase(directoryVector.begin(), directoryVector.end() - 1);
				filePointerScopeExit.dismiss();
				return;
			}
		}

		directoryVector = {};
	}

	FILE_POINTER_VECTOR_SIZE filePointerVectorSize = 0;
	readFileStreamSafe(inputFileStream, &filePointerVectorSize, FILE_POINTER_VECTOR_SIZE_SIZE);

	if (match) {
		for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
			filePointer = std::make_shared<File>(inputFileStream);
			filePointerVector.push_back(filePointer);

			// is this the file we are looking for?
			const std::optional<std::string> &NAME_OPTIONAL = filePointer->nameOptional;

			if (NAME_OPTIONAL.has_value() && NAME_OPTIONAL.value() == path.fileName) {
				// erase all but the last element
				// (there should always be at least one element in the vector at this point)
				filePointerVector.erase(filePointerVector.begin(), filePointerVector.end() - 1);
				filePointerScopeExit.dismiss();
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

	FILE_POINTER_VECTOR_SIZE fileVectorSize = (FILE_POINTER_VECTOR_SIZE)(filePointerVector.size() + binaryFilePointerVector.size());
	writeFileStreamSafe(outputFileStream, &fileVectorSize, FILE_POINTER_VECTOR_SIZE_SIZE);

	for (
		File::POINTER_VECTOR::const_iterator filePointerVectorIterator = filePointerVector.begin();
		filePointerVectorIterator != filePointerVector.end();
		filePointerVectorIterator++
	) {
		(*filePointerVectorIterator)->write(outputFileStream);
	}

	for (
		File::POINTER_VECTOR::const_iterator binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
		binaryFilePointerVectorIterator != binaryFilePointerVector.end();
		binaryFilePointerVectorIterator++
	) {
		(*binaryFilePointerVectorIterator)->write(outputFileStream);
	}
}

Ubi::BigFile::File::POINTER Ubi::BigFile::Directory::find(const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator) const {
	const Path::NAME_VECTOR &DIRECTORY_NAME_VECTOR = path.directoryNameVector;

	bool match = isMatch(DIRECTORY_NAME_VECTOR, directoryNameVectorIterator);
	File::POINTER filePointer = 0;

	if (directoryNameVectorIterator != DIRECTORY_NAME_VECTOR.end()) {
		for (VECTOR::const_iterator directoryVectorIterator = directoryVector.begin(); directoryVectorIterator != directoryVector.end(); directoryVectorIterator++) {
			filePointer = directoryVectorIterator->find(path, directoryNameVectorIterator);

			// if this is true we found the matching file, so exit early
			if (filePointer) {
				return filePointer;
			}
		}
	}

	if (!match) {
		return 0;
	}

	for (
		File::POINTER_VECTOR::const_iterator filePointerVectorIterator = filePointerVector.begin();
		filePointerVectorIterator != filePointerVector.end();
		filePointerVectorIterator++
	) {
		filePointer = *filePointerVectorIterator;

		// is this the file we are looking for?
		if (filePointer) {
			const std::optional<std::string> &NAME_OPTIONAL = filePointer->nameOptional;

			if (NAME_OPTIONAL.has_value() && NAME_OPTIONAL.value() == path.fileName) {
				return filePointer;
			}
		}
	}
	return 0;
}

Ubi::BigFile::File::POINTER Ubi::BigFile::Directory::find(const Path &path) const {
	return find(path, path.directoryNameVector.begin());
}

void Ubi::BigFile::Directory::createLayerMapPointer(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::LAYER_MAP_POINTER &layerMapPointer) const {
	createLayerMapPointer(inputFileStream, fileSystemPosition, layerMapPointer, binaryFilePointerVector);

	for (
		VECTOR::const_iterator directoryVectorIterator = directoryVector.begin();
		directoryVectorIterator != directoryVector.end();
		directoryVectorIterator++
		) {
		createLayerMapPointer(inputFileStream, fileSystemPosition, layerMapPointer, directoryVectorIterator->binaryFilePointerVector);
	}
}

void Ubi::BigFile::Directory::appendToTextureBoxNameMaskNameSetMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP &textureBoxNameMaskNameSetMap) const {
	appendToTextureBoxNameMaskNameSetMap(inputFileStream, fileSystemPosition, textureBoxNameMaskNameSetMap, binaryFilePointerVector);

	for (
		VECTOR::const_iterator directoryVectorIterator = directoryVector.begin();
		directoryVectorIterator != directoryVector.end();
		directoryVectorIterator++
	) {
		appendToTextureBoxNameMaskNameSetMap(inputFileStream, fileSystemPosition, textureBoxNameMaskNameSetMap, directoryVectorIterator->binaryFilePointerVector);
	}
}

void Ubi::BigFile::Directory::read(bool owner, std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &filePointerSetMap, const std::optional<File> &layerFileOptional) {
	DIRECTORY_VECTOR_SIZE directoryVectorSize = 0;
	readFileStreamSafe(inputFileStream, &directoryVectorSize, DIRECTORY_VECTOR_SIZE_SIZE);

	bool bftex = !owner

	&& (
		nameOptional.has_value()
		? nameOptional == "bftex"
		: true
	);

	for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
		directoryVector.emplace_back(
			this,
			inputFileStream,
			fileSystemSize,
			files,
			filePointerSetMap,

			// only if this directory matches the "bftex" name, pass the file
			// (if this directory has no name, any name matches, so the file is passed)
			bftex
			? layerFileOptional
			: std::nullopt
		);
	}

	FILE_POINTER_VECTOR_SIZE filePointerVectorSize = 0;
	readFileStreamSafe(inputFileStream, &filePointerVectorSize, FILE_POINTER_VECTOR_SIZE_SIZE);

	File::POINTER filePointer = 0;
	File::POINTER_SET_MAP::iterator filePointerSetMapIterator = {};
	bool set = isSet(bftex, layerFileOptional);

	for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
		filePointer = std::make_shared<File>(
			inputFileStream,
			fileSystemSize,

			set
			? layerFileOptional
			: std::nullopt,

			// the NAME_TEXTURE comparison is so we only convert textures in the texture folder specifically, even if the extension matches
			nameOptional.has_value()
			? nameOptional.value() == NAME_TEXTURE
			: false
		);

		const File &FILE = *filePointer;

		if (FILE.type == File::TYPE::BINARY) {
			binaryFilePointerVector.push_back(filePointer);
		} else {
			filePointerVector.push_back(filePointer);
		}

		// are there any other files at this position?
		filePointerSetMapIterator = filePointerSetMap.find(FILE.position);

		// if not, then create a new set
		if (filePointerSetMapIterator == filePointerSetMap.end()) {
			filePointerSetMapIterator = filePointerSetMap.insert({ FILE.position, {} }).first;
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

bool Ubi::BigFile::Directory::isMatch(const Path::NAME_VECTOR &directoryNameVector, Path::NAME_VECTOR::const_iterator &directoryNameVectorIterator) const {
	// should we care about this directory at all?
	if (directoryNameVectorIterator == directoryNameVector.end()) {
		return false;
	}

	// does this directory's name match the one we are trying to find?
	if (nameOptional.has_value() && nameOptional.value() != *directoryNameVectorIterator) {
		directoryNameVectorIterator = directoryNameVector.end();
		return false;
	}
	return ++directoryNameVectorIterator == directoryNameVector.end();
}

bool Ubi::BigFile::Directory::isSet(bool bftex, const std::optional<File> &layerFileOptional) const {
	if (bftex) {
		return false;
	}

	if (!layerFileOptional.has_value()) {
		return false;
	}

	const File &LAYER_FILE = layerFileOptional.value();

	Binary::RLE::LAYER_MAP_POINTER layerMapPointer = LAYER_FILE.layerMapPointer;

	if (!layerMapPointer) {
		return false;
	}

	// as per usual, if we don't have a name, anything matches
	if (!nameOptional.has_value()) {
		return true;
	}

	const Binary::RLE::SETS_SET &SETS_SET = LAYER_FILE.layerMapIterator->second.setsSet;
	return SETS_SET.find(nameOptional.value()) != SETS_SET.end();
}

void Ubi::BigFile::Directory::createLayerMapPointer(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::LAYER_MAP_POINTER &layerMapPointer, const File::POINTER_VECTOR &binaryFilePointerVector) const {
	Binary::Resource::POINTER resourcePointer = 0;

	for (
		File::POINTER_VECTOR::const_iterator binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
		binaryFilePointerVectorIterator != binaryFilePointerVector.end();
		binaryFilePointerVectorIterator++
		) {
		resourcePointer = (*binaryFilePointerVectorIterator)->createLayerMapPointer(inputFileStream, fileSystemPosition, layerMapPointer);
	}
}

void Ubi::BigFile::Directory::appendToTextureBoxNameMaskNameSetMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP &textureBoxNameMaskNameSetMap, const File::POINTER_VECTOR &binaryFilePointerVector) const {
	for (
		File::POINTER_VECTOR::const_iterator binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
		binaryFilePointerVectorIterator != binaryFilePointerVector.end();
		binaryFilePointerVectorIterator++
	) {
		(*binaryFilePointerVectorIterator)->appendToTextureBoxNameMaskNameSetMap(inputFileStream, fileSystemPosition, textureBoxNameMaskNameSetMap);
	}
}

const std::string Ubi::BigFile::Directory::NAME_TEXTURE = "texture";

Ubi::BigFile::Header::Header(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::SIZE &fileSystemPosition) {
	fileSystemPosition = (File::SIZE)inputFileStream.tellg();
	read(inputFileStream);

	fileSystemSize += (File::SIZE)(
		String::SIZE_SIZE

		+ SIGNATURE.size() + 1
		+ VERSION_SIZE
	);
}

Ubi::BigFile::Header::Header(std::ifstream &inputFileStream) {
	read(inputFileStream);
}

Ubi::BigFile::Header::Header(std::ifstream &inputFileStream, File::POINTER &filePointer) {
	// for path vectors
	if (filePointer) {
		inputFileStream.seekg(filePointer->position);
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

Ubi::BigFile::BigFile(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &filePointerSetMap, File &file)
	: header(inputFileStream, fileSystemSize, fileSystemPosition),
	directory(0, inputFileStream, fileSystemSize, files, filePointerSetMap, file) {
	// do all the steps necessary to prevent water causing a crash
	#ifdef LAYERS_ENABLED
	const Directory::VECTOR &DIRECTORY_VECTOR = directory.directoryVector;

	Directory::VECTOR_ITERATOR_VECTOR cubeVectorIterators = {};
	Directory::VECTOR_ITERATOR_VECTOR waterVectorIterators = {};

	for (
		Directory::VECTOR::const_iterator directoryVectorIterator = DIRECTORY_VECTOR.begin();
		directoryVectorIterator != DIRECTORY_VECTOR.end();
		directoryVectorIterator++
	) {
		const std::optional<std::string> &NAME_OPTIONAL = directoryVectorIterator->nameOptional;

		if (NAME_OPTIONAL.has_value()) {
			const std::string &NAME = NAME_OPTIONAL.value();

			if (NAME == Directory::NAME_CUBE) {
				cubeVectorIterators.push_back(directoryVectorIterator);
			} else if (NAME == Directory::NAME_WATER) {
				waterVectorIterators.push_back(directoryVectorIterator);
			}
		}
	}

	if (cubeVectorIterators.empty()) {
		return;
	}

	Binary::RLE::LAYER_MAP_POINTER layerMapPointer = std::make_shared<Binary::RLE::LAYER_MAP>();

	for (
		Directory::VECTOR_ITERATOR_VECTOR::iterator cubeVectorIteratorsIterator = cubeVectorIterators.begin();
		cubeVectorIteratorsIterator != cubeVectorIterators.end();
		cubeVectorIteratorsIterator++
	) {
		(*cubeVectorIteratorsIterator)->createLayerMapPointer(inputFileStream, fileSystemPosition, layerMapPointer);
	}

	Binary::RLE::LAYER_MAP &layerMap = *layerMapPointer;

	if (layerMap.empty()) {
		return;
	}

	Binary::RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP textureBoxNameMaskNameSetMap = {};

	for (
		Directory::VECTOR_ITERATOR_VECTOR::iterator waterVectorIteratorsIterator = waterVectorIterators.begin();
		waterVectorIteratorsIterator != waterVectorIterators.end();
		waterVectorIteratorsIterator++
	) {
		(*waterVectorIteratorsIterator)->appendToTextureBoxNameMaskNameSetMap(inputFileStream, fileSystemPosition, textureBoxNameMaskNameSetMap);
	}

	std::streampos position = inputFileStream.tellg();

	SCOPE_EXIT {
		inputFileStream.seekg(position);
	};

	File::POINTER layerFilePointer = 0;

	for (
		Binary::RLE::LAYER_MAP::iterator layerMapIterator = layerMap.begin();
		layerMapIterator != layerMap.end();
		layerMapIterator++
	) {
		for (
			Binary::RLE::TEXTURE_BOX_NAME_MASK_NAME_SET_MAP::iterator textureBoxNameMaskNameSetMapIterator = textureBoxNameMaskNameSetMap.begin();
			textureBoxNameMaskNameSetMapIterator != textureBoxNameMaskNameSetMap.end();
			textureBoxNameMaskNameSetMapIterator++
		) {
			Binary::RLE::Layer &layer = layerMapIterator->second;
			layer.water = textureBoxNameMaskNameSetMapIterator->first == layer.textureBoxNameOptional;

			if (!layer.water) {
				continue;
			}

			const Binary::RLE::MASK_NAME_SET &MASK_NAME_SET = textureBoxNameMaskNameSetMapIterator->second;

			Binary::RLE::MASK_MAP &maskMap = layer.maskMap;

			for (
				Binary::RLE::MASK_NAME_SET::const_iterator maskNameSetIterator = MASK_NAME_SET.begin();
				maskNameSetIterator != MASK_NAME_SET.end();
				maskNameSetIterator++
			) {
				layerFilePointer = directory.find(*maskNameSetIterator);

				if (!layerFilePointer) {
					continue;
				}

				std::streampos maskFileSystemPosition = (std::streampos)fileSystemPosition + (std::streampos)layerFilePointer->position;
				inputFileStream.seekg(maskFileSystemPosition);

				BigFile maskBigFile(inputFileStream);

				File::POINTER_VECTOR &maskFilePointerVector = maskBigFile.directory.filePointerVector;

				for (
					File::POINTER_VECTOR::iterator maskFilePointerVectorIterator = maskFilePointerVector.begin();
					maskFilePointerVectorIterator != maskFilePointerVector.end();
					maskFilePointerVectorIterator++
				) {
					File &maskFile = **maskFilePointerVectorIterator;

					if (!maskFile.nameOptional.has_value()) {
						continue;
					}

					Binary::RLE::FACE_STR_MAP::const_iterator &fileFaceStrMapIterator = Binary::RLE::FILE_FACE_STR_MAP.find(maskFile.nameOptional.value());

					if (fileFaceStrMapIterator == Binary::RLE::FILE_FACE_STR_MAP.end()) {
						continue;
					}

					inputFileStream.seekg(maskFileSystemPosition + (std::streampos)maskFile.position);
					maskMap.insert({ fileFaceStrMapIterator->second, Binary::RLE::createSliceMap(inputFileStream, maskFile.size) });
				}
			}
		}

		layerFilePointer = directory.find(layerMapIterator->first);

		if (layerFilePointer) {
			File &layerFile = *layerFilePointer;
			layerFile.layerMapPointer = layerMapPointer;
			layerFile.layerMapIterator = layerMapIterator;
		}
	}
	#endif
}

Ubi::BigFile::BigFile(std::ifstream &inputFileStream)
	: header(inputFileStream),
	directory(inputFileStream) {
}

Ubi::BigFile::BigFile(std::ifstream &inputFileStream, const Path &path, File::POINTER &filePointer)
	: header(inputFileStream, filePointer),
	directory(inputFileStream, path, path.directoryNameVector.begin(), filePointer) {
}

void Ubi::BigFile::write(std::ofstream &outputFileStream) const {
	header.write(outputFileStream);
	directory.write(outputFileStream);
}