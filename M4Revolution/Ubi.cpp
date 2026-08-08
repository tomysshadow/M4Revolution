#include "pch.h"
#include "Ubi.h"
#include <regex>

namespace Ubi {
	namespace String {
		std::optional<std::string> &swizzle(std::optional<std::string> &encryptedStringOptional) {
			if (!encryptedStringOptional.has_value()) {
				return encryptedStringOptional;
			}

			static constexpr unsigned char MASK = 85;

			std::string &encryptedString = encryptedStringOptional.value();

			for (
				std::string::iterator encryptedStringIterator = encryptedString.begin();
				encryptedStringIterator != encryptedString.end();
				encryptedStringIterator++
			) {
				char &encryptedChar = *encryptedStringIterator;
				
				unsigned char encryptedCharLeft = (unsigned char)((unsigned char)encryptedChar << 1);
				unsigned char encryptedCharRight = (unsigned char)((unsigned char)encryptedChar >> 1);

				encryptedChar = (encryptedCharLeft ^ encryptedCharRight) & MASK ^ encryptedCharLeft;
			}
			return encryptedStringOptional;
		}

		std::optional<std::string> readOptional(std::istream &inputStream, bool &nullTerminator, SIZE maxSize) {
			SIZE size = 0;
			readStream(inputStream, &size, sizeof(size));

			if (size > maxSize) {
				throw std::logic_error("size must not be greater than maxSize");
			}

			if (!size) {
				return std::nullopt;
			}

			std::unique_ptr<char[]> strPointer = makeUniqueArray<char>((size_t)size + 1);
			char* str = strPointer.get();
			readStream(inputStream, str, size);

			nullTerminator = !str[size - 1];
			str[size] = 0;
			return str;
		}

		std::optional<std::string> readOptional(std::istream &inputStream) {
			bool nullTerminator = true;
			return readOptional(inputStream, nullTerminator);
		}

		std::optional<std::string> readOptionalEncrypted(std::istream &inputStream) {
			std::optional<std::string> encryptedStringOptional = readOptional(inputStream);
			return swizzle(encryptedStringOptional);
		}

		void writeOptional(std::ostream &outputStream, const std::optional<std::string> &strOptional, bool nullTerminator) {
			SIZE size = strOptional.has_value() ? (SIZE)(strOptional.value().size() + nullTerminator) : 0;
			writeStream(outputStream, &size, sizeof(size));

			if (!size) {
				return;
			}

			writeStream(outputStream, strOptional.value().c_str(), size);
		}

		void writeOptionalEncrypted(std::ostream &outputStream, std::optional<std::string> &strOptional) {
			writeOptional(outputStream, swizzle(strOptional));
		}
	}

	namespace Binary {
		namespace RLE {
			void appendToSliceMap(std::istream &inputStream, std::streamsize size, SLICE_MAP &sliceMap) {
				std::optional<HeaderReader> headerReaderOptional = std::nullopt;
				readFileHeader(inputStream, headerReaderOptional, size);

				uint32_t waterSlices = 0;

				ROW sliceRow = 0;
				COL sliceCol = 0;

				uint32_t waterRLERegions = 0;

				uint32_t groups = 0;
				uint32_t subGroups = 0;

				uint32_t pixels = 0;
				std::streamoff pixelsSize = 0;

				static constexpr size_t WATER_FACE_FIELDS_SIZE = 20; // Type, Width, Height, SliceWidth, SliceHeight
				static constexpr size_t WATER_SLICE_FIELDS_SIZE = 8; // Width, Height
				static constexpr size_t WATER_RLE_REGION_FIELDS_SIZE = 20; // TextureCoordsInFace (X, Y,) TextureCoordsInSlice (X, Y,) RegionSize
				static constexpr size_t WATER_RLE_REGION_GROUP_FIELDS_SIZE = 4; // Unknown

				inputStream.seekg(WATER_FACE_FIELDS_SIZE, std::istream::cur);
				readStream(inputStream, &waterSlices, sizeof(waterSlices));

				for (uint32_t i = 0; i < waterSlices; i++) {
					// sliceRow and sliceCol are incremented by one
					// because they are indexed from zero here, but
					// we want them indexed by one for the face names
					readStream(inputStream, &sliceRow, sizeof(sliceRow));
					readStream(inputStream, &sliceCol, sizeof(sliceCol));
					sliceMap[sliceRow + 1].insert(sliceCol + 1);

					// normally these would be in seperate classes
					// there just isn't much point here because I don't really care about any of this data
					// I only really care about sliceRow/sliceCol and just want to skip the rest of this stuff
					inputStream.seekg(WATER_SLICE_FIELDS_SIZE, std::istream::cur);
					readStream(inputStream, &waterRLERegions, sizeof(waterRLERegions));

					for (uint32_t j = 0; j < waterRLERegions; j++) {
						inputStream.seekg(WATER_RLE_REGION_FIELDS_SIZE, std::istream::cur);
						readStream(inputStream, &groups, sizeof(groups));

						for (uint32_t l = 0; l < groups; l++) {
							inputStream.seekg(WATER_RLE_REGION_GROUP_FIELDS_SIZE, std::istream::cur);
							readStream(inputStream, &subGroups, sizeof(subGroups));

							for (uint32_t m = 0; m < subGroups; m++) {
								readStream(inputStream, &pixels, sizeof(pixels));

								pixelsSize = (std::streamoff)pixels;
								inputStream.seekg(pixelsSize + pixelsSize, std::istream::cur);
							}
						}
					}
				}
			}
		}

		Resource::Loader::Loader(std::istream &inputStream) {
			readStream(inputStream, &id, sizeof(id));
			readStream(inputStream, &version, sizeof(version));
			nameOptional = String::readOptionalEncrypted(inputStream);
		}

		Resource::Resource(Loader::POINTER loaderPointer, VERSION version)
			: LOADER_POINTER(loaderPointer) {
			if (version < loaderPointer->version) {
				throw Invalid();
			}
		}

		class TextureBox: public Resource {
			private:
			void create(std::istream &inputStream, RLE::LAYER_MAP &layerMap);

			public:
			static constexpr Resource::ID ID = 15;
			static constexpr Resource::VERSION VERSION = 5;

			TextureBox(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::LAYER_MAP &layerMap);
			TextureBox(Loader::POINTER loaderPointer, std::istream &inputStream);
		};

		class Water: public Resource {
			private:
			void create(std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap);

			static std::optional<std::string> getTextureBoxNameOptional(const std::string &resourceName);

			public:
			static constexpr Resource::ID ID = 42;
			static constexpr Resource::VERSION VERSION = 1;

			Water(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap);
			Water(Loader::POINTER loaderPointer, std::istream &inputStream);
		};

		class InteractiveOffsetProvider: public Resource {
			public:
			static constexpr Resource::ID ID = 43;
			static constexpr Resource::VERSION VERSION = 1;

			InteractiveOffsetProvider(Loader::POINTER loaderPointer, std::istream &inputStream);
		};

		class TextureAlignedOffsetProvider: public Resource {
			public:
			static constexpr Resource::ID ID = 44;
			static constexpr Resource::VERSION VERSION = 1;

			TextureAlignedOffsetProvider(Loader::POINTER loaderPointer, std::istream &inputStream);
		};

		class StateData: public Resource {
			private:
			void create(std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet);

			public:
			static constexpr Resource::ID ID = 45;
			static constexpr Resource::VERSION VERSION = 1;

			StateData(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet);
			StateData(Loader::POINTER loaderPointer, std::istream &inputStream);
		};

		void TextureBox::create(std::istream &inputStream, RLE::LAYER_MAP &layerMap) {
			RLE::Layer* layerPointer = nullptr;

			std::optional<std::string> layerFileOptional = String::readOptionalEncrypted(inputStream);

			if (layerFileOptional.has_value()) {
				RLE::Layer &layer = layerMap[layerFileOptional.value()];
				layer.textureBoxNameOptional = LOADER_POINTER->nameOptional;

				static constexpr size_t FIELDS_SIZE = 17;
				inputStream.seekg(FIELDS_SIZE, std::istream::cur);

				bool &isLayerMask = layer.isLayerMask;
				readStream(inputStream, &isLayerMask, sizeof(isLayerMask));

				static constexpr size_t FIELDS_SIZE2 = 4;
				inputStream.seekg(FIELDS_SIZE2, std::istream::cur);

				layerPointer = &layer;
			} else {
				static constexpr size_t FIELDS_SIZE = 22;
				inputStream.seekg(FIELDS_SIZE, std::istream::cur);
			}

			uint32_t sets = 0;
			readStream(inputStream, &sets, sizeof(sets));

			std::optional<std::string> setOptional = std::nullopt;

			for (uint32_t i = 0; i < sets; i++) {
				setOptional = String::readOptionalEncrypted(inputStream);

				if (layerPointer && setOptional.has_value()) {
					layerPointer->setsSet.insert(setOptional.value());
				}
			}

			uint32_t states = 0;
			uint32_t stateNames = 0;

			readStream(inputStream, &states, sizeof(states));

			for (uint32_t i = 0; i < states; i++) {
				static constexpr size_t STATES_FIELDS_SIZE = 4;

				inputStream.seekg(STATES_FIELDS_SIZE, std::istream::cur);
				readStream(inputStream, &stateNames, sizeof(stateNames));

				for (uint32_t j = 0; j < stateNames; j++) {
					String::readOptional(inputStream);
				}
			}
		}

		TextureBox::TextureBox(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::LAYER_MAP &layerMap)
			: Resource(loaderPointer, VERSION) {
			create(inputStream, layerMap);
		}

		TextureBox::TextureBox(Loader::POINTER loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, VERSION) {
			RLE::LAYER_MAP layerMap = {};
			create(inputStream, layerMap);
		}

		void Water::create(std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap) {
			std::optional<std::string> resourceNameOptional = String::readOptionalEncrypted(inputStream);

			static constexpr size_t WATER_FIELDS_SIZE = 9; // AssignReflectionAlpha, ReflectionAlphaAtEdge, ReflectionAlphaAtHorizon
			inputStream.seekg(WATER_FIELDS_SIZE, std::istream::cur);

			uint32_t resources = 0;
			readStream(inputStream, &resources, sizeof(resources));

			if (resourceNameOptional.has_value()) {
				const std::optional<std::string> &textureBoxNameOptional =
					getTextureBoxNameOptional(resourceNameOptional.value());

				if (textureBoxNameOptional.has_value()) {
					const std::string &textureBoxName =
						textureBoxNameOptional.value();

					for (uint32_t i = 0; i < resources; i++) {
						appendToMaskPathSet(inputStream, textureBoxMap[textureBoxName]);
					}
					return;
				}
			}

			for (uint32_t i = 0; i < resources; i++) {
				createResourcePointer(inputStream);
			}
		}

		std::optional<std::string> Water::getTextureBoxNameOptional(const std::string &resourceName) {
			static constexpr char PERIOD = '.';

			std::string::size_type periodIndex = resourceName.find(PERIOD);

			if (periodIndex == std::string::npos) {
				return std::nullopt;
			}

			// modifying these contexts would be hard, but seems unnecessary
			// so this is not implemented
			static const std::string CONTEXT_GLOBAL = "global";
			static const std::string CONTEXT_SHARED = "shared";

			const std::string &context = resourceName.substr(0, periodIndex);

			if (context == CONTEXT_GLOBAL || context == CONTEXT_SHARED) {
				return std::nullopt;
			}

			return resourceName.substr(
				periodIndex + sizeof(PERIOD),
				std::string::npos
			);
		}

		Water::Water(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap)
			: Resource(loaderPointer, VERSION) {
			create(inputStream, textureBoxMap);
		}

		Water::Water(Loader::POINTER loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, VERSION) {
			RLE::TEXTURE_BOX_MAP textureBoxMap = {};
			create(inputStream, textureBoxMap);
		}

		InteractiveOffsetProvider::InteractiveOffsetProvider(Loader::POINTER loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, VERSION) {
			static constexpr size_t FIELDS_SIZE = 33;
			inputStream.seekg(FIELDS_SIZE, std::istream::cur);
		}

		TextureAlignedOffsetProvider::TextureAlignedOffsetProvider(Loader::POINTER loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, VERSION) {
			static constexpr size_t FIELDS_SIZE = 65;
			inputStream.seekg(FIELDS_SIZE, std::istream::cur);
		}

		void StateData::create(std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet) {
			uint32_t nbrAliases = 0;
			readStream(inputStream, &nbrAliases, sizeof(nbrAliases));

			for (uint32_t i = 0; i < nbrAliases; i++) {
				String::readOptional(inputStream);
			}

			static constexpr size_t REFRESH_RATE_SIZE = 4;
			inputStream.seekg(REFRESH_RATE_SIZE, std::istream::cur);

			std::optional<std::string> maskPathOptional = String::readOptionalEncrypted(inputStream);

			if (maskPathOptional.has_value()) {
				maskPathSet.insert(maskPathOptional.value());
			}

			uint32_t resources = 0;
			readStream(inputStream, &resources, sizeof(resources));

			for (uint32_t i = 0; i < resources; i++) {
				createResourcePointer(inputStream);
			}

			static constexpr size_t WATER_FACE_BILERP_FIELDS_SIZE = 6;
			inputStream.seekg(WATER_FACE_BILERP_FIELDS_SIZE, std::istream::cur);
		}

		StateData::StateData(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet)
			: Resource(loaderPointer, VERSION) {
			create(inputStream, maskPathSet);
		}

		StateData::StateData(Loader::POINTER loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, VERSION) {
			RLE::MASK_PATH_SET maskPathSet = {};
			create(inputStream, maskPathSet);
		}

		HeaderCopier::HeaderCopier(std::streamsize fileSize, const std::streampos &filePosition)
			: fileSize(fileSize),
			filePosition(filePosition) {
		}

		void HeaderReader::throwReadPastEnd() {
			if (fileSize < inputStream.tellg() - filePosition) {
				throw ReadPastEnd();
			}
		}

		HeaderReader::HeaderReader(std::istream &inputStream, std::streamsize fileSize)
			: HeaderCopier(fileSize, inputStream.tellg()),
			inputStream(inputStream) {
			ID id = 0;
			readStream(inputStream, &id, sizeof(id));
			throwReadPastEnd();

			// we only support the UBI_B0_L ID
			// I obviously understand the actual game uses a serializer that
			// can also read/write text, but this appears totally unused
			// so I'm not implementing it
			if (id != UBI_B0_L) {
				throw Invalid();
			}
		}

		HeaderReader::~HeaderReader() {
			throwReadPastEnd();
		}

		void HeaderWriter::throwWrotePastEnd() {
			if (fileSize < outputStream.tellp() - filePosition) {
				throw WrotePastEnd();
			}
		}

		HeaderWriter::HeaderWriter(std::ostream &outputStream, std::streamsize fileSize)
			: HeaderCopier(fileSize, outputStream.tellp()),
			outputStream(outputStream) {
			writeStream(outputStream, &UBI_B0_L, sizeof(UBI_B0_L));
			throwWrotePastEnd();
		}

		HeaderWriter::~HeaderWriter() {
			throwWrotePastEnd();
		}

		void readFileHeader(
			std::istream &inputStream, std::optional<HeaderReader> &headerReaderOptional, std::streamsize size
		) {
			headerReaderOptional = std::nullopt;

			if (size != -1) {
				headerReaderOptional.emplace(inputStream, size);
			}
		}

		void writeFileHeader(
			std::ostream &outputStream, std::optional<HeaderWriter> &headerWriterOptional, std::streamsize size
		) {
			headerWriterOptional = std::nullopt;

			if (size != -1) {
				headerWriterOptional.emplace(outputStream, size);
			}
		}

		Resource::Loader::POINTER readFileLoader(
			std::istream &inputStream, std::optional<HeaderReader> &headerReaderOptional, std::streamsize size
		) {
			readFileHeader(inputStream, headerReaderOptional, size);
			return std::make_shared<Resource::Loader>(inputStream);
		}

		Resource::POINTER createResourcePointer(std::istream &inputStream, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::POINTER loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);

			switch (loaderPointer->id) {
				case TextureBox::ID:
				return std::make_shared<TextureBox>(loaderPointer, inputStream);
				case Water::ID:
				return std::make_shared<Water>(loaderPointer, inputStream);
				case InteractiveOffsetProvider::ID:
				return std::make_shared<InteractiveOffsetProvider>(loaderPointer, inputStream);
				case TextureAlignedOffsetProvider::ID:
				return std::make_shared<TextureAlignedOffsetProvider>(loaderPointer, inputStream);
				case StateData::ID:
				return std::make_shared<StateData>(loaderPointer, inputStream);
			}
			return nullptr;
		}

		Resource::POINTER appendToLayerMap(std::istream &inputStream, RLE::LAYER_MAP &layerMap, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::POINTER loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);
			Resource::POINTER resourcePointer = nullptr;

			switch (loaderPointer->id) {
				case TextureBox::ID:
				resourcePointer = std::make_shared<TextureBox>(loaderPointer, inputStream, layerMap);
			}
			return resourcePointer;
		}

		Resource::POINTER appendToTextureBoxMap(std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::POINTER loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);
			Resource::POINTER resourcePointer = nullptr;

			switch (loaderPointer->id) {
				case Water::ID:
				resourcePointer = std::make_shared<Water>(loaderPointer, inputStream, textureBoxMap);
			}
			return resourcePointer;
		}

		Resource::POINTER appendToMaskPathSet(std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::POINTER loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);
			Resource::POINTER resourcePointer = nullptr;

			switch (loaderPointer->id) {
				case StateData::ID:
				resourcePointer = std::make_shared<StateData>(loaderPointer, inputStream, maskPathSet);
			}
			return resourcePointer;
		}
	}

	BigFile::Path::Path(const NAME_VECTOR &directoryNameVector, const std::string &fileName)
		: directoryNameVector(directoryNameVector),
		fileName(fileName) {
	}

	BigFile::Path::Path(const std::string &copyString) {
		create(copyString);
	}

	BigFile::Path &BigFile::Path::operator=(const std::string &assignString) {
		clear();
		return create(assignString);
	}

	void BigFile::Path::clear() {
		directoryNameVector = {};
		fileName = "";
	}

	BigFile::Path& BigFile::Path::create(const std::string &file) {
		static constexpr char SEPERATOR = '/';

		// split up a string into a Path object
		std::string::size_type begin = 0;
		std::string::size_type end = 0;

		while ((begin = file.find_first_not_of(SEPERATOR, end)) != std::string::npos) {
			end = file.find(SEPERATOR, begin);

			if (end == std::string::npos) {
				fileName = file.substr(begin, std::string::npos);
				return *this;
			}

			directoryNameVector.push_back(file.substr(begin, end - begin));
		}
		return *this;
	}

	BigFile::File::File(std::istream &inputStream, SIZE &fileSystemSize, const std::optional<File> &layerFileOptional) {
		read(inputStream);
		rename(layerFileOptional);

		fileSystemSize += (SIZE)(
			sizeof(String::SIZE)

			+ (
				nameOptional.has_value()
				? nameOptional.value().size() + 1
				: 0
			)

			+ sizeof(size)
			+ sizeof(offset)
		);
	}

	BigFile::File::File(std::istream &inputStream) {
		read(inputStream);
	}

	BigFile::File::File(SIZE inputFileSize) : size(inputFileSize) {
	}

	void BigFile::File::write(std::ostream &outputStream) const {
		String::writeOptional(outputStream, nameOptional);
		writeStream(outputStream, &size, sizeof(size));
		writeStream(outputStream, &offset, sizeof(offset));
	}

	Binary::Resource::POINTER BigFile::File::appendToLayerMap(
		std::istream &inputStream,
		SIZE fileSystemOffset,
		Binary::RLE::LAYER_MAP &layerMap
	) const {
		std::streampos position = inputStream.tellg();
		Binary::Resource::POINTER resourcePointer = nullptr;

		try {
			inputStream.seekg(fileSystemOffset + (std::streamoff)this->offset);
			resourcePointer = Binary::appendToLayerMap(inputStream, layerMap, this->size);
		} catch (...) {
			// fail silently
		}

		inputStream.seekg(position);
		return resourcePointer;
	}

	Binary::Resource::POINTER BigFile::File::appendToTextureBoxMap(
		std::istream &inputStream,
		SIZE fileSystemOffset,
		Binary::RLE::TEXTURE_BOX_MAP &textureBoxMap
	) const {
		std::streampos position = inputStream.tellg();
		Binary::Resource::POINTER resourcePointer = nullptr;
	
		try {
			inputStream.seekg(fileSystemOffset + (std::streamoff)this->offset);
			resourcePointer = Binary::appendToTextureBoxMap(inputStream, textureBoxMap, this->size);
		} catch (...) {
			// fail silently
		}

		inputStream.seekg(position);
		return resourcePointer;
	}

	void BigFile::File::read(std::istream &inputStream) {
		nameOptional = String::readOptional(inputStream);
		readStream(inputStream, &size, sizeof(size));
		readStream(inputStream, &offset, sizeof(offset));
	}

	void BigFile::File::rename(const std::optional<File> &layerFileOptional) {
		#ifdef RENAME_ENABLED
		// predetermines what the new name will be after conversion
		// this is necessary so we will know the offset of the files before writing them
		if (!nameOptional.has_value()) {
			return;
		}

		const std::string &name = nameOptional.value();

		// note that these are case insensitive, because Myst 4 also uses case insensitive name extensions
		TYPE_EXTENSION_MAP::const_iterator nameTypeExtensionMapIterator =
			NAME_TYPE_EXTENSION_MAP.find(getNameExtension(name));

		if (nameTypeExtensionMapIterator == NAME_TYPE_EXTENSION_MAP.end()) {
			return;
		}

		type = nameTypeExtensionMapIterator->second.type;
		
		#ifdef LAYERS_ENABLED
		if (type == TYPE::IMAGE_STANDARD || type == TYPE::IMAGE_ZAP) {
			// only rename images in layers
			if (!layerFileOptional.has_value()) {
				type = TYPE::NONE;
				return;
			}

			const Binary::RLE::Layer &layer = layerFileOptional.value().layerMapIterator->second;

			if (layer.isLayerMask) {
				#ifdef GREYSCALE_ENABLED
				//greyScale = true;
				#else
				type = TYPE::NONE;
				return;
				#endif
			}

			#ifdef RGBA_ENABLED
			if (isWaterSlice(name, layer.waterMaskMap)) {
				rgba = true;
			}
			#endif
		}
		#endif

		const std::string &extension = nameTypeExtensionMapIterator->second.extension;

		nameOptional = name.substr(
			0,
			name.length() - extension.length() - sizeof(PERIOD)
		)

		+ PERIOD
		+ extension;
		#endif
	}

	std::string BigFile::File::getNameExtension(const std::string &name) {
		std::string::size_type periodIndex = name.rfind(PERIOD);

		return periodIndex == std::string::npos
		? ""

		: name.substr(
			periodIndex + sizeof(PERIOD),
			std::string::npos
		);
	}

	bool BigFile::File::isWaterSlice(const std::string &name, const Binary::RLE::MASK_MAP &waterMaskMap) {
		if (waterMaskMap.empty()) {
			return false;
		}

		// note: for TextureBox this must be lowercase
		// even though the file extension is case-insensitive
		static const std::regex FACE_SLICE(R"(^([a-z]+)_(\d{2})_(\d{2})\.)");

		std::smatch matches = {};

		if (!std::regex_search(name, matches, FACE_SLICE)
			|| matches.length() <= 3) {
			return false;
		}

		const std::string &faceStr = matches[1];

		Binary::RLE::FACE_STR_MAP::const_iterator faceStrMapIterator =
			Binary::RLE::WATER_SLICE_FACE_STR_MAP.find(faceStr);

		if (faceStrMapIterator == Binary::RLE::WATER_SLICE_FACE_STR_MAP.end()) {
			return false;
		}

		Binary::RLE::MASK_MAP::const_iterator waterMaskMapIterator =
			waterMaskMap.find(faceStrMapIterator->second);

		if (waterMaskMapIterator == waterMaskMap.end()) {
			return false;
		}

		// since these have leading zeros, I use base 10 specifically
		// (the ROW/COL should not be misinterpreted as octal)
		static constexpr int BASE = 10;

		const std::string &rowStr = matches[2];

		unsigned long row = 0;

		if (!stringToLong(rowStr.c_str(), row, BASE)) {
			return false;
		}

		const Binary::RLE::SLICE_MAP &sliceMap = waterMaskMapIterator->second;

		Binary::RLE::SLICE_MAP::const_iterator sliceMapIterator = sliceMap.find(row);

		if (sliceMapIterator == sliceMap.end()) {
			return false;
		}

		const std::string &colStr = matches[3];

		unsigned long col = 0;

		if (!stringToLong(colStr.c_str(), col, BASE)) {
			return false;
		}

		const Binary::RLE::COL_SET &colSet = sliceMapIterator->second;
		return colSet.find(col) != colSet.end();
	}

	const BigFile::File::TYPE_EXTENSION_MAP BigFile::File::NAME_TYPE_EXTENSION_MAP = {
		{"m4b", {TYPE::BIG_FILE, "m4b"}},
		{"bin", {TYPE::BINARY, "bin"}},
		{"jpg", {TYPE::IMAGE_STANDARD, "dds"}},
		{"zap", {TYPE::IMAGE_ZAP, "dds"}}
	};

	const std::string BigFile::Directory::NAME_CUBE = "cube";
	const std::string BigFile::Directory::NAME_WATER = "water";

	BigFile::Directory::Directory(
		Directory* ownerDirectory,
		std::istream &inputStream,
		File::SIZE &fileSystemSize,
		File::POINTER_VECTOR::size_type &files,
		File::POINTER_SET_MAP &filePointerSetMap,
		const std::optional<File> &layerFileOptional
	)
		: nameOptional(String::readOptional(inputStream)) {
		read((bool)ownerDirectory, inputStream, fileSystemSize, files, filePointerSetMap, layerFileOptional);
	}

	BigFile::Directory::Directory(std::istream &inputStream)
		: nameOptional(String::readOptional(inputStream)) {
		// in this case it is the same as not having an owner
		File::SIZE fileSystemSize = 0;
		File::POINTER_VECTOR::size_type files = 0;
		File::POINTER_SET_MAP filePointerSetMap = {};
		read(false, inputStream, fileSystemSize, files, filePointerSetMap, std::nullopt);
	}

	BigFile::Directory::Directory(std::istream &inputStream, const Path &path,
		File::POINTER &filePointer) {
		find(inputStream, path, path.directoryNameVector.begin(), filePointer);
	}

	BigFile::Directory::Directory(std::istream &inputStream, const Path &path,
		Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, File::POINTER &filePointer) {
		find(inputStream, path, directoryNameVectorIterator, filePointer);
	}

	void BigFile::Directory::write(std::ostream &outputStream) const {
		String::writeOptional(outputStream, nameOptional);

		DIRECTORY_VECTOR_SIZE directoryVectorSize = (DIRECTORY_VECTOR_SIZE)directoryVector.size();
		writeStream(outputStream, &directoryVectorSize, sizeof(directoryVectorSize));

		for (
			Directory::VECTOR::const_iterator directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			directoryVectorIterator->write(outputStream);
		}

		FILE_POINTER_VECTOR_SIZE filePointerVectorSize = (FILE_POINTER_VECTOR_SIZE)(filePointerVector.size()
			+ binaryFilePointerVector.size());

		writeStream(outputStream, &filePointerVectorSize, sizeof(filePointerVectorSize));

		for (
			File::POINTER_VECTOR::const_iterator filePointerVectorIterator = filePointerVector.begin();
			filePointerVectorIterator != filePointerVector.end();
			filePointerVectorIterator++
		) {
			(*filePointerVectorIterator)->write(outputStream);
		}

		for (
			File::POINTER_VECTOR::const_iterator binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
			binaryFilePointerVectorIterator != binaryFilePointerVector.end();
			binaryFilePointerVectorIterator++
		) {
			(*binaryFilePointerVectorIterator)->write(outputStream);
		}
	}

	BigFile::File::POINTER BigFile::Directory::find(const Path &path) const {
		return find(path, path.directoryNameVector.begin());
	}

	void BigFile::Directory::appendToLayerMap(
		std::istream &inputStream,
		File::SIZE fileSystemOffset,
		Binary::RLE::LAYER_MAP &layerMap
	) const {
		appendToLayerMap(inputStream, fileSystemOffset, layerMap, binaryFilePointerVector);

		for (
			VECTOR::const_iterator directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			appendToLayerMap(inputStream, fileSystemOffset, layerMap, directoryVectorIterator->binaryFilePointerVector);
		}
	}

	void BigFile::Directory::appendToTextureBoxMap(
		std::istream &inputStream,
		File::SIZE fileSystemOffset,
		Binary::RLE::TEXTURE_BOX_MAP &textureBoxMap
	) const {
		appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap, binaryFilePointerVector);

		for (
			VECTOR::const_iterator directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap, directoryVectorIterator->binaryFilePointerVector);
		}
	}

	void BigFile::Directory::read(
		bool owner,
		std::istream &inputStream,
		File::SIZE &fileSystemSize,
		File::POINTER_VECTOR::size_type &files,
		File::POINTER_SET_MAP &filePointerSetMap,
		const std::optional<File> &layerFileOptional
	) {
		DIRECTORY_VECTOR_SIZE directoryVectorSize = 0;
		readStream(inputStream, &directoryVectorSize, sizeof(directoryVectorSize));

		directoryVector.reserve(directoryVectorSize);

		bool bftex = !owner

		&& (
			nameOptional.has_value()
			? nameOptional == "bftex"
			: true
		);

		for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
			directoryVector.emplace_back(
				this,
				inputStream,
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

		bool set = isSet(bftex, layerFileOptional);

		File::POINTER filePointer = nullptr;

		FILE_POINTER_VECTOR_SIZE filePointerVectorSize = 0;
		readStream(inputStream, &filePointerVectorSize, sizeof(filePointerVectorSize));

		for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
			filePointer = std::make_shared<File>(
				inputStream,
				fileSystemSize,

				set
				? layerFileOptional
				: std::nullopt
			);

			const File &file = *filePointer;

			if (file.type == File::TYPE::BINARY) {
				binaryFilePointerVector.push_back(filePointer);
			} else {
				filePointerVector.push_back(filePointer);
			}

			filePointerSetMap[file.offset].insert(filePointer);
		}

		files += filePointerVectorSize;

		fileSystemSize += (File::SIZE)(
			sizeof(String::SIZE)

			+ (
				nameOptional.has_value()
				? nameOptional.value().size() + 1
				: 0
			)

			+ sizeof(directoryVectorSize)
			+ sizeof(filePointerVectorSize)
		);
	}

	void BigFile::Directory::find(std::istream &inputStream, const Path &path,
		Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, File::POINTER &filePointer) {
		filePointer = nullptr;

		const Path::NAME_VECTOR &DIRECTORY_NAME_VECTOR = path.directoryNameVector;

		// isMatch must be called here, modifies directoryNameVectorIterator
		nameOptional = String::readOptional(inputStream);
		bool match = isMatch(DIRECTORY_NAME_VECTOR, directoryNameVectorIterator);

		DIRECTORY_VECTOR_SIZE directoryVectorSize = 0;
		readStream(inputStream, &directoryVectorSize, sizeof(directoryVectorSize));

		if (directoryNameVectorIterator == DIRECTORY_NAME_VECTOR.end()) {
			// in this case we just read the directories and don't bother checking filePointer
			for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
				Directory directory(
					inputStream,
					path,
					directoryNameVectorIterator,
					filePointer
				);
			}
		} else {
			directoryVector.reserve(directoryVectorSize);

			for (DIRECTORY_VECTOR_SIZE i = 0; i < directoryVectorSize; i++) {
				directoryVector.emplace_back(
					inputStream,
					path,
					directoryNameVectorIterator,
					filePointer
				);

				// if this is true we found the matching file, so exit early
				if (filePointer) {
					// erase all but the last element
					// (there should always be at least one element in the vector at this point)
					directoryVector.erase(directoryVector.begin(), directoryVector.end() - 1);
					return;
				}
			}

			directoryVector = {};
		}

		FILE_POINTER_VECTOR_SIZE filePointerVectorSize = 0;
		readStream(inputStream, &filePointerVectorSize, sizeof(filePointerVectorSize));

		if (match) {
			filePointerVector.reserve(filePointerVectorSize);

			for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
				filePointer = std::make_shared<File>(inputStream);
				filePointerVector.push_back(filePointer);

				// is this the file we are looking for?
				if (filePointer->nameOptional == path.fileName) {
					// erase all but the last element
					// (there should always be at least one element in the vector at this point)
					filePointerVector.erase(filePointerVector.begin(), filePointerVector.end() - 1);
					return;
				}
			}

			filePointerVector = {};
		} else {
			for (FILE_POINTER_VECTOR_SIZE i = 0; i < filePointerVectorSize; i++) {
				File file(inputStream);
			}
		}
	}

	BigFile::File::POINTER BigFile::Directory::find(const Path &path,
		Path::NAME_VECTOR::const_iterator directoryNameVectorIterator) const {
		const Path::NAME_VECTOR &DIRECTORY_NAME_VECTOR = path.directoryNameVector;

		// isMatch must be called here, modifies directoryNameVectorIterator
		bool match = isMatch(DIRECTORY_NAME_VECTOR, directoryNameVectorIterator);
		File::POINTER filePointer = nullptr;

		if (directoryNameVectorIterator != DIRECTORY_NAME_VECTOR.end()) {
			for (
				VECTOR::const_iterator directoryVectorIterator = directoryVector.begin();
				directoryVectorIterator != directoryVector.end();
				directoryVectorIterator++
			) {
				filePointer = directoryVectorIterator->find(path, directoryNameVectorIterator);

				// if this is true we found the matching file, so exit early
				if (filePointer) {
					return filePointer;
				}
			}
		}

		if (!match) {
			return nullptr;
		}

		for (
			File::POINTER_VECTOR::const_iterator filePointerVectorIterator = filePointerVector.begin();
			filePointerVectorIterator != filePointerVector.end();
			filePointerVectorIterator++
		) {
			filePointer = *filePointerVectorIterator;

			// is this the file we are looking for?
			if (filePointer && filePointer->nameOptional == path.fileName) {
				return filePointer;
			}
		}
		return nullptr;
	}

	bool BigFile::Directory::isMatch(const Path::NAME_VECTOR &directoryNameVector,
		Path::NAME_VECTOR::const_iterator &directoryNameVectorIterator) const {
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

	bool BigFile::Directory::isSet(bool bftex, const std::optional<File> &layerFileOptional) const {
		if (bftex) {
			return false;
		}

		if (!layerFileOptional.has_value()) {
			return false;
		}

		const File &layerFile = layerFileOptional.value();

		Binary::RLE::LAYER_MAP_POINTER layerMapPointer = layerFile.layerMapPointer;

		if (!layerMapPointer) {
			return false;
		}

		// as per usual, if we don't have a name, anything matches
		if (!nameOptional.has_value()) {
			return true;
		}

		const Binary::RLE::SETS_SET &setsSet = layerFile.layerMapIterator->second.setsSet;
		return setsSet.find(nameOptional.value()) != setsSet.end();
	}

	void BigFile::Directory::appendToLayerMap(
		std::istream &inputStream,
		File::SIZE fileSystemOffset,
		Binary::RLE::LAYER_MAP &layerMap,
		const File::POINTER_VECTOR &binaryFilePointerVector
	) const {
		for (
			File::POINTER_VECTOR::const_iterator binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
			binaryFilePointerVectorIterator != binaryFilePointerVector.end();
			binaryFilePointerVectorIterator++
		) {
			(*binaryFilePointerVectorIterator)->appendToLayerMap(inputStream, fileSystemOffset, layerMap);
		}
	}

	void BigFile::Directory::appendToTextureBoxMap(
		std::istream &inputStream,
		File::SIZE fileSystemOffset,
		Binary::RLE::TEXTURE_BOX_MAP &textureBoxMap,
		const File::POINTER_VECTOR &binaryFilePointerVector
	) const {
		for (
			File::POINTER_VECTOR::const_iterator binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
			binaryFilePointerVectorIterator != binaryFilePointerVector.end();
			binaryFilePointerVectorIterator++
		) {
			(*binaryFilePointerVectorIterator)->appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap);
		}
	}

	BigFile::Header::Header(std::istream &inputStream, File::SIZE &fileSystemSize, File::SIZE &fileSystemOffset) {
		fileSystemOffset = (File::SIZE)inputStream.tellg();
		read(inputStream);

		fileSystemSize += (File::SIZE)(
			sizeof(String::SIZE)

			+ SIGNATURE.size() + 1
			+ sizeof(VERSION)
		);
	}

	BigFile::Header::Header(std::istream &inputStream) {
		read(inputStream);
	}

	BigFile::Header::Header(std::istream &inputStream, File::POINTER &filePointer) {
		// for path vectors
		if (filePointer) {
			inputStream.seekg(filePointer->offset);
		}

		read(inputStream);
	}

	void BigFile::Header::write(std::ostream &outputStream) const {
		String::writeOptional(outputStream, SIGNATURE);
		writeStream(outputStream, &CURRENT_VERSION, sizeof(CURRENT_VERSION));
	}

	void BigFile::Header::read(std::istream &inputStream) {
		bool nullTerminator = true;
		std::optional<std::string> signatureOptional =
			String::readOptional(inputStream, nullTerminator, (Ubi::String::SIZE)(SIGNATURE.size() + 1));

		// must exactly match, case sensitively
		if (signatureOptional != SIGNATURE) {
			throw Invalid();
		}

		VERSION version = 0;
		readStream(inputStream, &version, sizeof(version));

		if (version != CURRENT_VERSION) {
			throw Invalid();
		}
	}

	const std::string BigFile::Header::SIGNATURE = "UBI_BF_SIG";

	BigFile::File::POINTER BigFile::findFile(std::istream &stream, const Path::VECTOR &pathVector) {
		stream.seekg(0);

		File::POINTER filePointer = nullptr;
		std::streamoff offset = 0;

		for (
			Path::VECTOR::const_iterator pathVectorIterator = pathVector.begin();
			pathVectorIterator != pathVector.end();
			pathVectorIterator++
		) {
			BigFile bigFile(stream, *pathVectorIterator, filePointer);

			if (!filePointer) {
				throw std::logic_error("filePointer must not be nullptr");
			}

			stream.seekg(offset + (std::streamoff)filePointer->offset);
			offset = stream.tellg();
		}
		return filePointer;
	}

	BigFile::BigFile(
		std::istream &inputStream,
		File::SIZE &fileSystemSize,
		File::POINTER_VECTOR::size_type &files,
		File::POINTER_SET_MAP &filePointerSetMap,
		File &file
	)
		: header(inputStream, fileSystemSize, fileSystemOffset),
		directory(0, inputStream, fileSystemSize, files, filePointerSetMap, file) {
		// do all the steps necessary to prevent water causing a crash
		// note: the Binarizer seems hardcoded to put cubes and water in a cube and water directory
		// so we use that fact instead of loading every file in binarizer_loader.log like the game does
		#ifdef LAYERS_ENABLED
		const Directory::VECTOR &directoryVector = directory.directoryVector;

		Directory::VECTOR_ITERATOR_VECTOR cubeVectorIterators = {};
		Directory::VECTOR_ITERATOR_VECTOR waterVectorIterators = {};

		for (
			Directory::VECTOR::const_iterator directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			const std::optional<std::string> &nameOptional = directoryVectorIterator->nameOptional;

			if (nameOptional.has_value()) {
				const std::string &NAME = nameOptional.value();

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
		Binary::RLE::LAYER_MAP &layerMap = *layerMapPointer;

		for (
			Directory::VECTOR_ITERATOR_VECTOR::iterator cubeVectorIteratorsIterator = cubeVectorIterators.begin();
			cubeVectorIteratorsIterator != cubeVectorIterators.end();
			cubeVectorIteratorsIterator++
		) {
			(*cubeVectorIteratorsIterator)->appendToLayerMap(inputStream, fileSystemOffset, layerMap);
		}

		if (layerMap.empty()) {
			return;
		}

		Binary::RLE::TEXTURE_BOX_MAP textureBoxMap = {};

		for (
			Directory::VECTOR_ITERATOR_VECTOR::iterator waterVectorIteratorsIterator = waterVectorIterators.begin();
			waterVectorIteratorsIterator != waterVectorIterators.end();
			waterVectorIteratorsIterator++
		) {
			(*waterVectorIteratorsIterator)->appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap);
		}

		std::streampos position = inputStream.tellg();

		SCOPE_EXIT {
			inputStream.seekg(position);
		};

		File::POINTER layerFilePointer = nullptr;
		std::streamoff maskFileSystemOffset = 0;
		Binary::RLE::FACE_STR_MAP::const_iterator fileFaceStrMapIterator = {};
		Binary::RLE::MASK_MAP::iterator waterMaskMapIterator = {};

		for (
			Binary::RLE::LAYER_MAP::iterator layerMapIterator = layerMap.begin();
			layerMapIterator != layerMap.end();
			layerMapIterator++
		) {
			for (
				Binary::RLE::TEXTURE_BOX_MAP::iterator textureBoxMapIterator = textureBoxMap.begin();
				textureBoxMapIterator != textureBoxMap.end();
				textureBoxMapIterator++
			) {
				Binary::RLE::Layer &layer = layerMapIterator->second;

				if (layer.textureBoxNameOptional != textureBoxMapIterator->first) {
					continue;
				}

				const Binary::RLE::MASK_PATH_SET &maskPathSet = textureBoxMapIterator->second;

				Binary::RLE::MASK_MAP &waterMaskMap = layer.waterMaskMap;

				for (
					Binary::RLE::MASK_PATH_SET::const_iterator maskPathSetIterator = maskPathSet.begin();
					maskPathSetIterator != maskPathSet.end();
					maskPathSetIterator++
				) {
					layerFilePointer = directory.find(*maskPathSetIterator);

					if (!layerFilePointer) {
						continue;
					}

					maskFileSystemOffset = (std::streamoff)fileSystemOffset
						+ (std::streamoff)layerFilePointer->offset;

					inputStream.seekg(maskFileSystemOffset);

					BigFile maskBigFile(inputStream);

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

						fileFaceStrMapIterator = Binary::RLE::FILE_FACE_STR_MAP.find(maskFile.nameOptional.value());

						if (fileFaceStrMapIterator == Binary::RLE::FILE_FACE_STR_MAP.end()) {
							continue;
						}

						inputStream.seekg(maskFileSystemOffset + (std::streamoff)maskFile.offset);

						Binary::RLE::appendToSliceMap(inputStream, maskFile.size,
							waterMaskMap[fileFaceStrMapIterator->second]);
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

	BigFile::BigFile(std::istream &inputStream)
		: header(inputStream),
		directory(inputStream) {
	}

	BigFile::BigFile(std::istream &inputStream, const Path &path,
		File::POINTER &filePointer)
		: header(inputStream, filePointer),
		directory(inputStream, path, filePointer) {
	}

	void BigFile::write(std::ostream &outputStream) const {
		header.write(outputStream);
		directory.write(outputStream);
	}
}