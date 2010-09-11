/******************************************************************
 *
 * Project: Helper
 * File: FileOperations.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides a set of file operation functions.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "FileOperations.h"
#include "Helper.h"
#include "iDataObject.h"
#include "Buffer.h"


using namespace std;

#define PASTE_CLIPBOARD_LINK		0
#define PASTE_CLIPBOARD_HARDLINK	1

int PasteFilesFromClipboardSpecial(TCHAR *szDestination,UINT fPasteType);

int RenameFile(TCHAR *NewFileName,TCHAR *OldFileName)
{
	SHFILEOPSTRUCT	shfo;

	shfo.hwnd	= NULL;
	shfo.wFunc	= FO_RENAME;
	shfo.pFrom	= OldFileName;
	shfo.pTo	= NewFileName;
	shfo.fFlags	= FOF_ALLOWUNDO;

	return !SHFileOperation(&shfo);
}

BOOL PerformFileOperation(HWND Parent,TCHAR *Path,TCHAR *FileName,
TCHAR *Operation,TCHAR *Parameters)
{
	SHELLEXECUTEINFO	sei;
	HANDLE				hProcess = NULL;
	BOOL				bReturnValue;

	ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));
	sei.cbSize			= sizeof(SHELLEXECUTEINFO);
	sei.fMask			= SEE_MASK_FLAG_NO_UI|SEE_MASK_INVOKEIDLIST|SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd			= Parent;
	sei.lpVerb			= Operation;
	sei.lpFile			= FileName;
	sei.lpParameters	= Parameters;
	sei.nShow			= SW_SHOW;
	sei.hProcess		= hProcess;

	bReturnValue = ShellExecuteEx(&sei);

	if(!bReturnValue)
	{
		TCHAR ExePath[MAX_PATH];

		/* File has failed to be opened/executed. If the file has no program
		associated with it, invoke the "Open With" system dialog box. */
		if((int)FindExecutable(FileName,NULL,ExePath) == SE_ERR_NOASSOC)
		{
			sei.lpVerb		= _T("openas");
			bReturnValue	= ShellExecuteEx(&sei);
		}
	}

	return bReturnValue;
}

BOOL ShowFileProperties(TCHAR *FileName)
{
	BOOL ReturnValue;

	ReturnValue = PerformFileOperation(NULL,NULL,FileName,_T("properties"),NULL);

	return ReturnValue;
}

int DeleteFiles(HWND hwnd,TCHAR *FileNames,BOOL Permanent)
{
	SHFILEOPSTRUCT	shfo;
	FILEOP_FLAGS	Flags = NULL;

	if(FileNames == NULL)
		return -1;

	if(!Permanent)
		Flags = FOF_ALLOWUNDO;

	shfo.hwnd					= hwnd;
	shfo.wFunc					= FO_DELETE;
	shfo.pFrom					= FileNames;
	shfo.pTo					= NULL;
	shfo.fFlags					= Flags;
	shfo.fAnyOperationsAborted	= NULL;
	shfo.hNameMappings			= NULL;
	shfo.lpszProgressTitle		= NULL;

	return SHFileOperation(&shfo);
}

int DeleteFilesToRecycleBin(HWND hwnd,TCHAR *FileNameList)
{
	return DeleteFiles(hwnd,FileNameList,FALSE);
}

int DeleteFilesPermanently(HWND hwnd,TCHAR *FileNameList)
{
	return DeleteFiles(hwnd,FileNameList,TRUE);
}

HRESULT CreateNewFolder(TCHAR *Directory,TCHAR *szNewFolderName,int cchMax)
{
	WIN32_FIND_DATA	wfd;
	HANDLE			hFirstFile;
	WCHAR			szLongPath[32768];
	TCHAR			FolderName[32768];
	BOOL			res;
	int				i = 2;

	if(Directory == NULL)
		return E_INVALIDARG;

	if(Directory[lstrlen(Directory) - 1] == '\\')
	{
		/* DON'T add a backslash to a path that already has
		one. Since it is assumed that ALL paths this function
		handles may be longer than MAX_PATH, don't use any
		of the Path* functions. */
		StringCchPrintf(FolderName,SIZEOF_ARRAY(FolderName),
			_T("%sNew Folder"),Directory);
	}
	else
	{
		StringCchPrintf(FolderName,SIZEOF_ARRAY(FolderName),
			_T("%s\\New Folder"),Directory);
	}

	StringCchPrintf(szLongPath,SIZEOF_ARRAY(szLongPath),
		L"\\\\?\\%s",FolderName);

	while((hFirstFile = FindFirstFile(szLongPath,&wfd))
	!= INVALID_HANDLE_VALUE)
	{
		FindClose(hFirstFile);

		if(Directory[lstrlen(Directory) - 1] == '\\')
		{
			/* DON'T add a backslash to a path that already has
			one. Since it is assumed that ALL paths this function
			handles may be longer than MAX_PATH, don't use any
			of the Path* functions. */
			StringCchPrintf(FolderName,SIZEOF_ARRAY(FolderName),
				_T("%sNew Folder (%d)"),Directory,i);
		}
		else
		{
			StringCchPrintf(FolderName,SIZEOF_ARRAY(FolderName),
				_T("%s\\New Folder (%d)"),Directory,i);
		}

		StringCchPrintf(szLongPath,SIZEOF_ARRAY(szLongPath),
			L"\\\\?\\%s",FolderName);

		i++;
	}

	res = CreateDirectory(szLongPath,NULL);

	if(!res)
	{
		/* Directory was not created. */
		return E_FAIL;
	}

	StringCchCopy(szNewFolderName,cchMax,FolderName);

	return S_OK;
}

HRESULT SaveDirectoryListing(TCHAR *Directory,TCHAR *FileName)
{
	WIN32_FIND_DATA		wfd;
	HANDLE				hFirstFile;
	TCHAR				SearchPath[MAX_PATH];
	TCHAR				FileHeader[] = _T("Directory Listing for:");
	TCHAR				FileSubHeader[] = _T("Listing Generated on:");
	TCHAR				Buffer[MAX_PATH];
	TCHAR				Temp[MAX_PATH];
	IBufferManager		*pBufferManager = NULL;
	SYSTEMTIME			SystemTime;
	FILETIME			FileTime;
	FILETIME			LocalFileTime;

	if((Directory == NULL) || (FileName == NULL))
		return E_INVALIDARG;

	StringCchCopy(SearchPath,SIZEOF_ARRAY(SearchPath),Directory);
	StringCchCat(SearchPath,SIZEOF_ARRAY(SearchPath),_T("\\*"));

	pBufferManager = new CBufferManager();

	StringCchPrintf(Buffer,SIZEOF_ARRAY(Buffer),_T("%s %s"),FileHeader,Directory);
	pBufferManager->WriteLine(Buffer);

	GetLocalTime(&SystemTime);
	SystemTimeToFileTime(&SystemTime,&FileTime);
	LocalFileTimeToFileTime(&FileTime,&LocalFileTime);

	CreateFileTimeString(&LocalFileTime,Temp,SIZEOF_ARRAY(Temp),FALSE);

	StringCchPrintf(Buffer,SIZEOF_ARRAY(Buffer),_T("%s %s"),FileSubHeader,Temp);
	pBufferManager->WriteLine(Buffer);

	/* Write a blank line. */
	pBufferManager->WriteLine(EMPTY_STRING);
	pBufferManager->WriteLine(_T("Folder Contents:"));

	hFirstFile = FindFirstFile(SearchPath,&wfd);

	if(hFirstFile == INVALID_HANDLE_VALUE)
	{
		pBufferManager->WriteLine(_T("Empty"));
	}
	else
	{
		if(StrCmp(wfd.cFileName,_T(".")) != 0)
		{
			StringCchCopy(Buffer,SIZEOF_ARRAY(Buffer),wfd.cFileName);
			if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
			FILE_ATTRIBUTE_DIRECTORY)
			{
				StringCchCat(Buffer,SIZEOF_ARRAY(Buffer),_T(" (Folder)"));
			}
			pBufferManager->WriteLine(Buffer);
		}
	}

	while(FindNextFile(hFirstFile,&wfd) != 0)
	{
		if(StrCmp(wfd.cFileName,_T("..")) != 0)
		{
			StringCchCopy(Buffer,SIZEOF_ARRAY(Buffer),wfd.cFileName);
			if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
			FILE_ATTRIBUTE_DIRECTORY)
			{
				StringCchCat(Buffer,SIZEOF_ARRAY(Buffer),_T(" (Folder)"));
			}
			pBufferManager->WriteLine(Buffer);
		}
	}

	pBufferManager->WriteToFile(FileName);

	pBufferManager->Release();

	FindClose(hFirstFile);

	return S_OK;
}

HRESULT CopyFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject)
{
	return CopyFilesToClipboard(FileNameList,FALSE,pClipboardDataObject);
}

HRESULT CutFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject)
{
	return CopyFilesToClipboard(FileNameList,TRUE,pClipboardDataObject);
}

HRESULT CopyFilesToClipboard(std::list<std::wstring> FileNameList,
BOOL bMove,IDataObject **pClipboardDataObject)
{
	FORMATETC ftc[2];
	STGMEDIUM stg[2];
	HRESULT hr;

	BuildHDropList(&ftc[0],&stg[0],FileNameList);

	ftc[1].cfFormat			= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
	ftc[1].ptd				= NULL;
	ftc[1].dwAspect			= DVASPECT_CONTENT;
	ftc[1].lindex			= -1;
	ftc[1].tymed			= TYMED_HGLOBAL;
	
	HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE,sizeof(DWORD));

	DWORD *pdwCopyEffect = static_cast<DWORD *>(GlobalLock(hglb));

	if(bMove)
		*pdwCopyEffect = DROPEFFECT_MOVE;
	else
		*pdwCopyEffect = DROPEFFECT_COPY;

	GlobalUnlock(hglb);

	stg[1].pUnkForRelease	= 0;

	stg[1].hGlobal			= hglb;
	stg[1].tymed			= TYMED_HGLOBAL;

	hr = CreateDataObject(ftc,stg,pClipboardDataObject,2);

	IAsyncOperation *pAsyncOperation = NULL;

	(*pClipboardDataObject)->QueryInterface(IID_IAsyncOperation,(void **)&pAsyncOperation);

	pAsyncOperation->SetAsyncMode(TRUE);
	pAsyncOperation->Release();

	if(SUCCEEDED(hr))
	{
		hr = OleSetClipboard(*pClipboardDataObject);
	}

	return hr;
}

struct HANDLETOMAPPINGS
{
	UINT			uNumberOfMappings;
	LPSHNAMEMAPPING	lpSHNameMapping;
};

typedef struct
{
	SHFILEOPSTRUCT		shfo;
	void				(*PasteFilesCallback)(void *,list<PastedFile_t> *);
	list<PastedFile_t>	*pPastedFileList;
	void				*pData;
} PastedFilesInfo_t;

int PasteLinksToClipboardFiles(TCHAR *szDestination)
{
	return PasteFilesFromClipboardSpecial(szDestination,PASTE_CLIPBOARD_LINK);
}

int PasteHardLinks(TCHAR *szDestination)
{
	return PasteFilesFromClipboardSpecial(szDestination,PASTE_CLIPBOARD_HARDLINK);
}

/* TODO: Use CDropHandler. */
int PasteFilesFromClipboardSpecial(TCHAR *szDestination,UINT fPasteType)
{
	IDataObject	*ClipboardObject = NULL;
	DROPFILES	*pdf = NULL;
	FORMATETC	ftc;
	STGMEDIUM	stg;
	HRESULT		hr;
	TCHAR		szFileName[MAX_PATH];
	TCHAR		szLinkFileName[MAX_PATH];
	TCHAR		szOldFileName[MAX_PATH];
	int			nFilesCopied = -1;
	int			i = 0;

	hr = OleGetClipboard(&ClipboardObject);

	if(SUCCEEDED(hr))
	{
		ftc.cfFormat	= CF_HDROP;
		ftc.ptd			= NULL;
		ftc.dwAspect	= DVASPECT_CONTENT;
		ftc.lindex		= -1;
		ftc.tymed		= TYMED_HGLOBAL;

		hr = ClipboardObject->GetData(&ftc,&stg);

		if(SUCCEEDED(hr))
		{
			pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

			if(pdf != NULL)
			{
				nFilesCopied = DragQueryFile((HDROP)pdf,
					0xFFFFFFFF,NULL,0);

				for(i = 0;i < nFilesCopied;i++)
				{
					DragQueryFile((HDROP)pdf,i,szOldFileName,SIZEOF_ARRAY(szOldFileName));

					StringCchCopy(szLinkFileName,SIZEOF_ARRAY(szLinkFileName),szDestination);

					StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szOldFileName);
					PathStripPath(szFileName);

					PathAppend(szLinkFileName,szFileName);

					switch(fPasteType)
					{
					case PASTE_CLIPBOARD_LINK:
						PathRenameExtension(szLinkFileName,_T(".lnk"));
						CreateLinkToFile(szOldFileName,szLinkFileName,EMPTY_STRING);
						break;

					case PASTE_CLIPBOARD_HARDLINK:
						CreateHardLink(szLinkFileName,szOldFileName,NULL);
						break;
					}
				}

				GlobalUnlock(stg.hGlobal);
			}

			ReleaseStgMedium(&stg);
		}
		ClipboardObject->Release();
	}

	return nFilesCopied;
}

HRESULT CreateLinkToFile(TCHAR *PathToFile,TCHAR *PathToLink,TCHAR *LinkDescription)
{
	IShellLink		*pShellLink = NULL;
	IPersistFile	*pPersistFile = NULL;
	WCHAR			PathToLinkW[MAX_PATH];
	HRESULT			hr;

	hr = CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,
	IID_IShellLink,(LPVOID*)&pShellLink);

	if(SUCCEEDED(hr))
	{
		pShellLink->SetPath(PathToFile);
		pShellLink->SetDescription(LinkDescription);

		hr = pShellLink->QueryInterface(IID_IPersistFile,(LPVOID*)&pPersistFile);

		if(SUCCEEDED(hr))
		{
			#ifndef UNICODE
			MultiByteToWideChar(CP_ACP,0,PathToLink,-1,PathToLinkW,MAX_PATH);
			#else
			StringCchCopy(PathToLinkW,SIZEOF_ARRAY(PathToLinkW),PathToLink);
			#endif

			pPersistFile->Save(PathToLinkW,TRUE);

			pPersistFile->Release();
		}

		pShellLink->Release();
	}

	return hr;
}

HRESULT ResolveLink(HWND hwnd,DWORD fFlags,TCHAR *LinkFile,TCHAR *LinkPath,int nBufferSize)
{
	IShellLink		*pShellLink = NULL;
	IPersistFile	*pPersistFile = NULL;
	SHFILEINFO		shfi;
	WCHAR			LinkFileW[MAX_PATH];
	TCHAR			ResolvedFilePath[MAX_PATH];
	HRESULT			hr;

	SHGetFileInfo(LinkFile,NULL,&shfi,sizeof(shfi),SHGFI_ATTRIBUTES);

	if(!(shfi.dwAttributes & SFGAO_LINK))
	{
		StringCchCopy(LinkPath,nBufferSize,LinkFile);
		return E_UNEXPECTED;
	}

	hr = CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,
	IID_IShellLink,(LPVOID*)&pShellLink);

	if(hr == S_OK)
	{
		hr = pShellLink->QueryInterface(IID_IPersistFile,(LPVOID *)&pPersistFile);

		if(hr == S_OK)
		{
			#ifndef UNICODE
			MultiByteToWideChar(CP_ACP,0,LinkFile,-1,LinkFileW,MAX_PATH);
			#else
			StringCchCopy(LinkFileW,SIZEOF_ARRAY(LinkFileW),LinkFile);
			#endif

			hr = pPersistFile->Load(LinkFileW,STGM_READ);

			if(hr == S_OK)
			{
				pShellLink->Resolve(hwnd,fFlags);
				pShellLink->GetPath(ResolvedFilePath,MAX_PATH,NULL,SLGP_UNCPRIORITY);

				StringCchCopy(LinkPath,nBufferSize,ResolvedFilePath);
			}

			pPersistFile->Release();
		}

		pShellLink->Release();
	}

	return hr;
}

BOOL CreateBrowseDialog(HWND hOwner,TCHAR *Title,TCHAR *PathBuffer,int BufferSize)
{
	LPITEMIDLIST	pidl = NULL;
	TCHAR			FullFolderPath[MAX_PATH];
	BOOL			bSuccessful = FALSE;

	bSuccessful = CreateBrowseDialog(hOwner,Title,&pidl);

	if(bSuccessful)
	{
		SHGetPathFromIDList(pidl,FullFolderPath);

		if(BufferSize > lstrlen(FullFolderPath))
		{
			StringCchCopy(PathBuffer,BufferSize,FullFolderPath);
			bSuccessful = TRUE;
		}

		CoTaskMemFree(pidl);
	}

	return bSuccessful;
}

BOOL CreateBrowseDialog(HWND hOwner,TCHAR *Title,LPITEMIDLIST *ppidl)
{
	BROWSEINFO		BrowseInfo;
	TCHAR			FolderName[MAX_PATH];
	BOOL			bSuccessful = FALSE;

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);

	BrowseInfo.hwndOwner		= hOwner;
	BrowseInfo.pidlRoot			= NULL;
	BrowseInfo.pszDisplayName	= FolderName;
	BrowseInfo.lpszTitle		= Title;
	BrowseInfo.ulFlags			= BIF_NEWDIALOGSTYLE;
	BrowseInfo.lpfn				= NULL;

	*ppidl = SHBrowseForFolder(&BrowseInfo);

	bSuccessful = (*ppidl != NULL);

	CoUninitialize();

	return bSuccessful;
}

int CopyFilesToFolder(HWND hOwner,TCHAR *FileNameList,BOOL bMove)
{
	TCHAR	FolderPath[MAX_PATH];
	TCHAR	Title[] = _T("Select a folder to copy the selected files to, then press OK");
	BOOL	Succeeded;
	int		iResult = -1;

	Succeeded = CreateBrowseDialog(hOwner,Title,FolderPath,SIZEOF_ARRAY(FolderPath));

	if(!Succeeded)
		return 0;

	if(FolderPath != NULL)
	{
		SHFILEOPSTRUCT FileOp;

		if(bMove)
			FileOp.wFunc	= FO_MOVE;
		else
			FileOp.wFunc	= FO_COPY;

		FileOp.hwnd		= hOwner;
		FileOp.pFrom	= FileNameList;
		FileOp.pTo		= FolderPath;
		FileOp.fFlags	= FOF_ALLOWUNDO;

		iResult = SHFileOperation(&FileOp);
	}

	return iResult;
}

void DeleteFileSecurely(TCHAR *szFileName,UINT uOverwriteMethod)
{
	HANDLE			hFile;
	WIN32_FIND_DATA	wfd;
	HANDLE			hFindFile;
	HCRYPTPROV		hProv;
	LARGE_INTEGER	lRealFileSize;
	BYTE			Pass1Data;
	BYTE			Pass2Data;
	BYTE			Pass3Data;
	DWORD			nBytesWritten;
	BOOL			bFolder;
	int				i = 0;

	hFindFile = FindFirstFile(szFileName,&wfd);

	if(hFindFile == INVALID_HANDLE_VALUE)
		return;

	bFolder = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;

	FindClose(hFindFile);

	if(bFolder)
	{
		/*SHFILEOPSTRUCT shFileOp;

		shFileOp.hwnd	= NULL;
		shFileOp.wFunc	= FO_DELETE;
		shFileOp.pFrom	= szFileName;
		shFileOp.pTo	= NULL;

		SHFileOperation(&shFileOp);*/

		return;
	}

	/* Determine the actual size of the file on disk
	(i.e. how many clusters it is allocated). */
	GetRealFileSize(szFileName,&lRealFileSize);

	/* Open the file, block any sharing mode, to stop the file
	been opened while it is overwritten. */
	hFile = CreateFile(szFileName,FILE_WRITE_DATA,0,NULL,OPEN_EXISTING,
	NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return;

	/* Extend the file out to the end of its last sector. */
	SetFilePointerEx(hFile,lRealFileSize,NULL,FILE_BEGIN);
	SetEndOfFile(hFile);

	/* Start at the beginning of the file, and
	write in the first-pass data, 0x00 over
	the length of the whole file. */
	SetFilePointer(hFile,0,NULL,FILE_BEGIN);
	Pass1Data = 0x00;

	for(i = 0;i < lRealFileSize.QuadPart;i++)
	{
		WriteFile(hFile,(LPVOID)&Pass1Data,1,&nBytesWritten,NULL);
	}

	if(uOverwriteMethod == OVERWRITE_THREEPASS)
	{
		/* Start at the beginning of the file, and
		write in the second-pass data, 0xFF over
		the length of the whole file. */
		SetFilePointer(hFile,0,NULL,FILE_BEGIN);
		Pass2Data = 0xFF;

		for(i = 0;i < lRealFileSize.QuadPart;i++)
		{
			WriteFile(hFile,(LPVOID)&Pass2Data,1,&nBytesWritten,NULL);
		}

		/* Now, write in a *random* set of data, again byte by
		byte. This random data is obtained via the os's
		cryptography functions, and as such should be
		exteremely difficult to duplicate. */
		SetFilePointer(hFile,0,NULL,FILE_BEGIN);

		CryptAcquireContext(&hProv,_T("SecureDelete"),NULL,PROV_RSA_AES,CRYPT_NEWKEYSET);

		for(i = 0;i < lRealFileSize.QuadPart;i++)
		{
			CryptGenRandom(hProv,1,(LPBYTE)&Pass3Data);
			WriteFile(hFile,(LPVOID)&Pass3Data,1,&nBytesWritten,NULL);
		}

		CryptAcquireContext(&hProv,_T("SecureDelete"),NULL,PROV_RSA_AES,CRYPT_DELETEKEYSET);
	}

	FlushFileBuffers(hFile);

	CloseHandle(hFile);

	DeleteFile(szFileName);
}