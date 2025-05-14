#pragma once
#pragma comment(lib, "comctl32.lib")
//Below line adds info to the manifest to ensure that version 6 of the Windows Common Controls are loaded.  These are needed for Theme Awareness
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")
#include "resource.h"
