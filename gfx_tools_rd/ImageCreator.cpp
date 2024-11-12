#include "ImageCreator.h"
#include "base_rd.h"
#include <filesystem>
#include <stdlib.h>

namespace gfx_tools {
	ImageLoader* ImageCreator::CreateLoader(char* extension) {
		return GetSingletonInstance().CreateLoaderImp(extension);
	}

	ImageLoader* ImageCreator::CreateLoaderFromFileName(const char* fileName) {
		return GetSingletonInstance().CreateLoaderFromFileNameImp(fileName);
	}

	ImageLoader* ImageCreator::CreateAndFillLoaderForFile(char* fileName) {
		return GetSingletonInstance().CreateAndFillLoaderForFileImp(fileName);
	}

	void ImageCreator::RegisterImageSerializer(char* extension, ImageSerializerProc imageSerializerProc) {
		GetSingletonInstance().RegisterImageSerializerImp(extension, imageSerializerProc);
	}

	ImageCreator* ImageCreator::CreateSingletonInstance() {
		ubi::CriticalSection &criticalSection = ubi::InstanceManager::GetCriticalSection();
		criticalSection.Lock();

		if (!ms_SingletonInstance) {
			MAKE_SCOPE_EXIT(destroySingletonInstanceScopeExit) {
				DestroySingletonInstance();
			};

			ms_SingletonInstance = new ImageCreator();

			if (atexit(DestroySingletonInstance)) {
				return 0;
			}

			destroySingletonInstanceScopeExit.dismiss();
		}

		criticalSection.Unlock();
		return ms_SingletonInstance;
	}

	ImageCreator& ImageCreator::GetSingletonInstance() {
		if (!ms_SingletonInstance) {
			return *CreateSingletonInstance();
		}
		return *ms_SingletonInstance;
	}

	ImageCreator& ImageCreator::GetSingletonInstanceFast() {
		return *ms_SingletonInstance;
	}

	ImageCreator::IMAGE_SERIALIZER_PROC_MAP ImageCreator::EXTENSION_IMAGE_SERIALIZER_PROC_MAP = {
		{"ZAP", ImageSerializerZAP},
		{"TGA", ImageSerializerTGA},
		{"PNG", ImageSerializerPNG},
		{"JPG", ImageSerializerJPEG},
		{"JPEG", ImageSerializerJPEG},
		{"JIF", ImageSerializerJPEG},
		{"JFIF", ImageSerializerJPEG},
		{"BMP", ImageSerializerBMP},
		{"JTIF", ImageSerializerJPEG}
	};

	ImageCreator* ImageCreator::ms_SingletonInstance = 0;

	ImageLoader* ImageCreator::ImageSerializerZAP() {
		return new ImageLoaderMultipleBufferZAP();
	}

	ImageLoader* ImageCreator::ImageSerializerTGA() {
		return new ImageLoaderMultipleBufferTGA();
	}

	ImageLoader* ImageCreator::ImageSerializerPNG() {
		return new ImageLoaderMultipleBufferPNG();
	}

	ImageLoader* ImageCreator::ImageSerializerJPEG() {
		return new ImageLoaderMultipleBufferJPEG();
	}

	ImageLoader* ImageCreator::ImageSerializerBMP() {
		return new ImageLoaderMultipleBufferBMP();
	}

	void ImageCreator::DestroySingletonInstance() {
		delete ms_SingletonInstance;
	}

	ImageLoader* ImageCreator::CreateLoaderImp(const char* extension) {
		IMAGE_SERIALIZER_PROC_MAP::iterator imageSerializerProcMapIterator = EXTENSION_IMAGE_SERIALIZER_PROC_MAP.find(extension);

		if (imageSerializerProcMapIterator == EXTENSION_IMAGE_SERIALIZER_PROC_MAP.end()) {
			return 0;
		}
		return imageSerializerProcMapIterator->second();
	}

	ImageLoader* ImageCreator::CreateLoaderFromFileNameImp(const char* fileName) {
		if (!fileName) {
			return 0;
		}

		const char PERIOD = '.';

		const char* extension = strrchr(fileName, PERIOD);
		return CreateLoaderImp(extension ? extension + 1 : fileName);
	}

	ImageLoader* ImageCreator::CreateAndFillLoaderForFileImp(char* fileName) {
		ImageLoader* imageLoaderPointer = CreateLoaderFromFileNameImp(fileName);

		if (!imageLoaderPointer) {
			return 0;
		}

		MAKE_SCOPE_EXIT(imageLoaderPointerScopeExit) {
			imageLoaderPointer->Release();
		};

		ubi::InputFileStream inputFileStream(fileName);
		ubi::InputFileStream::SIZE size = inputFileStream.GetSize();

		RawBuffer::DATA data = imageLoaderPointer->CreateLODRawBuffer(0, (RawBuffer::SIZE)size);

		if (!data) {
			return 0;
		}
		
		if (inputFileStream.Read(data, 0, (ubi::Stream::SIZE)size) != size) {
			return 0;
		}

		imageLoaderPointerScopeExit.dismiss();
		return imageLoaderPointer;
	}

	void ImageCreator::RegisterImageSerializerImp(char* fileName, ImageSerializerProc imageSerializerProc) {
		EXTENSION_IMAGE_SERIALIZER_PROC_MAP[fileName] = imageSerializerProc;
	}
}