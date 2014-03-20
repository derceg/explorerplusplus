#pragma once

DWORD GetProcessImageName(DWORD dwProcessId, TCHAR *szImageName, DWORD nSize);
BOOL GetProcessOwner(DWORD dwProcessId, TCHAR *szOwner, size_t cchMax);
BOOL SetProcessTokenPrivilege(DWORD dwProcessId, const TCHAR *PrivilegeName, BOOL bEnablePrivilege);