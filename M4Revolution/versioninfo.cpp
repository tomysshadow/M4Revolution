#include "pch.h"
#include "versioninfo.h"

constexpr bool _versioninfoCurrentYear() {
	return VERSIONINFO_COPYRIGHT_YEAR[0] == __DATE__[7]
		&& VERSIONINFO_COPYRIGHT_YEAR[1] == __DATE__[8]
		&& VERSIONINFO_COPYRIGHT_YEAR[2] == __DATE__[9]
		&& VERSIONINFO_COPYRIGHT_YEAR[3] == __DATE__[10];
}

static_assert(_versioninfoCurrentYear(), "VERSIONINFO_COPYRIGHT_YEAR mismatch");