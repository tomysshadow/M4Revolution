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
				auto encryptedStringIterator = encryptedString.begin();
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

		std::optional<std::string> readOptional(std::istream &inputStream, bool &nullTerminator, Size maxSize) {
			Size size = 0;
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
			Size size = strOptional.has_value() ? (Size)(strOptional.value().size() + nullTerminator) : 0;
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
		namespace Rle {
			void appendToSliceMap(std::istream &inputStream, std::streamsize size, SliceMap &sliceMap) {
				std::optional<HeaderReader> headerReaderOptional = std::nullopt;
				readFileHeader(inputStream, headerReaderOptional, size);

				uint32_t waterSlices = 0;

				Row sliceRow = 0;
				Col sliceCol = 0;

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

		Resource::Resource(Loader::Pointer loaderPointer, Version version)
			: LOADER_POINTER(loaderPointer) {
			if (version < loaderPointer->version) {
				throw Invalid();
			}
		}

		class TextureBox: public Resource {
			private:
			void create(std::istream &inputStream, Rle::LayerMap &layerMap);

			public:
			static constexpr Resource::Id Id = 15;
			static constexpr Resource::Version Version = 5;

			TextureBox(Loader::Pointer loaderPointer, std::istream &inputStream, Rle::LayerMap &layerMap);
			TextureBox(Loader::Pointer loaderPointer, std::istream &inputStream);
		};

		class Water: public Resource {
			private:
			void create(std::istream &inputStream, Rle::TextureBoxMap &textureBoxMap);

			static std::optional<std::string> getTextureBoxNameOptional(const std::string &resourceName);

			public:
			static constexpr Resource::Id Id = 42;
			static constexpr Resource::Version Version = 1;

			Water(Loader::Pointer loaderPointer, std::istream &inputStream, Rle::TextureBoxMap &textureBoxMap);
			Water(Loader::Pointer loaderPointer, std::istream &inputStream);
		};

		class InteractiveOffsetProvider: public Resource {
			public:
			static constexpr Resource::Id Id = 43;
			static constexpr Resource::Version Version = 1;

			InteractiveOffsetProvider(Loader::Pointer loaderPointer, std::istream &inputStream);
		};

		class TextureAlignedOffsetProvider: public Resource {
			public:
			static constexpr Resource::Id Id = 44;
			static constexpr Resource::Version Version = 1;

			TextureAlignedOffsetProvider(Loader::Pointer loaderPointer, std::istream &inputStream);
		};

		class StateData: public Resource {
			private:
			void create(std::istream &inputStream, Rle::MaskPathSet &maskPathSet);

			public:
			static constexpr Resource::Id Id = 45;
			static constexpr Resource::Version Version = 1;

			StateData(Loader::Pointer loaderPointer, std::istream &inputStream, Rle::MaskPathSet &maskPathSet);
			StateData(Loader::Pointer loaderPointer, std::istream &inputStream);
		};

		void TextureBox::create(std::istream &inputStream, Rle::LayerMap &layerMap) {
			Rle::Layer* layerPointer = nullptr;

			auto layerFileOptional = String::readOptionalEncrypted(inputStream);

			if (layerFileOptional.has_value()) {
				Rle::Layer &layer = layerMap[layerFileOptional.value()];
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

		TextureBox::TextureBox(Loader::Pointer loaderPointer, std::istream &inputStream, Rle::LayerMap &layerMap)
			: Resource(loaderPointer, Version) {
			create(inputStream, layerMap);
		}

		TextureBox::TextureBox(Loader::Pointer loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, Version) {
			Rle::LayerMap layerMap = {};
			create(inputStream, layerMap);
		}

		void Water::create(std::istream &inputStream, Rle::TextureBoxMap &textureBoxMap) {
			auto resourceNameOptional = String::readOptionalEncrypted(inputStream);

			static constexpr size_t WATER_FIELDS_SIZE = 9; // AssignReflectionAlpha, ReflectionAlphaAtEdge, ReflectionAlphaAtHorizon
			inputStream.seekg(WATER_FIELDS_SIZE, std::istream::cur);

			uint32_t resources = 0;
			readStream(inputStream, &resources, sizeof(resources));

			if (resourceNameOptional.has_value()) {
				const auto &textureBoxNameOptional =
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

		Water::Water(Loader::Pointer loaderPointer, std::istream &inputStream, Rle::TextureBoxMap &textureBoxMap)
			: Resource(loaderPointer, Version) {
			create(inputStream, textureBoxMap);
		}

		Water::Water(Loader::Pointer loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, Version) {
			Rle::TextureBoxMap textureBoxMap = {};
			create(inputStream, textureBoxMap);
		}

		InteractiveOffsetProvider::InteractiveOffsetProvider(Loader::Pointer loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, Version) {
			static constexpr size_t FIELDS_SIZE = 33;
			inputStream.seekg(FIELDS_SIZE, std::istream::cur);
		}

		TextureAlignedOffsetProvider::TextureAlignedOffsetProvider(Loader::Pointer loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, Version) {
			static constexpr size_t FIELDS_SIZE = 65;
			inputStream.seekg(FIELDS_SIZE, std::istream::cur);
		}

		void StateData::create(std::istream &inputStream, Rle::MaskPathSet &maskPathSet) {
			uint32_t nbrAliases = 0;
			readStream(inputStream, &nbrAliases, sizeof(nbrAliases));

			for (uint32_t i = 0; i < nbrAliases; i++) {
				String::readOptional(inputStream);
			}

			static constexpr size_t REFRESH_RATE_SIZE = 4;
			inputStream.seekg(REFRESH_RATE_SIZE, std::istream::cur);

			auto maskPathOptional = String::readOptionalEncrypted(inputStream);

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

		StateData::StateData(Loader::Pointer loaderPointer, std::istream &inputStream, Rle::MaskPathSet &maskPathSet)
			: Resource(loaderPointer, Version) {
			create(inputStream, maskPathSet);
		}

		StateData::StateData(Loader::Pointer loaderPointer, std::istream &inputStream)
			: Resource(loaderPointer, Version) {
			Rle::MaskPathSet maskPathSet = {};
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
			Id id = 0;
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

		Resource::Loader::Pointer readFileLoader(
			std::istream &inputStream, std::optional<HeaderReader> &headerReaderOptional, std::streamsize size
		) {
			readFileHeader(inputStream, headerReaderOptional, size);
			return std::make_shared<Resource::Loader>(inputStream);
		}

		Resource::Pointer createResourcePointer(std::istream &inputStream, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::Pointer loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);

			switch (loaderPointer->id) {
				case TextureBox::Id:
				return std::make_shared<TextureBox>(loaderPointer, inputStream);
				case Water::Id:
				return std::make_shared<Water>(loaderPointer, inputStream);
				case InteractiveOffsetProvider::Id:
				return std::make_shared<InteractiveOffsetProvider>(loaderPointer, inputStream);
				case TextureAlignedOffsetProvider::Id:
				return std::make_shared<TextureAlignedOffsetProvider>(loaderPointer, inputStream);
				case StateData::Id:
				return std::make_shared<StateData>(loaderPointer, inputStream);
			}
			return nullptr;
		}

		Resource::Pointer appendToLayerMap(std::istream &inputStream, Rle::LayerMap &layerMap, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::Pointer loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);
			Resource::Pointer resourcePointer = nullptr;

			switch (loaderPointer->id) {
				case TextureBox::Id:
				resourcePointer = std::make_shared<TextureBox>(loaderPointer, inputStream, layerMap);
			}
			return resourcePointer;
		}

		Resource::Pointer appendToTextureBoxMap(std::istream &inputStream, Rle::TextureBoxMap &textureBoxMap, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::Pointer loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);
			Resource::Pointer resourcePointer = nullptr;

			switch (loaderPointer->id) {
				case Water::Id:
				resourcePointer = std::make_shared<Water>(loaderPointer, inputStream, textureBoxMap);
			}
			return resourcePointer;
		}

		Resource::Pointer appendToMaskPathSet(std::istream &inputStream, Rle::MaskPathSet &maskPathSet, std::streamsize size) {
			std::optional<HeaderReader> headerReaderOptional = std::nullopt;
			Resource::Loader::Pointer loaderPointer = readFileLoader(inputStream, headerReaderOptional, size);
			Resource::Pointer resourcePointer = nullptr;

			switch (loaderPointer->id) {
				case StateData::Id:
				resourcePointer = std::make_shared<StateData>(loaderPointer, inputStream, maskPathSet);
			}
			return resourcePointer;
		}
	}

	BigFile::Path::Path(const NameVector &directoryNameVector, const std::string &fileName)
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

	BigFile::File::File(std::istream &inputStream, Size &fileSystemSize, const std::optional<File> &layerFileOptional) {
		read(inputStream);
		rename(layerFileOptional);

		fileSystemSize += (Size)(
			sizeof(String::Size)

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

	BigFile::File::File(Size inputFileSize) : size(inputFileSize) {
	}

	void BigFile::File::write(std::ostream &outputStream) const {
		String::writeOptional(outputStream, nameOptional);
		writeStream(outputStream, &size, sizeof(size));
		writeStream(outputStream, &offset, sizeof(offset));
	}

	Binary::Resource::Pointer BigFile::File::appendToLayerMap(
		std::istream &inputStream,
		Size fileSystemOffset,
		Binary::Rle::LayerMap &layerMap
	) const {
		std::streampos position = inputStream.tellg();
		Binary::Resource::Pointer resourcePointer = nullptr;

		try {
			inputStream.seekg(fileSystemOffset + (std::streamoff)this->offset);
			resourcePointer = Binary::appendToLayerMap(inputStream, layerMap, this->size);
		} catch (...) {
			// fail silently
		}

		inputStream.seekg(position);
		return resourcePointer;
	}

	Binary::Resource::Pointer BigFile::File::appendToTextureBoxMap(
		std::istream &inputStream,
		Size fileSystemOffset,
		Binary::Rle::TextureBoxMap &textureBoxMap
	) const {
		std::streampos position = inputStream.tellg();
		Binary::Resource::Pointer resourcePointer = nullptr;
	
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
		auto nameTypeExtensionMapIterator =
			NAME_TYPE_EXTENSION_MAP.find(getNameExtension(name));

		if (nameTypeExtensionMapIterator == NAME_TYPE_EXTENSION_MAP.end()) {
			return;
		}

		type = nameTypeExtensionMapIterator->second.type;
		
		#ifdef LAYERS_ENABLED
		if (type == Type::IMAGE_STANDARD || type == Type::IMAGE_ZAP) {
			// only rename images in layers
			if (!layerFileOptional.has_value()) {
				type = Type::NONE;
				return;
			}

			const Binary::Rle::Layer &layer = layerFileOptional.value().layerMapIterator->second;

			if (layer.isLayerMask) {
				#ifdef GREYSCALE_ENABLED
				//greyScale = true;
				#else
				type = Type::NONE;
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

	bool BigFile::File::isWaterSlice(const std::string &name, const Binary::Rle::MaskMap &waterMaskMap) {
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

		auto faceStrMapIterator =
			Binary::Rle::WATER_SLICE_FACE_STR_MAP.find(faceStr);

		if (faceStrMapIterator == Binary::Rle::WATER_SLICE_FACE_STR_MAP.end()) {
			return false;
		}

		auto waterMaskMapIterator =
			waterMaskMap.find(faceStrMapIterator->second);

		if (waterMaskMapIterator == waterMaskMap.end()) {
			return false;
		}

		// since these have leading zeros, I use base 10 specifically
		// (the Row/Col should not be misinterpreted as octal)
		static constexpr int BASE = 10;

		const std::string &rowStr = matches[2];

		unsigned long row = 0;

		if (!stringToLong(rowStr.c_str(), row, BASE)) {
			return false;
		}

		const Binary::Rle::SliceMap &sliceMap = waterMaskMapIterator->second;
		auto sliceMapIterator = sliceMap.find(row);

		if (sliceMapIterator == sliceMap.end()) {
			return false;
		}

		const std::string &colStr = matches[3];

		unsigned long col = 0;

		if (!stringToLong(colStr.c_str(), col, BASE)) {
			return false;
		}

		const Binary::Rle::ColSet &colSet = sliceMapIterator->second;
		return colSet.find(col) != colSet.end();
	}

	const BigFile::File::TypeExtensionMap BigFile::File::NAME_TYPE_EXTENSION_MAP = {
		{"m4b", {Type::BIG_FILE, "m4b"}},
		{"bin", {Type::BINARY, "bin"}},
		{"jpg", {Type::IMAGE_STANDARD, "dds"}},
		{"zap", {Type::IMAGE_ZAP, "dds"}}
	};

	const std::string BigFile::Directory::NAME_CUBE = "cube";
	const std::string BigFile::Directory::NAME_WATER = "water";

	BigFile::Directory::Directory(
		Directory* ownerDirectory,
		std::istream &inputStream,
		File::Size &fileSystemSize,
		File::PointerVector::size_type &files,
		File::PointerSetMap &filePointerSetMap,
		const std::optional<File> &layerFileOptional
	)
		: nameOptional(String::readOptional(inputStream)) {
		read((bool)ownerDirectory, inputStream, fileSystemSize, files, filePointerSetMap, layerFileOptional);
	}

	BigFile::Directory::Directory(std::istream &inputStream)
		: nameOptional(String::readOptional(inputStream)) {
		// in this case it is the same as not having an owner
		File::Size fileSystemSize = 0;
		File::PointerVector::size_type files = 0;
		File::PointerSetMap filePointerSetMap = {};
		read(false, inputStream, fileSystemSize, files, filePointerSetMap, std::nullopt);
	}

	BigFile::Directory::Directory(std::istream &inputStream, const Path &path,
		File::Pointer &filePointer) {
		find(inputStream, path, path.directoryNameVector.begin(), filePointer);
	}

	BigFile::Directory::Directory(std::istream &inputStream, const Path &path,
		Path::NameVector::const_iterator directoryNameVectorIterator, File::Pointer &filePointer) {
		find(inputStream, path, directoryNameVectorIterator, filePointer);
	}

	void BigFile::Directory::write(std::ostream &outputStream) const {
		String::writeOptional(outputStream, nameOptional);

		DirectoryVectorSize directoryVectorSize = (DirectoryVectorSize)directoryVector.size();
		writeStream(outputStream, &directoryVectorSize, sizeof(directoryVectorSize));

		for (
			auto directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			directoryVectorIterator->write(outputStream);
		}

		FilePointerVectorSize filePointerVectorSize = (FilePointerVectorSize)(filePointerVector.size()
			+ binaryFilePointerVector.size());

		writeStream(outputStream, &filePointerVectorSize, sizeof(filePointerVectorSize));

		for (
			auto filePointerVectorIterator = filePointerVector.begin();
			filePointerVectorIterator != filePointerVector.end();
			filePointerVectorIterator++
		) {
			(*filePointerVectorIterator)->write(outputStream);
		}

		for (
			auto binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
			binaryFilePointerVectorIterator != binaryFilePointerVector.end();
			binaryFilePointerVectorIterator++
		) {
			(*binaryFilePointerVectorIterator)->write(outputStream);
		}
	}

	BigFile::File::Pointer BigFile::Directory::find(const Path &path) const {
		return find(path, path.directoryNameVector.begin());
	}

	void BigFile::Directory::appendToLayerMap(
		std::istream &inputStream,
		File::Size fileSystemOffset,
		Binary::Rle::LayerMap &layerMap
	) const {
		appendToLayerMap(inputStream, fileSystemOffset, layerMap, binaryFilePointerVector);

		for (
			auto directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			appendToLayerMap(inputStream, fileSystemOffset, layerMap, directoryVectorIterator->binaryFilePointerVector);
		}
	}

	void BigFile::Directory::appendToTextureBoxMap(
		std::istream &inputStream,
		File::Size fileSystemOffset,
		Binary::Rle::TextureBoxMap &textureBoxMap
	) const {
		appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap, binaryFilePointerVector);

		for (
			auto directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap, directoryVectorIterator->binaryFilePointerVector);
		}
	}

	void BigFile::Directory::read(
		bool owner,
		std::istream &inputStream,
		File::Size &fileSystemSize,
		File::PointerVector::size_type &files,
		File::PointerSetMap &filePointerSetMap,
		const std::optional<File> &layerFileOptional
	) {
		DirectoryVectorSize directoryVectorSize = 0;
		readStream(inputStream, &directoryVectorSize, sizeof(directoryVectorSize));

		directoryVector.reserve(directoryVectorSize);

		bool bftex = !owner

		&& (
			nameOptional.has_value()
			? nameOptional == "bftex"
			: true
		);

		for (DirectoryVectorSize i = 0; i < directoryVectorSize; i++) {
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

		File::Pointer filePointer = nullptr;

		FilePointerVectorSize filePointerVectorSize = 0;
		readStream(inputStream, &filePointerVectorSize, sizeof(filePointerVectorSize));

		for (FilePointerVectorSize i = 0; i < filePointerVectorSize; i++) {
			filePointer = std::make_shared<File>(
				inputStream,
				fileSystemSize,

				set
				? layerFileOptional
				: std::nullopt
			);

			const File &file = *filePointer;

			if (file.type == File::Type::BINARY) {
				binaryFilePointerVector.push_back(filePointer);
			} else {
				filePointerVector.push_back(filePointer);
			}

			filePointerSetMap[file.offset].insert(filePointer);
		}

		files += filePointerVectorSize;

		fileSystemSize += (File::Size)(
			sizeof(String::Size)

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
		Path::NameVector::const_iterator directoryNameVectorIterator, File::Pointer &filePointer) {
		filePointer = nullptr;

		const auto &directoryNameVector = path.directoryNameVector;

		// isMatch must be called here, modifies directoryNameVectorIterator
		nameOptional = String::readOptional(inputStream);
		bool match = isMatch(directoryNameVector, directoryNameVectorIterator);

		DirectoryVectorSize directoryVectorSize = 0;
		readStream(inputStream, &directoryVectorSize, sizeof(directoryVectorSize));

		if (directoryNameVectorIterator == directoryNameVector.end()) {
			// in this case we just read the directories and don't bother checking filePointer
			for (DirectoryVectorSize i = 0; i < directoryVectorSize; i++) {
				Directory directory(
					inputStream,
					path,
					directoryNameVectorIterator,
					filePointer
				);
			}
		} else {
			directoryVector.reserve(directoryVectorSize);

			for (DirectoryVectorSize i = 0; i < directoryVectorSize; i++) {
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

		FilePointerVectorSize filePointerVectorSize = 0;
		readStream(inputStream, &filePointerVectorSize, sizeof(filePointerVectorSize));

		if (match) {
			filePointerVector.reserve(filePointerVectorSize);

			for (FilePointerVectorSize i = 0; i < filePointerVectorSize; i++) {
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
			for (FilePointerVectorSize i = 0; i < filePointerVectorSize; i++) {
				File file(inputStream);
			}
		}
	}

	BigFile::File::Pointer BigFile::Directory::find(const Path &path,
		Path::NameVector::const_iterator directoryNameVectorIterator) const {
		const Path::NameVector &directoryNameVector = path.directoryNameVector;

		// isMatch must be called here, modifies directoryNameVectorIterator
		bool match = isMatch(directoryNameVector, directoryNameVectorIterator);
		File::Pointer filePointer = nullptr;

		if (directoryNameVectorIterator != directoryNameVector.end()) {
			for (
				auto directoryVectorIterator = directoryVector.begin();
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
			auto filePointerVectorIterator = filePointerVector.begin();
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

	bool BigFile::Directory::isMatch(const Path::NameVector &directoryNameVector,
		Path::NameVector::const_iterator &directoryNameVectorIterator) const {
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

		Binary::Rle::LayerMapPointer layerMapPointer = layerFile.layerMapPointer;

		if (!layerMapPointer) {
			return false;
		}

		// as per usual, if we don't have a name, anything matches
		if (!nameOptional.has_value()) {
			return true;
		}

		const Binary::Rle::SetsSet &setsSet = layerFile.layerMapIterator->second.setsSet;
		return setsSet.find(nameOptional.value()) != setsSet.end();
	}

	void BigFile::Directory::appendToLayerMap(
		std::istream &inputStream,
		File::Size fileSystemOffset,
		Binary::Rle::LayerMap &layerMap,
		const File::PointerVector &binaryFilePointerVector
	) const {
		for (
			auto binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
			binaryFilePointerVectorIterator != binaryFilePointerVector.end();
			binaryFilePointerVectorIterator++
		) {
			(*binaryFilePointerVectorIterator)->appendToLayerMap(inputStream, fileSystemOffset, layerMap);
		}
	}

	void BigFile::Directory::appendToTextureBoxMap(
		std::istream &inputStream,
		File::Size fileSystemOffset,
		Binary::Rle::TextureBoxMap &textureBoxMap,
		const File::PointerVector &binaryFilePointerVector
	) const {
		for (
			auto binaryFilePointerVectorIterator = binaryFilePointerVector.begin();
			binaryFilePointerVectorIterator != binaryFilePointerVector.end();
			binaryFilePointerVectorIterator++
		) {
			(*binaryFilePointerVectorIterator)->appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap);
		}
	}

	BigFile::Header::Header(std::istream &inputStream, File::Size &fileSystemSize, File::Size &fileSystemOffset) {
		fileSystemOffset = (File::Size)inputStream.tellg();
		read(inputStream);

		fileSystemSize += (File::Size)(
			sizeof(String::Size)

			+ SIGNATURE.size() + 1
			+ sizeof(Version)
		);
	}

	BigFile::Header::Header(std::istream &inputStream) {
		read(inputStream);
	}

	BigFile::Header::Header(std::istream &inputStream, File::Pointer &filePointer) {
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
		auto signatureOptional =
			String::readOptional(inputStream, nullTerminator, (Ubi::String::Size)(SIGNATURE.size() + 1));

		// must exactly match, case sensitively
		if (signatureOptional != SIGNATURE) {
			throw Invalid();
		}

		Version version = 0;
		readStream(inputStream, &version, sizeof(version));

		if (version != CURRENT_VERSION) {
			throw Invalid();
		}
	}

	const std::string BigFile::Header::SIGNATURE = "UBI_BF_SIG";

	BigFile::File::Pointer BigFile::findFile(std::istream &stream, const Path::Vector &pathVector) {
		stream.seekg(0);

		File::Pointer filePointer = nullptr;
		std::streamoff offset = 0;

		for (
			auto pathVectorIterator = pathVector.begin();
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
		File::Size &fileSystemSize,
		File::PointerVector::size_type &files,
		File::PointerSetMap &filePointerSetMap,
		File &file
	)
		: header(inputStream, fileSystemSize, fileSystemOffset),
		directory(0, inputStream, fileSystemSize, files, filePointerSetMap, file) {
		// do all the steps necessary to prevent water causing a crash
		// note: the Binarizer seems hardcoded to put cubes and water in a cube and water directory
		// so we use that fact instead of loading every file in binarizer_loader.log like the game does
		#ifdef LAYERS_ENABLED
		const Directory::Vector &directoryVector = directory.directoryVector;

		Directory::VectorIteratorVector cubeVectorIterators = {};
		Directory::VectorIteratorVector waterVectorIterators = {};

		for (
			auto directoryVectorIterator = directoryVector.begin();
			directoryVectorIterator != directoryVector.end();
			directoryVectorIterator++
		) {
			const auto &nameOptional = directoryVectorIterator->nameOptional;

			if (nameOptional.has_value()) {
				const std::string &name = nameOptional.value();

				if (name == Directory::NAME_CUBE) {
					cubeVectorIterators.push_back(directoryVectorIterator);
				} else if (name == Directory::NAME_WATER) {
					waterVectorIterators.push_back(directoryVectorIterator);
				}
			}
		}

		if (cubeVectorIterators.empty()) {
			return;
		}

		Binary::Rle::LayerMapPointer layerMapPointer = std::make_shared<Binary::Rle::LayerMap>();
		Binary::Rle::LayerMap &layerMap = *layerMapPointer;

		for (
			auto cubeVectorIteratorsIterator = cubeVectorIterators.begin();
			cubeVectorIteratorsIterator != cubeVectorIterators.end();
			cubeVectorIteratorsIterator++
		) {
			(*cubeVectorIteratorsIterator)->appendToLayerMap(inputStream, fileSystemOffset, layerMap);
		}

		if (layerMap.empty()) {
			return;
		}

		Binary::Rle::TextureBoxMap textureBoxMap = {};

		for (
			auto waterVectorIteratorsIterator = waterVectorIterators.begin();
			waterVectorIteratorsIterator != waterVectorIterators.end();
			waterVectorIteratorsIterator++
		) {
			(*waterVectorIteratorsIterator)->appendToTextureBoxMap(inputStream, fileSystemOffset, textureBoxMap);
		}

		std::streampos position = inputStream.tellg();

		SCOPE_EXIT {
			inputStream.seekg(position);
		};

		File::Pointer layerFilePointer = nullptr;
		std::streamoff maskFileSystemOffset = 0;
		Binary::Rle::FaceStrMap::const_iterator fileFaceStrMapIterator = {};
		Binary::Rle::MaskMap::iterator waterMaskMapIterator = {};

		for (
			auto layerMapIterator = layerMap.begin();
			layerMapIterator != layerMap.end();
			layerMapIterator++
		) {
			for (
				auto textureBoxMapIterator = textureBoxMap.begin();
				textureBoxMapIterator != textureBoxMap.end();
				textureBoxMapIterator++
			) {
				Binary::Rle::Layer &layer = layerMapIterator->second;

				if (layer.textureBoxNameOptional != textureBoxMapIterator->first) {
					continue;
				}

				const Binary::Rle::MaskPathSet &maskPathSet = textureBoxMapIterator->second;

				Binary::Rle::MaskMap &waterMaskMap = layer.waterMaskMap;

				for (
					auto maskPathSetIterator = maskPathSet.begin();
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

					File::PointerVector &maskFilePointerVector = maskBigFile.directory.filePointerVector;

					for (
						auto maskFilePointerVectorIterator = maskFilePointerVector.begin();
						maskFilePointerVectorIterator != maskFilePointerVector.end();
						maskFilePointerVectorIterator++
					) {
						File &maskFile = **maskFilePointerVectorIterator;

						if (!maskFile.nameOptional.has_value()) {
							continue;
						}

						fileFaceStrMapIterator = Binary::Rle::FILE_FACE_STR_MAP.find(maskFile.nameOptional.value());

						if (fileFaceStrMapIterator == Binary::Rle::FILE_FACE_STR_MAP.end()) {
							continue;
						}

						inputStream.seekg(maskFileSystemOffset + (std::streamoff)maskFile.offset);

						Binary::Rle::appendToSliceMap(inputStream, maskFile.size,
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
		File::Pointer &filePointer)
		: header(inputStream, filePointer),
		directory(inputStream, path, filePointer) {
	}

	void BigFile::write(std::ostream &outputStream) const {
		header.write(outputStream);
		directory.write(outputStream);
	}
}