#pragma once

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
			throw std::runtime_error("failed to lock global handle");
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
		SetLastError(NO_ERROR);

		if (resource) {
			// may be a no-op, in which case the last error must be NO_ERROR beforehand
			if (!UnlockResource(globalHandle)) {
				if (GetLastError() != NO_ERROR) {
					throw std::runtime_error("failed to unlock resource handle");
				}

				if (FreeResource(globalHandle)) {
					throw std::runtime_error("failed to free resource handle");
				}
			}
			return;
		}

		if (!GlobalUnlock(globalHandle)) {
			if (GetLastError() != NO_ERROR) {
				throw std::runtime_error("failed to unlock global handle");
			}

			if (GlobalFree(globalHandle)) {
				throw std::runtime_error("failed to free global handle");
			}
		}
		#endif
	}

	void duplicate(const GlobalHandleLock &globalHandleLock) {
		destroy();

		globalHandle = globalHandleLock.globalHandle;
		resource = globalHandleLock.resource;
		resourceSize = globalHandleLock.resourceSize;

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
			LoadResource(globalHandle);
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
			throw std::runtime_error("failed to load resource");
		}

		SetLastError(ERROR_SUCCESS);

		// may return zero without an error occuring, in which case the last error must be ERROR_SUCCESS beforehand
		resourceSize = SizeofResource(moduleHandle, resourceHandle);

		if (!resourceSize && GetLastError() != ERROR_SUCCESS) {
			throw std::runtime_error("failed to get resource size");
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

	const T& operator[](ptrdiff_t index) const {
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
		return resource ? resourceSize : GlobalSize(globalHandle);
		#endif
	}

	GlobalHandle getGlobalHandle() const {
		return globalHandle;
	}
};