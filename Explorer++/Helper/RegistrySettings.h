#ifndef REGISTRYSETTINGS_INCLUDED
#define REGISTRYSETTINGS_INCLUDED

#include <tchar.h>

namespace NRegistrySettings
{
	LONG	SaveDwordToRegistry(HKEY hKey,const TCHAR *szKey,DWORD dwValue);
	LONG	ReadDwordFromRegistry(HKEY hKey,const TCHAR *szKey,DWORD *pReturnValue);
	LONG	SaveStringToRegistry(HKEY hKey,const TCHAR *szKey,const TCHAR *szValue);
	LONG	ReadStringFromRegistry(HKEY hKey,const TCHAR *szKey,TCHAR *szOutput,DWORD BufferSize);
	LONG	ReadStringFromRegistry(HKEY hKey,std::wstring strKey,std::wstring &strOutput);
	LONG	SaveStringListToRegistry(HKEY hKey,const TCHAR *szBaseKeyName,const std::list<std::wstring> &strList);
	LONG	ReadStringListFromRegistry(HKEY hKey,const TCHAR *szBaseKeyName,std::list<std::wstring> &strList);
}

#endif