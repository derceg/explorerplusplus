#ifndef SETTINGS_INCLUDED
#define SETTINGS_INCLUDED

#include <windows.h>

LONG SaveSettings(LPCTSTR);
LONG LoadSettings(LPCTSTR);
LONG SaveMRUDirs(LPCTSTR,TCHAR *);
LONG LoadMRUDirs(LPCTSTR,TCHAR *);
int CountMRUDirs(LPCTSTR);

#endif