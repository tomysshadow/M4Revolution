#include "Work.h"

std::atomic<Work::Media::Pool::VECTOR::size_type> Work::Media::Pool::index = 0;
Work::Media::Pool::VECTOR vector = {};

Work::Media::Pool::Pool() {
	index++;
}

Work::Media::Pool::~Pool() {
	index--;
}