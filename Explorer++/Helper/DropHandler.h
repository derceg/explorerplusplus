// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include "Helper.h"
#include "FileOperations.h"
#include "ReferenceCount.h"

enum DragTypes_t
{
	DRAG_TYPE_LEFTCLICK,
	DRAG_TYPE_RIGHTCLICK
};

/* TODO: Switch to IReferenceCount in the future.
IUnknown needed to support CShellBrowser. */
__interface IDropFilesCallback : public IUnknown
{
	void OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt);
};

class CDropHandler : public CReferenceCount
{
public:

	/* As this class is reference counted, the constructor
	and destructor are both private. Use this method to
	get a new instance of this class. */
	static CDropHandler	*CreateNew();

	static HRESULT		GetDropFormats(std::list<FORMATETC> &ftcList);

	void	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);
	void	CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);

private:

	CDropHandler() = default;
	~CDropHandler() = default;

	void	HandleLeftClickDrop(IDataObject *pDataObject,POINTL *pptl);
	void	HandleRightClickDrop(void);

	BOOL	CheckDropFormatSupported(IDataObject *pDataObject,FORMATETC *pftc);

	HRESULT	CopyHDropData(IDataObject *pDataObject,BOOL bPrefferedEffect,DWORD dwEffect);
	HRESULT	CopyShellIDListData(IDataObject *pDataObject,std::list<std::wstring> &PastedFileList);
	HRESULT CopyAnsiFileDescriptorData(IDataObject *pDataObject,std::list<std::wstring> &PastedFileList);
	HRESULT CopyUnicodeFileDescriptorData(IDataObject *pDataObject,std::list<std::wstring> &PastedFileList);
	HRESULT CopyFileDescriptorData(IDataObject *pDataObject,FILEGROUPDESCRIPTORW *pfgd,std::list<std::wstring> &PastedFileList);
	HRESULT	CopyUnicodeTextData(IDataObject *pDataObject,std::list<std::wstring> &PastedFileList);
	HRESULT	CopyAnsiTextData(IDataObject *pDataObject,std::list<std::wstring> &PastedFileList);
	HRESULT	CopyDIBV5Data(IDataObject *pDataObject,std::list<std::wstring> &PastedFileList);

	void	CopyDroppedFiles(const HDROP &hd,BOOL bPreferredEffect,DWORD dwPreferredEffect);
	void	CopyDroppedFilesInternal(const std::list<std::wstring> &FullFilenameList,BOOL bCopy,BOOL bRenameOnCollision);
	void	CreateShortcutToDroppedFile(TCHAR *szFullFileName);
	HRESULT	CopyTextToFile(const TCHAR *pszDestDirectory, const WCHAR *pszText, TCHAR *pszFullFileNameOut, size_t outLen);
	BOOL	CheckItemLocations(int iDroppedItem);

	/* Holds the drop formats supported. */
	static FORMATETC	m_ftcHDrop;
	static FORMATETC	m_ftcFileDescriptorA;
	static FORMATETC	m_ftcFileDescriptorW;
	static FORMATETC	m_ftcShellIDList;
	static FORMATETC	m_ftcText;
	static FORMATETC	m_ftcUnicodeText;
	static FORMATETC	m_ftcDIBV5;

	IDataObject			*m_pDataObject;
	IDropFilesCallback	*m_pDropFilesCallback;
	DWORD				m_grfKeyState;
	POINTL				m_ptl;
	DWORD				m_dwEffect;
	HWND				m_hwndDrop;
	DragTypes_t			m_DragType;
	TCHAR				*m_szDestDirectory;
	BOOL				m_bRenameOnCollision;
};