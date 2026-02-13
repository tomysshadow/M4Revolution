#include "ImageCreator.h"
#include "base.h"
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

		SCOPE_EXIT {
			criticalSection.Unlock();
		};

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

	ImageCreator::IMAGE_SERIALIZER_PROC_MAP ImageCreator::extensionImageSerializerProcMap = {
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
		IMAGE_SERIALIZER_PROC_MAP::iterator imageSerializerProcMapIterator = extensionImageSerializerProcMap.find(extension);

		if (imageSerializerProcMapIterator == extensionImageSerializerProcMap.end()) {
			return 0;
		}
		return imageSerializerProcMapIterator->second();
	}

	ImageLoader* ImageCreator::CreateLoaderFromFileNameImp(const char* fileName) {
		if (!fileName) {
			return 0;
		}

		static const char PERIOD = '.';

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

		RawBuffer::POINTER pointer = imageLoaderPointer->CreateLODRawBuffer(0, (RawBuffer::SIZE)size);

		if (!pointer) {
			return 0;
		}
		
		if (inputFileStream.Read(pointer, 0, (ubi::Stream::SIZE)size) != size) {
			return 0;
		}

		imageLoaderPointerScopeExit.dismiss();
		return imageLoaderPointer;
	}

	void ImageCreator::RegisterImageSerializerImp(char* fileName, ImageSerializerProc imageSerializerProc) {
		extensionImageSerializerProcMap[fileName] = imageSerializerProc;
	}
}