#pragma once
#include "shared.h"
#include "Ubi.h"
#include "Work.h"
#include <atomic>

#include <nvtt/nvtt.h>

class M4Revolution {
	private:
	class Log {
		private:
		std::ifstream &inputFileStream;
		Ubi::BigFile::File::SIZE inputFileSize = 0;
		bool fileNames = false;

		int progress = 0;
		int files = 0;
		int copiedFiles = 0;

		public:
		Log(const char* title, std::ifstream &inputFileStream, Ubi::BigFile::File::SIZE inputFileSize, bool fileNames = false);
		Log(const Log &log) = delete;
		Log &operator=(const Log &log) = delete;
		void step();
		void copied();
		void converted(const Ubi::BigFile::File &file);
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

	static const Ubi::BigFile::Path::VECTOR AI_TRANSITION_FADE_PATH_VECTOR;

	std::ifstream inputFileStream = {};
	bool logFileNames = false;

	nvtt::Context context = {};
	nvtt::Surface surface = {};
	nvtt::CompressionOptions compressionOptions = {};
	nvtt::OutputOptions outputOptions = {};

	#ifdef MULTITHREADED
	std::atomic<Work::Data::VECTOR::size_type> dataVectorIndex = 0;
	Work::Data::VECTOR dataVector = {};
	#endif
	#ifdef SINGLETHREADED
	Work::Data data = {};
	#endif

	void convertZAP(Work::Tasks &tasks, Ubi::BigFile::File &file, std::streampos inputPosition);
	void fixLoading(Work::Tasks &tasks, Ubi::BigFile::File &file, Log &log);
	static void outputThread(const char* outputFileName, Work::Tasks &tasks, bool &yield);

	public:
	M4Revolution(const char* inputFileName, bool logFileNames = false, bool disableHardwareAcceleration = false);
	M4Revolution(const M4Revolution &m4Revolution) = delete;
	M4Revolution &operator=(const M4Revolution &m4Revolution) = delete;
	void editTransitionSpeed(const char* outputFileName);
	void editInertia(const char* outputFileName);
	void fixLoading(const char* outputFileName);
};