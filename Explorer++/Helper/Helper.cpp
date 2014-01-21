/******************************************************************
 *
 * Project: Helper
 * File: Helper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Contains various helper functions.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Helper.h"
#include "DriveInfo.h"
#include "Macros.h"


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
BOOL GetStringTableValue(void *pBlock, LANGANDCODEPAGE *plcp, UINT nItems,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax);

int CreateFileTimeString(const FILETIME *FileTime,
TCHAR *Buffer,int MaxCharacters,BOOL bFriendlyDate)
{
	SYSTEMTIME SystemTime;
	FILETIME LocalFileTime;
	TCHAR TempBuffer[512];
	TCHAR DateBuffer[512];
	TCHAR TimeBuffer[512];
	SYSTEMTIME CurrentTime;
	int iReturn1 = 0;
	int iReturn2 = 0;

	if(FileTime == NULL)
	{
		Buffer = NULL;
		return -1;
	}

	FileTimeToLocalFileTime(FileTime,&LocalFileTime);
	FileTimeToSystemTime(&LocalFileTime,&SystemTime);
	
	GetLocalTime(&CurrentTime);

	if(bFriendlyDate)
	{
		if((CurrentTime.wYear == SystemTime.wYear) &&
		(CurrentTime.wMonth == SystemTime.wMonth))
		{
			if(CurrentTime.wDay == SystemTime.wDay)
			{
				StringCchCopy(DateBuffer,SIZEOF_ARRAY(DateBuffer),_T("Today"));

				iReturn1 = 1;
			}
			else if(CurrentTime.wDay == (SystemTime.wDay + 1))
			{
				StringCchCopy(DateBuffer,SIZEOF_ARRAY(DateBuffer),_T("Yesterday"));

				iReturn1 = 1;
			}
			else
			{
				iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
					NULL, DateBuffer,512);
			}
		}
		else
		{
			iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
				NULL, DateBuffer,512);
		}
	}
	else
	{
		iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
			NULL, DateBuffer,512);
	}

	iReturn2 = GetTimeFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
		NULL, TimeBuffer,512);
	
	if(iReturn1 && iReturn2)
	{
		StringCchPrintf(TempBuffer,SIZEOF_ARRAY(TempBuffer),
			_T("%s, %s"),DateBuffer,TimeBuffer);

		if(MaxCharacters < (lstrlen(TempBuffer) + 1))
		{
			Buffer = NULL;

			return lstrlen(TempBuffer) + 1;
		}
		else
		{
			StringCchCopy(Buffer,MaxCharacters,TempBuffer);

			return lstrlen(TempBuffer) + 1;
		}
	}

	Buffer = NULL;

	return -1;
}

HINSTANCE StartCommandPrompt(const TCHAR *Directory,bool Elevated)
{
	HINSTANCE hNewInstance = NULL;

	TCHAR SystemPath[MAX_PATH];
	BOOL bRes = SHGetSpecialFolderPath(NULL,SystemPath,CSIDL_SYSTEM,0);

	if(bRes)
	{
		TCHAR CommandPath[MAX_PATH];
		PathCombine(CommandPath,SystemPath,_T("cmd.exe"));

		TCHAR Operation[32];

		if(Elevated)
		{
			StringCchCopy(Operation,SIZEOF_ARRAY(Operation),_T("runas"));
		}
		else
		{
			StringCchCopy(Operation,SIZEOF_ARRAY(Operation),_T("open"));
		}

		hNewInstance = ShellExecute(NULL,Operation,CommandPath,NULL,Directory,
		SW_SHOWNORMAL);
	}

	return hNewInstance;
}

BOOL GetRealFileSize(const std::wstring &strFilename,PLARGE_INTEGER lpRealFileSize)
{
	LARGE_INTEGER lFileSize;
	DWORD dwClusterSize;
	HANDLE hFile;
	TCHAR szRoot[MAX_PATH];

	/* Get a handle to the file. */
	hFile = CreateFile(strFilename.c_str(),GENERIC_READ,
	FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	/* Get the files size (count of number of actual
	number of bytes in file). */
	GetFileSizeEx(hFile,&lFileSize);

	*lpRealFileSize = lFileSize;

	if(lFileSize.QuadPart != 0)
	{
		StringCchCopy(szRoot,SIZEOF_ARRAY(szRoot),strFilename.c_str());
		PathStripToRoot(szRoot);

		/* Get the cluster size of the drive the file resides on. */
		GetClusterSize(szRoot, &dwClusterSize);

		if((lpRealFileSize->QuadPart % dwClusterSize) != 0)
		{
			/* The real size is the logical file size rounded up to the end of the
			nearest cluster. */
			lpRealFileSize->QuadPart += dwClusterSize - (lpRealFileSize->QuadPart % dwClusterSize);
		}
	}

	CloseHandle(hFile);

	return TRUE;
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
	HANDLE hFindFile = FindFirstFile(lpszFileName, &wfd);
	HRESULT hr = E_FAIL;

	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		hr = BuildFileAttributeString(wfd.dwFileAttributes, szOutput, cchMax);
		FindClose(hFindFile);
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

	HANDLE hFile = CreateFile(szFile, READ_CONTROL, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		PSID pSidOwner = NULL;
		PSECURITY_DESCRIPTOR pSD = NULL;
		DWORD dwRet = GetSecurityInfo(hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
			&pSidOwner, NULL, NULL, NULL, &pSD);

		if(dwRet == ERROR_SUCCESS)
		{
			success = FormatUserName(pSidOwner, szOwner, cchMax);
			LocalFree(pSD);
		}

		CloseHandle(hFile);
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

	HANDLE hFile = CreateFile(lpszFileName, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, NULL, NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION FileInfo;
		BOOL bRet = GetFileInformationByHandle(hFile, &FileInfo);

		if(bRet)
		{
			nLinks = FileInfo.nNumberOfLinks;
		}

		CloseHandle(hFile);
	}

	return nLinks;
}

int ReadFileProperty(const TCHAR *lpszFileName,DWORD dwPropertyType,TCHAR *lpszPropertyBuf,DWORD dwBufLen)
{
	HANDLE hFile;
	TCHAR szCommentStreamName[512];
	LPCSTR lpszProperty;
	DWORD dwNumBytesRead;
	DWORD dwSectionLength;
	DWORD dwPropertyCount;
	DWORD dwPropertyId;
	DWORD dwPropertyOffset = 0;
	DWORD dwPropertyLength;
	DWORD dwPropertyMarker;
	DWORD dwSectionOffset;
	DWORD dwCodepageOffset = 0;
	UINT uCodepage = CP_ACP;
	BOOL bFound = FALSE;
	unsigned int i = 0;

	StringCchPrintf(szCommentStreamName,SIZEOF_ARRAY(szCommentStreamName),
	_T("%s:%cSummaryInformation"),lpszFileName,0x5);
	hFile = CreateFile(szCommentStreamName,GENERIC_READ,FILE_SHARE_READ,NULL,
	OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	/* Constant offset. */
	SetFilePointer(hFile,0x2C,0,FILE_CURRENT);

	ReadFile(hFile,(LPBYTE)&dwSectionOffset,sizeof(dwSectionOffset),&dwNumBytesRead,NULL);

	/* The section offset is from the start of the stream. */
	SetFilePointer(hFile,dwSectionOffset,0,FILE_BEGIN);

	/* Since this is the only section, the section length gives the length from the
	start of the section to the end of the stream. The property count gives the
	number of properties associated with this file (author, comments, etc). */
	ReadFile(hFile,(LPBYTE)&dwSectionLength,sizeof(DWORD),&dwNumBytesRead,NULL);
	ReadFile(hFile,(LPBYTE)&dwPropertyCount,sizeof(DWORD),&dwNumBytesRead,NULL);

	for(i = 0;i < dwPropertyCount;i++)
	{
		ReadFile(hFile,(LPBYTE)&dwPropertyId,sizeof(dwPropertyId),&dwNumBytesRead,NULL);

		if(dwPropertyId == 1)
		{
			/* Property id 1 is the codepage that any string properties are encoded in.
			If present, it occurs before the strings to which it refers. */
			ReadFile(hFile,(LPBYTE)&dwCodepageOffset,sizeof(dwCodepageOffset),&dwNumBytesRead,NULL);
		}
		else if(dwPropertyId == dwPropertyType)
		{
			bFound = TRUE;

			/* The offset is from the start of this section. */
			ReadFile(hFile,(LPBYTE)&dwPropertyOffset,sizeof(dwPropertyOffset),&dwNumBytesRead,NULL);
		}
		else
		{
			/* Skip past the offset. */
			SetFilePointer(hFile,0x04,0,FILE_CURRENT);
		}
	}

	if(!bFound)
	{
		CloseHandle(hFile);
		return -1;
	}

	if(dwCodepageOffset)
	{
		SetFilePointer(hFile,dwSectionOffset + dwCodepageOffset + 4,0,FILE_BEGIN);
		ReadFile(hFile,(LPBYTE)&uCodepage,sizeof(uCodepage),&dwNumBytesRead,NULL);
	}

	/* Property offsets are given from the start of the section. */
	SetFilePointer(hFile,dwSectionOffset + dwPropertyOffset,0,FILE_BEGIN);

	/* Read the property marker. If this is not equal to 0x1E, then this is not a valid property
	(1E indicates a NULL terminated string prepended by dword string length). */
	ReadFile(hFile,(LPBYTE)&dwPropertyMarker,sizeof(dwPropertyMarker),&dwNumBytesRead,NULL);

	//if(dwPropertyMarker != 0x1E)
		//return -1;

	/* Read the length of the property (if the property is a string, this length includes the
	NULL byte). */
	ReadFile(hFile,(LPBYTE)&dwPropertyLength,sizeof(dwPropertyLength),&dwNumBytesRead,NULL);

	lpszProperty = (LPCSTR)malloc(dwPropertyLength);

	/* Read out the property of interest. */
	ReadFile(hFile,(LPBYTE)lpszProperty,dwPropertyLength,&dwNumBytesRead,NULL);
	
	if(dwNumBytesRead != dwPropertyLength)
	{
		/* This stream is not valid, and has probably been altered or damaged. */
		CloseHandle(hFile);
		return -1;
	}

	/* Convert the property string from its
	current codepage to UTF-16, as expected
	by the caller. */
	MultiByteToWideChar(uCodepage,MB_PRECOMPOSED,lpszProperty,
		dwPropertyLength,lpszPropertyBuf,dwBufLen);
	dwPropertyLength = lstrlen(lpszPropertyBuf) + 1;

	free((LPVOID)lpszProperty);
	CloseHandle(hFile);

	return dwPropertyLength;
}

BOOL ReadImageProperty(const TCHAR *lpszImage,UINT PropertyId,void *pPropBuffer,DWORD dwBufLen)
{
	Gdiplus::GdiplusStartupInput	StartupInput;
	Gdiplus::PropertyItem	*pPropItems = NULL;
	char				pTempBuffer[512];
	ULONG_PTR			Token;
	UINT				Size;
	UINT				NumProperties;
	Gdiplus::Status		res;
	BOOL				bFound = FALSE;
	unsigned int		i = 0;

	GdiplusStartup(&Token,&StartupInput,NULL);

	Gdiplus::Image *image = new Gdiplus::Image(lpszImage,FALSE);

	if(image->GetLastStatus() != Gdiplus::Ok)
	{
		delete image;
		return FALSE;
	}

	if(PropertyId == PropertyTagImageWidth)
	{
		UINT uWidth;

		uWidth = image->GetWidth();

		bFound = TRUE;

		StringCchPrintf((LPWSTR)pPropBuffer,dwBufLen,L"%u pixels",uWidth);
	}
	else if(PropertyId == PropertyTagImageHeight)
	{
		UINT uHeight;

		uHeight = image->GetHeight();

		bFound = TRUE;

		StringCchPrintf((LPWSTR)pPropBuffer,dwBufLen,L"%u pixels",uHeight);
	}
	else
	{
		image->GetPropertySize(&Size,&NumProperties);

		pPropItems = (Gdiplus::PropertyItem *)malloc(Size);
		res = image->GetAllPropertyItems(Size,NumProperties,pPropItems);

		if(res == Gdiplus::Ok)
		{
			for(i = 0;i < NumProperties;i++)
			{
				if(pPropItems[i].id == PropertyId)
				{
					bFound = TRUE;
					break;
				}
			}
		}

		if(!bFound && (PropertyId == PropertyTagExifDTOrig))
		{
			/* If the specified tag is PropertyTagExifDTOrig, we'll
			transparently fall back on PropertyTagDateTime. */
			for(i = 0;i < NumProperties;i++)
			{
				if(pPropItems[i].id == PropertyTagDateTime)
				{
					bFound = TRUE;
					break;
				}
			}
		}

		if(bFound)
		{
			memcpy(pTempBuffer,pPropItems[i].value,pPropItems[i].length);

			/* All property strings are ANSI. */
			MultiByteToWideChar(CP_ACP,0,pTempBuffer,-1,
				(WCHAR *)pPropBuffer,dwBufLen);
		}
		else
		{
			pPropBuffer = NULL;
		}

		free(pPropItems);
	}

	delete image;
	Gdiplus::GdiplusShutdown(Token);

	return bFound;
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

	if(szFileName != NULL)
	{
		ext = PathFindExtension(szFileName);

		if(ext == NULL || (ext + 1) == NULL)
			return FALSE;

		ext++;

		for(i = 0;i < SIZEOF_ARRAY(IMAGE_EXTS);i++)
		{
			if(lstrcmpi(ext,IMAGE_EXTS[i]) == 0)
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

				LANGANDCODEPAGE *plcp = NULL;
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
					uStructureSize = sizeof(LANGANDCODEPAGE);
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
						*pwLanguage = PRIMARYLANGID(plcp[0].wLanguage);
					}
					else if(subBlockType == STRING_TABLE_VALUE)
					{
						bSuccess = GetStringTableValue(pBlock, plcp, uLen / sizeof(LANGANDCODEPAGE),
							szVersionInfo, szVersionBuffer, cchMax);
					}
				}
			}

			free(pBlock);
		}
	}

	return bSuccess;
}

BOOL GetStringTableValue(void *pBlock, LANGANDCODEPAGE *plcp, UINT nItems,
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
					hr = pEditor->QueryInterface(IID_IWMHeaderInfo,(void **)&pWMHeaderInfo);

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