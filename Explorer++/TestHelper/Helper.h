#pragma once

DWORD GetCurrentProcessImageName(TCHAR *szProcessPath, DWORD cchMax);
void GetTestResourceDirectory(TCHAR *szResourceDirectory, size_t cchMax);
void GetTestResourceDirectoryIdl(LPITEMIDLIST *pidl);
void GetTestResourceFilePath(const TCHAR *szFile, TCHAR *szOutput, size_t cchMax);
void GetTestResourceFileIdl(const TCHAR *szFile, LPITEMIDLIST *pidl);