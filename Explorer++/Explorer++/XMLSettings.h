#ifndef XML_SETTINGS_INCLUDED
#define XML_SETTINGS_INCLUDED

#include <Windows.h>

BOOL LoadWindowPositionFromXML(WINDOWPLACEMENT *pwndpl);
BOOL LoadAllowMultipleInstancesFromXML(void);

#endif