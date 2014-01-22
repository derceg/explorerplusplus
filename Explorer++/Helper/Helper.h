#pragma once

#include <windows.h>
#include <winioctl.h>
#include <list>
#include <ShObjIdl.h>

/* Major version numbers for various versions of
Windows. */
#define WINDOWS_VISTA_SEVEN_MAJORVERSION	6
#define WINDOWS_XP_MAJORVERSION				5

/* SummaryInformation stream constants. */
#define PROPERTY_ID_TITLE		0x02
#define PROPERTY_ID_SUBJECT		0x03
#define PROPERTY_ID_AUTHOR		0x04
#define PROPERTY_ID_KEYWORDS	0x05
#define PROPERTY_ID_COMMENT		0x06

typedef struct
{
	WORD wLanguage;
	WORD wCodePage;
} LANGANDCODEPAGE;

typedef enum
{
	GROUP_ADMINISTRATORS,
	GROUP_POWERUSERS,
	GROUP_USERS,
	GROUP_USERSRESTRICTED
} GroupType_t;

/* File helpers. */
BOOL			CreateFileTimeString(const FILETIME *FileTime, TCHAR *szBuffer, size_t cchMax, BOOL bFriendlyDate);
BOOL			GetFileSizeEx(const TCHAR *szFileName, PLARGE_INTEGER lpFileSize);
BOOL			GetFileClusterSize(const std::wstring &strFilename,PLARGE_INTEGER lpRealFileSize);
BOOL			CompareFileTypes(const TCHAR *pszFile1,const TCHAR *pszFile2);
HRESULT			BuildFileAttributeString(const TCHAR *lpszFileName, TCHAR *szOutput, DWORD cchMax);
HRESULT			BuildFileAttributeString(DWORD dwFileAttributes, TCHAR *szOutput, DWORD cchMax);
BOOL			GetFileOwner(const TCHAR *szFile,TCHAR *szOwner,size_t cchMax);
DWORD			GetNumFileHardLinks(const TCHAR *lpszFileName);
int				ReadFileProperty(const TCHAR *lpszFileName,DWORD dwPropertyId,TCHAR *lpszPropertyBuf,DWORD dwBufLen);
BOOL			ReadImageProperty(const TCHAR *lpszImage,UINT PropertyId,void *pPropBuffer,DWORD dwBufLen);
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