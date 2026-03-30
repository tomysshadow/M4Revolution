#pragma once
#include "NonCopyable.h"
#include "IgnoreCaseComparer.h"
#include <functional>
#include <stdexcept>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <libzap.h>
#include <scope_guard.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "utils.h"