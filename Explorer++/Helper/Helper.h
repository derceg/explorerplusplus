// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <ShObjIdl.h>
#include <windows.h>
#include <winioctl.h>
#include <list>
#include <optional>
#include <string>

struct LangAndCodePage
{
	WORD wLanguage;
	WORD wCodePage;
};

enum class GroupType
{
	Administrators,
	PowerUsers,
	Users,
	UsersRestricted
};

/* File helpers. */
BOOL			CreateFileTimeString(const FILETIME *utcFileTime, TCHAR *szBuffer, size_t cchMax, BOOL bFriendlyDate);
BOOL			CreateSystemTimeString(const SYSTEMTIME *localSystemTime, TCHAR *szBuffer, size_t cchMax, BOOL bFriendlyDate);
BOOL			CreateFriendlySystemTimeString(const SYSTEMTIME *localSystemTime, TCHAR *szBuffer, size_t cchMax);
BOOL			GetFileSizeEx(const TCHAR *szFileName, PLARGE_INTEGER lpFileSize);
BOOL			CompareFileTypes(const TCHAR *pszFile1,const TCHAR *pszFile2);
HRESULT			BuildFileAttributeString(const TCHAR *lpszFileName, TCHAR *szOutput, size_t cchMax);
HRESULT			BuildFileAttributeString(DWORD dwFileAttributes, TCHAR *szOutput, size_t cchMax);
BOOL			GetFileOwner(const TCHAR *szFile,TCHAR *szOwner,size_t cchMax);
DWORD			GetNumFileHardLinks(const TCHAR *lpszFileName);
BOOL			ReadImageProperty(const TCHAR *lpszImage, PROPID propId, TCHAR *szProperty, int cchMax);
HRESULT			GetMediaMetadata(const TCHAR *szFileName, const TCHAR *szAttribute, BYTE **pszOutput);
BOOL			IsImage(const TCHAR *fileName);
BOOL			GetFileProductVersion(const TCHAR *szFullFileName, DWORD *pdwProductVersionLS, DWORD *pdwProductVersionMS);
BOOL			GetFileLanguage(const TCHAR *szFullFileName, WORD *pwLanguage);
BOOL			GetVersionInfoString(const TCHAR *szFullFileName, const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax);

/* Ownership and access. */
BOOL			CheckGroupMembership(GroupType groupType);
BOOL			FormatUserName(PSID sid, TCHAR *userName, size_t cchMax);

/* User interaction. */
BOOL			GetFileNameFromUser(HWND hwnd,TCHAR *fullFileName,UINT cchMax,const TCHAR *initialDirectory);

/* General helper functions. */
HINSTANCE		StartCommandPrompt(const std::wstring &directory, bool elevated);
void			GetCPUBrandString(char *pszCPUBrand, UINT cchBuf);
void			SetFORMATETC(FORMATETC *pftc, CLIPFORMAT cfFormat, DVTARGETDEVICE *ptd, DWORD dwAspect, LONG lindex, DWORD tymed);
bool			IsKeyDown(int nVirtKey);
std::wstring	CreateGUID();
std::optional<std::wstring>	GetLastErrorMessage(DWORD error);

/* See http://msdn.microsoft.com/en-us/library/windows/desktop/dd940435(v=vs.85).aspx. */
template <class T> void SafeRelease(T **ppT)
{
	if(*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}