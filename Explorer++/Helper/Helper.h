#pragma once

#include <windows.h>
#include <winioctl.h>
#include <list>
#include <ShObjIdl.h>

struct LangAndCodePage
{
	WORD wLanguage;
	WORD wCodePage;
};

enum GroupType_t
{
	GROUP_ADMINISTRATORS,
	GROUP_POWERUSERS,
	GROUP_USERS,
	GROUP_USERSRESTRICTED
};

/* File helpers. */
BOOL			CreateFileTimeString(const FILETIME *FileTime, TCHAR *szBuffer, size_t cchMax, BOOL bFriendlyDate);
BOOL			CreateSystemTimeString(const SYSTEMTIME *systemTime, TCHAR *szBuffer, size_t cchMax, BOOL bFriendlyDate);
BOOL			GetFileSizeEx(const TCHAR *szFileName, PLARGE_INTEGER lpFileSize);
BOOL			CompareFileTypes(const TCHAR *pszFile1,const TCHAR *pszFile2);
HRESULT			BuildFileAttributeString(const TCHAR *lpszFileName, TCHAR *szOutput, DWORD cchMax);
HRESULT			BuildFileAttributeString(DWORD dwFileAttributes, TCHAR *szOutput, DWORD cchMax);
BOOL			GetFileOwner(const TCHAR *szFile,TCHAR *szOwner,size_t cchMax);
DWORD			GetNumFileHardLinks(const TCHAR *lpszFileName);
BOOL			ReadImageProperty(const TCHAR *lpszImage, PROPID propId, TCHAR *szProperty, int cchMax);
HRESULT			GetMediaMetadata(const TCHAR *szFileName, const TCHAR *szAttribute, BYTE **pszOutput);
BOOL			IsImage(const TCHAR *FileName);
BOOL			GetFileProductVersion(const TCHAR *szFullFileName, DWORD *pdwProductVersionLS, DWORD *pdwProductVersionMS);
BOOL			GetFileLanguage(const TCHAR *szFullFileName, WORD *pwLanguage);
BOOL			GetVersionInfoString(const TCHAR *szFullFileName, const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax);

/* Ownership and access. */
BOOL			CheckGroupMembership(GroupType_t GroupType);
BOOL			FormatUserName(PSID sid, TCHAR *userName, size_t cchMax);

/* User interaction. */
BOOL			GetFileNameFromUser(HWND hwnd,TCHAR *FullFileName,UINT cchMax,const TCHAR *InitialDirectory);

/* General helper functions. */
HINSTANCE		StartCommandPrompt(const TCHAR *Directory, bool Elevated);
void			GetCPUBrandString(char *pszCPUBrand, UINT cchBuf);
void			SetFORMATETC(FORMATETC *pftc, CLIPFORMAT cfFormat, DVTARGETDEVICE *ptd, DWORD dwAspect, LONG lindex, DWORD tymed);
BOOL			CopyTextToClipboard(const std::wstring &str);
bool			IsKeyDown(int nVirtKey);

/* See http://msdn.microsoft.com/en-us/library/windows/desktop/dd940435(v=vs.85).aspx. */
template <class T> void SafeRelease(T **ppT)
{
	if(*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}