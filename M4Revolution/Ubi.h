#pragma once
#include "shared.h"
#include <optional>
#include <unordered_set>
#include <map>
#include <variant>
#include <vector>

#define CONVERSION_ENABLED

namespace Ubi {
	namespace String {
		typedef uint32_t SIZE;
		static const size_t SIZE_SIZE = sizeof(SIZE);

		std::optional<std::string> readOptional(std::ifstream &inputFileStream, bool &nullTerminator);
		std::optional<std::string> readOptional(std::ifstream &inputFileStream);
		void writeOptional(std::ofstream &outputFileStream, const std::optional<std::string> &strOptional, bool nullTerminator = true);
		std::string &swizzle(std::string &encryptedString);
	};

	namespace Binary {
		class Invalid : public std::invalid_argument {
			public:
			Invalid() noexcept : std::invalid_argument("Binary invalid") {
			}
		};

		class NotImplemented : public std::logic_error {
			public:
			NotImplemented() noexcept : std::logic_error("Binary not implemented") {
			}
		};

		class ReadPastEnd : public std::runtime_error {
			public:
			ReadPastEnd() noexcept : std::runtime_error("Binary read past end") {
			}
		};

		namespace RLE {
			enum struct FACE {
				BACK,
				FRONT,
				LEFT,
				RIGHT,
				TOP,
				BOTTOM
			};

			typedef uint32_t ROW;
			typedef uint32_t COL;
			typedef std::unordered_set<COL> COL_SET;
			typedef std::map<ROW, COL_SET> SLICE_MAP;
			typedef std::map<FACE, SLICE_MAP> MASK_MAP;
			typedef std::unordered_set<std::string> MASK_NAME_SET;
			typedef std::variant<MASK_NAME_SET, MASK_MAP> MASK_VARIANT;
			typedef std::map<std::string, MASK_VARIANT> TEXTURE_BOX_NAME_MASK_VARIANT_MAP;

			static SLICE_MAP readFile(std::ifstream &inputFileStream, std::streamsize size);
		};

		class Resource {
			public:
			typedef uint32_t ID;
			typedef uint32_t VERSION;
			typedef std::shared_ptr<Resource> POINTER;

			struct Loader {
				typedef std::shared_ptr<Loader> POINTER;

				ID id = 0;
				VERSION version = 1;
				std::optional<std::string> name = std::nullopt;

				Loader(std::ifstream &inputFileStream);
			};

			Resource(Loader::POINTER loaderPointer);
			Resource(const Resource &resource) = delete;
			Resource &operator=(const Resource &resource) = delete;

			protected:
			Loader::POINTER loaderPointer = 0;
		};

		namespace {
			class TextureBox : public virtual Resource {
				public:
				static const Resource::ID ID = 15;
				static const Resource::VERSION VERSION = 5;

				std::optional<std::string> layerFileOptional = "";

				TextureBox(Loader::POINTER loaderPointer, std::ifstream &inputFileStream);
				TextureBox(const TextureBox &textureBox) = delete;
				TextureBox &operator=(const TextureBox &textureBox) = delete;
			};

			class Water : public virtual Resource {
				private:
				void create(std::ifstream &inputFileStream, RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap);

				public:
				static const Resource::ID ID = 42;
				static const Resource::VERSION VERSION = 1;

				std::optional<std::string> textureBoxNameOptional = "";

				Water(Loader::POINTER loaderPointer, std::ifstream &inputFileStream, RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap);
				Water(Loader::POINTER loaderPointer, std::ifstream &inputFileStream);
				Water(const Water &water) = delete;
				Water &operator=(const Water &water) = delete;
			};

			class InteractiveOffsetProvider : public virtual Resource {
				public:
				static const Resource::ID ID = 43;
				static const Resource::VERSION VERSION = 1;

				InteractiveOffsetProvider(Loader::POINTER loaderPointer, std::ifstream &inputFileStream);
				InteractiveOffsetProvider(const InteractiveOffsetProvider &interactiveOffsetProvider) = delete;
				InteractiveOffsetProvider &operator=(const InteractiveOffsetProvider &interactiveOffsetProvider) = delete;
			};

			class TextureAlignedOffsetProvider : public virtual Resource {
				public:
				static const Resource::ID ID = 44;
				static const Resource::VERSION VERSION = 1;

				TextureAlignedOffsetProvider(Loader::POINTER loaderPointer, std::ifstream &inputFileStream);
				TextureAlignedOffsetProvider(const TextureAlignedOffsetProvider &textureAlignedOffsetProvider) = delete;
				TextureAlignedOffsetProvider &operator=(const TextureAlignedOffsetProvider &textureAlignedOffsetProvider) = delete;
			};

			class StateData : public virtual Resource {
				private:
				void create(std::ifstream &inputFileStream, RLE::MASK_NAME_SET &maskNameSet);

				public:
				static const Resource::ID ID = 45;
				static const Resource::VERSION VERSION = 1;

				std::optional<std::string> maskFileOptional = "";

				StateData(Loader::POINTER loaderPointer, std::ifstream &inputFileStream, RLE::MASK_NAME_SET &maskNameSet);
				StateData(Loader::POINTER loaderPointer, std::ifstream &inputFileStream);
				StateData(const StateData &stateData) = delete;
				StateData &operator=(const StateData &stateData) = delete;
			};

			class Header {
				private:
				void testReadPastEnd();

				typedef uint64_t ID;

				// "ubi/b0-l"
				static const ID UBI_B0_L = 0x6C2D30622F696275;

				std::ifstream &inputFileStream;
				std::streamsize fileSize = 0;
				std::streampos filePosition = 0;

				public:
				Header(std::ifstream &inputFileStream, std::streamsize fileSize);
				~Header();
				Header(const Header &header) = delete;
				Header &operator=(const Header &header) = delete;
			};
		};

		Resource::POINTER createResourcePointer(std::ifstream &inputFileStream);
		void getMaskNameSet(std::ifstream &inputFileStream, RLE::MASK_NAME_SET &maskNameSet);
		void readFile(std::ifstream &inputFileStream, std::streamsize size, RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap);
	};

	struct BigFile {
		typedef std::shared_ptr<BigFile> POINTER;

		struct Path {
			typedef std::vector<Path> VECTOR;
			typedef std::vector<std::string> NAME_VECTOR;

			NAME_VECTOR directoryNameVector = {};
			std::string fileName = "";
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
				BINARY_RESOURCE_IMAGE_DATA,
				BIG_FILE,
				JPEG,
				ZAP
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

			TYPE type = TYPE::NONE;

			File(std::ifstream &inputFileStream, SIZE &fileSystemSize, bool texture);
			File(std::ifstream &inputFileStream);
			File(SIZE inputFileSize);
			void write(std::ofstream &outputFileStream) const;
			void appendToTextureBoxNameMaskVariantMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap);

			private:
			void read(std::ifstream &inputFileStream);
			std::string getNameExtension();

			struct TypeExtension {
				TYPE type = TYPE::NONE;
				std::string extension = "";
			};

			typedef std::map<std::string, TypeExtension> TYPE_EXTENSION_MAP;

			static const char PERIOD = '.';
			static const TYPE_EXTENSION_MAP NAME_TYPE_EXTENSION_MAP;
		};

		struct Directory {
			typedef std::vector<Directory> VECTOR;
			typedef uint8_t DIRECTORY_VECTOR_SIZE;
			typedef uint32_t FILE_POINTER_VECTOR_SIZE;
			typedef std::vector<FILE_POINTER_VECTOR_SIZE> FILE_POINTER_VECTOR_SIZE_VECTOR;

			std::optional<std::string> nameOptional = std::nullopt;

			static const size_t DIRECTORY_VECTOR_SIZE_SIZE = sizeof(DIRECTORY_VECTOR_SIZE);
			Directory::VECTOR directoryVector = {};

			static const size_t FILE_POINTER_VECTOR_SIZE_SIZE = sizeof(FILE_POINTER_VECTOR_SIZE);
			File::POINTER_VECTOR binaryFilePointerVector = {};
			File::POINTER_VECTOR filePointerVector = {};

			Directory(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::SIZE &fileSystemPosition, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &fileVectorIteratorSetMap);
			Directory(std::ifstream &inputFileStream, const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, std::optional<File> &fileOptional);
			void write(std::ofstream &outputFileStream) const;
			void appendToTextureBoxNameMaskVariantMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap);

			private:
			void appendToTextureBoxNameMaskVariantMap(std::ifstream &inputFileStream, File::SIZE fileSystemPosition, Binary::RLE::TEXTURE_BOX_NAME_MASK_VARIANT_MAP &textureBoxNameMaskVariantMap, File::POINTER_VECTOR &binaryFilePointerVector);

			static const std::string NAME_TEXTURE;
			static const std::string NAME_WATER;
		};

		struct Header {
			typedef uint32_t VERSION;

			static const size_t VERSION_SIZE = sizeof(VERSION);

			class Invalid : public std::invalid_argument {
				public:
				Invalid() noexcept : std::invalid_argument("Header invalid") {
				}
			};

			Header(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::SIZE &fileSystemPosition);
			Header(std::ifstream &inputFileStream, std::optional<File> &fileOptional);
			void write(std::ofstream &outputFileStream) const;

			private:
			void read(std::ifstream &inputFileStream);

			static const std::string SIGNATURE;
			static const VERSION CURRENT_VERSION = 1;
		};

		File::SIZE fileSystemPosition = 0;
		Header header;
		Directory directory;

		BigFile(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_VECTOR::size_type &files, File::POINTER_SET_MAP &fileVectorIteratorSetMap);
		BigFile(std::ifstream &inputFileStream, const Path &path, std::optional<File> &fileOptional);
		void write(std::ofstream &outputFileStream) const;
	};
};