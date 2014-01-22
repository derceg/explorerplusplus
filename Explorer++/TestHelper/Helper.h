#pragma once

DWORD GetCurrentProcessImageName(TCHAR *szProcessPath, DWORD cchMax);
void GetTestResourceDirectory(TCHAR *szResourceDirectory, size_t cchMax);
void GetTestResourceFilePath(const TCHAR *szFile, TCHAR *szOutput, size_t cchMax);