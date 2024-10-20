// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/bimap.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <wil/resource.h>
#include <windows.h>
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
BOOL CreateFileTimeString(const FILETIME *utcFileTime, TCHAR *szBuffer, size_t cchMax,
	BOOL bFriendlyDate);
BOOL CreateSystemTimeString(const SYSTEMTIME *localSystemTime, TCHAR *szBuffer, size_t cchMax,
	BOOL bFriendlyDate);
BOOL CreateFriendlySystemTimeString(const SYSTEMTIME *localSystemTime, TCHAR *szBuffer,
	size_t cchMax);
BOOL GetFileSizeEx(const TCHAR *szFileName, PLARGE_INTEGER lpFileSize);
BOOL CompareFileTypes(const TCHAR *pszFile1, const TCHAR *pszFile2);
std::wstring BuildFileAttributesString(DWORD fileAttributes);
BOOL GetFileOwner(const TCHAR *szFile, TCHAR *szOwner, size_t cchMax);
DWORD GetNumFileHardLinks(const TCHAR *lpszFileName);
BOOL ReadImageProperty(const TCHAR *lpszImage, PROPID propId, TCHAR *szProperty, int cchMax);
HRESULT GetMediaMetadata(const TCHAR *szFileName, const TCHAR *szAttribute, BYTE **pszOutput);
BOOL IsImage(const TCHAR *fileName);
BOOL GetFileProductVersion(const TCHAR *szFullFileName, DWORD *pdwProductVersionLS,
	DWORD *pdwProductVersionMS);
BOOL GetFileLanguage(const TCHAR *szFullFileName, WORD *pwLanguage);
BOOL GetVersionInfoString(const TCHAR *szFullFileName, const TCHAR *szVersionInfo,
	TCHAR *szVersionBuffer, UINT cchMax);

/* Ownership and access. */
BOOL CheckGroupMembership(GroupType groupType);
BOOL FormatUserName(PSID sid, TCHAR *userName, size_t cchMax);

/* User interaction. */
BOOL GetFileNameFromUser(HWND hwnd, TCHAR *fullFileName, UINT cchMax,
	const TCHAR *initialDirectory);

/* General helper functions. */
HINSTANCE StartCommandPrompt(const std::wstring &directory, bool elevated);
HRESULT GetCPUBrandString(std::wstring &cpuBrand);
void SetFORMATETC(FORMATETC *pftc, CLIPFORMAT cfFormat, DVTARGETDEVICE *ptd, DWORD dwAspect,
	LONG lindex, DWORD tymed);
bool IsKeyDown(int nVirtKey);
void SendSimulatedKeyPress(HWND hwnd, UINT key);
bool IsExtendedKey(UINT key);
std::wstring CreateGUID();
std::optional<std::wstring> GetLastErrorMessage(DWORD error);
bool IsWindowsPE();
bool IsProcessRTL();
wil::unique_hmodule LoadSystemLibrary(const std::wstring &libraryName);

template <typename L, typename R>
boost::bimap<L, R> MakeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list)
{
	return boost::bimap<L, R>(list.begin(), list.end());
}

// boost::numeric_cast() throws by default. Although exceptions are enabled in the codebase, they're
// only used when absolutely necessary. This overflow handler uses a CHECK() instead and is useful
// in the case where a conversion should always succeed, without any overflow.
// In the case where there is overflow, a CHECK() is the better option, as it will always log an
// error message before terminating the application. An exception thrown from a random location, on
// the other hand, may or may not result in a crash dump being created.
struct CheckedOverflowHandler
{
	void operator()(boost::numeric::range_check_result result)
	{
		CHECK_EQ(result, boost::numeric::cInRange);
	}
};

template <typename Target, typename Source>
Target CheckedNumericCast(Source source)
{
	using convertor = boost::numeric::converter<Target, Source,
		boost::numeric::conversion_traits<Target, Source>, CheckedOverflowHandler>;
	return convertor::convert(source);
}
