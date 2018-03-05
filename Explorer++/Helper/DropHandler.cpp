/******************************************************************
 *
 * Project: Helper
 * File: DropHandler.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Manages drag and drop functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "DropHandler.h"
#include "Helper.h"
#include "RegistrySettings.h"
#include "ShellHelper.h"
#include "ContextMenuManager.h"
#include "Macros.h"
#include "Logging.h"


#define WM_APP_COPYOPERATIONFINISHED	(WM_APP + 1)
#define SUBCLASS_ID	10000

struct HANDLETOMAPPINGS
{
	UINT			uNumberOfMappings;
	LPSHNAMEMAPPING	lpSHNameMapping;
};

struct PastedFilesInfo_t
{
	CReferenceCount			*pReferenceCount;

	HWND					hwnd;
	std::list<std::wstring>	FullFilenameList;
	std::wstring			strDestDirectory;
	BOOL					bCopy;
	BOOL					bRenameOnCollision;

	IAsyncOperation			*pao;

	IDropFilesCallback		*pDropFilesCallback;
	POINT					pt;
};

struct AsyncOperationInfo_t
{
	IAsyncOperation	*pao;
	HRESULT			hr;
	DWORD			dwEffect;
};

int CopyFileDescriptorAToW(FILEDESCRIPTORW *pfdw, const FILEDESCRIPTORA *pfda);
DWORD WINAPI CopyDroppedFilesInternalAsyncStub(LPVOID lpParameter);
BOOL CopyDroppedFilesInternalAsync(PastedFilesInfo_t *ppfi);
LRESULT CALLBACK DropWindowSubclass(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

/* TODO: */
void CreateDropOptionsMenu(HWND hDrop,LPCITEMIDLIST pidlDirectory,IDataObject *pDataObject);

/* Drop formats supported. */
FORMATETC	CDropHandler::m_ftcHDrop = {CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcFileDescriptorA = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA),NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcFileDescriptorW = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW),NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcShellIDList = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST),NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcText = {CF_TEXT,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcUnicodeText = {CF_UNICODETEXT,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	CDropHandler::m_ftcDIBV5 = {CF_DIBV5,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};

CDropHandler::CDropHandler()
{
	
}

CDropHandler::~CDropHandler()
{
	
}

CDropHandler *CDropHandler::CreateNew()
{
	return new CDropHandler();
}

HRESULT CDropHandler::GetDropFormats(std::list<FORMATETC> &ftcList)
{
	ftcList.push_back(m_ftcHDrop);
	ftcList.push_back(m_ftcFileDescriptorA);
	ftcList.push_back(m_ftcFileDescriptorW);
	ftcList.push_back(m_ftcShellIDList);
	ftcList.push_back(m_ftcText);
	ftcList.push_back(m_ftcUnicodeText);
	ftcList.push_back(m_ftcDIBV5);

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
	m_dwEffect			= *pdwEffect;
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
	m_dwEffect			= DROPEFFECT_COPY;
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

	/* Check if the data has a preferred drop effect
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
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	HRESULT hrCopy = E_FAIL;
	std::list<std::wstring> PastedFileList;

	if(CheckDropFormatSupported(pDataObject,&m_ftcHDrop))
	{
		/* CopyHDropData may copy the data
		in a background thread, so it
		notifies the caller itself (rather
		than returning a list of files
		in PastedFileList). */
		LOG(debug) << _T("Helper - Copying CF_HDROP data");
		hrCopy = CopyHDropData(pDataObject,bPrefferedEffect,dwEffect);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcShellIDList))
	{
		LOG(debug) << _T("Helper - Copying CFSTR_SHELLIDLIST data");
		hrCopy = CopyShellIDListData(pDataObject,PastedFileList);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcFileDescriptorA))
	{
		LOG(debug) << _T("Helper - Copying CFSTR_FILEDESCRIPTORA data");
		hrCopy = CopyAnsiFileDescriptorData(pDataObject,PastedFileList);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcFileDescriptorW))
	{
		LOG(debug) << _T("Helper - Copying CFSTR_FILEDESCRIPTORW data");
		hrCopy = CopyUnicodeFileDescriptorData(pDataObject,PastedFileList);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcUnicodeText))
	{
		LOG(debug) << _T("Helper - Copying CF_UNICODETEXT data");
		hrCopy = CopyUnicodeTextData(pDataObject,PastedFileList);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcText))
	{
		LOG(debug) << _T("Helper - Copying CF_TEXT data");
		hrCopy = CopyAnsiTextData(pDataObject,PastedFileList);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcDIBV5))
	{
		LOG(debug) << _T("Helper - Copying CF_DIBV5 data");
		hrCopy = CopyDIBV5Data(pDataObject,PastedFileList);
	}

	if(hrCopy == S_OK &&
		PastedFileList.size() > 0)
	{
		/* The data was copied successfully, so notify
		the caller via the specified callback interface. */
		if(m_pDropFilesCallback != NULL)
		{
			m_pDropFilesCallback->OnDropFile(PastedFileList,&pt);
		}
	}
}

/* Some applications (e.g. Thunderbird) may indicate via
QueryGetData() that they support a particular drop format,
even though a corresponding call to GetData() fails.
Therefore, we'll actually attempt to query the data using
GetData(). */
BOOL CDropHandler::CheckDropFormatSupported(IDataObject *pDataObject,FORMATETC *pftc)
{
	HRESULT hr;

	/* When using QueryGetData(), the line index
	must be -1. This may not be the case when calling
	GetData(). */
	LONG lindex = pftc->lindex;
	pftc->lindex = -1;
	
	hr = pDataObject->QueryGetData(pftc);

	pftc->lindex = lindex;

	if(hr != S_OK)
	{
		return FALSE;
	}

	STGMEDIUM stg;
	hr = pDataObject->GetData(pftc,&stg);

	if(hr != S_OK)
	{
		return FALSE;
	}

	ReleaseStgMedium(&stg);

	return TRUE;
}

HRESULT CDropHandler::CopyHDropData(IDataObject *pDataObject,
	BOOL bPrefferedEffect,DWORD dwEffect)
{
	STGMEDIUM stg;
	HRESULT hr = pDataObject->GetData(&m_ftcHDrop,&stg);

	if(hr == S_OK)
	{
		HDROP hd = reinterpret_cast<HDROP>(GlobalLock(stg.hGlobal));

		if(hd != NULL)
		{
			CopyDroppedFiles(hd,bPrefferedEffect,dwEffect);
			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyShellIDListData(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
{
	/* Once this function actually
	copies the specified files, this
	can be removed. */
	UNREFERENCED_PARAMETER(PastedFileList);

	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcShellIDList,&stg);

	if(hr == S_OK)
	{
		CIDA *pcida = (CIDA *)GlobalLock(stg.hGlobal);

		if(pcida != NULL)
		{
			IShellFolder *pShellFolder = NULL;

			LPCITEMIDLIST pidlDirectory = HIDA_GetPIDLFolder(pcida);

			hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&pShellFolder));

			if(SUCCEEDED(hr))
			{
				LPCITEMIDLIST pidlItem = NULL;
				IStorage *pStorage = NULL;

				for(unsigned int i = 0;i < pcida->cidl;i++)
				{
					pidlItem = HIDA_GetPIDLItem(pcida,i);

					hr = pShellFolder->BindToStorage(pidlItem, NULL, IID_PPV_ARGS(&pStorage));

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

HRESULT CDropHandler::CopyAnsiFileDescriptorData(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcFileDescriptorA,&stg);

	if(hr == S_OK)
	{
		FILEGROUPDESCRIPTORA *pfgda = (FILEGROUPDESCRIPTORA *)GlobalLock(stg.hGlobal);

		if(pfgda != NULL)
		{
			FILEGROUPDESCRIPTORW *pfgdw = (FILEGROUPDESCRIPTORW *)malloc(sizeof(FILEGROUPDESCRIPTORW) + ((pfgda->cItems - 1) * sizeof(FILEDESCRIPTORW)));

			if(pfgdw != NULL)
			{
				int nSuccessfullyCopied = 0;

				for(UINT i = 0; i < pfgda->cItems; i++)
				{
					int iRet = CopyFileDescriptorAToW(&pfgdw->fgd[nSuccessfullyCopied], &pfgda->fgd[i]);

					if(iRet != 0)
					{
						nSuccessfullyCopied++;
					}
				}

				pfgdw->cItems = nSuccessfullyCopied;

				CopyFileDescriptorData(pDataObject, pfgdw, PastedFileList);

				free(pfgdw);
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

int CopyFileDescriptorAToW(FILEDESCRIPTORW *pfdw, const FILEDESCRIPTORA *pfda)
{
	pfdw->dwFlags = pfda->dwFlags;
	pfdw->clsid = pfda->clsid;
	pfdw->sizel = pfda->sizel;
	pfdw->pointl = pfda->pointl;
	pfdw->dwFileAttributes = pfda->dwFileAttributes;
	pfdw->ftCreationTime = pfda->ftCreationTime;
	pfdw->ftLastAccessTime = pfda->ftLastAccessTime;
	pfdw->ftLastWriteTime = pfda->ftLastWriteTime;
	pfdw->nFileSizeHigh = pfda->nFileSizeHigh;
	pfdw->nFileSizeLow = pfda->nFileSizeLow;
	return MultiByteToWideChar(CP_ACP, 0, pfda->cFileName, -1, pfdw->cFileName, SIZEOF_ARRAY(pfdw->cFileName));
}

HRESULT CDropHandler::CopyUnicodeFileDescriptorData(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcFileDescriptorW,&stg);

	if(hr == S_OK)
	{
		FILEGROUPDESCRIPTORW *pfgd = (FILEGROUPDESCRIPTORW *)GlobalLock(stg.hGlobal);

		if(pfgd != NULL)
		{
			CopyFileDescriptorData(pDataObject,pfgd,PastedFileList);
			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyFileDescriptorData(IDataObject *pDataObject,
	FILEGROUPDESCRIPTORW *pfgd,std::list<std::wstring> &PastedFileList)
{
	FILETIME *pftCreationTime = NULL;
	FILETIME *pftLastAccessTime = NULL;
	FILETIME *pftLastWriteTime = NULL;
	DWORD dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	DWORD nBytesToWrite = 0;

	HRESULT hr = E_FAIL;

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
		BOOL bDataCopied = FALSE;
		BOOL bDataRetrieved = FALSE;
		LPBYTE pBuffer = NULL;

		SetFORMATETC(&ftcfchg,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
			NULL,DVASPECT_CONTENT,i,TYMED_HGLOBAL);
		SetFORMATETC(&ftcfcis,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
			NULL,DVASPECT_CONTENT,i,TYMED_ISTREAM);
		SetFORMATETC(&ftcfcstg,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
			NULL,DVASPECT_CONTENT,i,TYMED_ISTORAGE);

		BOOL bDataExtracted = FALSE;

		if(CheckDropFormatSupported(pDataObject,&ftcfchg))
		{
			hr = pDataObject->GetData(&ftcfchg,&stgFileContents);

			if(hr == S_OK)
			{
				bDataExtracted = TRUE;
			}
		}
		else if(CheckDropFormatSupported(pDataObject,&ftcfcis))
		{
			hr = pDataObject->GetData(&ftcfcis,&stgFileContents);

			if(hr == S_OK)
			{
				bDataExtracted = TRUE;
			}
		}
		else if(CheckDropFormatSupported(pDataObject,&ftcfcstg))
		{
			hr = pDataObject->GetData(&ftcfcstg,&stgFileContents);

			if(hr == S_OK)
			{
				bDataExtracted = TRUE;
			}
		}

		if(bDataExtracted)
		{
			/* Some applications (e.g. Thunderbird) may return data
			in a different format than the one requested. So, take
			action based on what they return, rather than what
			was requested. */
			switch(stgFileContents.tymed)
			{
			case TYMED_HGLOBAL:
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
					}
				}
				break;

			case TYMED_ISTREAM:
				{
					STATSTG sstg;
					ULONG cbRead;

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
				break;

			case TYMED_ISTORAGE:
				{
					TCHAR szFullFileName[MAX_PATH];
					StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_szDestDirectory);
					PathAppend(szFullFileName,pfgd->fgd[i].cFileName);

					IStorage *pStorage = NULL;
					hr = StgCreateStorageEx(szFullFileName,STGM_READWRITE|STGM_TRANSACTED|STGM_CREATE,STGFMT_STORAGE,
						0,NULL,NULL,IID_PPV_ARGS(&pStorage));

					if(hr == S_OK)
					{
						hr = stgFileContents.pstg->CopyTo(0,NULL,NULL,pStorage);

						if(hr == S_OK)
						{
							hr = pStorage->Commit(STGC_DEFAULT);

							if(hr == S_OK)
							{
								PastedFileList.push_back(pfgd->fgd[i].cFileName);

								bDataCopied = TRUE;
							}
						}

						pStorage->Release();
					}
				}
				break;
			}

			ReleaseStgMedium(&stgFileContents);
		}

		if(bDataRetrieved &&
			!bDataCopied)
		{
			TCHAR szFullFileName[MAX_PATH];

			StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_szDestDirectory);
			PathAppend(szFullFileName,pfgd->fgd[i].cFileName);

			HANDLE hFile = CreateFile(szFullFileName,GENERIC_WRITE,0,NULL,
				CREATE_ALWAYS,dwFileAttributes,NULL);

			if(hFile != INVALID_HANDLE_VALUE)
			{
				DWORD nBytesWritten;

				SetFileTime(hFile,pftCreationTime,pftLastAccessTime,pftLastWriteTime);

				WriteFile(hFile,pBuffer,nBytesToWrite,&nBytesWritten,NULL);

				CloseHandle(hFile);

				TCHAR szFileName[MAX_PATH];
				StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
				PathStripPath(szFileName);

				PastedFileList.push_back(szFileName);
			}

			FORMATETC ftc;
			ftc.cfFormat	= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
			ftc.ptd			= NULL;
			ftc.dwAspect	= DVASPECT_CONTENT;
			ftc.lindex		= -1;
			ftc.tymed		= TYMED_HGLOBAL;

			HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE,sizeof(DWORD));

			if(hGlobal != NULL)
			{
				DWORD *pdwCopyEffect = (DWORD *) GlobalLock(hGlobal);

				if(pdwCopyEffect != NULL)
				{
					*pdwCopyEffect = DROPEFFECT_COPY;
					GlobalUnlock(hGlobal);

					STGMEDIUM stg;
					stg.tymed = TYMED_HGLOBAL;
					stg.pUnkForRelease = NULL;
					stg.hGlobal = hGlobal;

					pDataObject->SetData(&ftc, &stg, FALSE);
				}

				GlobalFree(hGlobal);
			}
		}

		if(pBuffer != NULL)
		{
			free(pBuffer);
		}
	}

	return hr;
}

HRESULT CDropHandler::CopyUnicodeTextData(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
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

			hr = CopyTextToFile(m_szDestDirectory, pText, szFullFileName, SIZEOF_ARRAY(szFullFileName));

			if(hr == S_OK)
			{
				TCHAR szFileName[MAX_PATH];
				StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
				PathStripPath(szFileName);

				PastedFileList.push_back(szFileName);
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyAnsiTextData(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
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

			int iRet = MultiByteToWideChar(CP_ACP,0,pText,-1,pszUnicodeText,
				static_cast<int>(strlen(pText) + 1));

			if(iRet != 0)
			{
				TCHAR szFullFileName[MAX_PATH];

				hr = CopyTextToFile(m_szDestDirectory, pszUnicodeText,
					szFullFileName, SIZEOF_ARRAY(szFullFileName));

				if(hr == S_OK)
				{
					TCHAR szFileName[MAX_PATH];
					StringCchCopy(szFileName, SIZEOF_ARRAY(szFileName), szFullFileName);
					PathStripPath(szFileName);

					PastedFileList.push_back(szFileName);
				}
			}

			delete[] pszUnicodeText;

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT CDropHandler::CopyDIBV5Data(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
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

				PastedFileList.push_back(szFileName);
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
	IShellFolder *pShellFolder = NULL;
	IDropTarget *pDrop = NULL;
	LPITEMIDLIST pidlDirectory = NULL;
	DWORD dwe;
	HRESULT hr;

	hr = GetIdlFromParsingName(m_szDestDirectory,&pidlDirectory);

	if(SUCCEEDED(hr))
	{
		hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&pShellFolder));

		if(SUCCEEDED(hr))
		{
			dwe = m_dwEffect;

			hr = pShellFolder->CreateViewObject(m_hwndDrop, IID_PPV_ARGS(&pDrop));

			if(SUCCEEDED(hr))
			{
				pDrop->DragEnter(m_pDataObject,MK_RBUTTON,m_ptl,&dwe);

				dwe = m_dwEffect;
				pDrop->Drop(m_pDataObject,m_grfKeyState,m_ptl,&dwe);

				pDrop->DragLeave();

				pDrop->Release();
			}

			pShellFolder->Release();
		}

		CoTaskMemFree(pidlDirectory);
	}
}

/* Loop through each of the dropped files. Add them to
the list of files that are to be copied. Then, copy the
files themselves in a separate background thread,
update the filename list if any files were renamed
(due to a collision), and then send the list back
to the caller.

Differences between drag and drop/paste:
 - Effect may already be specified on paste.
 - No drop point used when pasting files.
*/
void CDropHandler::CopyDroppedFiles(const HDROP &hd,BOOL bPreferredEffect,DWORD dwPreferredEffect)
{
	std::list<std::wstring> CopyFilenameList;
	std::list<std::wstring> MoveFilenameList;

	BOOL bRenameOnCollision = m_bRenameOnCollision;

	int nDroppedFiles = DragQueryFile(hd,0xFFFFFFFF,NULL,NULL);

	for(int i = 0;i < nDroppedFiles;i++)
	{
		TCHAR szFullFileName[MAX_PATH];
		DragQueryFile(hd,i,szFullFileName,SIZEOF_ARRAY(szFullFileName));

		TCHAR szSourceDirectory[MAX_PATH];
		StringCchCopy(szSourceDirectory,SIZEOF_ARRAY(szSourceDirectory),szFullFileName);
		PathRemoveFileSpec(szSourceDirectory);

		/* Force files to be renamed when they are copied and pasted
		in the same directory. */
		if(lstrcmpi(m_szDestDirectory,szSourceDirectory) == 0)
		{
			bRenameOnCollision = TRUE;
		}

		DWORD dwEffect;

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

			dwEffect = DetermineDragEffect(m_grfKeyState,
			m_dwEffect,TRUE,bOnSameDrive);
		}

		TCHAR szFileName[MAX_PATH];
		StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
		PathStripPath(szFileName);

		if(dwEffect & DROPEFFECT_MOVE)
		{
			MoveFilenameList.push_back(szFullFileName);
		}
		else if(dwEffect & DROPEFFECT_COPY)
		{
			CopyFilenameList.push_back(szFullFileName);
		}
		else if(dwEffect & DROPEFFECT_LINK)
		{
			CreateShortcutToDroppedFile(szFullFileName);
		}
	}

	CopyDroppedFilesInternal(CopyFilenameList,TRUE,bRenameOnCollision);
	CopyDroppedFilesInternal(MoveFilenameList,FALSE,bRenameOnCollision);
}

void CDropHandler::CopyDroppedFilesInternal(const std::list<std::wstring> &FullFilenameList,
	BOOL bCopy,BOOL bRenameOnCollision)
{
	if(FullFilenameList.size() == 0)
	{
		return;
	}

	PastedFilesInfo_t *ppfi = new PastedFilesInfo_t;
	ppfi->pReferenceCount		= this;
	ppfi->hwnd					= m_hwndDrop;
	ppfi->FullFilenameList		= FullFilenameList;
	ppfi->strDestDirectory		= m_szDestDirectory;
	ppfi->bCopy					= bCopy;
	ppfi->bRenameOnCollision	= bRenameOnCollision;
	ppfi->pao					= NULL;
	ppfi->pDropFilesCallback	= m_pDropFilesCallback;
	ppfi->pt.x					= m_ptl.x;
	ppfi->pt.y					= m_ptl.y;

	IAsyncOperation *pao = NULL;
	BOOL bAsyncSupported = FALSE;

	/* Does the drop source support asynchronous copy? */
	HRESULT hr = m_pDataObject->QueryInterface(IID_PPV_ARGS(&pao));

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

		ppfi->pao = pao;

		/* The copy operation is going to occur on a background thread,
		which means that we can't release this object until the background
		thread has completed. Use reference counting to ensure this
		condition is met. */
		AddRef();

		/* The drop source needs to be notified of the status of the copy
		once it has finished. This notification however, needs to occur on
		the thread that the object was created in. */
		SetWindowSubclass(m_hwndDrop,DropWindowSubclass,SUBCLASS_ID,NULL);

		HANDLE hThread = CreateThread(NULL,0,CopyDroppedFilesInternalAsyncStub,
			reinterpret_cast<LPVOID>(ppfi),0,NULL);

		if(hThread != NULL)
		{
			CloseHandle(hThread);
		}
	}
	else
	{
		/* Copy the files within this thread. */
		CopyDroppedFilesInternalAsync(ppfi);

		if(ppfi->pDropFilesCallback != NULL)
		{
			ppfi->pDropFilesCallback->Release();
		}
		
		delete ppfi;
	}
}

LRESULT CALLBACK DropWindowSubclass(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);
	UNREFERENCED_PARAMETER(dwRefData);

	switch(uMsg)
	{
	case WM_APP_COPYOPERATIONFINISHED:
		{
			AsyncOperationInfo_t *paoi = reinterpret_cast<AsyncOperationInfo_t *>(wParam);
			paoi->pao->EndOperation(paoi->hr,NULL,paoi->dwEffect);
			paoi->pao->Release();

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
	assert(lpParameter != NULL);

	CoInitializeEx(0,COINIT_APARTMENTTHREADED);
	PastedFilesInfo_t *ppfi = reinterpret_cast<PastedFilesInfo_t *>(lpParameter);
	BOOL bRes = CopyDroppedFilesInternalAsync(ppfi);
	CoUninitialize();

	AsyncOperationInfo_t aoi;

	aoi.pao = ppfi->pao;

	if(bRes)
	{
		aoi.hr = S_OK;

		if(ppfi->bCopy)
		{
			aoi.dwEffect = FO_COPY;
		}
		else
		{
			aoi.dwEffect = FO_MOVE;
		}
	}
	else
	{
		aoi.hr = E_FAIL;
		aoi.dwEffect = DROPEFFECT_NONE;
	}

	/* Signal back to the main thread. We can't call EndOperation()
	from here, as it needs to be called on the original thread. */
	SendMessage(ppfi->hwnd,WM_APP_COPYOPERATIONFINISHED,
		reinterpret_cast<WPARAM>(&aoi),NULL);

	ppfi->pReferenceCount->Release();

	if(ppfi->pDropFilesCallback != NULL)
	{
		ppfi->pDropFilesCallback->Release();
	}

	delete ppfi;

	return 0;
}

BOOL CopyDroppedFilesInternalAsync(PastedFilesInfo_t *ppfi)
{
	TCHAR *pszFullFilenames = NFileOperations::BuildFilenameList(ppfi->FullFilenameList);

	UINT wFunc = 0;

	if(ppfi->bCopy)
	{
		wFunc = FO_COPY;
	}
	else
	{
		wFunc = FO_MOVE;
	}

	FILEOP_FLAGS fFlags = FOF_WANTMAPPINGHANDLE;

	if(ppfi->bRenameOnCollision)
	{
		fFlags |= FOF_RENAMEONCOLLISION;
	}

	SHFILEOPSTRUCT shfo;
	shfo.hwnd	= ppfi->hwnd;
	shfo.wFunc	= wFunc;
	shfo.pFrom	= pszFullFilenames;
	shfo.pTo	= ppfi->strDestDirectory.c_str();
	shfo.fFlags	= fFlags;
	BOOL bRes = (!SHFileOperation(&shfo) && !shfo.fAnyOperationsAborted);

	free(pszFullFilenames);

	if(bRes)
	{
		std::list<std::wstring> FilenameList;

		for each(auto FullFilename in ppfi->FullFilenameList)
		{
			TCHAR szFilename[MAX_PATH];
			StringCchCopy(szFilename,SIZEOF_ARRAY(szFilename),FullFilename.c_str());
			PathStripPath(szFilename);

			FilenameList.push_back(szFilename);
		}

		if(shfo.hNameMappings != NULL)
		{
			HANDLETOMAPPINGS *phtm = reinterpret_cast<HANDLETOMAPPINGS *>(shfo.hNameMappings);

			for(int i = 0;i < static_cast<int>(phtm->uNumberOfMappings);i++)
			{
				for(auto itr = FilenameList.begin();itr != FilenameList.end();itr++)
				{
					TCHAR szOldFileName[MAX_PATH];
					StringCchCopy(szOldFileName,SIZEOF_ARRAY(szOldFileName),
						phtm->lpSHNameMapping[i].pszOldPath);
					PathStripPath(szOldFileName);

					if(lstrcmp(szOldFileName,itr->c_str()) == 0)
					{
						TCHAR szNewFileName[MAX_PATH];
						StringCchCopy(szNewFileName,SIZEOF_ARRAY(szNewFileName),
							phtm->lpSHNameMapping[i].pszNewPath);
						PathStripPath(szNewFileName);

						*itr = szNewFileName;
						break;
					}
				}
			}

			SHFreeNameMappings(shfo.hNameMappings);
		}

		if(ppfi->pDropFilesCallback != NULL)
		{
			ppfi->pDropFilesCallback->OnDropFile(FilenameList,&ppfi->pt);
		}
	}

	return bRes;
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

	NFileOperations::CreateLinkToFile(szFullFileName,szLink,EMPTY_STRING);
}

HRESULT CDropHandler::CopyTextToFile(const TCHAR *pszDestDirectory,
	const WCHAR *pszText,TCHAR *pszFullFileNameOut, size_t outLen)
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

		/* UTF-16 LE BOM. */
		WriteFile(hFile,reinterpret_cast<LPCVOID>("\xFF\xFE"),2,
			&nBytesWritten,NULL);

		WriteFile(hFile,(LPCVOID)pszText,
			lstrlen(pszText) * sizeof(WCHAR),
			&nBytesWritten,NULL);

		CloseHandle(hFile);

		StringCchCopy(pszFullFileNameOut,outLen,szFullFileName);

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

/* TODO: */
void CreateDropOptionsMenu(HWND hDrop,LPCITEMIDLIST pidlDirectory,IDataObject *pDataObject)
{
	UNREFERENCED_PARAMETER(hDrop);
	UNREFERENCED_PARAMETER(pidlDirectory);
	UNREFERENCED_PARAMETER(pDataObject);

	/*list<ContextMenuHandler_t> ContextMenuHandlers;
	MENUITEMINFO mii;

	HMENU hMenu = CreatePopupMenu();

	CContextMenuManager cmm(CMT_DRAGDROP_HANDLERS,
		pidlDirectory,pDataObject,NULL);

	cmm.AddMenuEntries(hMenu,0,1,1000);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN,0,0,0,hDrop,NULL);*/
}