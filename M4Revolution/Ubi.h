#pragma once
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>

#define RENAME_ENABLED
#define LAYERS_ENABLED
//#define GREYSCALE_ENABLED
#define RGBA_ENABLED

namespace Ubi {
	namespace String {
		using Size = uint32_t;

		std::optional<std::string> &swizzle(std::optional<std::string> &encryptedStringOptional);
		std::optional<std::string> readOptional(std::istream &inputStream, bool &nullTerminator, Size maxSize = (Size)-1);
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

		class ReadPastEnd : public std::out_of_range {
			public:
			ReadPastEnd() noexcept : std::out_of_range("Binary read past end") {
			}
		};

		class WrotePastEnd : public std::out_of_range {
			public:
			WrotePastEnd() noexcept : std::out_of_range("Binary wrote past end") {
			}
		};

		/*
		for water slices, DXT is not supported and RGBA must be used instead
		in order to detect which images are water slices, we need to read the RLE files
		which are pointed to by the Water files (typically named water.bin)
		*/
		namespace Rle {
			enum struct Face {
				BACK,
				FRONT,
				LEFT,
				RIGHT,
				TOP,
				BOTTOM
			};

			// these maps take the water slice/file name and turn them into faces
			using FaceStrMap = std::map<std::string, Face, std::less<>>;

			static const FaceStrMap WATER_SLICE_FACE_STR_MAP = {
				{"back", Face::BACK},
				{"front", Face::FRONT},
				{"left", Face::LEFT},
				{"right", Face::RIGHT},
				{"top", Face::TOP},
				{"bottom", Face::BOTTOM}
			};

			static const FaceStrMap FILE_FACE_STR_MAP = {
				{"back.rle", Face::BACK},
				{"front.rle", Face::FRONT},
				{"left.rle", Face::LEFT},
				{"right.rle", Face::RIGHT},
				{"top.rle", Face::TOP},
				{"bottom.rle", Face::BOTTOM}
			};

			// the Water file specifies what resources it is intended to affect
			// this set has the name of those resources as its key
			// the value is the name of the masks (there can be multiple) which
			// contain the RLE files to use for them
			using MaskPathSet = std::unordered_set<std::string>;
			using TextureBoxMap = std::map<std::string, MaskPathSet, std::less<>>;

			// each layer contains sets, within which are the slices
			// this is a set of all those sets for a given layer (confusing, I know, but the strings should be unique)
			using SetsSet = std::unordered_set<std::string>;

			// MaskMap is a map of Face > Row > Col
			// this allows us to tell which slices are water slices
			// e.g. if the map has a BACK face with Row 1 and Col 1, then 
			// the file back_01_01.jpg is a water slice
			using Row = uint32_t;
			using Col = uint32_t;
			using ColSet = std::unordered_set<Col>;
			using SliceMap = std::unordered_map<Row, ColSet>;
			using MaskMap = std::unordered_map<Face, SliceMap>;

			struct Layer {
				std::optional<std::string> textureBoxNameOptional = std::nullopt;
				SetsSet setsSet = {};
				bool isLayerMask = false;
				MaskMap waterMaskMap = {};
			};

			using LayerMap = std::map<std::string, Layer, std::less<>>;
			using LayerMapPointer = std::shared_ptr<LayerMap>;

			void appendToSliceMap(std::istream &inputStream, std::streamsize size, SliceMap &sliceMap);
		};

		// this is the abstract class on which all resources are based
		class Resource : NonCopyable {
			public:
			using Id = uint32_t;
			using Version = uint32_t;
			using Pointer = std::shared_ptr<Resource>;

			// this struct contains the ID and Version of the resource
			// this is used to determine which resource type to create
			struct Loader {
				using Pointer = std::shared_ptr<Loader>;

				Id id = 0;
				Version version = 1;
				std::optional<std::string> nameOptional = std::nullopt;

				Loader(std::istream &inputStream);
			};

			const Loader::Pointer LOADER_POINTER;

			Resource(Loader::Pointer loaderPointer, Version version);
		};
		
		class HeaderCopier {
			protected:
			using Id = uint64_t;

			// "ubi/b0-l"
			static constexpr Id UBI_B0_L = 0x6C2D30622F696275;

			std::streamsize fileSize = 0;
			std::streampos filePosition;

			HeaderCopier(std::streamsize fileSize, const std::streampos &filePosition);
		};

		class HeaderReader : private HeaderCopier, NonCopyable {
			private:
			void throwReadPastEnd();

			std::istream &inputStream;

			public:
			HeaderReader(std::istream &inputStream, std::streamsize fileSize);
			~HeaderReader();
		};

		class HeaderWriter : private HeaderCopier, NonCopyable {
			private:
			void throwWrotePastEnd();

			std::ostream &outputStream;

			public:
			HeaderWriter(std::ostream &outputStream, std::streamsize fileSize);
			~HeaderWriter();
		};

		// a basic factory pattern going on here for the creation of resources
		void readFileHeader(
			std::istream &inputStream, std::optional<HeaderReader> &headerReaderOptional, std::streamsize size = -1
		);

		void writeFileHeader(
			std::ostream &outputStream, std::optional<HeaderWriter> &headerWriterOptional, std::streamsize size = -1
		);

		Resource::Loader::Pointer readFileLoader(
			std::istream &inputStream, std::optional<HeaderReader> &headerReaderOptional, std::streamsize size = -1
		);

		Resource::Pointer createResourcePointer(std::istream &inputStream, std::streamsize size = -1);
		Resource::Pointer appendToLayerMap(std::istream &inputStream, Rle::LayerMap &layerMap, std::streamsize size = -1);
		Resource::Pointer appendToTextureBoxMap(std::istream &inputStream, Rle::TextureBoxMap &textureBoxMap, std::streamsize size = -1);
		Resource::Pointer appendToMaskPathSet(std::istream &inputStream, Rle::MaskPathSet &maskPathSet, std::streamsize size = -1);
	};

	struct BigFile {
		using Pointer = std::shared_ptr<BigFile>;

		struct Path {
			using Vector = std::vector<Path>;
			using NameVector = std::vector<std::string>;

			NameVector directoryNameVector = {};
			std::string fileName = "";

			Path() = default;
			Path(const NameVector &directoryNameVector, const std::string &fileName);
			Path(const std::string &copyString);
			Path &operator=(const std::string &assignString);
			void clear();

			private:
			Path& create(const std::string &file);
		};

		struct File {
			using Size = uint32_t;
			using Pointer = std::shared_ptr<File>;
			using PointerSet = std::unordered_set<Pointer>;
			using PointerSetMap = std::map<Size, PointerSet>; // must be sorted by size
			using PointerVector = std::vector<Pointer>;
			using PointerVectorPointer = std::shared_ptr<PointerVector>;

			enum struct Type {
				NONE = 0,
				BINARY,
				BIG_FILE,
				IMAGE_STANDARD,
				IMAGE_ZAP
			};

			// the name in the output file (so example.dds, not example.jpg)
			std::optional<std::string> nameOptional = std::nullopt;

			// initially the size in the input file, to be potentially overwritten later (if converted)
			Size size = 0;

			// initially the offset in the input file, to be overwritten later (with the stream offset)
			Size offset = 0;

			// the effective size of the file's padding (not stored to the file, used temporarily by the output thread)
			Size padding = 0;

			// used for water slices
			// if this file is a layer, layerInformationPointer is non-zero and
			// points to the layer information, and layerMapIterator is an iterator
			// into the layerMap field of layerInformationPointer (it is never the end of the map)
			Binary::Rle::LayerMapPointer layerMapPointer = nullptr;
			Binary::Rle::LayerMap::const_iterator layerMapIterator = {};

			// metadata for conversion
			Type type = Type::NONE;
			//bool greyScale = false;
			bool rgba = false;

			File(std::istream &inputStream, Size &fileSystemSize, const std::optional<File> &layerFileOptional);
			File(std::istream &inputStream);
			File(Size inputFileSize);
			void write(std::ostream &outputStream) const;

			Binary::Resource::Pointer appendToLayerMap(
				std::istream &inputStream,
				Size fileSystemOffset,
				Binary::Rle::LayerMap &layerMap
			) const;

			Binary::Resource::Pointer appendToTextureBoxMap(
				std::istream &inputStream,
				Size fileSystemOffset,
				Binary::Rle::TextureBoxMap &textureBoxMap
			) const;

			private:
			void read(std::istream &inputStream);
			void rename(const std::optional<File> &layerFileOptional);

			static std::string getNameExtension(const std::string &name);
			static bool isWaterSlice(const std::string &name, const Binary::Rle::MaskMap &waterMaskMap);

			struct TypeExtension {
				Type type = Type::NONE;
				std::string extension = "";
			};

			using TypeExtensionMap = std::map<std::string, TypeExtension, IgnoreCaseComparer>;

			static const TypeExtensionMap NAME_TYPE_EXTENSION_MAP;
			static constexpr char PERIOD = '.';
		};

		struct Directory {
			using Vector = std::vector<Directory>;
			using VectorIteratorVector = std::vector<Vector::const_iterator>;
			using DirectoryVectorSize = uint8_t;
			using FilePointerVectorSize = uint32_t;

			// some directories with names that are hardcoded by the binarizer
			// normally these would be loaded through the binarizer's log file, but
			// we are only really interested in the ones in these particular directories
			static const std::string NAME_CUBE;
			static const std::string NAME_WATER;

			std::optional<std::string> nameOptional = std::nullopt;

			// the directories that this directory owns
			Directory::Vector directoryVector = {};

			// the files that this directory owns
			// binaryFilePointerVector is seperate so we can easily loop just the Binary files
			// (this is useful for finding Water/Cube binary files)
			File::PointerVector binaryFilePointerVector = {};
			File::PointerVector filePointerVector = {};

			Directory(
				Directory* ownerDirectory,
				std::istream &inputStream,
				File::Size &fileSystemSize,
				File::PointerVector::size_type &files,
				File::PointerSetMap &filePointerSetMap,
				const std::optional<File> &layerFileOptional
			);
			
			Directory(std::istream &inputStream);

			Directory(std::istream &inputStream, const Path &path,
				File::Pointer &filePointer);

			Directory(std::istream &inputStream, const Path &path,
				Path::NameVector::const_iterator directoryNameVectorIterator, File::Pointer &filePointer);

			void write(std::ostream &outputStream) const;
			File::Pointer find(const Path &path) const;

			void appendToLayerMap(
				std::istream &inputStream,
				File::Size fileSystemOffset,
				Binary::Rle::LayerMap &layerMap
			) const;

			void appendToTextureBoxMap(
				std::istream &inputStream,
				File::Size fileSystemOffset,
				Binary::Rle::TextureBoxMap &textureBoxMap
			) const;

			private:
			void read(
				bool owner,
				std::istream &inputStream,
				File::Size &fileSystemSize,
				File::PointerVector::size_type &files,
				File::PointerSetMap &filePointerSetMap,
				const std::optional<File> &layerFileOptional
			);

			void find(std::istream &inputStream, const Path &path,
				Path::NameVector::const_iterator directoryNameVectorIterator, File::Pointer &filePointer);

			File::Pointer find(const Path &path,
				Path::NameVector::const_iterator directoryNameVectorIterator) const;

			bool isMatch(const Path::NameVector &directoryNameVector,
				Path::NameVector::const_iterator &directoryNameVectorIterator) const;

			bool isSet(bool bftex, const std::optional<File> &layerFileOptional) const;

			void appendToLayerMap(
				std::istream &inputStream,
				File::Size fileSystemOffset,
				Binary::Rle::LayerMap &layerMap,
				const File::PointerVector &binaryFilePointerVector
			) const;

			void appendToTextureBoxMap(
				std::istream &inputStream,
				File::Size fileSystemOffset,
				Binary::Rle::TextureBoxMap &textureBoxMap,
				const File::PointerVector &binaryFilePointerVector
			) const;
		};

		struct Header {
			using Version = uint32_t;

			class Invalid : public std::invalid_argument {
				public:
				Invalid() noexcept : std::invalid_argument("Header invalid") {
				}
			};

			Header(std::istream &inputStream, File::Size &fileSystemSize, File::Size &fileSystemOffset);
			Header(std::istream &inputStream);
			Header(std::istream &inputStream, File::Pointer &filePointer);
			void write(std::ostream &outputStream) const;

			private:
			void read(std::istream &inputStream);

			static const std::string SIGNATURE;
			static constexpr Version CURRENT_VERSION = 1;
		};

		private:
		File::Size fileSystemOffset = 0;

		public:
		static File::Pointer findFile(std::istream &stream, const Path::Vector &pathVector);

		Header header;
		Directory directory;

		BigFile(
			std::istream &inputStream,
			File::Size &fileSystemSize,
			File::PointerVector::size_type &files,
			File::PointerSetMap &filePointerSetMap,
			File &file
		);

		BigFile(std::istream &inputStream);

		BigFile(std::istream &inputStream, const Path &path,
			File::Pointer &filePointer);

		void write(std::ostream &outputStream) const;
	};
};