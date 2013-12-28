#pragma once

#include <Windows.h>

BOOL LoadWindowPositionFromRegistry(WINDOWPLACEMENT *pwndpl);
BOOL LoadAllowMultipleInstancesFromRegistry(void);