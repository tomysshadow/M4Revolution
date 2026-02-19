#pragma once
#include "Ubi.h"
#include "Work.h"
#include <nvtt/nvtt.h>

#ifdef WINDOWS
#define D3D9
//#define EXTENTS_MAKE_SQUARE
//#define EXTENTS_MAKE_POWER_OF_TWO
#define TO_NEXT_POWER_OF_TWO
#endif

class M4Revolution : NonCopyable {
	private:
	void destroy();

	class Log : NonCopyable {
		private:
		std::istream* inputStreamPointer = 0;
		Ubi::BigFile::File::SIZE inputFileSize = 0;
		bool fileNames = false;
		std::optional<std::chrono::steady_clock::time_point> beginOptional = std::nullopt;

		int progress = 0;
		int files = 0;
		int filesCopying = 0;

		public:
		static void replaced(const std::string &file);

		Log(
			const std::string &title,
			std::istream* inputStreamPointer = 0,
			Ubi::BigFile::File::SIZE inputFileSize = 0,
			bool fileNames = false,
			bool slow = false
		);

		~Log();
		void step();
		void copying();
		void converting(const Ubi::BigFile::File &file);
		void finishing();
	};

	class CompressionOptions {
		private:
		nvtt::CompressionOptions rgba;
		nvtt::CompressionOptions dxt1;
		nvtt::CompressionOptions dxt5;

		public:
		CompressionOptions();
		const nvtt::CompressionOptions &get(
			const Ubi::BigFile::File &file, const nvtt::Surface &surface, bool hasAlpha
		) const;
	};

	struct OutputHandler : public nvtt::OutputHandler, NonCopyable {
		OutputHandler(Work::FileTask &fileTask);
		virtual ~OutputHandler() override = default;
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) override;
		virtual void endImage() override;
		virtual bool writeData(const void* data, int size) override;

		Work::FileTask &fileTask;

		unsigned int size = 0;
	};

	struct ErrorHandler : public nvtt::ErrorHandler, NonCopyable {
		virtual ~ErrorHandler() override = default;
		virtual void error(nvtt::Error e) override;

		bool result = true;
	};

	bool logFileNames = false;

	nvtt::Context context;

	#ifdef MULTITHREADED
	PTP_POOL pool = NULL;
	#endif

	Work::FileTask::POINTER_QUEUE::size_type maxFileTasks = 0;
	Work::Convert::Configuration configuration;
	Work::Tasks tasks = {};

	void waitFiles(Work::FileTask::POINTER_QUEUE::size_type fileTasks);

	void copyFiles(
		std::istream &inputStream,
		Ubi::BigFile::File::SIZE inputPosition,
		Ubi::BigFile::File::SIZE inputCopyPosition,
		Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
		std::streampos bigFileInputPosition,
		Log &log
	);

	void convertFile(
		std::istream &inputStream,
		std::streampos ownerBigFileInputPosition,
		Ubi::BigFile::File &file,
		Work::Convert::FileWorkCallback fileWorkCallback
	);

	void convertFile(
		std::istream &inputStream,
		std::streampos bigFileInputPosition,
		Ubi::BigFile::File &file,
		Log &log
	);

	void stepFile(
		Ubi::BigFile::File::SIZE inputPosition,
		Ubi::BigFile::File::SIZE &inputFilePosition,
		Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
		Ubi::BigFile::File::POINTER filePointer,
		Log &log
	);

	void fixLoading(std::istream &inputStream, std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file, Log &log);

	static const Ubi::BigFile::Path::VECTOR TRANSITION_FADE_PATH_VECTOR;
	static const CompressionOptions COMPRESSION_OPTIONS;

	static void toggleFullScreen(std::ifstream &inputFileStream);
	static void toggleCameraInertia(std::fstream &fileStream);
	static void editSoundFadeOutTime(std::fstream &fileStream);
	static void editTransitionTime(std::fstream &fileStream);
	#ifdef WINDOWS
	static void replaceGfxTools();
	#endif
	static Ubi::BigFile::File createInputFile(std::istream &inputStream);
	static void convertSurface(Work::Convert &convert, nvtt::Surface &surface, bool hasAlpha);
	static void convertImageStandardWorkCallback(Work::Convert* convertPointer);
	static void convertImageZAPWorkCallback(Work::Convert* convertPointer);
	#ifdef MULTITHREADED
	static VOID CALLBACK convertFileProc(PTP_CALLBACK_INSTANCE instance, PVOID parameter, PTP_WORK work);
	#endif
	static bool outputBigFiles(Work::Output &output, std::streampos bigFileInputPosition, Work::Tasks &tasks);
	static void outputData(std::ostream &outputStream, Work::FileTask &fileTask, bool &yield);
	static void outputFiles(Work::Output &output, Work::FileTask::FILE_VARIANT &fileVariant);
	static void outputThread(Work::Tasks &tasks, bool &yield);
	#ifdef WINDOWS
	static bool getDLLExportRVA(const char* libFileName, const char* procName, unsigned long &dllExportRVA);

	template <typename IMAGE_NT_HEADERSXX>
	static unsigned long getPositionFromRVA(IMAGE_NT_HEADERSXX* imageNtHeadersPointer, unsigned long rva) {
		if (!imageNtHeadersPointer) {
			throw std::invalid_argument("imageNtHeadersPointer must not be NULL");
		}
		
		IMAGE_NT_HEADERSXX &imageNtHeaders = *imageNtHeadersPointer;

		if (imageNtHeaders.Signature != IMAGE_NT_SIGNATURE) {
			throw std::invalid_argument("Signature must be IMAGE_NT_SIGNATURE");
		}

		// if it's in the PE header, the RVA is equivalent to the position
		if (rva < imageNtHeaders.OptionalHeader.SizeOfHeaders) {
			return rva;
		}

		PIMAGE_SECTION_HEADER imageSectionHeaderPointer = (PIMAGE_SECTION_HEADER)(imageNtHeadersPointer + 1);

		for (WORD i = 0; i < imageNtHeaders.FileHeader.NumberOfSections; i++) {
			IMAGE_SECTION_HEADER &imageSectionHeader = *imageSectionHeaderPointer++;

			// test the RVA falls within the section's virtual memory
			if (rva - imageSectionHeader.VirtualAddress >= imageSectionHeader.Misc.VirtualSize) {
				continue;
			}

			// now turn it into the position
			rva += imageSectionHeader.PointerToRawData - imageSectionHeader.VirtualAddress;

			// test the position falls within initialized data
			if (rva - imageSectionHeader.PointerToRawData >= imageSectionHeader.SizeOfRawData) {
				throw std::invalid_argument("rva must not point to uninitialized data");
			}
			return rva;
		}

		throw std::invalid_argument("rva must not point out of bounds");
	}

	static unsigned long getPositionFromRVA(std::istream &inputStream, unsigned long rva);
	#endif

	public:
	class Aborted : public std::logic_error {
		public:
		Aborted(const char* message) noexcept : std::logic_error(message) {
		}
	};

	M4Revolution(
		const std::filesystem::path &path,
		bool logFileNames = false,
		bool disableHardwareAcceleration = false,
		uint32_t maxThreads = 0,
		Work::FileTask::POINTER_QUEUE::size_type maxFileTasks = 0,
		std::optional<Work::Convert::Configuration> configurationOptional = std::nullopt
	);
	
	~M4Revolution();
	void toggleFullScreen();
	void toggleCameraInertia();
	void editSoundFadeOutTime();
	void editTransitionTime();
	void fixLoading();
	void restoreBackup();
};