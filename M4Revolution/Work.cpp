#include "Work.h"

Work::OutputFile::StreamLock::StreamLock(std::mutex &mutex, std::ofstream &stream, std::streampos position)
 : lock(mutex),
 stream(stream) {
	stream.seekp(position);
}

std::ofstream &Work::OutputFile::StreamLock::get() {
	return stream;
}

Work::OutputFile::OutputFile(const char* name)
 : stream(name, std::ios::binary) {
}

Work::OutputFile::StreamLock Work::OutputFile::lock(std::streampos position) {
	return StreamLock(mutex, stream, position);
}

std::atomic<Work::Media::Pool::VECTOR::size_type> Work::Media::Pool::index = 0;
Work::Media::Pool::VECTOR vector = {};

Work::Media::Pool::Pool(SIZE &size) : size(size) {
	index++;

	vector.emplace_back(new unsigned char[size]);
}

Work::Media::Pool::~Pool() {
	index--;
}