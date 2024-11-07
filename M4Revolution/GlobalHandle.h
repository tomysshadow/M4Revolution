#pragma once
#include "shared.h"

template <typename T = void*> class GlobalHandleLock {
	public:
	#ifdef MACINTOSH
	typedef Handle GlobalHandle;
	#endif
	#ifdef WINDOWS
	typedef HGLOBAL GlobalHandle;
	#endif

	private:
	void create() {
		#ifdef MACINTOSH
		HLock(globalHandle);
		lock = *globalHandle;
		#endif
		#ifdef WINDOWS
		lock = resource ? (T*)LockResource(globalHandle) : (T*)GlobalLock(globalHandle);
		#endif

		if (!lock) {
			throw std::runtime_error("Failed to Lock Global Handle");
		}
	}

	void destroy() {
		#ifdef MACINTOSH
		HUnlock(globalHandle);

		if (resource) {
			ReleaseResource(globalHandle);
		}
		#endif
		#ifdef WINDOWS
		if (resource) {
			if (!UnlockResource(globalHandle)) {
				if (FreeResource(globalHandle) != NULL) {
					throw std::runtime_error("Failed to Free Resource Handle");
				}
			}
		} else {
			if (!GlobalUnlock(globalHandle)) {
				if (GetLastError() != NO_ERROR) {
					throw std::runtime_error("Failed to Unlock Global Handle");
				}

				if (GlobalFree(globalHandle) != NULL) {
					throw std::runtime_error("Failed to Free Global Handle");
				}
			}
		}
		#endif
	}

	void duplicate(const GlobalHandleLock &globalHandleLock) {
		globalHandle = globalHandleLock.globalHandle;

		create();
	}

	GlobalHandle globalHandle = NULL;
	T* lock = NULL;
	bool resource = false;
	size_t resourceSize = 0;

	public:
	GlobalHandleLock(GlobalHandle globalHandle)
		: globalHandle(globalHandle) {
		if (!globalHandle) {
			throw std::invalid_argument("globalHandle must not be NULL");
		}

		create();
	}

	#ifdef MACINTOSH
	GlobalHandleLock(bool resource, GlobalHandle globalHandle)
		: resource(resource),
		globalHandle(globalHandle) {
		if (!globalHandle) {
			throw std::invalid_argument("globalHandle must not be NULL");
		}

		if (resource) {
			LoadResource(resourceHandle);
		}

		create();
	}
	#endif
	#ifdef WINDOWS
	GlobalHandleLock(HMODULE moduleHandle, HRSRC resourceHandle) {
		resource = true;

		if (!resourceHandle) {
			throw std::invalid_argument("resourceHandle must not be NULL");
		}

		globalHandle = LoadResource(moduleHandle, resourceHandle);

		if (!globalHandle) {
			throw std::runtime_error("Failed to Load Resource");
		}

		resourceSize = SizeofResource(moduleHandle, resourceHandle);

		if (!resourceSize && GetLastError() != ERROR_SUCCESS) {
			throw std::runtime_error("Failed to Get Resource Size");
		}

		create();
	}
	#endif

	~GlobalHandleLock() {
		destroy();
	}

	GlobalHandleLock(const GlobalHandleLock &globalHandleLock) {
		duplicate(globalHandleLock);
	}

	GlobalHandleLock &operator=(const GlobalHandleLock &globalHandleLock) {
		if (this == &globalHandleLock) {
			return *this;
		}

		duplicate(globalHandleLock);
		return *this;
	}

	T* get() const {
		return lock;
	}

	T& operator[](ptrdiff_t index) {
		return get()[index];
	}

	T operator[](ptrdiff_t index) const {
		return get()[index];
	}

	T& operator*() const {
		return *get();
	}

	T* operator->() const {
		return get();
	}

	operator bool() const {
		return get();
	}

	size_t size() const {
		#ifdef MACINTOSH
		return GetHandleSize(globalHandle);
		#endif
		#ifdef WINDOWS
		if (resource) {
			return resourceSize;
		}
		return GlobalSize(globalHandle);
		#endif
	}

	GlobalHandle getGlobalHandle() const {
		return globalHandle;
	}
};