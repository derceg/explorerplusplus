#ifndef REGISTRY_SETTINGS_INCLUDED
#define REGISTRY_SETTINGS_INCLUDED

#include <Windows.h>

BOOL LoadWindowPositionFromRegistry(WINDOWPLACEMENT *pwndpl);
BOOL LoadAllowMultipleInstancesFromRegistry(void);

#endif