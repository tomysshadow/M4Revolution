#pragma once
#include "shared.h"
#include "Ubi.h"
#include "Work.h"
#include <nvtt/nvtt.h>

#ifdef WINDOWS
	#define D3D9
	//#define EXTENTS_MAKE_SQUARE
	//#define EXTENTS_MAKE_POWER_OF_TWO
	#define TO_NEXT_POWER_OF_TWO
#endif

class M4Revolution {
	private:
	void destroy();

	class Log {
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

		Log(const std::string &title, std::istream* inputStreamPointer = 0, Ubi::BigFile::File::SIZE inputFileSize = 0, bool fileNames = false, bool slow = false);
		~Log();
		Log(const Log &log) = delete;
		Log &operator=(const Log &log) = delete;
		void step();
		void copying();
		void converting(const Ubi::BigFile::File &file);
		void finishing();
	};

	class CompressionOptions {
		private:
		nvtt::CompressionOptions rgba = {};
		nvtt::CompressionOptions dxt1 = {};
		nvtt::CompressionOptions dxt5 = {};

		public:
		CompressionOptions();
		const nvtt::CompressionOptions &get(const Ubi::BigFile::File &file, const nvtt::Surface &surface, bool hasAlpha) const;
	};

	struct OutputHandler : public nvtt::OutputHandler {
		OutputHandler(Work::FileTask &fileTask);
		OutputHandler(const OutputHandler &outputHandler) = delete;
		OutputHandler &operator=(const OutputHandler &outputHandler) = delete;
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel);
		virtual void endImage();
		virtual bool writeData(const void* data, int size);

		Work::FileTask &fileTask;

		unsigned int size = 0;
	};

	struct ErrorHandler : public nvtt::ErrorHandler {
		virtual void error(nvtt::Error e);

		bool result = true;
	};

	bool logFileNames = false;

	nvtt::Context context = {};

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
	static void toggleSoundFading(std::fstream &fileStream);
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
	M4Revolution(const M4Revolution &m4Revolution) = delete;
	M4Revolution &operator=(const M4Revolution &m4Revolution) = delete;
	void toggleFullScreen();
	void toggleCameraInertia();
	void toggleSoundFading();
	void editTransitionTime();
	void fixLoading();
	void restoreBackup();
};