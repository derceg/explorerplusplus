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
#include <list>
#include "DropHandler.h"
#include "Helper.h"


struct HANDLETOMAPPINGS
{
	UINT			uNumberOfMappings;
	LPSHNAMEMAPPING	lpSHNameMapping;
};

typedef struct
{
	CDropHandler		*pDropHandler;

	SHFILEOPSTRUCT		shfo;
	IDropFilesCallback	*pDropFilesCallback;
	list<PastedFile_t>	*pPastedFileList;
	POINT				pt;
}PastedFilesInfo_t;

DWORD WINAPI CopyDroppedFilesInternalAsyncStub(LPVOID lpParameter);

CDropHandler::CDropHandler()
{
	
}

CDropHandler::~CDropHandler()
{

}

void CDropHandler::Drop(IDataObject *pDataObject,DWORD grfKeyState,
POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,
TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback)
{
	m_pDataObject		= pDataObject;
	m_grfKeyState		= grfKeyState;
	m_ptl				= ptl;
	m_pdwEffect			= pdwEffect;
	m_hwndDrop			= hwndDrop;
	m_DragType			= DragType;
	m_szDestDirectory	= szDestDirectory;
	m_pDropFilesCallback	= pDropFilesCallback;

	switch(m_DragType)
	{
	case DRAG_TYPE_LEFTCLICK:
		HandleLeftClickDrop(m_pDataObject,m_szDestDirectory,&m_ptl);
		break;

	case DRAG_TYPE_RIGHTCLICK:
		HandleRightClickDrop();
		break;
	}
}

void CDropHandler::CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback)
{
	m_pDataObject		= pDataObject;
	m_hwndDrop			= hwndDrop;
	m_szDestDirectory	= szDestDirectory;
	m_pDropFilesCallback	= pDropFilesCallback;

	POINTL ptl = {0,0};

	HandleLeftClickDrop(m_pDataObject,m_szDestDirectory,&ptl);
}

void CDropHandler::HandleLeftClickDrop(IDataObject *pDataObject,TCHAR *pszDestDirectory,POINTL *pptl)
{
	FORMATETC ftcHDrop = {CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	FORMATETC ftcFileDescriptor = {RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM stg;
	DROPFILES *pdf = NULL;
	DWORD *pdwEffect = NULL;
	DWORD dwEffect;
	BOOL bPrefferedEffect = FALSE;
	POINT pt;
	HRESULT hr;

	pt.x = pptl->x;
	pt.y = pptl->y;

	FORMATETC ftc;

	/* Check if the data has a preffered drop effect
	(i.e. copy or move). */
	ftc.cfFormat	= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
	ftc.ptd			= NULL;
	ftc.dwAspect	= DVASPECT_CONTENT;
	ftc.lindex		= -1;
	ftc.tymed		= TYMED_HGLOBAL;

	hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		pdwEffect = (DWORD *)GlobalLock(stg.hGlobal);

		if(pdwEffect != NULL)
		{
			dwEffect = *pdwEffect;
			bPrefferedEffect = TRUE;

			GlobalUnlock(stg.hGlobal);
		}
	}

	if(pDataObject->QueryGetData(&ftcHDrop) == S_OK)
	{
		hr = pDataObject->GetData(&ftcHDrop,&stg);

		if(hr == S_OK)
		{
			pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

			if(pdf != NULL)
			{
				CopyDroppedFiles(pdf,bPrefferedEffect,dwEffect);

				GlobalUnlock(stg.hGlobal);
			}
		}
	}
	else if(pDataObject->QueryGetData(&ftcFileDescriptor) == S_OK)
	{
		FORMATETC ftcfchg;
		FORMATETC ftcfcis;
		FORMATETC ftcfcstg;
		STGMEDIUM stgFileContents;
		HANDLE hFile;
		FILETIME *pftCreationTime = NULL;
		FILETIME *pftLastAccessTime = NULL;
		FILETIME *pftLastWriteTime = NULL;
		FILEGROUPDESCRIPTOR *pfgd = NULL;
		TCHAR szFullFileName[MAX_PATH];
		DWORD dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		DWORD nBytesToWrite = 0;
		DWORD nBytesWritten;
		LPBYTE pBuffer = NULL;
		BOOL bDataRetrieved = FALSE;
		unsigned int i = 0;

		hr = pDataObject->GetData(&ftcFileDescriptor,&stg);

		if(hr == S_OK)
		{
			pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(stg.hGlobal);

			if(pfgd != NULL)
			{
				for(i = 0;i < pfgd->cItems;i++)
				{
					if(pfgd->fgd[i].dwFlags & FD_ATTRIBUTES)
					{
						dwFileAttributes = pfgd->fgd[i].dwFileAttributes;
					}

					if(pfgd->fgd[i].dwFlags & FD_FILESIZE)
					{
						nBytesToWrite = pfgd->fgd[i].nFileSizeLow;
					}

					if(pfgd->fgd[i].dwFlags & FD_LINKUI)
					{
						/* TODO: Treat as shortcut... */
					}

					if(pfgd->fgd[i].dwFlags & FD_CREATETIME)
					{
						pftCreationTime = &pfgd->fgd[i].ftCreationTime;
					}

					if(pfgd->fgd[i].dwFlags & FD_ACCESSTIME)
					{
						pftLastAccessTime = &pfgd->fgd[i].ftLastAccessTime;
					}

					if(pfgd->fgd[i].dwFlags & FD_WRITESTIME)
					{
						pftLastWriteTime = &pfgd->fgd[i].ftLastWriteTime;
					}

					/*if(pfgd->fgd[i].dwFlags & FD_UNICODE)
					{
					}*/

					ftcfchg.cfFormat	= RegisterClipboardFormat(CFSTR_FILECONTENTS);
					ftcfchg.ptd			= NULL;
					ftcfchg.dwAspect	= DVASPECT_CONTENT;
					ftcfchg.lindex		= -1;
					ftcfchg.tymed		= TYMED_HGLOBAL;

					ftcfcis.cfFormat	= RegisterClipboardFormat(CFSTR_FILECONTENTS);
					ftcfcis.ptd			= NULL;
					ftcfcis.dwAspect	= DVASPECT_CONTENT;
					ftcfcis.lindex		= -1;
					ftcfcis.tymed		= TYMED_ISTREAM;

					ftcfcstg.cfFormat	= RegisterClipboardFormat(CFSTR_FILECONTENTS);
					ftcfcstg.ptd		= NULL;
					ftcfcstg.dwAspect	= DVASPECT_CONTENT;
					ftcfcstg.lindex		= -1;
					ftcfcstg.tymed		= TYMED_ISTORAGE;

					hr = pDataObject->QueryGetData(&ftcfchg);
					hr = pDataObject->QueryGetData(&ftcfcis);
					hr = pDataObject->QueryGetData(&ftcfcstg);

					if(pDataObject->QueryGetData(&ftcfchg) == S_OK)
					{
						ftcfchg.lindex = i - 1;

						hr = pDataObject->GetData(&ftcfchg,&stgFileContents);

						if(hr == S_OK)
						{
							if(!(pfgd->fgd[i].dwFlags & FD_FILESIZE))
								nBytesToWrite = GlobalSize(stgFileContents.hGlobal);

							pBuffer = (LPBYTE)GlobalLock(stgFileContents.hGlobal);

							if(pBuffer != NULL)
								bDataRetrieved = TRUE;
						}
					}
					else if(pDataObject->QueryGetData(&ftcfcis) == S_OK)
					{
						STATSTG sstg;
						ULONG cbRead;

						//ftcfcis.lindex = i;

						hr = pDataObject->GetData(&ftcfcis,&stgFileContents);

						if(hr == S_OK)
						{
							hr = stgFileContents.pstm->Stat(&sstg,STATFLAG_NONAME);

							if(hr == S_OK)
							{
								pBuffer = (LPBYTE)malloc(sstg.cbSize.LowPart * sizeof(BYTE));

								if(pBuffer != NULL)
								{
									/* If the file size isn't explicitly given,
									use the size of the stream. */
									if(!(pfgd->fgd[i].dwFlags & FD_FILESIZE))
										nBytesToWrite = sstg.cbSize.LowPart;

									stgFileContents.pstm->Read(pBuffer,sstg.cbSize.LowPart,&cbRead);

									bDataRetrieved = TRUE;
								}
							}
						}
					}
					else if(pDataObject->QueryGetData(&ftcfcstg) == S_OK)
					{
						IStream *pStream;
						STATSTG sstg;
						ULONG cbRead;

						ftcfcstg.lindex = i;

						hr = pDataObject->GetData(&ftcfcstg,&stgFileContents);

						if(hr == S_OK)
						{
							hr = stgFileContents.pstg->Stat(&sstg,STATFLAG_DEFAULT);

							if(hr == S_OK)
							{
								hr = stgFileContents.pstg->OpenStream(sstg.pwcsName,NULL,
									STGM_READ|STGM_SHARE_EXCLUSIVE,0,&pStream);

								if(hr == S_OK)
								{
									CoTaskMemFree(sstg.pwcsName);

									hr = pStream->Stat(&sstg,STATFLAG_NONAME);

									if(hr == S_OK)
									{
										pBuffer = (LPBYTE)malloc(sstg.cbSize.LowPart * sizeof(BYTE));

										if(pBuffer != NULL)
										{
											/* If the file size isn't explicitly given,
											use the size of the stream. */
											if(!(pfgd->fgd[i].dwFlags & FD_FILESIZE))
												nBytesToWrite = sstg.cbSize.LowPart;

											pStream->Read(pBuffer,sstg.cbSize.LowPart,&cbRead);

											bDataRetrieved = TRUE;
										}
									}
								}
							}
						}
					}

					if(bDataRetrieved)
					{
						StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),pszDestDirectory);
						PathAppend(szFullFileName,pfgd->fgd[i].cFileName);

						POINT pt;
						pt.x = pptl->x;
						pt.y = pptl->y;

						/* TODO: Fix. */
						/*if(m_pDropFilesCallback != NULL)
							m_pDropFilesCallback->OnDropFile(szFullFileName,&pt);*/

						hFile = CreateFile(szFullFileName,GENERIC_WRITE,0,NULL,
							CREATE_ALWAYS,dwFileAttributes,NULL);

						if(hFile != INVALID_HANDLE_VALUE)
						{
							SetFileTime(hFile,pftCreationTime,pftLastAccessTime,pftLastWriteTime);

							WriteFile(hFile,pBuffer,nBytesToWrite,&nBytesWritten,NULL);

							CloseHandle(hFile);
						}

						if(pDataObject->QueryGetData(&ftcfchg) == S_OK)
						{
							if(pBuffer != NULL)
							{
								GlobalUnlock(stgFileContents.hGlobal);
							}
						}
						else if(pDataObject->QueryGetData(&ftcfcis) == S_OK)
						{
							free(pBuffer);
						}

						HGLOBAL hglb = NULL;
						DWORD *pdwCopyEffect = NULL;
						STGMEDIUM stg1;

						ftc.cfFormat	= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
						ftc.ptd			= NULL;
						ftc.dwAspect	= DVASPECT_CONTENT;
						ftc.lindex		= -1;
						ftc.tymed		= TYMED_HGLOBAL;

						hglb = GlobalAlloc(GMEM_MOVEABLE,sizeof(DWORD));

						pdwCopyEffect = (DWORD *)GlobalLock(hglb);

						*pdwCopyEffect = DROPEFFECT_COPY;

						GlobalUnlock(hglb);

						stg1.tymed			= TYMED_HGLOBAL;
						stg1.pUnkForRelease	= 0;
						stg1.hGlobal		= hglb;

						pDataObject->SetData(&ftc,&stg1,FALSE);
					}
				}

				GlobalUnlock(stg.hGlobal);
			}
		}
	}
}

void CDropHandler::HandleRightClickDrop(void)
{
	IShellFolder *pDesktop = NULL;
	IShellFolder *pShellFolder = NULL;
	IDropTarget *pDrop = NULL;
	LPITEMIDLIST pidlDirectory = NULL;
	DWORD dwe;
	HRESULT hr;

	hr = GetIdlFromParsingName(m_szDestDirectory,&pidlDirectory);

	if(SUCCEEDED(hr))
	{
		hr = SHGetDesktopFolder(&pDesktop);

		if(SUCCEEDED(hr))
		{
			hr = pDesktop->BindToObject(pidlDirectory,0,IID_IShellFolder,(void **)&pShellFolder);

			if(SUCCEEDED(hr))
			{
				dwe = *m_pdwEffect;

				hr = pShellFolder->CreateViewObject(m_hwndDrop,IID_IDropTarget,(void **)&pDrop);

				if(SUCCEEDED(hr))
				{
					pDrop->DragEnter(m_pDataObject,MK_RBUTTON,m_ptl,&dwe);

					dwe = *m_pdwEffect;
					pDrop->Drop(m_pDataObject,m_grfKeyState,m_ptl,&dwe);

					pDrop->DragLeave();

					pDrop->Release();
				}

				pShellFolder->Release();
			}

			pDesktop->Release();
		}

		CoTaskMemFree(pidlDirectory);
	}
}

/* Loop through each of the dropped files. Add them to
the list of files that are to be copied. Then, copy the
files themselves in a separate background thread,
update the filename list if any files were renamed
(due to a collision), and then send the list back
to the callee.

Differences between drag and drop/paste:
 - Effect may already be specified on paste.
 - No drop point used when pasting files.
*/
void CDropHandler::CopyDroppedFiles(DROPFILES *pdf,BOOL bPreferredEffect,DWORD dwPreferredEffect)
{
	IBufferManager *pbmCopy = NULL;
	IBufferManager *pbmMove = NULL;
	list<PastedFile_t> PastedFileListCopy;
	list<PastedFile_t> PastedFileListMove;
	PastedFile_t PastedFile;
	TCHAR szFullFileName[MAX_PATH];
	DWORD dwEffect;
	int nDroppedFiles;
	int i = 0;

	pbmCopy = new CBufferManager();
	pbmMove = new CBufferManager();

	nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

	for(i = 0;i < nDroppedFiles;i++)
	{
		/* Determine the name of the dropped file. */
		DragQueryFile((HDROP)pdf,i,szFullFileName,
			SIZEOF_ARRAY(szFullFileName));

		if(bPreferredEffect)
		{
			dwEffect = dwPreferredEffect;
		}
		else
		{
			/*bOnSameDrive = CheckItemLocations(i);

			dwEffect = DetermineCurrentDragEffect(m_grfKeyState,
			*m_pdwEffect,TRUE,bOnSameDrive);*/
		}

		dwEffect = DROPEFFECT_COPY;

		StringCchCopy(PastedFile.szFileName,SIZEOF_ARRAY(PastedFile.szFileName),
			szFullFileName);
		PathStripPath(PastedFile.szFileName);

		if(dwEffect == DROPEFFECT_MOVE)
		{
			pbmMove->WriteListEntry(szFullFileName);
			PastedFileListMove.push_back(PastedFile);
		}
		else if(dwEffect == DROPEFFECT_COPY)
		{
			pbmCopy->WriteListEntry(szFullFileName);
			PastedFileListCopy.push_back(PastedFile);
		}
		else if(dwEffect == DROPEFFECT_LINK)
		{
			CreateShortcutToDroppedFile(szFullFileName);
		}
	}

	CopyDroppedFilesInternal(pbmCopy,&PastedFileListCopy,TRUE,FALSE);
	CopyDroppedFilesInternal(pbmMove,&PastedFileListMove,FALSE,FALSE);

	pbmCopy->Release();
	pbmMove->Release();
}

void CDropHandler::CopyDroppedFilesInternal(IBufferManager *pbm,
list<PastedFile_t> *pPastedFileList,BOOL bCopy,BOOL bRenameOnCollision)
{
	HANDLE hThread;
	TCHAR *szFileNameList = NULL;
	DWORD dwBufferSize;

	pbm->QueryBufferSize(&dwBufferSize);

	if(dwBufferSize > 1)
	{
		szFileNameList = (TCHAR *)malloc(dwBufferSize * sizeof(TCHAR));

		if(szFileNameList != NULL)
		{
			pbm->QueryBuffer(szFileNameList,dwBufferSize);

			PastedFilesInfo_t *ppfi = NULL;

			ppfi = (PastedFilesInfo_t *)malloc(sizeof(PastedFilesInfo_t));

			if(ppfi != NULL)
			{
				TCHAR *pszDestDirectory = NULL;

				pszDestDirectory = (TCHAR *)malloc((MAX_PATH + 1) * sizeof(TCHAR));

				if(pszDestDirectory != NULL)
				{
					StringCchCopy(pszDestDirectory,MAX_PATH,m_szDestDirectory);
					pszDestDirectory[lstrlen(pszDestDirectory) + 1] = '\0';

					ppfi->pDropHandler = this;

					ppfi->shfo.hwnd		= m_hwndDrop;
					ppfi->shfo.wFunc	= bCopy == TRUE ? FO_COPY : FO_MOVE;
					ppfi->shfo.pFrom	= szFileNameList;
					ppfi->shfo.pTo		= pszDestDirectory;
					ppfi->shfo.fFlags	= (bRenameOnCollision == TRUE ? FOF_RENAMEONCOLLISION : 0)|FOF_WANTMAPPINGHANDLE;

					ppfi->pDropFilesCallback	= m_pDropFilesCallback;
					ppfi->pPastedFileList		= new list<PastedFile_t>(*pPastedFileList);
					ppfi->pt.x					= m_ptl.x;
					ppfi->pt.y					= m_ptl.y;

					/* The actual files will be copied in a background thread. */
					hThread = CreateThread(NULL,0,CopyDroppedFilesInternalAsyncStub,ppfi,0,NULL);

					CloseHandle(hThread);
				}
				else
				{
					free(szFileNameList);
				}
			}
		}
	}
}

DWORD WINAPI CopyDroppedFilesInternalAsyncStub(LPVOID lpParameter)
{
	PastedFilesInfo_t *ppfi = NULL;

	ppfi = (PastedFilesInfo_t *)lpParameter;

	return ppfi->pDropHandler->CopyDroppedFilesInternalAsync(lpParameter);
}

DWORD WINAPI CDropHandler::CopyDroppedFilesInternalAsync(LPVOID lpParameter)
{
	PastedFilesInfo_t *ppfi = NULL;
	int iReturn;

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

		if(ppfi->pDropFilesCallback != NULL)
			ppfi->pDropFilesCallback->OnDropFile(ppfi->pPastedFileList,&ppfi->pt);
	}

	delete ppfi->pPastedFileList;
	free((void *)ppfi->shfo.pFrom);
	free((void *)ppfi->shfo.pTo);
	free((void *)ppfi);

	CoUninitialize();

	return 0;
}

void CDropHandler::CreateShortcutToDroppedFile(TCHAR *szFullFileName)
{
	TCHAR	szLink[MAX_PATH];
	TCHAR	szFileName[MAX_PATH];

	StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
	PathStripPath(szFileName);
	PathRenameExtension(szFileName,_T(".lnk"));
	StringCchCopy(szLink,SIZEOF_ARRAY(szLink),m_szDestDirectory);
	PathAppend(szLink,szFileName);

	CreateLinkToFile(szFullFileName,szLink,EMPTY_STRING);
}

BOOL CDropHandler::CheckItemLocations(int iDroppedItem)
{
	FORMATETC	ftc;
	STGMEDIUM	stg;
	DROPFILES	*pdf = NULL;
	TCHAR		szFullFileName[MAX_PATH];
	HRESULT		hr;
	BOOL		bOnSameDrive = FALSE;
	int			nDroppedFiles;

	ftc.cfFormat	= CF_HDROP;
	ftc.ptd			= NULL;
	ftc.dwAspect	= DVASPECT_CONTENT;
	ftc.lindex		= -1;
	ftc.tymed		= TYMED_HGLOBAL;

	hr = m_pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

		if(pdf != NULL)
		{
			/* Request a count of the number of files that have been dropped. */
			nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

			if(iDroppedItem < nDroppedFiles)
			{
				/* Determine the name of the first dropped file. */
				DragQueryFile((HDROP)pdf,iDroppedItem,szFullFileName,
					SIZEOF_ARRAY(szFullFileName));

				if(PathIsSameRoot(m_szDestDirectory,szFullFileName))
					bOnSameDrive = TRUE;
				else
					bOnSameDrive = FALSE;
			}

			GlobalUnlock(stg.hGlobal);
		}
	}

	return bOnSameDrive;
}