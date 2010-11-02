#ifndef SETTINGS_INCLUDED
#define SETTINGS_INCLUDED

#include <windows.h>

BOOL LoadWindowPosition(WINDOWPLACEMENT *pwndpl);
LONG SaveSettings(LPCTSTR);
LONG LoadSettings(LPCTSTR);
LONG SaveMRUDirs(LPCTSTR,TCHAR *);
LONG LoadMRUDirs(LPCTSTR,TCHAR *);
int CountMRUDirs(LPCTSTR);

#endif