#pragma once
#include "shared.h"
#include <vector>
#include <unordered_set>
#include <map>
#include <optional>

#define CONVERSION_ENABLED

namespace Ubi {
	namespace String {
		typedef uint32_t SIZE;
		static const size_t SIZE_SIZE = sizeof(SIZE);

		std::optional<std::string> readOptional(std::ifstream &inputFileStream);
		void writeOptional(std::ofstream &outputFileStream, const std::optional<std::string> &strOptional);
	};

	struct BigFile {
		struct Path {
			typedef std::vector<Path> VECTOR;
			typedef std::vector<std::string> NAME_VECTOR;

			NAME_VECTOR directoryNameVector = {};
			std::string fileName = "";
		};

		struct File {
			typedef std::vector<File> VECTOR;
			typedef uint32_t SIZE;
			typedef std::unordered_set<File*> POINTER_SET;
			typedef std::map<SIZE, POINTER_SET> POINTER_SET_MAP;

			enum struct TYPE {
				NONE = 0,
				RECURSIVE,
				JPEG,
				ZAP,
				TEXTURE
			};

			// the name in the output file (so example.dds, not example.jpg)
			std::optional<std::string> nameOptional = std::nullopt;

			// initially the size in the input file, to be potentially overwritten later (if converted)
			SIZE size = 0;
			static const size_t SIZE_SIZE = sizeof(size);

			// initially the position in the input file, to be overwritten later (with the stream position)
			SIZE position = 0;
			static const size_t POSITION_SIZE = sizeof(position);

			TYPE type = TYPE::NONE;

			File(std::ifstream &inputFileStream, SIZE &fileSystemSize, bool texture);
			File(std::ifstream &inputFileStream);
			void write(std::ofstream &outputFileStream);

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
			typedef uint32_t FILE_VECTOR_SIZE;

			std::optional<std::string> nameOptional = std::nullopt;

			static const size_t DIRECTORY_VECTOR_SIZE_SIZE = sizeof(DIRECTORY_VECTOR_SIZE);
			Directory::VECTOR directoryVector = {};

			static const size_t FILE_VECTOR_SIZE_SIZE = sizeof(FILE_VECTOR_SIZE);
			File::VECTOR fileVector = {};

			Directory(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_SET_MAP &fileVectorIteratorSetMap);
			Directory(std::ifstream &inputFileStream, const Path &path, Path::NAME_VECTOR::const_iterator directoryNameVectorIterator, std::optional<File> &fileOptional);
			void write(std::ofstream &outputFileStream);

			private:
			static const std::string NAME_TEXTURE;
		};

		struct Header {
			typedef uint32_t VERSION;

			static const size_t VERSION_SIZE = sizeof(VERSION);

			class Invalid : public std::invalid_argument {
				public:
				Invalid() noexcept : std::invalid_argument("Header invalid") {
				}
			};

			Header(std::ifstream &inputFileStream, File::SIZE &fileSystemSize);
			Header(std::ifstream &inputFileStream, std::optional<File> &fileOptional);
			void write(std::ofstream &outputFileStream);

			private:
			void read(std::ifstream &inputFileStream);

			static const std::string SIGNATURE;
			static const VERSION CURRENT_VERSION = 1;
		};

		Header header;
		Directory directory;

		BigFile(std::ifstream &inputFileStream, File::SIZE &fileSystemSize, File::POINTER_SET_MAP &fileVectorIteratorSetMap);
		BigFile(std::ifstream &inputFileStream, const Path &path, std::optional<File> &fileOptional);
		void write(std::ofstream &outputFileStream);
	};
};