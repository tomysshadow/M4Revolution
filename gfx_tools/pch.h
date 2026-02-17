#pragma once
#include "NonCopyable.h"
#include <functional>
#include <stdexcept>
#include <ctype.h>
#include <string.h>
#include <libzap.h>
#include <scope_guard.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include "utils.h"