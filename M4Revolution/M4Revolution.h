#pragma once
#include "shared.h"
#include "Ubi.h"
#include "Work.h"

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
		OutputHandler(Work::OutputFile &outputFile, std::streampos outputPosition);
		OutputHandler(const OutputHandler &outputHandler) = delete;
		OutputHandler &operator=(const OutputHandler &outputHandler) = delete;
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel);
		virtual void endImage();
		virtual bool writeData(const void* data, int size);

		Work::OutputFile &outputFile;
		std::streampos outputPosition = 0;

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
	PTP_POOL pool = NULL;
	#endif

	// unique_ptr is fine here since the M4Revolution class can't be copied anyway
	Work::Media media = {};

	void convertZAP(Work::OutputFile &outputFile, Ubi::BigFile::File::SIZE &size, std::streampos outputPosition);
	void fixLoading(Work::OutputFile &outputFile, Ubi::BigFile::File::SIZE &size, std::streampos outputPosition, Log &log);
	
	#ifdef MULTITHREADED
	static VOID CALLBACK workCallback(PTP_CALLBACK_INSTANCE instance, PVOID parameter, PTP_WORK work);
	#endif

	public:
	M4Revolution(const char* inputFileName, bool logFileNames = false, bool disableHardwareAcceleration = false);
	M4Revolution(const M4Revolution &m4Revolution) = delete;
	M4Revolution &operator=(const M4Revolution &m4Revolution) = delete;
	void editTransitionSpeed(const char* outputFileName);
	void editInertia(const char* outputFileName);
	void fixLoading(const char* outputFileName);
};