// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddressBar.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/iDataObject.h"
#include "../Helper/iDropSource.h"
#include "../Helper/ShellHelper.h"

AddressBar *AddressBar::Create(HWND parent, IExplorerplusplus *expp,
	Navigation *navigation, MainToolbar *mainToolbar)
{
	return new AddressBar(parent, expp, navigation, mainToolbar);
}

AddressBar::AddressBar(HWND parent, IExplorerplusplus *expp, Navigation *navigation,
	MainToolbar *mainToolbar) :
	CBaseWindow(CreateAddressBar(parent)),
	m_expp(expp),
	m_navigation(navigation),
	m_mainToolbar(mainToolbar)
{
	Initialize(parent);
}

HWND AddressBar::CreateAddressBar(HWND parent)
{
	return CreateComboBox(parent, WS_CHILD | WS_VISIBLE | WS_TABSTOP |
		CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
}

void AddressBar::Initialize(HWND parent)
{
	HIMAGELIST SmallIcons;
	Shell_GetImageLists(NULL, &SmallIcons);
	SendMessage(m_hwnd, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(SmallIcons));

	HWND hEdit = reinterpret_cast<HWND>(SendMessage(m_hwnd, CBEM_GETEDITCONTROL, 0, 0));
	SetWindowSubclass(hEdit, EditSubclassStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));

	/* Turn on auto complete for the edit control within the combobox.
	This will let the os complete paths as they are typed. */
	SHAutoComplete(hEdit, SHACF_FILESYSTEM | SHACF_AUTOSUGGEST_FORCE_ON);

	SetWindowSubclass(parent, ParentWndProcStub, PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));

	m_expp->AddTabsInitializedObserver([this] {
		m_connections.push_back(m_expp->GetTabContainer()->tabSelectedSignal.AddObserver(boost::bind(&AddressBar::OnTabSelected, this, _1)));
	});

	m_connections.push_back(m_navigation->navigationCompletedSignal.AddObserver(boost::bind(&AddressBar::OnNavigationCompleted, this, _1)));
}

AddressBar::~AddressBar()
{
	RemoveWindowSubclass(GetParent(m_hwnd), ParentWndProcStub, PARENT_SUBCLASS_ID);
}

LRESULT CALLBACK AddressBar::EditSubclassStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	AddressBar *addressBar = reinterpret_cast<AddressBar *>(dwRefData);

	return addressBar->EditSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK AddressBar::EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
			OnGo();
			return 0;
			break;
		}
		break;

	case WM_SETFOCUS:
		m_mainToolbar->UpdateToolbarButtonStates();
		break;

	case WM_MOUSEWHEEL:
		if (m_expp->OnMouseWheel(MOUSEWHEEL_SOURCE_OTHER, wParam, lParam))
		{
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK AddressBar::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	AddressBar *addressBar = reinterpret_cast<AddressBar *>(dwRefData);
	return addressBar->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK AddressBar::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case CBEN_DRAGBEGIN:
				OnBeginDrag();
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

/* Called when the user presses 'Enter' while
the address bar has focus, or when the 'Go'
toolbar button to the right of the address
bar is pressed.

The path entered may be relative to the current
directory, or absolute.
Basic procedure:
1. Path is expanded (if possible)
2. Any special character sequences ("..", ".") are removed
3. If the path is a URL, pass it straight out, else
4. If the path is relative, add it onto onto the current directory
*/
void AddressBar::OnGo()
{
	TCHAR szPath[MAX_PATH];
	TCHAR szFullFilePath[MAX_PATH];
	TCHAR szCurrentDirectory[MAX_PATH];

	/* Retrieve the combobox text, and determine if it is a
	valid path. */
	SendMessage(m_hwnd, WM_GETTEXT, SIZEOF_ARRAY(szPath), (LPARAM)szPath);

	const Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory), szCurrentDirectory);
	DecodePath(szPath, szCurrentDirectory, szFullFilePath, SIZEOF_ARRAY(szFullFilePath));

	m_expp->OpenItem(szFullFilePath, FALSE, FALSE);
}

void AddressBar::OnBeginDrag()
{
	IDragSourceHelper *pDragSourceHelper = NULL;
	IDropSource *pDropSource = NULL;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_ALL,
		IID_PPV_ARGS(&pDragSourceHelper));

	if (SUCCEEDED(hr))
	{
		hr = CreateDropSource(&pDropSource, DRAG_TYPE_LEFTCLICK);

		if (SUCCEEDED(hr))
		{
			const Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
			LPITEMIDLIST pidlDirectory = selectedTab.GetShellBrowser()->QueryCurrentDirectoryIdl();

			FORMATETC ftc[2];
			STGMEDIUM stg[2];

			SetFORMATETC(&ftc[0], (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
				NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);

			HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, 1000);

			FILEGROUPDESCRIPTOR *pfgd = static_cast<FILEGROUPDESCRIPTOR *>(GlobalLock(hglb));

			pfgd->cItems = 1;

			FILEDESCRIPTOR *pfd = (FILEDESCRIPTOR *)((LPBYTE)pfgd + sizeof(UINT));

			/* File information (name, size, date created, etc). */
			pfd[0].dwFlags = FD_ATTRIBUTES | FD_FILESIZE;
			pfd[0].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			pfd[0].nFileSizeLow = 16384;
			pfd[0].nFileSizeHigh = 0;

			/* The name of the file will be the folder name, followed by .lnk. */
			TCHAR szDisplayName[MAX_PATH];
			GetDisplayName(pidlDirectory, szDisplayName, SIZEOF_ARRAY(szDisplayName), SHGDN_INFOLDER);
			StringCchCat(szDisplayName, SIZEOF_ARRAY(szDisplayName), _T(".lnk"));
			StringCchCopy(pfd[0].cFileName, SIZEOF_ARRAY(pfd[0].cFileName), szDisplayName);

			GlobalUnlock(hglb);

			stg[0].pUnkForRelease = 0;
			stg[0].hGlobal = hglb;
			stg[0].tymed = TYMED_HGLOBAL;

			/* File contents. */
			SetFORMATETC(&ftc[1], (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
				NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);

			hglb = GlobalAlloc(GMEM_MOVEABLE, 16384);

			IShellLink *pShellLink = NULL;
			IPersistStream *pPersistStream = NULL;

			hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pShellLink));

			if (SUCCEEDED(hr))
			{
				TCHAR szPath[MAX_PATH];

				GetDisplayName(pidlDirectory, szPath, SIZEOF_ARRAY(szPath), SHGDN_FORPARSING);

				pShellLink->SetPath(szPath);

				hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistStream));

				if (SUCCEEDED(hr))
				{
					IStream *pStream = NULL;

					hr = CreateStreamOnHGlobal(hglb, FALSE, &pStream);

					if (SUCCEEDED(hr))
					{
						hr = pPersistStream->Save(pStream, TRUE);
					}
				}
			}

			GlobalUnlock(hglb);

			stg[1].pUnkForRelease = 0;
			stg[1].hGlobal = hglb;
			stg[1].tymed = TYMED_HGLOBAL;

			IDataObject *pDataObject = NULL;
			POINT pt = { 0,0 };

			hr = CreateDataObject(ftc, stg, &pDataObject, 2);

			pDragSourceHelper->InitializeFromWindow(m_hwnd, &pt, pDataObject);

			DWORD dwEffect;

			DoDragDrop(pDataObject, pDropSource, DROPEFFECT_LINK, &dwEffect);

			CoTaskMemFree(pidlDirectory);

			pDataObject->Release();
			pDropSource->Release();
		}

		pDragSourceHelper->Release();
	}
}

void AddressBar::OnTabSelected(const Tab &tab)
{
	UpdateTextAndIcon(tab);
}

void AddressBar::OnNavigationCompleted(const Tab &tab)
{
	if (m_expp->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateTextAndIcon(tab);
	}
}

void AddressBar::UpdateTextAndIcon(const Tab &tab)
{
	PIDLPointer pidl(tab.GetShellBrowser()->QueryCurrentDirectoryIdl());

	auto text = GetFolderPathForDisplay(pidl.get());

	if (!text)
	{
		return;
	}

	SHFILEINFO shfi;
	DWORD_PTR dwRet = SHGetFileInfo(reinterpret_cast<LPTSTR>(pidl.get()), NULL, &shfi,
		NULL, SHGFI_PIDL | SHGFI_SYSICONINDEX);

	if (dwRet == 0)
	{
		return;
	}

	SendMessage(m_hwnd, CB_RESETCONTENT, 0, 0);

	TCHAR displayText[MAX_PATH];
	StringCchCopy(displayText, SIZEOF_ARRAY(displayText), text->c_str());

	COMBOBOXEXITEM cbItem;
	cbItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_INDENT | CBEIF_SELECTEDIMAGE;
	cbItem.iItem = -1;
	cbItem.iImage = shfi.iIcon;
	cbItem.iSelectedImage = shfi.iIcon;
	cbItem.iIndent = 1;
	cbItem.iOverlay = 1;
	cbItem.pszText = displayText;
	SendMessage(m_hwnd, CBEM_SETITEM, 0, reinterpret_cast<LPARAM>(&cbItem));
}