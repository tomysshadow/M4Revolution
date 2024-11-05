#pragma once
#include "shared.h"
#include "IgnoreCaseComparer.h"
#include <unordered_set>
#include <map>
#include <vector>

#define RENAME_ENABLED
#define LAYERS_ENABLED
//#define GREYSCALE_ENABLED
#define RGBA_ENABLED

namespace Ubi {
	namespace String {
		typedef uint32_t SIZE;
		static const size_t SIZE_SIZE = sizeof(SIZE);

		std::optional<std::string> &swizzle(std::optional<std::string> &encryptedStringOptional);
		std::optional<std::string> readOptional(std::istream &inputStream, bool &nullTerminator);
		std::optional<std::string> readOptional(std::istream &inputStream);
		std::optional<std::string> readOptionalEncrypted(std::istream &inputStream);
		void writeOptional(std::ostream &outputStream, const std::optional<std::string> &strOptional, bool nullTerminator = true);
		void writeOptionalEncrypted(std::ostream &outputStream, std::optional<std::string> &strOptional);
	};

	namespace Binary {
		class Invalid : public std::invalid_argument {
			public:
			Invalid() noexcept : std::invalid_argument("Binary invalid") {
			}
		};

		class ReadPastEnd : public std::runtime_error {
			public:
			ReadPastEnd() noexcept : std::runtime_error("Binary read past end") {
			}
		};

		class WrotePastEnd : public std::runtime_error {
			public:
			WrotePastEnd() noexcept : std::runtime_error("Binary wrote past end") {
			}
		};

		namespace BinarizerLoader {
			typedef std::vector<std::optional<std::string>> RESOURCE_OPTIONAL_VECTOR;

			static const std::string AI_SND_TRANSITION_RESOURCE = "/common/ai/aisndtransition/ai_snd_transition.ai";

			RESOURCE_OPTIONAL_VECTOR getResourceOptionalVector(std::istream &inputStream, std::streamsize size);
			void setResourceOptionalVector(std::ostream &outputStream, std::streamsize size, const RESOURCE_OPTIONAL_VECTOR &resourceOptionalVector);
		};

		/*
		for water slices, DXT5 is not supported and an uncompressed format must be used instead
		in order to detect which images are water slices, we need to read the RLE files
		which are pointed to by the Water files (typically named water.bin)
		*/
		namespace RLE {
			enum struct FACE {
				BACK,
				FRONT,
				LEFT,
				RIGHT,
				TOP,
				BOTTOM
			};

			// these maps take the water slice/file name and turn them into faces
			typedef std::map<std::string, FACE> FACE_STR_MAP;

			static const FACE_STR_MAP WATER_SLICE_FACE_STR_MAP = {
				{"back", Ubi::Binary::RLE::FACE::BACK},
				{"front", Ubi::Binary::RLE::FACE::FRONT},
				{"left", Ubi::Binary::RLE::FACE::LEFT},
				{"right", Ubi::Binary::RLE::FACE::RIGHT},
				{"top", Ubi::Binary::RLE::FACE::TOP},
				{"bottom", Ubi::Binary::RLE::FACE::BOTTOM}
			};

			static const FACE_STR_MAP FILE_FACE_STR_MAP = {
				{"back.rle", Ubi::Binary::RLE::FACE::BACK},
				{"front.rle", Ubi::Binary::RLE::FACE::FRONT},
				{"left.rle", Ubi::Binary::RLE::FACE::LEFT},
				{"right.rle", Ubi::Binary::RLE::FACE::RIGHT},
				{"top.rle", Ubi::Binary::RLE::FACE::TOP},
				{"bottom.rle", Ubi::Binary::RLE::FACE::BOTTOM}
			};

			// the Water file specifies what resources it is intended to affect
			// this set has the name of those resources as its key
			// the value is the name of the masks (there can be multiple) which
			// contain the RLE files to use for them
			typedef std::unordered_set<std::string> MASK_PATH_SET;
			typedef std::map<std::string, MASK_PATH_SET> TEXTURE_BOX_MAP;

			// each layer contains sets, within which are the slices
			// this is a set of all those sets for a given layer (confusing, I know, but the strings should be unique)
			typedef std::unordered_set<std::string> SETS_SET;

			// MASK_MAP is a map of FACE > ROW > COL
			// this allows us to tell which slices are water slices
			// e.g. if the map has a BACK face with ROW 1 and COL 1, then 
			// the file back_01_01.jpg is a water slice
			typedef uint32_t ROW;
			typedef uint32_t COL;
			typedef std::unordered_set<COL> COL_SET;
			typedef std::map<ROW, COL_SET> SLICE_MAP;
			typedef std::map<FACE, SLICE_MAP> MASK_MAP;

			struct Layer {
				std::optional<std::string> textureBoxNameOptional = std::nullopt;
				SETS_SET setsSet = {};
				bool isLayerMask = false;
				MASK_MAP waterMaskMap = {};
			};

			typedef std::map<std::string, Layer> LAYER_MAP;
			typedef std::shared_ptr<LAYER_MAP> LAYER_MAP_POINTER;

			SLICE_MAP createSliceMap(std::istream &inputStream, std::streamsize size);
		};

		// this is the abstract class on which all resources are based
		class Resource {
			public:
			typedef uint32_t ID;
			typedef uint32_t VERSION;
			typedef std::shared_ptr<Resource> POINTER;

			// this struct contains the ID and Version of the resource
			// this is used to determine which resource type to create
			struct Loader {
				typedef std::shared_ptr<Loader> POINTER;

				ID id = 0;
				VERSION version = 1;
				std::optional<std::string> nameOptional = std::nullopt;

				Loader(std::istream &inputStream);
			};

			const Loader::POINTER LOADER_POINTER;

			Resource(Loader::POINTER loaderPointer, VERSION version);
			Resource(const Resource &resource) = delete;
			Resource &operator=(const Resource &resource) = delete;
		};

		// anonymous namespace so these can't be created directly, instead you need
		// to go through createResourcePointer
		namespace {
			class TextureBox : public virtual Resource {
				private:
				void create(std::istream &inputStream, Binary::RLE::LAYER_MAP &layerMap);

				public:
				static const Resource::ID ID = 15;
				static const Resource::VERSION VERSION = 5;

				TextureBox(Loader::POINTER loaderPointer, std::istream &inputStream, Binary::RLE::LAYER_MAP &layerMap);
				TextureBox(Loader::POINTER loaderPointer, std::istream &inputStream);
				TextureBox(const TextureBox &textureBox) = delete;
				TextureBox &operator=(const TextureBox &textureBox) = delete;
			};

			class Water : public virtual Resource {
				private:
				void create(std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap);

				static std::optional<std::string> getTextureBoxNameOptional(const std::string &resourceName);

				public:
				static const Resource::ID ID = 42;
				static const Resource::VERSION VERSION = 1;

				Water(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap);
				Water(Loader::POINTER loaderPointer, std::istream &inputStream);
				Water(const Water &water) = delete;
				Water &operator=(const Water &water) = delete;
			};

			class InteractiveOffsetProvider : public virtual Resource {
				public:
				static const Resource::ID ID = 43;
				static const Resource::VERSION VERSION = 1;

				InteractiveOffsetProvider(Loader::POINTER loaderPointer, std::istream &inputStream);
				InteractiveOffsetProvider(const InteractiveOffsetProvider &interactiveOffsetProvider) = delete;
				InteractiveOffsetProvider &operator=(const InteractiveOffsetProvider &interactiveOffsetProvider) = delete;
			};

			class TextureAlignedOffsetProvider : public virtual Resource {
				public:
				static const Resource::ID ID = 44;
				static const Resource::VERSION VERSION = 1;

				TextureAlignedOffsetProvider(Loader::POINTER loaderPointer, std::istream &inputStream);
				TextureAlignedOffsetProvider(const TextureAlignedOffsetProvider &textureAlignedOffsetProvider) = delete;
				TextureAlignedOffsetProvider &operator=(const TextureAlignedOffsetProvider &textureAlignedOffsetProvider) = delete;
			};

			class StateData : public virtual Resource {
				private:
				void create(std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet);

				public:
				static const Resource::ID ID = 45;
				static const Resource::VERSION VERSION = 1;

				StateData(Loader::POINTER loaderPointer, std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet);
				StateData(Loader::POINTER loaderPointer, std::istream &inputStream);
				StateData(const StateData &stateData) = delete;
				StateData &operator=(const StateData &stateData) = delete;
			};
			
			class HeaderCopier {
				protected:
				typedef uint64_t ID;

				// "ubi/b0-l"
				static const ID UBI_B0_L = 0x6C2D30622F696275;

				std::streamsize fileSize = 0;
				std::streampos filePosition = 0;

				HeaderCopier(std::streamsize fileSize, std::streampos filePosition);
			};

			class HeaderReader : private HeaderCopier {
				private:
				void throwReadPastEnd();

				std::istream &inputStream;

				public:
				HeaderReader(std::istream &inputStream, std::streamsize fileSize);
				~HeaderReader();
				HeaderReader(const HeaderReader &headerReader) = delete;
				HeaderReader &operator=(const HeaderReader &headerReader) = delete;
			};

			class HeaderWriter : private HeaderCopier {
				private:
				void throwWrotePastEnd();

				std::ostream &outputStream;

				public:
				HeaderWriter(std::ostream &outputStream, std::streamsize fileSize);
				~HeaderWriter();
				HeaderWriter(const HeaderReader &headerWriter) = delete;
				HeaderWriter &operator=(const HeaderWriter &headerWriter) = delete;
			};
		};

		// a basic factory pattern going on here for the creation of resources
		void readFileHeader(std::istream &inputStream, std::optional<Ubi::Binary::HeaderReader> &headerReaderOptional, std::streamsize size = -1);
		void writeFileHeader(std::ostream &outputStream, std::optional<Ubi::Binary::HeaderWriter> &headerWriterOptional, std::streamsize size = -1);
		Resource::Loader::POINTER readFileLoader(std::istream &inputStream, std::optional<Ubi::Binary::HeaderReader> &headerReaderOptional, std::streamsize size = -1);
		Resource::POINTER createResourcePointer(std::istream &inputStream, std::streamsize size = -1);
		Resource::POINTER createLayerMap(std::istream &inputStream, Binary::RLE::LAYER_MAP &layerMap, std::streamsize size = -1);
		Resource::POINTER createMaskPathSet(std::istream &inputStream, RLE::MASK_PATH_SET &maskPathSet, std::streamsize size = -1);
		Resource::POINTER createTextureBoxMap(std::istream &inputStream, RLE::TEXTURE_BOX_MAP &textureBoxMap, std::streamsize size = -1);
	};

	struct BigFile {
		typedef std::shared_ptr<BigFile> POINTER;

		struct Path {
			typedef std::vector<Path> VECTOR;
			typedef std::vector<std::string> NAME_VECTOR;

			NAME_VECTOR directoryNameVector = {};
			std::string fileName = "";

			Path();
			Path(const NAME_VECTOR &directoryNameVector, const std::string &fileName);
			Path(const std::string &copyString);
			Path &operator=(const std::string &assignString);
			void clear();

			private:
			Path& create(const std::string &file);
		};

		struct File {
			typedef uint32_t SIZE;
			typedef std::shared_ptr<File> POINTER;
			typedef std::unordered_set<POINTER> POINTER_SET;
			typedef std::map<SIZE, POINTER_SET> POINTER_SET_MAP;
			typedef std::vector<POINTER> POINTER_VECTOR;
			typedef std::shared_ptr<POINTER_VECTOR> POINTER_VECTOR_POINTER;

			enum struct TYPE {
				NONE = 0,
				BINARY,
				BIG_FILE,
				IMAGE_STANDARD,
				IMAGE_ZAP
			};

			// the name in the output file (so example.dds, not example.jpg)
			std::optional<std::string> nameOptional = std::nullopt;

			// initially the size in the input file, to be potentially overwritten later (if converted)
			SIZE size = 0;
			static const size_t SIZE_SIZE = sizeof(size);

			// initially the position in the input file, to be overwritten later (with the stream position)
			SIZE position = 0;
			static const size_t POSITION_SIZE = sizeof(position);

			// the effective size of the file's padding (not stored to the file, used temporarily by the output thread)
			SIZE padding = 0;

			// used for water slices
			// if this file is a layer, layerInformationPointer is non-zero and
			// points to the layer information, and layerMapIterator is an iterator
			// into the layerMap field of layerInformationPointer (it is never the end of the map)
			Binary::RLE::LAYER_MAP_POINTER layerMapPointer = 0;
			Binary::RLE::LAYER_MAP::const_iterator layerMapIterator = {};

			// metadata for conversion
			TYPE type = TYPE::NONE;
			//bool greyScale = false;
			bool rgba = false;

			File(std::istream &inputStream, SIZE &fileSystemSize, const std::optional<File> &layerFileOptional);
			File(std::istream &inputStream);
			File(SIZE inputFileSize);
			void write(std::ostream &outputStream) const;

			Binary::Resource::POINTER createLayerMap(
				std::istream &inputStream, File::SIZE fileSystemPosition,
				Binary::RLE::LAYER_MAP &layerMap
			) const;

			Binary::Resource::POINTER appendToTextureBoxMap(
				std::istream &inputStream,
				File::SIZE fileSystemPosition,
				Binary::RLE::TEXTURE_BOX_MAP &textureBoxMap
			) const;

			private:
			void read(std::istream &inputStream);
			void rename(const std::optional<File> &layerFileOptional);

			static std::string getNameExtension(const std::string &name);
			static bool isWaterSlice(const std::string &name, const Binary::RLE::MASK_MAP &waterMaskMap);

			struct TypeExtension {
				TYPE type = TYPE::NONE;
				std::string extension = "";
			};

			typedef std::map<std::string, TypeExtension, IgnoreCaseComparer> TYPE_EXTENSION_MAP;

			static const TYPE_EXTENSION_MAP NAME_TYPE_EXTENSION_MAP;
			static const char PERIOD = '.';
		};

		struct Directory {
			typedef std::vector<Directory> VECTOR;
			typedef std::vector<VECTOR::const_iterator> VECTOR_ITERATOR_VECTOR;
			typedef uint8_t DIRECTORY_VECTOR_SIZE;
			typedef uint32_t FILE_POINTER_VECTOR_SIZE;

			// some directories with names that are hardcoded by the binarizer
			// normally these would be loaded through the binarizer's log file, but
			// we are only really interested in the ones in these particular directories
			static const std::string NAME_CUBE;
			static const std::string NAME_WATER;

			std::optional<std::string> nameOptional = std::nullopt;

			// the directories that this directory owns
			static const size_t DIRECTORY_VECTOR_SIZE_SIZE = sizeof(DIRECTORY_VECTOR_SIZE);
			Directory::VECTOR directoryVector = {};

			// the files that this directory owns
			// binaryFilePointerVector is seperate so we can easily loop just the Binary files
			// (this is useful for finding Water/Cube binary files)
			static const size_t FILE_POINTER_VECTOR_SIZE_SIZE = sizeof(FILE_POINTER_VECTOR_SIZE);
			File::POINTER_VECTOR binaryFilePointerVector = {};
			File::POINTER_VECTOR filePointerVector = {};

			Directory(
				Directory* ownerDirectory,
				std::istream &inputStream,
				File::SIZE &fileSystemSize,
				File::POINTER_VECTOR::size_type &files,
				File::POINTER_SET_MAP &filePointerSetMap,
				const std::optional<File> &layerFileOptional
			);
			
			Directory(std::istream &inputStream);
			Directory(std::istream &inputStream, const Path &path, File::POINTER &filePointer);
			Directory(std::istream &inputStream, const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, File::POINTER &filePointer);
			void write(std::ostream &outputStream) const;
			File::POINTER find(const Path &path) const;

			void createLayerMap(
				std::istream &inputStream,
				File::SIZE fileSystemPosition,
				Binary::RLE::LAYER_MAP &layerMap
			) const;

			void appendToTextureBoxMap(
				std::istream &inputStream,
				File::SIZE fileSystemPosition,
				Binary::RLE::TEXTURE_BOX_MAP &textureBoxMap
			) const;

			private:
			void read(
				bool owner,
				std::istream &inputStream,
				File::SIZE &fileSystemSize,
				File::POINTER_VECTOR::size_type &files,
				File::POINTER_SET_MAP &filePointerSetMap,
				const std::optional<File> &layerFileOptional
			);

			void find(std::istream &inputStream, const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, File::POINTER &filePointer);
			File::POINTER find(const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator) const;
			bool isMatch(const Path::NAME_VECTOR &directoryNameVector, Path::NAME_VECTOR::const_iterator &directoryNameVectorIterator) const;
			bool isSet(bool bftex, const std::optional<File> &layerFileOptional) const;

			void createLayerMap(
				std::istream &inputStream,
				File::SIZE fileSystemPosition,
				Binary::RLE::LAYER_MAP &layerMap,
				const File::POINTER_VECTOR &binaryFilePointerVector
			) const;

			void appendToTextureBoxMap(
				std::istream &inputStream,
				File::SIZE fileSystemPosition,
				Binary::RLE::TEXTURE_BOX_MAP &textureBoxMap,
				const File::POINTER_VECTOR &binaryFilePointerVector
			) const;
		};

		struct Header {
			typedef uint32_t VERSION;

			static const size_t VERSION_SIZE = sizeof(VERSION);

			class Invalid : public std::invalid_argument {
				public:
				Invalid() noexcept : std::invalid_argument("Header invalid") {
				}
			};

			Header(std::istream &inputStream, File::SIZE &fileSystemSize, File::SIZE &fileSystemPosition);
			Header(std::istream &inputStream);
			Header(std::istream &inputStream, File::POINTER &filePointer);
			void write(std::ostream &outputStream) const;

			private:
			void read(std::istream &inputStream);

			static const std::string SIGNATURE;
			static const VERSION CURRENT_VERSION = 1;
		};

		private:
		File::SIZE fileSystemPosition = 0;

		public:
		static File::POINTER findFile(std::istream &stream, const BigFile::Path::VECTOR &pathVector);

		Header header;
		Directory directory;

		BigFile(std::istream &inputStream, File::SIZE &fileSystemSize, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &filePointerSetMap, File &file);
		BigFile(std::istream &inputStream);
		BigFile(std::istream &inputStream, const Path &path, File::POINTER &filePointer);
		void write(std::ostream &outputStream) const;
	};
};