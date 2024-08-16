#pragma once
#include "shared.h"
#include "Ubi.h"
#include "Work.h"

#include <nvtt/nvtt.h>

class M4Revolution {
	private:
	typedef uint32_t COLOR32;

	class Log {
		private:
		std::ifstream &inputFileStream;
		Ubi::BigFile::File::SIZE inputFileSize = 0;
		bool fileNames = false;

		int progress = 0;
		int files = 0;
		int filesCopying = 0;

		public:
		Log(const char* title, std::ifstream &inputFileStream, Ubi::BigFile::File::SIZE inputFileSize, bool fileNames = false);
		Log(const Log &log) = delete;
		Log &operator=(const Log &log) = delete;
		void step();
		void copying();
		void converting(const Ubi::BigFile::File &file);
		void finishing();
	};

	struct OutputHandler : public nvtt::OutputHandler {
		OutputHandler(Work::FileTask &fileTask, Work::Memory &memory);
		OutputHandler(const OutputHandler &outputHandler) = delete;
		OutputHandler &operator=(const OutputHandler &outputHandler) = delete;
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel);
		virtual void endImage();
		virtual bool writeData(const void* data, int size);

		Work::FileTask &fileTask;
		Work::Memory &memory;

		unsigned int size = 0;
	};

	struct ErrorHandler : public nvtt::ErrorHandler {
		virtual void error(nvtt::Error e);

		bool result = true;
	};

	static const Ubi::BigFile::Path::VECTOR AI_TRANSITION_FADE_PATH_VECTOR;

	Work::Tasks tasks = {};
	std::ifstream inputFileStream = {};
	bool logFileNames = false;

	Work::Memory convertMemory;
	Work::Memory copyMemory;

	nvtt::Context context = {};
	nvtt::Surface surface = {};
	nvtt::CompressionOptions compressionOptions = {};
	nvtt::OutputOptions outputOptions = {};

	void convertZAP(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file);

	void copyFiles(
		Ubi::BigFile::File::SIZE inputPosition,
		Ubi::BigFile::File::SIZE inputCopyPosition,
		Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
		std::streampos bigFileInputPosition,
		Log &log
	);

	void convertFile(std::streampos bigFileInputPosition, Ubi::BigFile::File &file, Log &log);

	void stepFile(
		Ubi::BigFile::File::SIZE inputPosition,
		Ubi::BigFile::File::SIZE &inputFilePosition,
		Ubi::BigFile::File::POINTER_VECTOR_POINTER &filePointerVectorPointer,
		Ubi::BigFile::File &file,
		Log &log
	);

	void fixLoading(std::streampos ownerBigFileInputPosition, Ubi::BigFile::File &file, Log &log);

	static void color32X(COLOR32* color32Pointer, size_t stride, size_t size);

	static bool outputBigFiles(Work::Output &output, std::streampos bigFileInputPosition, Work::Tasks &tasks);
	static void outputData(Work::Output &output, Work::FileTask &fileTask, bool &yield);
	static void outputFiles(Work::Output &output, Work::FileTask::QUEUE &fileTaskQueue);
	static void outputThread(const char* outputFileName, Work::Tasks &tasks, bool &yield);

	public:
	M4Revolution(const char* inputFileName, bool logFileNames = false, bool disableHardwareAcceleration = false);
	M4Revolution(const M4Revolution &m4Revolution) = delete;
	M4Revolution &operator=(const M4Revolution &m4Revolution) = delete;
	void editTransitionSpeed(const char* outputFileName);
	void editInertia(const char* outputFileName);
	void fixLoading(const char* outputFileName);
};