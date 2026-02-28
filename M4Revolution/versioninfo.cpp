#pragma once
#include "versioninfo.h"

#ifdef RC_INVOKED
VS_VERSION_INFO VERSIONINFO
#ifdef VERSIONINFO_VERSION
 FILEVERSION VERSIONINFO_MAJORVERSION,VERSIONINFO_MINORVERSION,VERSIONINFO_BUGFIXVERSION,0
 PRODUCTVERSION VERSIONINFO_MAJORVERSION,VERSIONINFO_MINORVERSION,VERSIONINFO_BUGFIXVERSION,0
#endif
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "100904b0"
        BEGIN
           #ifdef VERSIONINFO_COMPANY_NAME
            VALUE "CompanyName", VERSIONINFO_COMPANY_NAME
           #endif
			
           #ifdef VERSIONINFO_FILE_DESCRIPTION
            VALUE "FileDescription", VERSIONINFO_FILE_DESCRIPTION
           #endif
			
           #ifdef VERSIONINFO_VERSION
            VALUE "FileVersion", VERSIONINFO_VERSION ".0"
            VALUE "ProductVersion", VERSIONINFO_VERSION ".0"
           #endif
			
           #ifdef VERSIONINFO_FILE_NAME
            VALUE "InternalName", VERSIONINFO_FILE_NAME
            VALUE "OriginalFilename", VERSIONINFO_FILE_NAME
           #endif
			
           #if defined(VERSIONINFO_COPYRIGHT_NAME) && defined(VERSIONINFO_COPYRIGHT_YEAR)
            VALUE "LegalCopyright", "Copyright (C) " VERSIONINFO_COPYRIGHT_NAME " " VERSIONINFO_COPYRIGHT_YEAR
           #endif
			
           #ifdef VERSIONINFO_PRODUCT_NAME
            VALUE "ProductName", VERSIONINFO_PRODUCT_NAME
           #endif
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
       VALUE "Translation", VERSIONINFO_LANG_ID, VERSIONINFO_CHARSET_ID
    END
END
#else
#ifdef VERSIONINFO_COPYRIGHT_YEAR
constexpr bool _versioninfoCurrentYear() {
	return VERSIONINFO_COPYRIGHT_YEAR[0] == __DATE__[7]
		&& VERSIONINFO_COPYRIGHT_YEAR[1] == __DATE__[8]
		&& VERSIONINFO_COPYRIGHT_YEAR[2] == __DATE__[9]
		&& VERSIONINFO_COPYRIGHT_YEAR[3] == __DATE__[10];
}

static_assert(_versioninfoCurrentYear(), "VERSIONINFO_COPYRIGHT_YEAR mismatch");
#endif
#endif

