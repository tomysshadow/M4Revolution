#pragma once
#include "NonCopyable.h"
#include "IgnoreCaseComparer.h"
#include "Locale.h"
#include <memory>
#include <functional>
#include <stdexcept>
#include <string>
#include <optional>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <libzap.h>
#include <scope_guard.hpp>

#ifdef _WIN32
	#define WINDOWS
#else
	#ifdef _WIN16
		#define WINDOWS
	#endif
#endif
#ifndef WINDOWS
	#define MACINTOSH
#endif

#ifdef WINDOWS
#include <Windows.h>
#endif

#include "resource.h"

#include "utils.h"