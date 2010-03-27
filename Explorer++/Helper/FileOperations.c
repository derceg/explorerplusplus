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
DWORD WINAPI PasteFilesThread(LPVOID lpParameter);

int RenameFile(TCHAR *NewFileName,TCHAR *OldFileName)
{
	SHFILEOPSTRUCT	shfo;

	shfo.hwnd	= NULL;
	shfo.wFunc	= FO_RENAME;
	shfo.pFrom	= OldFileName;
	shfo.pTo	= NewFileName;
	shfo.fFlags	= FOF_ALLOWUNDO|FOF_SILENT;

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
	TCHAR				FileSubHeader[] = _T("Lisitng Generated on:");
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

HRESULT CopyFiles(TCHAR *szFileNameList,int iListSize,IDataObject **pClipboardDataObject)
{
	return CopyFilesToClipboard(szFileNameList,iListSize,FALSE,pClipboardDataObject);
}

HRESULT CutFiles(TCHAR *szFileNameList,int iListSize,IDataObject **pClipboardDataObject)
{
	return CopyFilesToClipboard(szFileNameList,iListSize,TRUE,pClipboardDataObject);
}

HRESULT CopyFilesToClipboard(TCHAR *FileNameList,size_t iListSize,
BOOL bMove,IDataObject **pClipboardDataObject)
{
	DROPFILES	*df = NULL;
	FORMATETC	ftc[2];
	STGMEDIUM	stg[2];
	HGLOBAL		hglb = NULL;
	TCHAR		*ptr = NULL;
	DWORD		*pdwCopyEffect = NULL;
	size_t		iTempListSize = 0;
	HRESULT		hr;

	/* Manually calculate the list size if it is
	not supplied. */
	if(iListSize == -1)
	{
		ptr = FileNameList;

		while(*ptr != '\0')
		{
			iTempListSize += lstrlen(ptr) + 1;

			ptr += lstrlen(ptr) + 1;
		}

		iListSize = iTempListSize;

		iListSize++;
		iListSize *= sizeof(TCHAR);
	}

	ftc[0].cfFormat			= CF_HDROP;
	ftc[0].ptd				= NULL;
	ftc[0].dwAspect			= DVASPECT_CONTENT;
	ftc[0].lindex			= -1;
	ftc[0].tymed			= TYMED_HGLOBAL;

	ftc[1].cfFormat			= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
	ftc[1].ptd				= NULL;
	ftc[1].dwAspect			= DVASPECT_CONTENT;
	ftc[1].lindex			= -1;
	ftc[1].tymed			= TYMED_HGLOBAL;
	
	hglb = GlobalAlloc(GMEM_MOVEABLE,sizeof(DWORD));

	pdwCopyEffect = (DWORD *)GlobalLock(hglb);

	if(bMove)
		*pdwCopyEffect = DROPEFFECT_MOVE;
	else
		*pdwCopyEffect = DROPEFFECT_COPY;

	GlobalUnlock(hglb);

	stg[1].pUnkForRelease	= 0;

	stg[1].hGlobal			= hglb;
	stg[1].tymed			= TYMED_HGLOBAL;

	hglb = GlobalAlloc(GMEM_MOVEABLE,sizeof(DROPFILES) + iListSize);

	df = (DROPFILES *)GlobalLock(hglb);
	df->pFiles	= sizeof(DROPFILES);

	#ifdef UNICODE
	df->fWide = 1;
	#else
	df->fWide = 0;
	#endif

	LPBYTE pData = NULL;

	pData = (LPBYTE)df + sizeof(DROPFILES);

	memcpy(pData,FileNameList,iListSize);

	GlobalUnlock(hglb);

	stg[0].pUnkForRelease	= 0;
	stg[0].hGlobal			= hglb;
	stg[0].tymed			= TYMED_HGLOBAL;

	hr = CreateDataObject(ftc,stg,pClipboardDataObject,2);

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

void PasteFilesFromClipboard(HWND hwnd,TCHAR *Directory,BOOL bPasteLinks,
void (*PasteFilesCallback)(void *,list<PastedFile_t> *),
void *pData)
{
	IDataObject		*pClipboardObject = NULL;
	IBufferManager	*pBufferManager = NULL;
	PastedFilesInfo_t	*ppfi = NULL;
	PastedFile_t	PastedFile;
	list<PastedFile_t>	*pPastedFileList = NULL;
	FORMATETC		ftc;
	FORMATETC		ftcHDrop = {CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	FORMATETC		ftcFileDescriptor = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM		stg0;
	STGMEDIUM		stg1;
	HRESULT			hr;
	DROPFILES		*pdf = NULL;
	DWORD			*pdwEffect = NULL;
	TCHAR			*FileNameList = NULL;
	TCHAR			*szDestDirectory = NULL;
	TCHAR			OldFileName[MAX_PATH];
	TCHAR			szSource[MAX_PATH];
	DWORD			dwBufferSize;
	BOOL			bCopy;
	BOOL			bMove;
	int				NumberOfFilesDropped;
	int				i = 0;

	SetCursor(LoadCursor(NULL,IDC_WAIT));

	hr = OleGetClipboard(&pClipboardObject);

	if(hr != S_OK)
		return;

	if(pClipboardObject->QueryGetData(&ftcHDrop) == S_OK)
	{
		ftc.cfFormat	= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
		ftc.ptd			= NULL;
		ftc.dwAspect	= DVASPECT_CONTENT;
		ftc.lindex		= -1;
		ftc.tymed		= TYMED_HGLOBAL;

		hr = pClipboardObject->GetData(&ftc,&stg0);

		if(hr == S_OK)
		{
			pdwEffect = (DWORD *)GlobalLock(stg0.hGlobal);

			if(pdwEffect != NULL)
			{
				ftc.cfFormat	= CF_HDROP;
				pClipboardObject->GetData(&ftc,&stg1);

				pdf = (DROPFILES *)GlobalLock(stg1.hGlobal);

				if(pdf != NULL)
				{
					NumberOfFilesDropped = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,0);

					bCopy = (*pdwEffect & DROPEFFECT_COPY) == DROPEFFECT_COPY;
					bMove = (*pdwEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE;

					pBufferManager = new CBufferManager();

					pPastedFileList = new list<PastedFile_t>;

					for(i = 0;i < NumberOfFilesDropped;i++)
					{
						DragQueryFile((HDROP)pdf,i,OldFileName,SIZEOF_ARRAY(OldFileName));

						if(i == 0)
						{
							StringCchCopy(szSource,SIZEOF_ARRAY(szSource),OldFileName);
							PathRemoveFileSpec(szSource);
						}

						pBufferManager->WriteListEntry(OldFileName);

						StringCchCopy(PastedFile.szFileName,SIZEOF_ARRAY(PastedFile.szFileName),
							OldFileName);
						PathStripPath(PastedFile.szFileName);
						pPastedFileList->push_back(PastedFile);
					}

					pBufferManager->QueryBufferSize(&dwBufferSize);
					FileNameList = (TCHAR *)malloc(dwBufferSize * sizeof(TCHAR));
					szDestDirectory = (TCHAR *)malloc((MAX_PATH + 1) * sizeof(TCHAR));

					if(FileNameList != NULL && szDestDirectory != NULL)
					{
						pBufferManager->QueryBuffer(FileNameList,dwBufferSize);
						StringCchCopy(szDestDirectory,MAX_PATH,Directory);
						szDestDirectory[lstrlen(szDestDirectory) + 1] = '\0';

						ppfi = (PastedFilesInfo_t *)malloc(sizeof(PastedFilesInfo_t));

						if(ppfi != NULL)
						{
							if(bCopy)
								ppfi->shfo.wFunc	= FO_COPY;
							else if(bMove)
								ppfi->shfo.wFunc	= FO_MOVE;

							ppfi->shfo.hwnd		= hwnd;
							ppfi->shfo.pFrom	= FileNameList;
							ppfi->shfo.pTo		= szDestDirectory;
							ppfi->shfo.fFlags	= FOF_WANTMAPPINGHANDLE;

							if(lstrcmpi(szSource,Directory) == 0)
								ppfi->shfo.fFlags |= FOF_RENAMEONCOLLISION;

							ppfi->PasteFilesCallback	= PasteFilesCallback;
							ppfi->pData					= pData;
							ppfi->pPastedFileList		= pPastedFileList;

							/* The actual files will be pasted in a background thread. */
							HANDLE hThread;
							hThread = CreateThread(NULL,0,PasteFilesThread,ppfi,0,NULL);
							CloseHandle(hThread);
						}
					}

					pBufferManager->Release();

					GlobalUnlock(stg1.hGlobal);
				}
				GlobalUnlock(stg0.hGlobal);
			}
		}
	}
	else if(pClipboardObject->QueryGetData(&ftcFileDescriptor) == S_OK)
	{

	}

	SetCursor(LoadCursor(NULL,IDC_ARROW));

	pClipboardObject->Release();
}

DWORD WINAPI PasteFilesThread(LPVOID lpParameter)
{
	PastedFilesInfo_t	*ppfi = NULL;
	int					iReturn;

	ppfi = (PastedFilesInfo_t *)lpParameter;

	CoInitializeEx(0,COINIT_APARTMENTTHREADED);
	iReturn = SHFileOperation(&ppfi->shfo);

	if(iReturn == 0)
	{
		HANDLETOMAPPINGS *phtm = NULL;
		TCHAR szOldFileName[MAX_PATH];
		TCHAR szNewFileName[MAX_PATH];

		if(ppfi->shfo.hNameMappings != NULL)
		{
			phtm = (HANDLETOMAPPINGS *)ppfi->shfo.hNameMappings;

			list<PastedFile_t>::iterator itr;
			int j = 0;

			for(j = 0;j < (int)phtm->uNumberOfMappings;j++)
			{
				for(itr = ppfi->pPastedFileList->begin();itr != ppfi->pPastedFileList->end();itr++)
				{
					StringCchCopy(szOldFileName,SIZEOF_ARRAY(szOldFileName),
						phtm->lpSHNameMapping[j].pszOldPath);
					PathStripPath(szOldFileName);

					if(lstrcmp(szOldFileName,itr->szFileName) == 0)
					{
						StringCchCopy(szNewFileName,SIZEOF_ARRAY(szNewFileName),
							phtm->lpSHNameMapping[j].pszNewPath);
						PathStripPath(szNewFileName);

						StringCchCopy(itr->szFileName,SIZEOF_ARRAY(itr->szFileName),
							szNewFileName);
						break;
					}
				}
			}

			SHFreeNameMappings(ppfi->shfo.hNameMappings);
		}

		if(ppfi->PasteFilesCallback != NULL)
			ppfi->PasteFilesCallback(ppfi->pData,ppfi->pPastedFileList);
	}

	delete ppfi->pPastedFileList;
	free((void *)ppfi->shfo.pFrom);
	free((void *)ppfi->shfo.pTo);
	free((void *)ppfi);

	CoUninitialize();

	return 0;
}

int PasteLinksToClipboardFiles(TCHAR *szDestination)
{
	return PasteFilesFromClipboardSpecial(szDestination,PASTE_CLIPBOARD_LINK);
}

int PasteHardLinks(TCHAR *szDestination)
{
	return PasteFilesFromClipboardSpecial(szDestination,PASTE_CLIPBOARD_HARDLINK);
}

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

/* TODO: Fix this function. */
void GetDeletedFileDate(TCHAR *szFileName,FILETIME *pFileTime,TCHAR *szOriginalLocation)
{
	//HANDLE	hFile;
	//char	chCheck;
	//TCHAR	*ptr = NULL;
	//DWORD	dwRecordSize;
	//DWORD	dwRecordNumber = -1;
	//DWORD	nBytesRead;
	//DWORD	dwFileRecordNumber;
	//DWORD	dwPtr = 0;

	//ptr = szFileName;

	///* Skip past the first two bytes of the filename, so
	//that the files record number can be read. */
	//ptr += 0x02;

	//dwFileRecordNumber = _ctoi(ptr);

	////hFile = CreateFile(_T("C:\\RECYCLER\\S-1-5-21-1659004503-484061587-839522115-1003\\INFO2"),
	//	//FILE_READ_DATA,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	//hFile = CreateFile(_T("C:\\INFO2"),
	//	FILE_READ_DATA,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);

	///* Skip to byte 13. Byte 13 and 14 represent the
	//record size for each file. */
	//SetFilePointer(hFile,0x0C,0,FILE_BEGIN);

	//ReadFile(hFile,(LPVOID)&dwRecordSize,sizeof(dwRecordSize),&nBytesRead,NULL);

	///* Pointer at end of file header. */

	///* Skip ahead 4 bytes (there is no 4 byte
	//clearance before the ASCII filename, but
	//these bytes will need to be skipped for
	//the first filename). */
	//SetFilePointer(hFile,0x04,0,FILE_CURRENT);

	//while((dwFileRecordNumber != dwRecordNumber) &&
	//(dwPtr != INVALID_SET_FILE_POINTER))
	//{
	//	/* Pointer to the beginning of the ASCII filename. */

	//	/* If the first letter of the path is NULL, this record
	//	is invalid (i.e. the file has been deleted from the
	//	recycle bin). */
	//	ReadFile(hFile,(LPVOID)&chCheck,sizeof(char),&nBytesRead,NULL);

	//	if(chCheck == NULL)
	//	{
	//		dwPtr = SetFilePointer(hFile,0x104 - 1,0,FILE_CURRENT);
	//		SetFilePointer(hFile,0x14,0,FILE_CURRENT);
	//		SetFilePointer(hFile,0x104 * sizeof(WCHAR),0,FILE_CURRENT);
	//	}
	//	else
	//	{
	//		/* Skip past the ASCII filename (total size
	//		is equivalent to MAX_PATH. i.e. 260 bytes. */
	//		dwPtr = SetFilePointer(hFile,0x104 - 1,0,FILE_CURRENT);

	//		if(dwPtr != INVALID_SET_FILE_POINTER)
	//		{
	//			/* Record number. 4 byte value. */
	//			ReadFile(hFile,(LPVOID)&dwRecordNumber,sizeof(DWORD),&nBytesRead,NULL);

	//			if(dwFileRecordNumber != dwRecordNumber)
	//			{
	//				/* Skip past the next 16 bytes (various file data). */
	//				SetFilePointer(hFile,0x10,0,FILE_CURRENT);

	//				/* Skip past the UNICODE filename. */
	//				SetFilePointer(hFile,0x104 * sizeof(WCHAR),0,FILE_CURRENT);
	//			}
	//		}
	//	}
	//}

	//if(dwFileRecordNumber == dwRecordNumber)
	//{
	//	/* Move to the start of the date record. */
	//	SetFilePointer(hFile,0x04,0,FILE_CURRENT);

	//	/* Read out the files deletion time. */
	//	ReadFile(hFile,(LPVOID)pFileTime,sizeof(FILETIME),&nBytesRead,NULL);

	//	if(szOriginalLocation != NULL)
	//	{
	//		/* Move back to the end of the ASCII filename. */
	//		SetFilePointer(hFile,-0x10,0,FILE_CURRENT);

	//		/* Move back to the start of the ASCII filename. */
	//		SetFilePointer(hFile,-0x104,0,FILE_CURRENT);

	//		/* Read out the files original location. */
	//		ReadFile(hFile,(LPVOID)szOriginalLocation,MAX_PATH * sizeof(TCHAR),&nBytesRead,NULL);
	//	}

	//	//CreateFileTimeString(pFileTime,szTime,512,TRUE);
	//}

	//CloseHandle(hFile);
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