// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Helper.h"
#include "FileWrappers.h"
#include "Macros.h"
#include "TimeHelper.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


enum VersionSubBlockType_t
{
	ROOT,
	TRANSLATION,
	STRING_TABLE_VALUE
};

void EnterAttributeIntoString(BOOL bEnter, TCHAR *String, int Pos, TCHAR chAttribute);
BOOL GetFileVersionValue(const TCHAR *szFullFileName, VersionSubBlockType_t subBlockType,
	WORD *pwLanguage, DWORD *pdwProductVersionLS, DWORD *pdwProductVersionMS,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax);
BOOL GetStringTableValue(void *pBlock, LangAndCodePage *plcp, UINT nItems,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax);

BOOL CreateFileTimeString(const FILETIME *utcFileTime,
	TCHAR *szBuffer, size_t cchMax, BOOL bFriendlyDate)
{
	SYSTEMTIME localSystemTime;
	BOOL ret = FileTimeToLocalSystemTime(utcFileTime, &localSystemTime);

	if (!ret)
	{
		return FALSE;
	}

	return CreateSystemTimeString(&localSystemTime, szBuffer, cchMax, bFriendlyDate);
}

BOOL CreateSystemTimeString(const SYSTEMTIME *localSystemTime,
	TCHAR *szBuffer, size_t cchMax, BOOL bFriendlyDate)
{
	if (bFriendlyDate)
	{
		BOOL ret = CreateFriendlySystemTimeString(localSystemTime, szBuffer, cchMax);

		if (ret)
		{
			return TRUE;
		}
	}

	TCHAR DateBuffer[512];
	int iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, localSystemTime,
		NULL, DateBuffer, SIZEOF_ARRAY(DateBuffer));

	TCHAR TimeBuffer[512];
	int iReturn2 = GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP | TIME_NOSECONDS, localSystemTime,
		NULL, TimeBuffer, SIZEOF_ARRAY(TimeBuffer));

	if ((iReturn1 != 0) && (iReturn2 != 0))
	{
		StringCchPrintf(szBuffer, cchMax, _T("%s %s"), DateBuffer, TimeBuffer);
		return TRUE;
	}

	return FALSE;
}

BOOL CreateFriendlySystemTimeString(const SYSTEMTIME *localSystemTime,
	TCHAR *szBuffer, size_t cchMax)
{
	using namespace boost::gregorian;
	using namespace boost::posix_time;

	FILETIME localFileTime;
	BOOL ret = SystemTimeToFileTime(localSystemTime, &localFileTime);

	if (!ret)
	{
		return FALSE;
	}

	TCHAR dateComponent[512];
	bool dateComponentSet = false;

	ptime inputPosixTime = from_ftime<ptime>(localFileTime);
	date inputDate = inputPosixTime.date();

	date today = day_clock::local_day();
	date yesterday = today - days(1);

	if (inputDate == today)
	{
		StringCchCopy(dateComponent, SIZEOF_ARRAY(dateComponent), _T("Today"));
		dateComponentSet = true;
	}
	else if (inputDate == yesterday)
	{
		StringCchCopy(dateComponent, SIZEOF_ARRAY(dateComponent), _T("Yesterday"));
		dateComponentSet = true;
	}

	if (!dateComponentSet)
	{
		return FALSE;
	}

	TCHAR timeComponent[512];
	int timeFormatted = GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP | TIME_NOSECONDS, localSystemTime,
		NULL, timeComponent, SIZEOF_ARRAY(timeComponent));

	if (timeFormatted == 0)
	{
		return FALSE;
	}

	HRESULT hr = StringCchPrintf(szBuffer, cchMax, _T("%s, %s"), dateComponent, timeComponent);

	if (SUCCEEDED(hr))
	{
		return TRUE;
	}

	return FALSE;
}

HINSTANCE StartCommandPrompt(const TCHAR *Directory, bool Elevated)
{
	HINSTANCE hNewInstance = NULL;

	TCHAR SystemPath[MAX_PATH];
	BOOL bRes = SHGetSpecialFolderPath(NULL, SystemPath, CSIDL_SYSTEM, 0);

	if(bRes)
	{
		TCHAR CommandPath[MAX_PATH];
		TCHAR *szRet = PathCombine(CommandPath, SystemPath, _T("cmd.exe"));

		if(szRet != NULL)
		{
			TCHAR Operation[32];

			if(Elevated)
			{
				StringCchCopy(Operation, SIZEOF_ARRAY(Operation), _T("runas"));
			}
			else
			{
				StringCchCopy(Operation, SIZEOF_ARRAY(Operation), _T("open"));
			}

			hNewInstance = ShellExecute(NULL, Operation, CommandPath, NULL, Directory,
				SW_SHOWNORMAL);
		}
	}

	return hNewInstance;
}

BOOL GetFileSizeEx(const TCHAR *szFileName, PLARGE_INTEGER lpFileSize)
{
	BOOL bSuccess = FALSE;

	HFilePtr hFile = CreateFilePtr(szFileName, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, NULL, NULL);

	if(hFile)
	{
		bSuccess = GetFileSizeEx(hFile.get(), lpFileSize);
	}

	return bSuccess;
}

BOOL CompareFileTypes(const TCHAR *pszFile1,const TCHAR *pszFile2)
{
	SHFILEINFO shfi1;
	SHFILEINFO shfi2;

	DWORD_PTR result1 = SHGetFileInfo(pszFile1,NULL,&shfi1,sizeof(shfi1),SHGFI_TYPENAME);
	DWORD_PTR result2 = SHGetFileInfo(pszFile2,NULL,&shfi2,sizeof(shfi2),SHGFI_TYPENAME);

	if(result1 != 0 && result2 != 0 &&
		StrCmp(shfi1.szTypeName,shfi2.szTypeName) == 0)
	{
		return TRUE;
	}

	return FALSE;
}

HRESULT BuildFileAttributeString(const TCHAR *lpszFileName, TCHAR *szOutput, DWORD cchMax)
{
	/* FindFirstFile is used instead of GetFileAttributes() or
	GetFileAttributesEx() because of its behaviour
	in relation to system files that normally
	won't have their attributes given (such as the
	pagefile, which neither of the two functions
	above can retrieve the attributes of). */
	WIN32_FIND_DATA wfd;
	HFindFilePtr hFindFile = FindFirstFilePtr(lpszFileName, &wfd);
	HRESULT hr = E_FAIL;

	if(hFindFile)
	{
		hr = BuildFileAttributeString(wfd.dwFileAttributes, szOutput, cchMax);
	}

	return hr;
}

HRESULT BuildFileAttributeString(DWORD dwFileAttributes, TCHAR *szOutput, DWORD cchMax)
{
	TCHAR szAttributes[8];
	int i = 0;

	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE, szAttributes, i++, 'A');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_HIDDEN, szAttributes, i++, 'H');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_READONLY, szAttributes, i++, 'R');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_SYSTEM, szAttributes, i++, 'S');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY, szAttributes, i++, 'D');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED, szAttributes, i++, 'C');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED, szAttributes, i++, 'E');

	szAttributes[i] = '\0';

	return StringCchCopy(szOutput, cchMax, szAttributes);
}

void EnterAttributeIntoString(BOOL bEnter, TCHAR *String, int Pos, TCHAR chAttribute)
{
	if(bEnter)
	{
		String[Pos] = chAttribute;
	}
	else
	{
		String[Pos] = '-';
	}
}

BOOL GetFileOwner(const TCHAR *szFile, TCHAR *szOwner, size_t cchMax)
{
	BOOL success = FALSE;

	HFilePtr hFile = CreateFilePtr(szFile, READ_CONTROL, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	if(hFile)
	{
		PSID pSidOwner = NULL;
		PSECURITY_DESCRIPTOR pSD = NULL;
		DWORD dwRet = GetSecurityInfo(hFile.get(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
			&pSidOwner, NULL, NULL, NULL, &pSD);

		if(dwRet == ERROR_SUCCESS)
		{
			success = FormatUserName(pSidOwner, szOwner, cchMax);
			LocalFree(pSD);
		}
	}

	return success;
}

BOOL FormatUserName(PSID sid, TCHAR *userName, size_t cchMax)
{
	BOOL success = FALSE;

	TCHAR accountName[512];
	DWORD accountNameLength = SIZEOF_ARRAY(accountName);
	TCHAR domainName[512];
	DWORD domainNameLength = SIZEOF_ARRAY(domainName);
	SID_NAME_USE eUse;
	BOOL bRet = LookupAccountSid(NULL, sid, accountName, &accountNameLength,
		domainName, &domainNameLength, &eUse);

	if(bRet)
	{
		StringCchPrintf(userName, cchMax, _T("%s\\%s"), domainName, accountName);
		success = TRUE;
	}
	else
	{
		LPTSTR stringSid;
		bRet = ConvertSidToStringSid(sid, &stringSid);

		if(bRet)
		{
			StringCchCopy(userName, cchMax, stringSid);

			LocalFree(stringSid);
			success = TRUE;
		}
	}

	return success;
}

BOOL CheckGroupMembership(GroupType_t GroupType)
{
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	PSID psid;
	DWORD dwGroup = 0;
	BOOL bMember = FALSE;
	BOOL bRet;

	switch(GroupType)
	{
	case GROUP_ADMINISTRATORS:
		dwGroup = DOMAIN_ALIAS_RID_ADMINS;
		break;

	case GROUP_POWERUSERS:
		dwGroup = DOMAIN_ALIAS_RID_POWER_USERS;
		break;

	case GROUP_USERS:
		dwGroup = DOMAIN_ALIAS_RID_USERS;
		break;

	case GROUP_USERSRESTRICTED:
		dwGroup = DOMAIN_ALIAS_RID_GUESTS;
		break;
	}

	bRet = AllocateAndInitializeSid(&sia,2,SECURITY_BUILTIN_DOMAIN_RID,
		dwGroup,0,0,0,0,0,0,&psid);

	if(bRet)
	{
		CheckTokenMembership(NULL,psid,&bMember);

		FreeSid(psid);
	}

	return bMember;
}

DWORD GetNumFileHardLinks(const TCHAR *lpszFileName)
{
	DWORD nLinks = 0;

	HFilePtr hFile = CreateFilePtr(lpszFileName, FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	if(hFile)
	{
		BY_HANDLE_FILE_INFORMATION FileInfo;
		BOOL bRet = GetFileInformationByHandle(hFile.get(), &FileInfo);

		if(bRet)
		{
			nLinks = FileInfo.nNumberOfLinks;
		}
	}

	return nLinks;
}

BOOL ReadImageProperty(const TCHAR *lpszImage, PROPID propId, TCHAR *szProperty, int cchMax)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR token;
	Gdiplus::Status status = GdiplusStartup(&token, &gdiplusStartupInput, NULL);

	if(status != Gdiplus::Ok)
	{
		return FALSE;
	}

	BOOL bSuccess = FALSE;

	/* This object needs to be
	deleted before GdiplusShutdown
	is called. If it were stack
	allocated, it would go out of
	scope _after_ the call to
	GdiplusShutdown. By allocating
	it on the heap, the lifetime
	can be directly controlled. */
	Gdiplus::Image *image = new Gdiplus::Image(lpszImage, FALSE);

	if(image->GetLastStatus() == Gdiplus::Ok)
	{
		if(propId == PropertyTagImageWidth)
		{
			bSuccess = TRUE;
			StringCchPrintf(szProperty, cchMax, _T("%u pixels"), image->GetWidth());
		}
		else if(propId == PropertyTagImageHeight)
		{
			bSuccess = TRUE;
			StringCchPrintf(szProperty, cchMax, _T("%u pixels"), image->GetHeight());
		}
		else
		{
			UINT size = image->GetPropertyItemSize(propId);

			if(size != 0)
			{
				Gdiplus::PropertyItem *propertyItem = reinterpret_cast<Gdiplus::PropertyItem *>(malloc(size));

				if(propertyItem != NULL)
				{
					status = image->GetPropertyItem(propId, size, propertyItem);

					if(status == Gdiplus::Ok)
					{
						if(propertyItem->type == PropertyTagTypeASCII)
						{
							int iRes = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<LPCSTR>(propertyItem->value), -1,
								szProperty, cchMax);

							if(iRes != 0)
							{
								bSuccess = TRUE;
							}
						}
					}

					free(propertyItem);
				}
			}
		}
	}

	delete image;
	Gdiplus::GdiplusShutdown(token);

	return bSuccess;
}

BOOL GetFileNameFromUser(HWND hwnd,TCHAR *FullFileName,UINT cchMax,const TCHAR *InitialDirectory)
{
	/* As per the documentation for
	the OPENFILENAME structure, the
	length of the filename buffer
	should be at least 256. */
	assert(cchMax >= 256);

	TCHAR *Filter = _T("Text Document (*.txt)\0*.txt\0All Files\0*.*\0\0");
	OPENFILENAME ofn;
	BOOL bRet;

	ofn.lStructSize			= sizeof(ofn);
	ofn.hwndOwner			= hwnd;
	ofn.lpstrFilter			= Filter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= FullFileName;
	ofn.nMaxFile			= cchMax;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= InitialDirectory;
	ofn.lpstrTitle			= NULL;
	ofn.Flags				= OFN_ENABLESIZING|OFN_OVERWRITEPROMPT|OFN_EXPLORER;
	ofn.lpstrDefExt			= _T("txt");
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.pvReserved			= NULL;
	ofn.dwReserved			= NULL;
	ofn.FlagsEx				= NULL;

	bRet = GetSaveFileName(&ofn);

	return bRet;
}

BOOL IsImage(const TCHAR *szFileName)
{
	static const TCHAR *IMAGE_EXTS[] = {_T("bmp"),_T("ico"),
	_T("gif"),_T("jpg"),_T("exf"),_T("png"),_T("tif"),_T("wmf"),_T("emf"),_T("tiff")};
	TCHAR *ext;
	int i = 0;

	ext = PathFindExtension(szFileName);

	if(ext == NULL || (ext + 1) == NULL)
	{
		return FALSE;
	}

	ext++;

	for(i = 0;i < SIZEOF_ARRAY(IMAGE_EXTS);i++)
	{
		if(lstrcmpi(ext, IMAGE_EXTS[i]) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL GetFileProductVersion(const TCHAR *szFullFileName,
	DWORD *pdwProductVersionLS, DWORD *pdwProductVersionMS)
{
	return GetFileVersionValue(szFullFileName, ROOT, NULL,
		pdwProductVersionLS, pdwProductVersionMS,
		NULL, NULL, 0);
}

BOOL GetFileLanguage(const TCHAR *szFullFileName, WORD *pwLanguage)
{
	return GetFileVersionValue(szFullFileName, TRANSLATION,
		pwLanguage, NULL, NULL, NULL, NULL, 0);
}

BOOL GetVersionInfoString(const TCHAR *szFullFileName, const TCHAR *szVersionInfo,
	TCHAR *szVersionBuffer, UINT cchMax)
{
	return GetFileVersionValue(szFullFileName, STRING_TABLE_VALUE,
		NULL, NULL, NULL, szVersionInfo, szVersionBuffer, cchMax);
}

BOOL GetFileVersionValue(const TCHAR *szFullFileName, VersionSubBlockType_t subBlockType,
	WORD *pwLanguage, DWORD *pdwProductVersionLS, DWORD *pdwProductVersionMS,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax)
{
	BOOL bSuccess = FALSE;
	DWORD dwLen = GetFileVersionInfoSize(szFullFileName, NULL);

	if(dwLen > 0)
	{
		void *pBlock = malloc(dwLen);

		if(pBlock != NULL)
		{
			BOOL bRet = GetFileVersionInfo(szFullFileName, NULL, dwLen, pBlock);

			if(bRet)
			{
				TCHAR szSubBlock[64];
				LPVOID *pBuffer = NULL;
				UINT uStructureSize = 0;

				LangAndCodePage *plcp = NULL;
				VS_FIXEDFILEINFO *pvsffi = NULL;

				if(subBlockType == ROOT)
				{
					StringCchCopy(szSubBlock, SIZEOF_ARRAY(szSubBlock), _T("\\"));
					pBuffer = reinterpret_cast<LPVOID *>(&pvsffi);
					uStructureSize = sizeof(VS_FIXEDFILEINFO);
				}
				else if(subBlockType == TRANSLATION ||
					subBlockType == STRING_TABLE_VALUE)
				{
					StringCchCopy(szSubBlock, SIZEOF_ARRAY(szSubBlock), _T("\\VarFileInfo\\Translation"));
					pBuffer = reinterpret_cast<LPVOID *>(&plcp);
					uStructureSize = sizeof(LangAndCodePage);
				}

				UINT uLen;
				bRet = VerQueryValue(pBlock, szSubBlock, pBuffer, &uLen);

				if(bRet && (uLen >= uStructureSize))
				{
					bSuccess = TRUE;

					if(subBlockType == ROOT)
					{
						*pdwProductVersionLS = pvsffi->dwProductVersionLS;
						*pdwProductVersionMS = pvsffi->dwProductVersionMS;
					}
					else if(subBlockType == TRANSLATION)
					{
						*pwLanguage = plcp[0].wLanguage;
					}
					else if(subBlockType == STRING_TABLE_VALUE)
					{
						bSuccess = GetStringTableValue(pBlock, plcp, uLen / sizeof(LangAndCodePage),
							szVersionInfo, szVersionBuffer, cchMax);
					}
				}
			}

			free(pBlock);
		}
	}

	return bSuccess;
}

BOOL GetStringTableValue(void *pBlock, LangAndCodePage *plcp, UINT nItems,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax)
{
	BOOL bSuccess = FALSE;
	LANGID UserLangId = GetUserDefaultLangID();

	for(UINT i = 0; i < nItems; i++)
	{
		/* If the bottom eight bits of the language id's match, use this
		version information (since this means that the version information
		and the users default language are the same). Also use this version
		information if the language is not specified (i.e. wLanguage is 0). */
		if((plcp[i].wLanguage & 0xFF) == (UserLangId & 0xFF) ||
			plcp[i].wLanguage == 0)
		{
			TCHAR szSubBlock[64];
			StringCchPrintf(szSubBlock, SIZEOF_ARRAY(szSubBlock),
				_T("\\StringFileInfo\\%04X%04X\\%s"), plcp[i].wLanguage,
				plcp[i].wCodePage, szVersionInfo);

			TCHAR *szBuffer;
			UINT uLen;
			BOOL bRet = VerQueryValue(pBlock, szSubBlock, reinterpret_cast<LPVOID *>(&szBuffer), &uLen);

			if(bRet && (uLen > 0))
			{
				StringCchCopy(szVersionBuffer, cchMax, szBuffer);
				bSuccess = TRUE;
				break;
			}
		}
	}

	return bSuccess;
}

void GetCPUBrandString(char *pszCPUBrand,UINT cchBuf)
{
	int CPUInfo[4] = {-1};
	char szCPUBrand[64];

	/* Refer to cpuid documentation at:
	http://msdn.microsoft.com/en-us/library/hskdteyh(v=vs.100).aspx */
	__cpuid(CPUInfo,0x80000002);
	memcpy(szCPUBrand,CPUInfo,sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000003);
	memcpy(szCPUBrand + 16,CPUInfo,sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000004);
	memcpy(szCPUBrand + 32,CPUInfo,sizeof(CPUInfo));

	StringCchCopyA(pszCPUBrand,cchBuf,szCPUBrand);
}

HRESULT GetMediaMetadata(const TCHAR *szFileName,const TCHAR *szAttribute,BYTE **pszOutput)
{
	typedef HRESULT (WINAPI *WMCREATEEDITOR_PROC)(IWMMetadataEditor **);
	WMCREATEEDITOR_PROC pWMCreateEditor = NULL;
	HMODULE hWMVCore;
	IWMMetadataEditor *pEditor = NULL;
	IWMHeaderInfo *pWMHeaderInfo = NULL;
	HRESULT hr = E_FAIL;

	hWMVCore = LoadLibrary(_T("wmvcore.dll"));

	if(hWMVCore != NULL)
	{
		pWMCreateEditor = (WMCREATEEDITOR_PROC)GetProcAddress(hWMVCore,"WMCreateEditor");

		if(pWMCreateEditor != NULL)
		{
			hr = pWMCreateEditor(&pEditor);

			if(SUCCEEDED(hr))
			{
				hr = pEditor->Open(szFileName);

				if(SUCCEEDED(hr))
				{
					hr = pEditor->QueryInterface(IID_PPV_ARGS(&pWMHeaderInfo));

					if(SUCCEEDED(hr))
					{
						WORD wStreamNum;
						WMT_ATTR_DATATYPE Type;
						WORD cbLength;

						/* Any stream. Should be zero for MP3 files. */
						wStreamNum = 0;

						hr = pWMHeaderInfo->GetAttributeByName(&wStreamNum,szAttribute,&Type,NULL,&cbLength);

						if(SUCCEEDED(hr))
						{
							*pszOutput = (BYTE *)malloc(cbLength);

							if(*pszOutput != NULL)
							{
								hr = pWMHeaderInfo->GetAttributeByName(&wStreamNum,szAttribute,&Type,
									*pszOutput,&cbLength);
							}
						}

						pWMHeaderInfo->Release();
					}
				}

				pEditor->Release();
			}
		}

		FreeLibrary(hWMVCore);
	}

	return hr;
}

void SetFORMATETC(FORMATETC *pftc, CLIPFORMAT cfFormat,
	DVTARGETDEVICE *ptd, DWORD dwAspect, LONG lindex,
	DWORD tymed)
{
	pftc->cfFormat = cfFormat;
	pftc->tymed = tymed;
	pftc->lindex = lindex;
	pftc->dwAspect = dwAspect;
	pftc->ptd = ptd;
}

BOOL CopyTextToClipboard(const std::wstring &str)
{
	if(!OpenClipboard(NULL))
	{
		return FALSE;
	}

	EmptyClipboard();

	size_t stringSize = (str.size() + 1) * sizeof(TCHAR);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, stringSize);
	BOOL bRes = FALSE;

	if(hGlobal != NULL)
	{
		LPVOID pMem = GlobalLock(hGlobal);

		if(pMem != NULL)
		{
			memcpy(pMem, str.c_str(), stringSize);
			GlobalUnlock(hGlobal);

			HANDLE hData = SetClipboardData(CF_UNICODETEXT, hGlobal);

			if(hData != NULL)
			{
				bRes = TRUE;
			}
		}
	}

	CloseClipboard();

	return bRes;
}

bool IsKeyDown(int nVirtKey)
{
	SHORT status = (GetKeyState(nVirtKey) & 0x8000);

	return (status != 0);
}