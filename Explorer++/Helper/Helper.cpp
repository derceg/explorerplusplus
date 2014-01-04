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
#include <sstream>
#include "Helper.h"
#include "DriveInfo.h"
#include "FileOperations.h"
#include "ShellHelper.h"
#include "Macros.h"


/* Local helpers. */
void	EnterAttributeIntoString(BOOL bEnter,TCHAR *String,int Pos,TCHAR chAttribute);

void FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,
size_t cchBuf)
{
	FormatSizeString(lFileSize,pszFileSize,cchBuf,FALSE,SIZE_FORMAT_NONE);
}

void FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,
size_t cchBuf,BOOL bForceSize,SizeDisplayFormat_t sdf)
{
	TCHAR *pszSizeTypes[] = {_T("bytes"),_T("KB"),_T("MB"),_T("GB"),_T("TB"),_T("PB")};

	double fFileSize = static_cast<double>(lFileSize.QuadPart);
	int iSizeIndex = 0;

	if(bForceSize)
	{
		switch(sdf)
		{
		case SIZE_FORMAT_BYTES:
			iSizeIndex = 0;
			break;

		case SIZE_FORMAT_KBYTES:
			iSizeIndex = 1;
			break;

		case SIZE_FORMAT_MBYTES:
			iSizeIndex = 2;
			break;

		case SIZE_FORMAT_GBYTES:
			iSizeIndex = 3;
			break;

		case SIZE_FORMAT_TBYTES:
			iSizeIndex = 4;
			break;

		case SIZE_FORMAT_PBYTES:
			iSizeIndex = 5;
			break;
		}

		for(int i = 0;i < iSizeIndex;i++)
		{
			fFileSize /= 1024;
		}
	}
	else
	{
		while((fFileSize / 1024) >= 1)
		{
			fFileSize /= 1024;

			iSizeIndex++;
		}

		if(iSizeIndex > (SIZEOF_ARRAY(pszSizeTypes) - 1))
		{
			StringCchCopy(pszFileSize,cchBuf,EMPTY_STRING);
			return;
		}
	}

	int iPrecision;

	if(iSizeIndex == 0)
	{
		iPrecision = 0;
	}
	else
	{
		if(fFileSize < 10)
		{
			iPrecision = 2;
		}
		else if(fFileSize < 100)
		{
			iPrecision = 1;
		}
		else
		{
			iPrecision = 0;
		}
	}

	int iLeast = static_cast<int>((fFileSize - static_cast<int>(fFileSize)) *
		pow(10.0,iPrecision + 1));

	/* Setting the precision will cause automatic rounding. Therefore,
	if the least significant digit to be dropped is greater than 0.5,
	reduce it to below 0.5. */
	if(iLeast >= 5)
	{
		fFileSize -= 5.0 * pow(10.0,-(iPrecision + 1));
	}

	std::wstringstream ss;
	ss.imbue(std::locale(""));
	ss.precision(iPrecision);

	ss << std::fixed << fFileSize << _T(" ") << pszSizeTypes[iSizeIndex];
	std::wstring str = ss.str();
	StringCchCopy(pszFileSize,cchBuf,str.c_str());
}

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

BOOL lCheckMenuItem(HMENU hMenu,UINT ItemID,BOOL bCheck)
{
	if(bCheck)
	{
		CheckMenuItem(hMenu,ItemID,MF_CHECKED);
		return TRUE;
	}
	else
	{
		CheckMenuItem(hMenu,ItemID,MF_UNCHECKED);
		return FALSE;
	}
}

BOOL lEnableMenuItem(HMENU hMenu,UINT ItemID,BOOL bEnable)
{
	if(bEnable)
	{
		EnableMenuItem(hMenu,ItemID,MF_ENABLED);
		return TRUE;
	}
	else
	{
		EnableMenuItem(hMenu,ItemID,MF_GRAYED);
		return FALSE;
	}
}

BOOL GetRealFileSize(const std::wstring &strFilename,PLARGE_INTEGER lpRealFileSize)
{
	LARGE_INTEGER lFileSize;
	LONG ClusterSize;
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
		ClusterSize = GetClusterSize(szRoot);

		if((lpRealFileSize->QuadPart % ClusterSize) != 0)
		{
			/* The real size is the logical file size rounded up to the end of the
			nearest cluster. */
			lpRealFileSize->QuadPart += ClusterSize - (lpRealFileSize->QuadPart % ClusterSize);
		}
	}

	CloseHandle(hFile);

	return TRUE;
}

BOOL FileTimeToLocalSystemTime(const LPFILETIME lpFileTime,LPSYSTEMTIME lpLocalTime)
{
	SYSTEMTIME SystemTime;

	FileTimeToSystemTime(lpFileTime,&SystemTime);

	return SystemTimeToTzSpecificLocalTime(NULL,&SystemTime,lpLocalTime);
}

BOOL LocalSystemTimeToFileTime(const LPSYSTEMTIME lpLocalTime,LPFILETIME lpFileTime)
{
	SYSTEMTIME SystemTime;

	TzSpecificLocalTimeToSystemTime(NULL,lpLocalTime,&SystemTime);

	return SystemTimeToFileTime(&SystemTime,lpFileTime);
}

BOOL SetProcessTokenPrivilege(DWORD ProcessId,const TCHAR *PrivilegeName,BOOL bEnablePrivilege)
{
	HANDLE hProcess;
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);

	if(hProcess == NULL)
		return FALSE;

	OpenProcessToken(hProcess,TOKEN_ALL_ACCESS,&hToken);

	CloseHandle(hProcess);

	LookupPrivilegeValue(NULL,PrivilegeName,&luid);

	tp.PrivilegeCount				= 1;
	tp.Privileges[0].Luid			= luid;

	if(bEnablePrivilege)
		tp.Privileges[0].Attributes	= SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes	= 0;

	BOOL Res = AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);

	CloseHandle(hToken);

	return Res;
}

BOOL CompareFileTypes(const TCHAR *pszFile1,const TCHAR *pszFile2)
{
	SHFILEINFO shfi1;
	SHFILEINFO shfi2;

	SHGetFileInfo(pszFile1,NULL,&shfi1,sizeof(shfi1),SHGFI_TYPENAME);
	SHGetFileInfo(pszFile2,NULL,&shfi2,sizeof(shfi2),SHGFI_TYPENAME);

	if(StrCmp(shfi1.szTypeName,shfi2.szTypeName) == 0)
		return TRUE;

	return FALSE;
}

TCHAR *PrintComma(unsigned long nPrint)
{
	LARGE_INTEGER lPrint;

	lPrint.LowPart = nPrint;
	lPrint.HighPart = 0;

	return PrintCommaLargeNum(lPrint);
}

TCHAR *PrintCommaLargeNum(LARGE_INTEGER lPrint)
{
	static TCHAR szBuffer[14];
	TCHAR *p = &szBuffer[SIZEOF_ARRAY(szBuffer) - 1];
	static TCHAR chComma = ',';
	unsigned long long nTemp = (unsigned long long)(lPrint.LowPart + (lPrint.HighPart * pow(2.0,32.0)));
	int i = 0;

	if(nTemp == 0)
	{
		StringCchPrintf(szBuffer,SIZEOF_ARRAY(szBuffer),_T("%d"),0);
		return szBuffer;
	}

	*p = (TCHAR)'\0';

	while(nTemp != 0)
	{
		if(i%3 == 0 && i != 0)
			*--p = chComma;

		*--p = '0' + (TCHAR)(nTemp % 10);

		nTemp /= 10;

		i++;
	}

	return p;
}

BOOL lShowWindow(HWND hwnd,BOOL bShowWindow)
{
	int WindowShowState;

	if(bShowWindow)
		WindowShowState = SW_SHOW;
	else
		WindowShowState = SW_HIDE;

	return ShowWindow(hwnd,WindowShowState);
}

int GetRectHeight(const RECT *rc)
{
	return rc->bottom - rc->top;
}

int GetRectWidth(const RECT *rc)
{
	return rc->right - rc->left;
}

DWORD BuildFileAttributeString(const TCHAR *lpszFileName,TCHAR *Buffer,DWORD BufSize)
{
	HANDLE hFindFile;
	WIN32_FIND_DATA wfd;

	/* FindFirstFile is used instead of GetFileAttributes() or
	GetFileAttributesEx() because of its behaviour
	in relation to system files that normally
	won't have their attributes given (such as the
	pagefile, which neither of the two functions
	above can retrieve the attributes of). */
	hFindFile = FindFirstFile(lpszFileName,&wfd);

	if(hFindFile == INVALID_HANDLE_VALUE)
	{
		StringCchCopy(Buffer,BufSize,EMPTY_STRING);
		return 0;
	}

	BuildFileAttributeStringInternal(wfd.dwFileAttributes,Buffer,BufSize);

	FindClose(hFindFile);

	return wfd.dwFileAttributes;
}

void BuildFileAttributeStringInternal(DWORD dwFileAttributes,TCHAR *szOutput,DWORD cchMax)
{
	TCHAR szAttributes[8];
	int i = 0;

	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE,szAttributes,i++,'A');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_HIDDEN,szAttributes,i++,'H');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_READONLY,szAttributes,i++,'R');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_SYSTEM,szAttributes,i++,'S');
	EnterAttributeIntoString((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY,
		szAttributes,i++,'D');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED,szAttributes,i++,'C');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED,szAttributes,i++,'E');

	szAttributes[i] = '\0';

	StringCchCopy(szOutput,cchMax,szAttributes);
}

void EnterAttributeIntoString(BOOL bEnter,TCHAR *String,int Pos,TCHAR chAttribute)
{
	if(bEnter)
		String[Pos] = chAttribute;
	else
		String[Pos] = '-';
}

size_t GetFileOwner(const TCHAR *szFile,TCHAR *szOwner,DWORD BufSize)
{
	HANDLE hFile;
	PSID pSid;
	PSECURITY_DESCRIPTOR pSecurityDescriptor;
	DWORD dwRes;
	TCHAR szAccountName[512];
	DWORD dwAccountName = SIZEOF_ARRAY(szAccountName);
	TCHAR szDomainName[512];
	DWORD dwDomainName = SIZEOF_ARRAY(szDomainName);
	size_t ReturnLength = 0;
	SID_NAME_USE eUse;
	LPTSTR StringSid;
	BOOL bRes;

	/* The SE_SECURITY_NAME privilege is needed to call GetSecurityInfo on the given file. */
	bRes = SetProcessTokenPrivilege(GetCurrentProcessId(),SE_SECURITY_NAME,TRUE);

	if(!bRes)
		return 0;

	hFile = CreateFile(szFile,STANDARD_RIGHTS_READ|ACCESS_SYSTEM_SECURITY,FILE_SHARE_READ,NULL,OPEN_EXISTING,
	FILE_FLAG_BACKUP_SEMANTICS,NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		pSid = (PSID)GlobalAlloc(GMEM_FIXED,sizeof(PSID));

		pSecurityDescriptor = (PSECURITY_DESCRIPTOR)GlobalAlloc(GMEM_FIXED,sizeof(PSECURITY_DESCRIPTOR));

		dwRes = GetSecurityInfo(hFile,SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,&pSid,
			NULL,NULL,NULL,&pSecurityDescriptor);

		if(dwRes != ERROR_SUCCESS)
		{
			CloseHandle(hFile);
			return 0;
		}

		bRes = LookupAccountSid(NULL,pSid,szAccountName,&dwAccountName,
			szDomainName,&dwDomainName,&eUse);

		/* LookupAccountSid failed. */
		if(bRes == 0)
		{
			bRes = ConvertSidToStringSid(pSid,&StringSid);

			if(bRes != 0)
			{
				StringCchCopy(szOwner,BufSize,StringSid);

				LocalFree(StringSid);
				ReturnLength = lstrlen(StringSid);
			}
		}
		else
		{
			StringCchPrintf(szOwner,BufSize,_T("%s\\%s"),szDomainName,szAccountName);

			ReturnLength = lstrlen(szAccountName);
		}

		LocalFree(&pSecurityDescriptor);
		CloseHandle(hFile);
	}

	/* Reset the privilege. */
	SetProcessTokenPrivilege(GetCurrentProcessId(),SE_SECURITY_NAME,FALSE);

	return ReturnLength;
}

BOOL GetProcessOwner(TCHAR *szOwner,DWORD BufSize)
{
	HANDLE hProcess;
	HANDLE hToken;
	TOKEN_USER *pTokenUser = NULL;
	SID_NAME_USE eUse;
	LPTSTR StringSid;
	TCHAR szAccountName[512];
	DWORD dwAccountName = SIZEOF_ARRAY(szAccountName);
	TCHAR szDomainName[512];
	DWORD dwDomainName = SIZEOF_ARRAY(szDomainName);
	DWORD ReturnLength;
	DWORD dwSize = 0;
	BOOL bRes;
	BOOL bReturn = FALSE;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,GetCurrentProcessId());

	if(hProcess != NULL)
	{
		bRes = OpenProcessToken(hProcess,TOKEN_ALL_ACCESS,&hToken);

		if(bRes)
		{
			GetTokenInformation(hToken,TokenUser,NULL,0,&dwSize);

			pTokenUser = (PTOKEN_USER)GlobalAlloc(GMEM_FIXED,dwSize);

			if(pTokenUser != NULL)
			{
				GetTokenInformation(hToken,TokenUser,(LPVOID)pTokenUser,dwSize,&ReturnLength);

				bRes = LookupAccountSid(NULL,pTokenUser->User.Sid,szAccountName,&dwAccountName,
					szDomainName,&dwDomainName,&eUse);

				/* LookupAccountSid failed. */
				if(bRes == 0)
				{
					bRes = ConvertSidToStringSid(pTokenUser->User.Sid,&StringSid);

					if(bRes != 0)
					{
						StringCchCopy(szOwner,BufSize,StringSid);

						LocalFree(StringSid);

						bReturn = TRUE;
					}
				}
				else
				{
					StringCchPrintf(szOwner,BufSize,_T("%s\\%s"),szDomainName,szAccountName);

					bReturn = TRUE;
				}

				GlobalFree(pTokenUser);
			}
		}
		CloseHandle(hProcess);
	}

	if(!bReturn)
		StringCchCopy(szOwner,BufSize,EMPTY_STRING);

	return bReturn;
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

struct LANGCODEPAGE
{
	WORD wLanguage;
	WORD wCodePage;
} *lpTranslate;

BOOL GetVersionInfoString(const TCHAR *szFileName,const TCHAR *szVersionInfo,TCHAR *szBuffer,UINT cbBufLen)
{
	LPVOID lpData;
	TCHAR szSubBlock[64];
	TCHAR *lpszLocalBuf = NULL;
	LANGID UserLangId;
	DWORD dwLen;
	DWORD dwHandle = NULL;
	UINT cbTranslate;
	UINT cbLen;
	BOOL bRet = FALSE;
	unsigned int i = 0;

	dwLen = GetFileVersionInfoSize(szFileName,&dwHandle);

	if(dwLen > 0)
	{
		lpData = malloc(dwLen);

		if(lpData != NULL)
		{
			if(GetFileVersionInfo(szFileName,0,dwLen,lpData) != 0)
			{
				UserLangId = GetUserDefaultLangID();

				VerQueryValue(lpData,_T("\\VarFileInfo\\Translation"),(LPVOID *)&lpTranslate,&cbTranslate);

				for(i = 0;i < (cbTranslate / sizeof(LANGCODEPAGE));i++)
				{
					/* If the bottom eight bits of the language id's match, use this
					version information (since this means that the version information
					and the users default language are the same). Also use this version
					information if the language is not specified (i.e. wLanguage is 0). */
					if((lpTranslate[i].wLanguage & 0xFF) == (UserLangId & 0xFF) ||
						lpTranslate[i].wLanguage == 0)
					{
						StringCchPrintf(szSubBlock,SIZEOF_ARRAY(szSubBlock),
							_T("\\StringFileInfo\\%04X%04X\\%s"),lpTranslate[i].wLanguage,
							lpTranslate[i].wCodePage,szVersionInfo);

						if(VerQueryValue(lpData,szSubBlock,(LPVOID *)&lpszLocalBuf,&cbLen) != 0)
						{
							/* The buffer may be NULL if the specified data was not found
							within the file. */
							if(lpszLocalBuf != NULL)
							{
								StringCchCopy(szBuffer,cbBufLen,lpszLocalBuf);

								bRet = TRUE;
								break;
							}
						}
					}
				}
			}
			free(lpData);
		}
	}

	return bRet;
}

DWORD GetNumFileHardLinks(const TCHAR *lpszFileName)
{
	HANDLE hFile;
	BY_HANDLE_FILE_INFORMATION FileInfo;
	BOOL bRes;

	hFile = CreateFile(lpszFileName,FILE_READ_ATTRIBUTES,FILE_SHARE_READ,NULL,
	OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return 0;

	bRes = GetFileInformationByHandle(hFile,&FileInfo);

	CloseHandle(hFile);

	if(bRes == 0)
	{
		return 0;
	}

	return FileInfo.nNumberOfLinks;
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

	#ifdef UNICODE
	/* Convert the property string from its current codepage to UTF-16, as expected by our caller. */
	MultiByteToWideChar(uCodepage,MB_PRECOMPOSED,
		lpszProperty,dwPropertyLength,
		lpszPropertyBuf,dwBufLen);
	dwPropertyLength = lstrlen(lpszPropertyBuf) + 1;
	#else
	StringCchCopy(lpszPropertyBuf,dwBufLen,lpszProperty);
	#endif

	free((LPVOID)lpszProperty);
	CloseHandle(hFile);

	return dwPropertyLength;
}

BOOL ReadImageProperty(const TCHAR *lpszImage,UINT PropertyId,void *pPropBuffer,DWORD dwBufLen)
{
	Gdiplus::GdiplusStartupInput	StartupInput;
	WCHAR				wszImage[MAX_PATH];
	Gdiplus::PropertyItem	*pPropItems = NULL;
	char				pTempBuffer[512];
	ULONG_PTR			Token;
	UINT				Size;
	UINT				NumProperties;
	Gdiplus::Status		res;
	BOOL				bFound = FALSE;
	unsigned int		i = 0;

	GdiplusStartup(&Token,&StartupInput,NULL);

	#ifndef UNICODE
	MultiByteToWideChar(CP_ACP,0,lpszImage,
	-1,wszImage,SIZEOF_ARRAY(wszImage));
	#else
	StringCchCopy(wszImage,SIZEOF_ARRAY(wszImage),lpszImage);
	#endif

	Gdiplus::Image *image = new Gdiplus::Image(wszImage,FALSE);

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
			memcpy(pTempBuffer,pPropItems[i].value,pPropItems[i].length);
		else
			pPropBuffer = NULL;

		/* All property strings are ANSI. */
		#ifndef UNICODE
		StringCchCopy((char *)pPropBuffer,dwBufLen,pTempBuffer);
		#else
		MultiByteToWideChar(CP_ACP,0,pTempBuffer,
			-1,(WCHAR *)pPropBuffer,dwBufLen);
		#endif

		free(pPropItems);
	}

	delete image;
	Gdiplus::GdiplusShutdown(Token);

	return bFound;
}

int ReadFileSlack(const TCHAR *FileName,TCHAR *pszSlack,int iBufferLen)
{
	HANDLE			hFile;
	DWORD			FileSize;
	DWORD			nBytesRead = 0;
	DWORD			FileSectorSize;
	TCHAR			*pszSlackTemp = NULL;
	DWORD			BytesPerSector;
	LARGE_INTEGER	lRealFileSize;
	TCHAR			Root[MAX_PATH];
	int				SpareSectors;
	BOOL			res;

	/* The SE_MANAGE_VOLUME_NAME privilege is needed to set
	the valid data length of a file. */
	SetProcessTokenPrivilege(GetCurrentProcessId(),SE_MANAGE_VOLUME_NAME,TRUE);

	if(GetLastError() != ERROR_SUCCESS)
		return -1;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,
	FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,
	NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	StringCchCopy(Root,SIZEOF_ARRAY(Root),FileName);
	PathStripToRoot(Root);

	res = GetDiskFreeSpace(Root,NULL,&BytesPerSector,NULL,NULL);

	if(res)
	{
		/* Get the file's logical size. */
		FileSize = GetFileSize(hFile,NULL);

		/* Determine the files actual size (i.e. the
		size it physically takes up on disk). */
		GetRealFileSize(FileName,&lRealFileSize);

		if(lRealFileSize.QuadPart > FileSize)
		{
			FileSectorSize = GetFileSectorSize(FileName);

			/* Determine the number of sectors at the end of the file
			that are currently not in use. */
			SpareSectors = (int)((lRealFileSize.QuadPart / BytesPerSector) - FileSectorSize);

			/* Extend the file to the physical end of file. */
			SetFilePointerEx(hFile,lRealFileSize,NULL,FILE_BEGIN);
			SetEndOfFile(hFile);
			SetFileValidData(hFile,lRealFileSize.QuadPart);

			if((FileSectorSize * BytesPerSector) > FileSize)
			{
				/* Move the file pointer back to the logical end of file, so that all data
				after the logical end of file can be read. */
				SetFilePointer(hFile,FileSectorSize * BytesPerSector,NULL,FILE_BEGIN);

				pszSlackTemp = (TCHAR *)malloc(SpareSectors * BytesPerSector);

				if(pszSlackTemp != NULL)
				{
					/* Read out the data contained after the logical end of file. */
					ReadFile(hFile,(LPVOID)pszSlackTemp,SpareSectors * BytesPerSector,&nBytesRead,NULL);

					memcpy_s(pszSlack,iBufferLen,pszSlackTemp,nBytesRead);
				}
			}

			/* Now shrink the file back to its original size. */
			SetFilePointer(hFile,FileSize,NULL,FILE_BEGIN);
			SetEndOfFile(hFile);
			SetFileValidData(hFile,FileSize);
		}
	}

	CloseHandle(hFile);

	return nBytesRead;
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

void TabCtrl_SwapItems(HWND hTabCtrl,int iItem1,int iItem2)
{
	TCITEM tcItem;
	LPARAM lParam1;
	LPARAM lParam2;
	TCHAR szText1[512];
	TCHAR szText2[512];
	int iImage1;
	int iImage2;
	BOOL res;

	tcItem.mask			= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText		= szText1;
	tcItem.cchTextMax	= SIZEOF_ARRAY(szText1);

	res = TabCtrl_GetItem(hTabCtrl,iItem1,&tcItem);

	if(!res)
		return;

	lParam1 = tcItem.lParam;
	iImage1 = tcItem.iImage;

	tcItem.mask			= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText		= szText2;
	tcItem.cchTextMax	= SIZEOF_ARRAY(szText2);

	res = TabCtrl_GetItem(hTabCtrl,iItem2,&tcItem);

	if(!res)
		return;

	lParam2 = tcItem.lParam;
	iImage2 = tcItem.iImage;

	tcItem.mask		= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText	= szText1;
	tcItem.lParam	= lParam1;
	tcItem.iImage	= iImage1;

	TabCtrl_SetItem(hTabCtrl,iItem2,&tcItem);

	tcItem.mask		= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText	= szText2;
	tcItem.lParam	= lParam2;
	tcItem.iImage	= iImage2;

	TabCtrl_SetItem(hTabCtrl,iItem1,&tcItem);
}

void TabCtrl_SetItemText(HWND Tab,int iTab,TCHAR *Text)
{
	TCITEM tcItem;

	if(Text == NULL)
		return;

	tcItem.mask			= TCIF_TEXT;
	tcItem.pszText		= Text;

	SendMessage(Tab,TCM_SETITEM,(WPARAM)(int)iTab,(LPARAM)&tcItem);
}

BOOL CheckWildcardMatch(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive)
{
	/* Handles multiple wildcard patterns. If the wildcard pattern contains ':', 
	split the pattern into multiple subpatterns.
	For example "*.h: *.cpp" would match against "*.h" and "*.cpp" */
	BOOL bMultiplePattern = FALSE;

	for(int i = 0; i < lstrlen(szWildcard); i++)
	{
		if(szWildcard[i] == ':')
		{
			bMultiplePattern = TRUE;
			break;
		}
	}

	if(!bMultiplePattern)
	{
		return CheckWildcardMatchInternal(szWildcard,szString,bCaseSensitive);
	}
	else
	{
		TCHAR szWildcardPattern[512];
		TCHAR *szSinglePattern = NULL;
		TCHAR *szSearchPattern = NULL;
		TCHAR *szRemainingPattern = NULL;

		StringCchCopy(szWildcardPattern,SIZEOF_ARRAY(szWildcardPattern),szWildcard);

		szSinglePattern = cstrtok_s(szWildcardPattern,_T(":"),&szRemainingPattern);
		PathRemoveBlanks(szSinglePattern);

		while(szSinglePattern != NULL)
		{
			if(CheckWildcardMatchInternal(szSinglePattern,szString,bCaseSensitive))
			{
				return TRUE;
			}

			szSearchPattern = szRemainingPattern;
			szSinglePattern = cstrtok_s(szSearchPattern,_T(":"),&szRemainingPattern);
			PathRemoveBlanks(szSinglePattern);
		}
	}

	return FALSE;
}

BOOL CheckWildcardMatchInternal(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive)
{
	BOOL bMatched;
	BOOL bCurrentMatch = TRUE;

	while(*szWildcard != '\0' && *szString != '\0' && bCurrentMatch)
	{
		switch(*szWildcard)
		{
		/* Match against the next part of the wildcard string.
		If there is a match, then return true, else consume
		the next character, and check again. */
		case '*':
			bMatched = FALSE;

			if(*(szWildcard + 1) != '\0')
			{
				bMatched = CheckWildcardMatch(++szWildcard,szString,bCaseSensitive);
			}

			while(*szWildcard != '\0' && *szString != '\0' && !bMatched)
			{
				/* Consume one more character on the input string,
				and keep (recursively) trying to match. */
				bMatched = CheckWildcardMatch(szWildcard,++szString,bCaseSensitive);
			}

			if(bMatched)
			{
				while(*szWildcard != '\0')
					szWildcard++;

				szWildcard--;

				while(*szString != '\0')
					szString++;
			}

			bCurrentMatch = bMatched;
			break;

		case '?':
			szString++;
			break;

		default:
			if(bCaseSensitive)
			{
				bCurrentMatch = (*szWildcard == *szString);
			}
			else
			{
				TCHAR szCharacter1[1];
				LCMapString(LOCALE_USER_DEFAULT,LCMAP_LOWERCASE,szWildcard,1,szCharacter1,SIZEOF_ARRAY(szCharacter1));

				TCHAR szCharacter2[1];
				LCMapString(LOCALE_USER_DEFAULT,LCMAP_LOWERCASE,szString,1,szCharacter2,SIZEOF_ARRAY(szCharacter2));

				bCurrentMatch = (szCharacter1[0] == szCharacter2[0]);
			}

			szString++;
			break;
		}

		szWildcard++;
	}

	/* Skip past any trailing wildcards. */
	while(*szWildcard == '*')
		szWildcard++;

	if(*szWildcard == '\0' && *szString == '\0' && bCurrentMatch)
		return TRUE;

	return FALSE;
}

TCHAR *DecodePrinterStatus(DWORD dwStatus)
{
	if(dwStatus == 0)
		return _T("Ready");
	else if(dwStatus & PRINTER_STATUS_BUSY)
		return _T("Busy");
	else if(dwStatus & PRINTER_STATUS_ERROR)
		return _T("Error");
	else if(dwStatus & PRINTER_STATUS_INITIALIZING)
		return _T("Initializing");
	else if(dwStatus & PRINTER_STATUS_IO_ACTIVE)
		return _T("Active");
	else if(dwStatus & PRINTER_STATUS_NOT_AVAILABLE)
		return _T("Unavailable");
	else if(dwStatus & PRINTER_STATUS_OFFLINE)
		return _T("Offline");
	else if(dwStatus & PRINTER_STATUS_OUT_OF_MEMORY)
		return _T("Out of memory");
	else if(dwStatus & PRINTER_STATUS_NO_TONER)
		return _T("Out of toner");

	return NULL;
}

BOOL IsImage(const TCHAR *szFileName)
{
	static const TCHAR *ImageExts[] = {_T("bmp"),_T("ico"),
	_T("gif"),_T("jpg"),_T("exf"),_T("png"),_T("tif"),_T("wmf"),_T("emf"),_T("tiff")};
	TCHAR *ext;
	int i = 0;

	if(szFileName != NULL)
	{
		ext = PathFindExtension(szFileName);

		if(ext == NULL || (ext + 1) == NULL)
			return FALSE;

		ext++;

		for(i = 0;i < SIZEOF_ARRAY(ImageExts);i++)
		{
			if(lstrcmpi(ext,ImageExts[i]) == 0)
				return TRUE;
		}
	}

	return FALSE;
}

void ReplaceCharacters(TCHAR *str,char ch,char replacement)
{
	int  i = 0;

	for(i = 0;i < lstrlen(str);i++)
	{
		if(str[i] == ch)
			str[i] = replacement;
	}
}

TCHAR *GetToken(TCHAR *ptr,TCHAR *Buffer,TCHAR *BufferLength)
{
	TCHAR *p;
	int i = 0;

	if(ptr == NULL || *ptr == '\0')
	{
		*Buffer = NULL;
		return NULL;
	}

	p = ptr;

	while(*p == ' ' || *p == '\t')
		p++;

	if(*p == '\"')
	{
		p++;
		while(*p != '\0' && *p != '\"')
		{
			Buffer[i++] = *p;
			p++;
		}
		p++;
	}
	else
	{
		while(*p != '\0' && *p != ' ' && *p != '\t')
		{
			Buffer[i++] = *p;
			p++;
		}
	}

	Buffer[i] = '\0';

	while(*p == ' ' || *p == '\t')
		p++;

	return p;
}

void AddGripperStyle(UINT *fStyle,BOOL bAddGripper)
{
	if(bAddGripper)
	{
		/* Remove the no-gripper style (if present). */
		if((*fStyle & RBBS_NOGRIPPER) == RBBS_NOGRIPPER)
			*fStyle &= ~RBBS_NOGRIPPER;

		/* Only add the gripper style if it isn't already present. */
		if((*fStyle & RBBS_GRIPPERALWAYS) != RBBS_GRIPPERALWAYS)
			*fStyle |= RBBS_GRIPPERALWAYS;
	}
	else
	{
		/* Remove the gripper style (if present). */
		if((*fStyle & RBBS_GRIPPERALWAYS) == RBBS_GRIPPERALWAYS)
			*fStyle &= ~RBBS_GRIPPERALWAYS;

		/* Only add the gripper style if it isn't already present. */
		if((*fStyle & RBBS_NOGRIPPER) != RBBS_NOGRIPPER)
			*fStyle |= RBBS_NOGRIPPER;
	}
}

/* Adds or removes the specified window
style from a window. */
void AddWindowStyle(HWND hwnd,UINT fStyle,BOOL bAdd)
{
	LONG_PTR fCurrentStyle;

	fCurrentStyle = GetWindowLongPtr(hwnd,GWL_STYLE);

	if(bAdd)
	{
		/* Only add the style if it isn't already present. */
		if((fCurrentStyle & fStyle) != fStyle)
			fCurrentStyle |= fStyle;
	}
	else
	{
		/* Only remove the style if it is present. */
		if((fCurrentStyle & fStyle) == fStyle)
			fCurrentStyle &= ~fStyle;
	}

	SetWindowLongPtr(hwnd,GWL_STYLE,fCurrentStyle);
}

DWORD GetCurrentProcessImageName(TCHAR *szImageName,DWORD nSize)
{
	HANDLE	hProcess;
	DWORD	dwProcessId;
	DWORD	dwRet = 0;

	dwProcessId = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,dwProcessId);

	if(hProcess != NULL)
	{
		dwRet = GetModuleFileNameEx(hProcess,NULL,szImageName,nSize);
		CloseHandle(hProcess);
	}

	return dwRet;
}

WORD GetFileLanguage(const TCHAR *szFullFileName)
{
	LANGANDCODEPAGE	*plcp = NULL;
	DWORD			dwLen;
	DWORD			dwHandle;
	WORD			wLanguage = 0;
	UINT			uLen;
	void			*pTranslateInfo = NULL;

	dwLen = GetFileVersionInfoSize(szFullFileName,&dwHandle);

	if(dwLen > 0)
	{
		pTranslateInfo = malloc(dwLen);

		if(pTranslateInfo != NULL)
		{
			GetFileVersionInfo(szFullFileName,NULL,dwLen,pTranslateInfo);
			VerQueryValue(pTranslateInfo,_T("\\VarFileInfo\\Translation"),
				(LPVOID *)&plcp,&uLen);

			if(uLen >= sizeof(LANGANDCODEPAGE))
				wLanguage = PRIMARYLANGID(plcp[0].wLanguage);

			free(pTranslateInfo);
		}
	}

	return wLanguage;
}

BOOL GetFileProductVersion(const TCHAR *szFullFileName,
DWORD *pdwProductVersionLS,DWORD *pdwProductVersionMS)
{
	VS_FIXEDFILEINFO	*pvsffi = NULL;
	DWORD			dwLen;
	DWORD			dwHandle;
	UINT			uLen;
	BOOL			bSuccess = FALSE;
	void			*pData = NULL;

	*pdwProductVersionLS = 0;
	*pdwProductVersionMS = 0;

	dwLen = GetFileVersionInfoSize(szFullFileName,&dwHandle);

	if(dwLen > 0)
	{
		pData = malloc(dwLen);

		if(pData != NULL)
		{
			GetFileVersionInfo(szFullFileName,NULL,dwLen,pData);
			VerQueryValue(pData,_T("\\"),
				(LPVOID *)&pvsffi,&uLen);

			/* To retrieve the product version numbers:
			HIWORD(pvsffi->dwProductVersionMS);
			LOWORD(pvsffi->dwProductVersionMS);
			HIWORD(pvsffi->dwProductVersionLS);
			LOWORD(pvsffi->dwProductVersionLS); */

			if(uLen > 0)
			{
				*pdwProductVersionLS = pvsffi->dwProductVersionLS;
				*pdwProductVersionMS = pvsffi->dwProductVersionMS;

				bSuccess = TRUE;
			}

			free(pData);
		}
	}

	return bSuccess;
}

void GetCPUBrandString(char *pszCPUBrand,UINT cchBuf)
{
	int CPUInfo[4] = {-1};
	char szCPUBrand[64];

	/* Refer to cpuid documentation at:
	ms-help://MS.VSCC.v90/MS.MSDNQTR.v90.en/dv_vclang/html/f8c344d3-91bf-405f-8622-cb0e337a6bdc.htm */
	__cpuid(CPUInfo,0x80000002);
	memcpy(szCPUBrand,CPUInfo,sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000003);
	memcpy(szCPUBrand + 16,CPUInfo,sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000004);
	memcpy(szCPUBrand + 32,CPUInfo,sizeof(CPUInfo));

	StringCchCopyA(pszCPUBrand,cchBuf,szCPUBrand);
}

void ReplaceCharacterWithString(const TCHAR *szBaseString,TCHAR *szOutput,
UINT cchMax,TCHAR chToReplace,const TCHAR *szReplacement)
{
	TCHAR szNewString[1024];
	int iBase = 0;
	int i = 0;

	szNewString[0] = '\0';
	for(i = 0;i < lstrlen(szBaseString);i++)
	{
		if(szBaseString[i] == '&')
		{
			StringCchCatN(szNewString,SIZEOF_ARRAY(szNewString),
				&szBaseString[iBase],i - iBase);
			StringCchCat(szNewString,SIZEOF_ARRAY(szNewString),szReplacement);

			iBase = i + 1;
		}
	}

	StringCchCatN(szNewString,SIZEOF_ARRAY(szNewString),
		&szBaseString[iBase],i - iBase);

	StringCchCopy(szOutput,cchMax,szNewString);
}

/* Centers one window (hChild) with respect to
another (hParent), as per the Windows UX
Guidelines (2009).
This means placing the child window 45% of the
way from the top of the parent window (with 55%
of the space left between the bottom of the
child window and the bottom of the parent window).*/
void CenterWindow(HWND hParent,HWND hChild)
{
	RECT rcParent;
	RECT rcChild;
	POINT ptOrigin;

	GetClientRect(hParent,&rcParent);
	GetClientRect(hChild,&rcChild);

	/* Take the offset between the two windows, and map it back to the
	desktop. */
	ptOrigin.x = (GetRectWidth(&rcParent) - GetRectWidth(&rcChild)) / 2;
	ptOrigin.y = (LONG)((GetRectHeight(&rcParent) - GetRectHeight(&rcChild)) * 0.45);
	MapWindowPoints(hParent,HWND_DESKTOP,&ptOrigin,1);

	SetWindowPos(hChild,NULL,ptOrigin.x,ptOrigin.y,
		0,0,SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
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

void UpdateToolbarBandSizing(HWND hRebar,HWND hToolbar)
{
	REBARBANDINFO rbbi;
	SIZE sz;
	int nBands;
	int iBand = -1;
	int i = 0;

	nBands = (int)SendMessage(hRebar,RB_GETBANDCOUNT,0,0);

	for(i = 0;i < nBands;i++)
	{
		rbbi.cbSize	= sizeof(rbbi);
		rbbi.fMask	= RBBIM_CHILD;
		SendMessage(hRebar,RB_GETBANDINFO,i,(LPARAM)&rbbi);

		if(rbbi.hwndChild == hToolbar)
		{
			iBand = i;
			break;
		}
	}

	if(iBand != -1)
	{
		SendMessage(hToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

		rbbi.cbSize		= sizeof(rbbi);
		rbbi.fMask		= RBBIM_IDEALSIZE;
		rbbi.cxIdeal	= sz.cx;
		SendMessage(hRebar,RB_SETBANDINFO,iBand,(LPARAM)&rbbi);
	}
}

void MergeDateTime(SYSTEMTIME *pstOutput,const SYSTEMTIME *pstDate,const SYSTEMTIME *pstTime)
{
	/* Date fields. */
	pstOutput->wYear		= pstDate->wYear;
	pstOutput->wMonth		= pstDate->wMonth;
	pstOutput->wDayOfWeek	= pstDate->wDayOfWeek;
	pstOutput->wDay			= pstDate->wDay;

	/* Time fields. */
	pstOutput->wHour			= pstTime->wHour;
	pstOutput->wMinute			= pstTime->wMinute;
	pstOutput->wSecond			= pstTime->wSecond;
	pstOutput->wMilliseconds	= pstTime->wMilliseconds;
}

void GetWindowString(HWND hwnd,std::wstring &str)
{
	int iLen = GetWindowTextLength(hwnd);

	TCHAR *szTemp = new TCHAR[iLen + 1];
	GetWindowText(hwnd,szTemp,iLen + 1);

	str = szTemp;

	delete[] szTemp;
}

BOOL lCheckDlgButton(HWND hDlg,int ButtonId,BOOL bCheck)
{
	UINT uCheck;

	if(bCheck)
	{
		uCheck = BST_CHECKED;
	}
	else
	{
		uCheck = BST_UNCHECKED;
	}

	return CheckDlgButton(hDlg,ButtonId,uCheck);
}

void TrimStringLeft(std::wstring &str,const std::wstring &strWhitespace)
{
	size_t pos = str.find_first_not_of(strWhitespace);
	str.erase(0,pos);
}

void TrimStringRight(std::wstring &str,const std::wstring &strWhitespace)
{
	size_t pos = str.find_last_not_of(strWhitespace);
	str.erase(pos + 1);
}

void TrimString(std::wstring &str,const std::wstring &strWhitespace)
{
	TrimStringLeft(str,strWhitespace);
	TrimStringRight(str,strWhitespace);
}

void AddStyleToToolbar(UINT *fStyle,UINT fStyleToAdd)
{
	if((*fStyle & fStyleToAdd) != fStyleToAdd)
		*fStyle |= fStyleToAdd;
}