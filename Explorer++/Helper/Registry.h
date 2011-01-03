#ifndef REGISTRY_INCLUDED
#define REGISTRY_INCLUDED

#include <tchar.h>

LONG	SaveDwordToRegistry(HKEY hKey,const TCHAR *KeyName,DWORD Value);
LONG	ReadDwordFromRegistry(HKEY hKey,TCHAR *KeyName,DWORD *pReturnValue);
LONG	SaveStringToRegistry(HKEY hKey,const TCHAR *KeyName,const TCHAR *String);
LONG	ReadStringFromRegistry(HKEY hKey,TCHAR *KeyName,TCHAR *String,DWORD BufferSize);
LONG	SaveStringListToRegistry(HKEY hKey,const TCHAR *szBaseKeyName,const std::list<std::wstring> &strList);
LONG	ReadStringListFromRegistry(HKEY hKey,const TCHAR *szBaseKeyName,std::list<std::wstring> &strList);

#endif