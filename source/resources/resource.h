#pragma once
#ifndef RESOURCESRC_H
#define RESOURCESRC_H

// Dummy values for Visual Studio's editor (it doesn't see the preprocessor defs and fails to open "Resources.rc").
// The resource compiler itself will get the proper values from the commandline definitions and works as expected.
#ifndef rsc_MajorVersion
#define rsc_MajorVersion    9
#define rsc_MinorVersion    9
#define rsc_RevisionID      9
#define rsc_BuildID         9
#define rsc_Repository      "github.com/user/repository"
#define rsc_FullName        "fullname"
#define rsc_Name            "name"
#define rsc_Extension       ".ext"
#define rsc_Copyright       "2999 cyanea-bt"
#define rsc_UpdateURL       "github.com/user/repository/releases"
#define rsc_FILE_1			"resource.h"
#endif

//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by Resources.rc
//
#define IDR_TEXT1                       101

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#endif	// RESOURCESRC_H
