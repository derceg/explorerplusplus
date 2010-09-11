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
#include "Registry.h"


#define WM_APP_COPYOPERATIONFINISHED	(WM_APP + 1)
#define SUBCLASS_ID	10000

struct HANDLETOMAPPINGS
{
	UINT			uNumberOfMappings;
	LPSHNAMEMAPPING	lpSHNameMapping;
};

DWORD WINAPI CopyDroppedFilesInternalAsyncStub(LPVOID lpParameter);
HRESULT	CopyDroppedFilesInternalAsync(PastedFilesInfo_t *ppfi);
LRESULT CALLBACK DropWindowSubclass(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

/* TODO: */
void CreateDropOptionsMenu(LPCITEMIDLIST pidlDirectory,IDataObject *pDataObject,HWND hDrop);

/* Drop formats supported. */
FORMATETC	CDropHandler::m_ftcHDrop = {CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcFileDescriptor = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcShellIDList = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST),NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcText = {CF_TEXT,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcUnicodeText = {CF_UNICODETEXT,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcDIBV5 = {CF_DIBV5,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};

CDropHandler::CDropHandler()
{
	m_lRefCount = 1;
}

CDropHandler::~CDropHandler()
{

}

HRESULT __stdcall CDropHandler::QueryInterface(REFIID iid, void **ppvObject)
{
	if(ppvObject == NULL)
	{
		return E_POINTER;
	}

	*ppvObject = NULL;

	if(IsEqualIID(iid,IID_IUnknown))
	{
		*ppvObject = this;
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CDropHandler::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CDropHandler::Release(void)
{
	LONG lCount = InterlockedDecrement(&m_lRefCount);

	if(lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}

HRESULT CDropHandler::GetDropFormats(list<FORMATETC> *pftcList)
{
	if(pftcList == NULL)
	{
		return E_FAIL;
	}

	pftcList->push_back(m_ftcHDrop);
	pftcList->push_back(m_ftcFileDescriptor);
	pftcList->push_back(m_ftcShellIDList);
	pftcList->push_back(m_ftcText);
	pftcList->push_back(m_ftcUnicodeText);
	pftcList->push_back(m_ftcDIBV5);

	return S_OK;
}

void CDropHandler::Drop(IDataObject *pDataObject,DWORD grfKeyState,
POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,
TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,
BOOL bRenameOnCollision)
{
	m_pDataObject		= pDataObject;
	m_grfKeyState		= grfKeyState;
	m_ptl				= ptl;
	m_pdwEffect			= pdwEffect;
	m_hwndDrop			= hwndDrop;
	m_DragType			= DragType;
	m_szDestDirectory	= szDestDirectory;
	m_pDropFilesCallback	= pDropFilesCallback;
	m_bRenameOnCollision	= bRenameOnCollision;

	switch(m_DragType)
	{
	case DRAG_TYPE_LEFTCLICK:
		HandleLeftClickDrop(m_pDataObject,&m_ptl);
		break;

	case DRAG_TYPE_RIGHTCLICK:
		HandleRightClickDrop();
		break;
	}
}

void CDropHandler::CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,
TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,
BOOL bRenameOnCollision)
{
	m_pDataObject		= pDataObject;
	m_hwndDrop			= hwndDrop;
	m_szDestDirectory	= szDestDirectory;
	m_pDropFilesCallback	= pDropFilesCallback;
	m_bRenameOnCollision	= bRenameOnCollision;

	POINTL ptl = {0,0};

	HandleLeftClickDrop(m_pDataObject,&ptl);
}

void CDropHandler::HandleLeftClickDrop(IDataObject *pDataObject,POINTL *pptl)
{
	FORMATETC ftc;
	STGMEDIUM stg;
	DWORD *pdwEffect = NULL;
	DWORD dwEffect = DROPEFFECT_NONE;
	BOOL bPrefferedEffect = FALSE;
	POINT pt;

	pt.x = pptl->x;
	pt.y = pptl->y;

	SetFORMATETC(&ftc,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT),
		NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

	/* Check if the data has a preffered drop effect
	(i.e. copy or move). */
	HRESULT hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		pdwEffect = (DWORD *)GlobalLock(stg.hGlobal);

		if(pdwEffect != NULL)
		{
			if(*pdwEffect != DROPEFFECT_NONE)
			{
				dwEffect = *pdwEffect;
				bPrefferedEffect = TRUE;

				GlobalUnlock(stg.hGlobal);
			}
		}

		ReleaseStgMedium(&stg);
	}

	HRESULT hrCopy = E_FAIL;
	list<PastedFile_t> PastedFileList;

	if(pDataObject->QueryGetData(&m_ftcHDrop) == S_OK)
	{
		hrCopy = CopyHDropData(pDataObject,bPrefferedEffect,dwEffect,
			&PastedFileList);
	}
	else if(pDataObject->QueryGetData(&m_ftcShellIDList) == S_OK)
	{
		hrCopy = CopyShellIDListData(pDataObject,&PastedFileList);
	}
	else if(pDataObject->QueryGetData(&m_ftcFileDescriptor) == S_OK)
	{
		hrCopy = CopyFileDescriptorData(pDataObject,&PastedFileList);
	}
	else if(pDataObject->QueryGetData(&m_ftcUnicodeText) == S_OK)
	{
		hrCopy = CopyUnicodeTextData(pDataObject,&PastedFileList);
	}
	else if(pDataObject->QueryGetData(&m_ftcText) == S_OK)
	{
		hrCopy = CopyAnsiTextData(pDataObject,&PastedFileList);
	}
	else if(pDataObject->QueryGetData(&m_ftcDIBV5) == S_OK)
	{
		hrCopy = CopyDIBV5Data(pDataObject,&PastedFileList);
	}

	if(hrCopy == S_OK &&
		PastedFileList.size() > 0)
	{
		/* The data was copied successfully, so notify
		the caller via the specified callback interface. */
		if(m_pDropFilesCallback != NULL)
			m_pDropFilesCallback->OnDropFile(&PastedFileList,&pt);
	}
}

HRESULT CDropHandler::CopyHDropData(IDataObject *pDataObject,
	BOOL bPrefferedEffect,DWORD dwEffect,list<PastedFile_t> *pPastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcHDrop,&stg);

	if(hr == S_OK)
	{
		DROPFILES *pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

		if(pdf != NULL)
		{
			CopyDroppedFiles(pdf,bPrefferedEffect,dwEffect);

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyShellIDListData(IDataObject *pDataObject,
	list<PastedFile_t> *pPastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcShellIDList,&stg);

	if(hr == S_OK)
	{
		CIDA *pcida = (CIDA *)GlobalLock(stg.hGlobal);

		if(pcida != NULL)
		{
			IShellFolder *pShellFolder = NULL;
			HRESULT hr;

			LPCITEMIDLIST pidlDirectory = HIDA_GetPIDLFolder(pcida);

			hr = BindToShellFolder(pidlDirectory,&pShellFolder);

			if(SUCCEEDED(hr))
			{
				LPCITEMIDLIST pidlItem = NULL;
				IStorage *pStorage = NULL;

				for(unsigned int i = 0;i < pcida->cidl;i++)
				{
					pidlItem = HIDA_GetPIDLItem(pcida,i);

					hr = pShellFolder->BindToStorage(pidlItem,NULL,IID_IStorage,(LPVOID *)&pStorage);

					if(SUCCEEDED(hr))
					{
						/* TODO: Copy the files. */
						pStorage->Release();
					}
				}

				pShellFolder->Release();
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyFileDescriptorData(IDataObject *pDataObject,
	list<PastedFile_t> *pPastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcFileDescriptor,&stg);

	if(hr == S_OK)
	{
		FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(stg.hGlobal);

		if(pfgd != NULL)
		{
			FILETIME *pftCreationTime = NULL;
			FILETIME *pftLastAccessTime = NULL;
			FILETIME *pftLastWriteTime = NULL;
			DWORD dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			DWORD nBytesToWrite = 0;

			for(unsigned int i = 0;i < pfgd->cItems;i++)
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
					/* TODO: Complete. */
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

				FORMATETC ftcfchg;
				FORMATETC ftcfcis;
				FORMATETC ftcfcstg;
				STGMEDIUM stgFileContents = {0};
				BOOL bDataRetrieved = FALSE;
				LPBYTE pBuffer = NULL;

				SetFORMATETC(&ftcfchg,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
					NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);
				SetFORMATETC(&ftcfcis,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
					NULL,DVASPECT_CONTENT,-1,TYMED_ISTREAM);
				SetFORMATETC(&ftcfcstg,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
					NULL,DVASPECT_CONTENT,-1,TYMED_ISTORAGE);

				if(pDataObject->QueryGetData(&ftcfchg) == S_OK)
				{
					ftcfchg.lindex = i;

					hr = pDataObject->GetData(&ftcfchg,&stgFileContents);

					if(hr == S_OK)
					{
						pBuffer = (LPBYTE)malloc(GlobalSize(stgFileContents.hGlobal) * sizeof(BYTE));

						if(pBuffer != NULL)
						{
							if(!(pfgd->fgd[i].dwFlags & FD_FILESIZE))
								nBytesToWrite = (DWORD)GlobalSize(stgFileContents.hGlobal);

							LPBYTE pTemp = (LPBYTE)GlobalLock(stgFileContents.hGlobal);

							if(pTemp != NULL)
							{
								memcpy(pBuffer,pTemp,GlobalSize(stgFileContents.hGlobal));

								GlobalUnlock(stgFileContents.hGlobal);

								bDataRetrieved = TRUE;
							}

							ReleaseStgMedium(&stgFileContents);
						}
					}
				}
				else if(pDataObject->QueryGetData(&ftcfcis) == S_OK)
				{
					STATSTG sstg;
					ULONG cbRead;

					ftcfcis.lindex = i;

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

						ReleaseStgMedium(&stgFileContents);
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

						ReleaseStgMedium(&stgFileContents);
					}
				}

				if(bDataRetrieved)
				{
					TCHAR szFullFileName[MAX_PATH];

					StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_szDestDirectory);
					PathAppend(szFullFileName,pfgd->fgd[i].cFileName);

					HANDLE hFile = CreateFile(szFullFileName,GENERIC_WRITE,0,NULL,
						CREATE_ALWAYS,dwFileAttributes,NULL);

					if(hFile != INVALID_HANDLE_VALUE)
					{
						PastedFile_t pf;
						DWORD nBytesWritten;

						SetFileTime(hFile,pftCreationTime,pftLastAccessTime,pftLastWriteTime);

						WriteFile(hFile,pBuffer,nBytesToWrite,&nBytesWritten,NULL);

						CloseHandle(hFile);

						StringCchCopy(pf.szFileName,SIZEOF_ARRAY(pf.szFileName),szFullFileName);
						PathStripPath(pf.szFileName);

						pPastedFileList->push_back(pf);
					}

					HGLOBAL hglb = NULL;
					DWORD *pdwCopyEffect = NULL;
					FORMATETC ftc;
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

				if(pBuffer != NULL)
				{
					free(pBuffer);
				}
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyUnicodeTextData(IDataObject *pDataObject,
	list<PastedFile_t> *pPastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcUnicodeText,&stg);

	if(hr == S_OK)
	{
		WCHAR *pText = static_cast<WCHAR *>(GlobalLock(stg.hGlobal));

		if(pText != NULL)
		{
			TCHAR szFullFileName[MAX_PATH];

			hr = CopyTextToFile(m_szDestDirectory,pText,szFullFileName);

			if(hr == S_OK)
			{
				PastedFile_t pf;

				StringCchCopy(pf.szFileName,SIZEOF_ARRAY(pf.szFileName),szFullFileName);
				PathStripPath(pf.szFileName);

				pPastedFileList->push_back(pf);
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyAnsiTextData(IDataObject *pDataObject,
	list<PastedFile_t> *pPastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcText,&stg);

	if(hr == S_OK)
	{
		char *pText = static_cast<char *>(GlobalLock(stg.hGlobal));

		if(pText != NULL)
		{
			WCHAR *pszUnicodeText = new WCHAR[strlen(pText) + 1];

			MultiByteToWideChar(CP_ACP,0,pText,-1,pszUnicodeText,
				static_cast<int>(strlen(pText) + 1));

			TCHAR szFullFileName[MAX_PATH];

			hr = CopyTextToFile(m_szDestDirectory,pszUnicodeText,szFullFileName);

			if(hr == S_OK)
			{
				PastedFile_t pf;

				StringCchCopy(pf.szFileName,SIZEOF_ARRAY(pf.szFileName),szFullFileName);
				PathStripPath(pf.szFileName);

				pPastedFileList->push_back(pf);
			}

			delete[] pszUnicodeText;

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyDIBV5Data(IDataObject *pDataObject,
	list<PastedFile_t> *pPastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcDIBV5,&stg);

	if(hr == S_OK)
	{
		BITMAPINFO *pbmp = static_cast<BITMAPINFO *>(GlobalLock(stg.hGlobal));

		if(pbmp != NULL)
		{
			SYSTEMTIME st;
			FILETIME ft;
			FILETIME lft;
			TCHAR szTime[512];

			GetLocalTime(&st);
			SystemTimeToFileTime(&st,&ft);
			LocalFileTimeToFileTime(&ft,&lft);
			CreateFileTimeString(&lft,szTime,SIZEOF_ARRAY(szTime),FALSE);

			for(int i = 0;i < lstrlen(szTime);i++)
			{
				if(szTime[i] == '/')
				{
					szTime[i] = '-';
				}
				else if(szTime[i] == ':')
				{
					szTime[i] = '.';
				}
			}

			TCHAR szFullFileName[MAX_PATH];
			TCHAR szFileName[MAX_PATH];

			/* TODO: Move text into string table. */
			StringCchPrintf(szFileName,SIZEOF_ARRAY(szFileName),
				_T("Clipboard Image (%s).bmp"),szTime);

			PathCombine(szFullFileName,m_szDestDirectory,
				szFileName);

			HANDLE hFile = CreateFile(szFullFileName,
				GENERIC_WRITE,0,NULL,CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL,NULL);

			if(hFile != INVALID_HANDLE_VALUE)
			{
				DWORD dwSize = static_cast<DWORD>(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (GlobalSize(stg.hGlobal) - sizeof(BITMAPINFOHEADER)));

				LPBYTE pData = new BYTE[dwSize];

				BITMAPFILEHEADER *pbfh = (BITMAPFILEHEADER *)pData;

				/* 'BM'. */
				pbfh->bfType		= 0x4D42;

				pbfh->bfSize		= pbmp->bmiHeader.biSize;
				pbfh->bfReserved1	= 0;
				pbfh->bfReserved2	= 0;
				pbfh->bfOffBits		= sizeof(BITMAPFILEHEADER);

				BITMAPINFOHEADER *pb5h = (BITMAPINFOHEADER *)(pData + sizeof(BITMAPFILEHEADER));

				memcpy(pb5h,&pbmp->bmiHeader,sizeof(BITMAPINFOHEADER));

				RGBQUAD *prgb = (RGBQUAD *)(pData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

				memcpy(prgb,pbmp->bmiColors,GlobalSize(stg.hGlobal) - sizeof(BITMAPINFOHEADER));

				DWORD nBytesWritten;

				WriteFile(hFile,(LPCVOID)pData,
					dwSize,
					&nBytesWritten,NULL);

				CloseHandle(hFile);

				delete[] pData;

				PastedFile_t pf;

				StringCchCopy(pf.szFileName,SIZEOF_ARRAY(pf.szFileName),szFullFileName);
				PathStripPath(pf.szFileName);

				pPastedFileList->push_back(pf);
			}

			GlobalUnlock(stg.hGlobal);
		}

		/* Must release the storage medium. */
		ReleaseStgMedium(&stg);
	}

	return hr;
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
	TCHAR szSourceDirectory[MAX_PATH];
	DWORD dwEffect;
	BOOL bRenameOnCollision = m_bRenameOnCollision;
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

		StringCchCopy(szSourceDirectory,SIZEOF_ARRAY(szSourceDirectory),szFullFileName);
		PathRemoveFileSpec(szSourceDirectory);

		/* Force files to be renamed when they are copied and pasted
		in the same directory. */
		if(lstrcmpi(m_szDestDirectory,szSourceDirectory) == 0)
		{
			bRenameOnCollision = TRUE;
		}

		if(bPreferredEffect)
		{
			dwEffect = dwPreferredEffect;
		}
		else
		{
			BOOL bOnSameDrive;

			/* If no preferred drop effect is specified,
			decide whether to copy/move files based on their
			locations. */
			bOnSameDrive = CheckItemLocations(i);

			dwEffect = DetermineCurrentDragEffect(m_grfKeyState,
			*m_pdwEffect,TRUE,bOnSameDrive);
		}

		StringCchCopy(PastedFile.szFileName,SIZEOF_ARRAY(PastedFile.szFileName),
			szFullFileName);
		PathStripPath(PastedFile.szFileName);

		if(dwEffect & DROPEFFECT_MOVE)
		{
			pbmMove->WriteListEntry(szFullFileName);
			PastedFileListMove.push_back(PastedFile);
		}
		else if(dwEffect & DROPEFFECT_COPY)
		{
			pbmCopy->WriteListEntry(szFullFileName);
			PastedFileListCopy.push_back(PastedFile);
		}
		else if(dwEffect & DROPEFFECT_LINK)
		{
			CreateShortcutToDroppedFile(szFullFileName);
		}
	}

	CopyDroppedFilesInternal(pbmCopy,&PastedFileListCopy,TRUE,bRenameOnCollision);
	CopyDroppedFilesInternal(pbmMove,&PastedFileListMove,FALSE,bRenameOnCollision);

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

					ppfi->shfo.hwnd		= m_hwndDrop;
					ppfi->shfo.wFunc	= bCopy == TRUE ? FO_COPY : FO_MOVE;
					ppfi->shfo.pFrom	= szFileNameList;
					ppfi->shfo.pTo		= pszDestDirectory;
					ppfi->shfo.fFlags	= (bRenameOnCollision == TRUE ? FOF_RENAMEONCOLLISION : 0)|FOF_WANTMAPPINGHANDLE;

					ppfi->pDropFilesCallback	= m_pDropFilesCallback;
					ppfi->pPastedFileList		= new list<PastedFile_t>(*pPastedFileList);
					ppfi->dwEffect				= bCopy == TRUE ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
					ppfi->pt.x					= m_ptl.x;
					ppfi->pt.y					= m_ptl.y;
					ppfi->pFrom					= szFileNameList;
					ppfi->pTo					= pszDestDirectory;

					ppfi->pDropHandler	= this;
					ppfi->m_hDrop		= m_hwndDrop;

					IAsyncOperation *pao = NULL;
					BOOL bAsyncSupported = FALSE;
					HRESULT hr;

					/* Does the drop source support asynchronous copy? */
					hr = m_pDataObject->QueryInterface(IID_IAsyncOperation,(void **)&pao);

					if(hr == S_OK)
					{
						pao->GetAsyncMode(&bAsyncSupported);

						if(!bAsyncSupported)
						{
							pao->Release();
						}
					}

					if(bAsyncSupported)
					{
						pao->StartOperation(NULL);

						ppfi->pao	= pao;

						/* The copy operation is going to occur on a background thread,
						which means that we can't release this object until the background
						thread has completed. Use reference counting to ensure this
						condition is met. */
						AddRef();

						/* The drop source needs to be notified of the status of the copy
						once it has finished. This notification however, needs to occur on
						the thread that the object was created in. Therefore, we'll subclass
						the drop window, so that we can send it a private user message
						once the drop has finished. */
						SetWindowSubclass(m_hwndDrop,DropWindowSubclass,SUBCLASS_ID,NULL);

						/* Create a new thread, which we'll use to copy
						the dropped files. */
						hThread = CreateThread(NULL,0,CopyDroppedFilesInternalAsyncStub,ppfi,0,NULL);

						CloseHandle(hThread);
					}
					else
					{
						/* Copy the files within this thread. */
						CopyDroppedFilesInternalAsync(ppfi);

						free((void *)ppfi);
					}
				}
				else
				{
					free(szFileNameList);
				}
			}
		}
	}
}

LRESULT CALLBACK DropWindowSubclass(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	switch(uMsg)
	{
	case WM_APP_COPYOPERATIONFINISHED:
		{
			PastedFilesInfo_t *ppfi = reinterpret_cast<PastedFilesInfo_t *>(wParam);

			ppfi->pao->EndOperation(ppfi->hrCopy,NULL,ppfi->dwEffect);

			ppfi->pDropHandler->Release();

			free((void *)ppfi);

			RemoveWindowSubclass(hwnd,DropWindowSubclass,SUBCLASS_ID);
			return 0;
		}
		break;

	/* TODO: The window we're subclassing may be destroyed
	while the subclass is active. This should be handled in
	some way. */
	/*case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd,DropWindowSubclass,SUBCLASS_ID);
		break;*/
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

DWORD WINAPI CopyDroppedFilesInternalAsyncStub(LPVOID lpParameter)
{
	CoInitializeEx(0,COINIT_APARTMENTTHREADED);

	PastedFilesInfo_t *ppfi = (PastedFilesInfo_t *)lpParameter;

	ppfi->hrCopy = CopyDroppedFilesInternalAsync(ppfi);

	CoUninitialize();

	/* Signal back to the main thread. We can't call EndOperation()
	from here, as it needs to be called on the original thread. */
	PostMessage(ppfi->m_hDrop,WM_APP_COPYOPERATIONFINISHED,
		reinterpret_cast<WPARAM>(ppfi),NULL);

	return 0;
}

HRESULT CopyDroppedFilesInternalAsync(PastedFilesInfo_t *ppfi)
{
	HRESULT hr = E_FAIL;
	int iReturn;
	
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

		hr = S_OK;
	}

	delete ppfi->pPastedFileList;
	free((void *)ppfi->shfo.pFrom);
	free((void *)ppfi->shfo.pTo);

	return hr;
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

HRESULT CDropHandler::CopyTextToFile(IN TCHAR *pszDestDirectory,
	IN WCHAR *pszText,OUT TCHAR *pszFullFileNameOut)
{
	SYSTEMTIME st;
	FILETIME ft;
	FILETIME lft;
	HRESULT hr = E_FAIL;
	TCHAR szTime[512];

	GetLocalTime(&st);
	SystemTimeToFileTime(&st,&ft);
	LocalFileTimeToFileTime(&ft,&lft);
	CreateFileTimeString(&lft,szTime,SIZEOF_ARRAY(szTime),FALSE);

	for(int i = 0;i < lstrlen(szTime);i++)
	{
		if(szTime[i] == '/')
		{
			szTime[i] = '-';
		}
		else if(szTime[i] == ':')
		{
			szTime[i] = '.';
		}
	}

	TCHAR szFullFileName[MAX_PATH];
	TCHAR szFileName[MAX_PATH];

	/* TODO: Move text into string table. */
	StringCchPrintf(szFileName,SIZEOF_ARRAY(szFileName),
		_T("Clipboard Text (%s).txt"),szTime);

	PathCombine(szFullFileName,pszDestDirectory,
		szFileName);

	HANDLE hFile = CreateFile(szFullFileName,
		GENERIC_WRITE,0,NULL,CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD nBytesWritten;

		WriteFile(hFile,(LPCVOID)pszText,
			lstrlen(pszText) * sizeof(WCHAR),
			&nBytesWritten,NULL);

		CloseHandle(hFile);

		StringCchCopy(pszFullFileNameOut,MAX_PATH,
			szFullFileName);

		hr = S_OK;
	}

	return hr;
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

		ReleaseStgMedium(&stg);
	}

	return bOnSameDrive;
}

void CreateDropOptionsMenu(LPCITEMIDLIST pidlDirectory,IDataObject *pDataObject,HWND hDrop)
{
	IShellExtInit *p = NULL;
	HRESULT hr;

	/* Load any registered drag and drop handlers. They will
	be listed under:

	HKEY_CLASSES_ROOT\Directory\shellex\DragDropHandlers

	Pick out the CLSID_GUID from the (Default) key, and look
	it up in:

	HKEY_LOCAL_MACHINE\Software\Classes\CLSID
	
	Load the DLL it refers to, and use CoCreateInstance to
	create the class.
	*/

	HKEY hKey;
	LONG lRes;
	HMENU hMenu;
	
	hMenu = CreatePopupMenu();

	lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,_T("Directory\\shellex\\DragDropHandlers"),0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		HKEY hSubKey;
		TCHAR szKeyName[512];
		TCHAR szCLSID[256];
		DWORD dwLen;
		LONG lSubKeyRes;
		int iIndex = 0;

		/* TODO: RegCloseKey(). */
		
		dwLen = SIZEOF_ARRAY(szKeyName);

		/* Enumerate each of the sub-keys. */
		while((lRes = RegEnumKeyEx(hKey,iIndex,szKeyName,&dwLen,NULL,NULL,NULL,NULL)) == ERROR_SUCCESS)
		{
			TCHAR szSubKey[512];

			StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\%s"),_T("Directory\\shellex\\DragDropHandlers"),szKeyName);

			lSubKeyRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,szSubKey,0,KEY_READ,&hSubKey);

			if(lSubKeyRes == ERROR_SUCCESS)
			{
				lSubKeyRes = ReadStringFromRegistry(hSubKey,NULL,szCLSID,SIZEOF_ARRAY(szCLSID));

				if(lSubKeyRes == ERROR_SUCCESS)
				{
					HKEY hCLSIDKey;
					TCHAR szCLSIDKey[512];

					StringCchPrintf(szCLSIDKey,SIZEOF_ARRAY(szCLSIDKey),_T("%s\\%s"),_T("Software\\Classes\\CLSID"),szCLSID);

					/* Open the CLSID key. */
					lSubKeyRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szCLSIDKey,0,KEY_READ,&hCLSIDKey);

					if(lSubKeyRes == ERROR_SUCCESS)
					{
						HKEY hDllKey;

						/* Open InProcServer32. */
						lSubKeyRes = RegOpenKeyEx(hCLSIDKey,_T("InProcServer32"),0,KEY_READ,&hDllKey);

						if(lSubKeyRes == ERROR_SUCCESS)
						{
							TCHAR szDLL[MAX_PATH];

							lSubKeyRes = ReadStringFromRegistry(hDllKey,NULL,szDLL,SIZEOF_ARRAY(szDLL));

							if(lSubKeyRes == ERROR_SUCCESS)
							{
								HMODULE hDLL;

								/* Now, load the DLL it refers to. */
								hDLL = LoadLibrary(szDLL);

								if(hDLL != NULL)
								{
									CLSID clsid;

									hr = CLSIDFromString(szCLSID,&clsid);

									if(hr == NO_ERROR)
									{
										/* Finally, call CoCreateInstance. */
										hr = CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,IID_IUnknown,(LPVOID *)&p);

										if(hr == S_OK)
										{
											IShellExtInit *pShellExtInit = NULL;

											hr = p->QueryInterface(IID_IShellExtInit,(void **)&pShellExtInit);

											if(SUCCEEDED(hr))
											{
												hr = pShellExtInit->Initialize(pidlDirectory,pDataObject,NULL);
											}
										}
									}
								}
							}
						}
					}
				}
			}

			dwLen = SIZEOF_ARRAY(szKeyName);
			iIndex++;
		}
	}

	TrackPopupMenu(hMenu,TPM_LEFTALIGN,0,0,0,hDrop,NULL);
}