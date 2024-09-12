#pragma once
#include "shared.h"
#include "Ubi.h"
#include "Work.h"

#include <nvtt/nvtt.h>

class M4Revolution {
	private:
	void destroy();

	typedef uint32_t COLOR32;

	class Log {
		private:
		std::istream &inputStream;
		Ubi::BigFile::File::SIZE inputFileSize = 0;
		bool fileNames = false;
		std::optional<std::chrono::steady_clock::time_point> beginOptional = std::nullopt;

		int progress = 0;
		int files = 0;
		int filesCopying = 0;

		public:
		Log(const char* title, std::istream &inputStream, Ubi::BigFile::File::SIZE inputFileSize = 0, bool fileNames = false, bool slow = false);
		~Log();
		Log(const Log &log) = delete;
		Log &operator=(const Log &log) = delete;
		void step();
		void copying();
		void converting(const Ubi::BigFile::File &file);
		void finishing();
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

	std::string inputFileName = "";
	std::string outputFileName = "";
	bool logFileNames = false;

	nvtt::Context context = {};
	nvtt::CompressionOptions compressionOptionsDXT5 = {};
	nvtt::CompressionOptions compressionOptionsRGBA = {};

	#ifdef MULTITHREADED
	PTP_POOL pool = NULL;
	#endif

	Work::FileTask::POINTER_QUEUE::size_type maxFileTasks = 0;

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

	Ubi::BigFile::File createInputFile(std::istream &inputStream);

	static void color32X(COLOR32* color32Pointer, size_t stride, size_t size);
	static void convertSurface(Work::Convert &convert, nvtt::Surface &surface);
	static void convertJPEGWorkCallback(Work::Convert* convertPointer);
	static void convertZAPWorkCallback(Work::Convert* convertPointer);
	#ifdef MULTITHREADED
	static VOID CALLBACK convertFileProc(PTP_CALLBACK_INSTANCE instance, PVOID parameter, PTP_WORK work);
	#endif
	static bool outputBigFiles(Work::Output &output, std::streampos bigFileInputPosition, Work::Tasks &tasks);
	static void outputData(std::ostream &outputStream, Work::FileTask &fileTask, bool &yield);
	static void outputFiles(Work::Output &output, Work::FileTask::FILE_VARIANT &fileVariant);
	static void outputThread(const std::string &outputFileName, Work::Tasks &tasks, bool &yield);

	public:
	M4Revolution(
		const std::string &inputFileName,
		const std::string &outputFileName,
		bool logFileNames = false,
		bool disableHardwareAcceleration = false,
		uint32_t maxThreads = 0,
		Work::FileTask::POINTER_QUEUE::size_type maxFileTasks = 0
	);
	
	~M4Revolution();
	M4Revolution(const M4Revolution &m4Revolution) = delete;
	M4Revolution &operator=(const M4Revolution &m4Revolution) = delete;
	void editTransitionTime();
	void editInertiaLevels();
	void fixLoading();
	bool restoreBackup();
};