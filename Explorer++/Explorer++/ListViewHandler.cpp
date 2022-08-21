// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "FolderView.h"
#include "IDropFilesCallback.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "Navigation.h"
#include "NewMenuClient.h"
#include "ResourceHelper.h"
#include "ServiceProvider.h"
#include "SetFileAttributesDialog.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellView.h"
#include "SortMenuBuilder.h"
#include "TabContainer.h"
#include "ViewModeHelper.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/ClipboardHelper.h"
#include "../Helper/ContextMenuManager.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <winrt/base.h>

LRESULT CALLBACK Explorerplusplus::ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pexpp = reinterpret_cast<Explorerplusplus *>(dwRefData);

	return pexpp->ListViewSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::ListViewSubclassProc(HWND ListView, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_MENUSELECT:
		SendMessage(m_hContainer, WM_MENUSELECT, wParam, lParam);
		break;

	case WM_SETFOCUS:
		FocusChanged(WindowFocusSource::ListView);
		break;

	case WM_LBUTTONDBLCLK:
	{
		LV_HITTESTINFO ht;
		DWORD dwPos;
		POINT mousePos;

		dwPos = GetMessagePos();
		mousePos.x = GET_X_LPARAM(dwPos);
		mousePos.y = GET_Y_LPARAM(dwPos);
		ScreenToClient(m_hActiveListView, &mousePos);

		ht.pt = mousePos;
		ListView_HitTest(ListView, &ht);

		/* NM_DBLCLK for the listview is sent both on double clicks
		(by default), as well as in the situation when LVS_EX_ONECLICKACTIVATE
		is active (in which case it is sent on a single mouse click).
		Therefore, because we only want to navigate up one folder on
		a DOUBLE click, we'll handle the event here. */
		if (ht.flags == LVHT_NOWHERE)
		{
			/* The user has double clicked in the whitespace
			area for this tab, so go up one folder... */
			m_navigation->OnNavigateUp();
			return 0;
		}
	}
	break;

	case WM_RBUTTONDOWN:
		if ((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON) && !(wParam & MK_MBUTTON))
		{
			LV_HITTESTINFO lvhti;

			lvhti.pt.x = LOWORD(lParam);
			lvhti.pt.y = HIWORD(lParam);

			/* Test to see if the mouse click was
			on an item or not. */
			ListView_HitTest(m_hActiveListView, &lvhti);

			HDC hdc;
			TCHAR szText[MAX_PATH];
			RECT rc;
			SIZE sz;

			UINT uViewMode = m_pActiveShellBrowser->GetViewMode();

			if (uViewMode == ViewMode::List)
			{
				if (!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
				{
					ListView_GetItemRect(m_hActiveListView, lvhti.iItem, &rc, LVIR_LABEL);
					ListView_GetItemText(m_hActiveListView, lvhti.iItem, 0, szText,
						SIZEOF_ARRAY(szText));

					hdc = GetDC(m_hActiveListView);
					GetTextExtentPoint32(hdc, szText, lstrlen(szText), &sz);
					ReleaseDC(m_hActiveListView, hdc);

					rc.right = rc.left + sz.cx;

					if (!PtInRect(&rc, lvhti.pt))
					{
						m_blockNextListViewSelection = true;
					}
				}
			}
		}
		break;

	case WM_RBUTTONUP:
		m_blockNextListViewSelection = false;
		break;

	case WM_MOUSEMOVE:
		m_blockNextListViewSelection = false;
		break;

	case WM_MOUSEWHEEL:
		if (OnMouseWheel(MousewheelSource::ListView, wParam, lParam))
		{
			return 0;
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
		case HDN_BEGINDRAG:
			return FALSE;

		case HDN_ENDDRAG:
		{
			/* When the drag ends, the dragged item
			is shifted into position, and all later
			items are shifted down one. Therefore,
			take the item out of its position in the
			list, and move it into its new position. */
			NMHEADER *pnmHeader = nullptr;
			Column_t column;
			int i = 0;

			pnmHeader = (NMHEADER *) lParam;

			auto currentColumns = m_pActiveShellBrowser->GetCurrentColumns();

			i = 0;
			auto itr = currentColumns.begin();
			while (i < (pnmHeader->iItem + 1) && itr != currentColumns.end())
			{
				if (itr->bChecked)
				{
					i++;
				}

				itr++;
			}

			if (itr != currentColumns.begin())
				itr--;

			column = *itr;
			currentColumns.erase(itr);

			i = 0;
			itr = currentColumns.begin();
			while (i < (pnmHeader->pitem->iOrder + 1) && itr != currentColumns.end())
			{
				if (itr->bChecked)
				{
					i++;
				}

				itr++;
			}

			if (itr != currentColumns.begin())
				itr--;

			currentColumns.insert(itr, column);

			m_pActiveShellBrowser->SetCurrentColumns(currentColumns);

			Tab &tab = m_tabContainer->GetSelectedTab();
			tab.GetShellBrowser()->GetNavigationController()->Refresh();

			return TRUE;
		}
		}
		break;
	}

	return DefSubclassProc(ListView, msg, wParam, lParam);
}

LRESULT Explorerplusplus::OnListViewKeyDown(LPARAM lParam)
{
	LV_KEYDOWN *keyDown = reinterpret_cast<LV_KEYDOWN *>(lParam);

	switch (keyDown->wVKey)
	{
	case VK_RETURN:
		if (IsKeyDown(VK_MENU))
		{
			m_pActiveShellBrowser->ShowPropertiesForSelectedFiles();
		}
		else
		{
			OpenAllSelectedItems(
				DetermineOpenDisposition(IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT)));
		}
		break;

	case 'V':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			OnListViewPaste();
		}
		break;
	}

	return 0;
}

BOOL Explorerplusplus::OnListViewItemChanging(const NMLISTVIEW *changeData)
{
	if (changeData->uChanged != LVIF_STATE)
	{
		return FALSE;
	}

	int tabId = DetermineListViewObjectIndex(changeData->hdr.hwndFrom);

	if (tabId == -1)
	{
		return FALSE;
	}

	Tab &tab = m_tabContainer->GetTab(tabId);
	ViewMode viewMode = tab.GetShellBrowser()->GetViewMode();

	bool previouslySelected = WI_IsFlagSet(changeData->uOldState, LVIS_SELECTED);
	bool currentlySelected = WI_IsFlagSet(changeData->uNewState, LVIS_SELECTED);

	if (viewMode == +ViewMode::List && !previouslySelected && currentlySelected)
	{
		if (m_blockNextListViewSelection)
		{
			m_blockNextListViewSelection = false;
			return TRUE;
		}
	}

	return FALSE;
}

int Explorerplusplus::DetermineListViewObjectIndex(HWND hListView)
{
	for (auto &item : m_tabContainer->GetAllTabs())
	{
		if (item.second->GetShellBrowser()->GetListView() == hListView)
		{
			return item.first;
		}
	}

	return -1;
}

void Explorerplusplus::OnListViewRClick(POINT *pCursorPos)
{
	/* It may be possible for the active tab/folder
	to change while the menu is been shown (e.g. if
	not handled correctly, the back/forward buttons
	on a mouse will change the directory without
	destroying the popup menu).
	If this happens, the shell browser used will not
	stay constant throughout this function, and will
	cause various problems (e.g. changing from a
	computer where new items can be created to one
	where they cannot while the popup menu is still
	active will cause the 'new' entry to stay on the
	menu incorrectly).
	Due to the possibility of unforseen problems,
	this function should NOT access the current
	shell browser once the menu has been destroyed. */

	SetForegroundWindow(m_hContainer);

	if (IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_MENU))
	{
		LVHITTESTINFO lvhti;

		lvhti.pt = *pCursorPos;
		ScreenToClient(m_hActiveListView, &lvhti.pt);
		ListView_HitTest(m_hActiveListView, &lvhti);

		if (!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
		{
			if (ListView_GetItemState(m_hActiveListView, lvhti.iItem, LVIS_SELECTED)
				!= LVIS_SELECTED)
			{
				ListViewHelper::SelectAllItems(m_hActiveListView, FALSE);
				ListViewHelper::SelectItem(m_hActiveListView, lvhti.iItem, TRUE);
			}
		}
	}

	if (ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		OnListViewBackgroundRClick(pCursorPos);
	}
	else
	{
		OnListViewItemRClick(pCursorPos);
	}
}

void Explorerplusplus::OnListViewBackgroundRClick(POINT *pCursorPos)
{
	if (IsWindows8OrGreater())
	{
		OnListViewBackgroundRClickWindows8OrGreater(pCursorPos);
	}
	else
	{
		OnListViewBackgroundRClickWindows7(pCursorPos);
	}
}

void Explorerplusplus::OnListViewBackgroundRClickWindows8OrGreater(POINT *pCursorPos)
{
	const auto &selectedTab = m_tabContainer->GetSelectedTab();
	auto pidlDirectory = selectedTab.GetShellBrowser()->GetDirectoryIdl();

	FileContextMenuManager fcmm(selectedTab.GetShellBrowser()->GetListView(), pidlDirectory.get(),
		{});

	FileContextMenuInfo fcmi;
	fcmi.uFrom = FROM_LISTVIEW;

	StatusBar statusBar(m_hStatusBar);

	auto serviceProvider = winrt::make_self<ServiceProvider>();

	auto newMenuClient = winrt::make<NewMenuClient>(this);
	serviceProvider->RegisterService(IID_INewMenuClient, newMenuClient.get());

	winrt::com_ptr<IFolderView2> folderView =
		winrt::make<FolderView>(selectedTab.GetShellBrowserWeak());
	serviceProvider->RegisterService(IID_IFolderView, folderView.get());

	auto shellView = winrt::make<ShellView>(selectedTab.GetShellBrowserWeak(), this, false);
	serviceProvider->RegisterService(SID_DefView, shellView.get());

	fcmm.ShowMenu(this, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID, pCursorPos, &statusBar,
		serviceProvider.get(), reinterpret_cast<DWORD_PTR>(&fcmi), FALSE, IsKeyDown(VK_SHIFT));
}

void Explorerplusplus::OnListViewBackgroundRClickWindows7(POINT *pCursorPos)
{
	auto parentMenu = InitializeRightClickMenu();
	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	unique_pidl_absolute pidlParent(ILCloneFull(pidlDirectory.get()));
	ILRemoveLastID(pidlParent.get());

	wil::com_ptr_nothrow<IShellFolder> pShellFolder;
	HRESULT hr = BindToIdl(pidlParent.get(), IID_PPV_ARGS(&pShellFolder));

	if (FAILED(hr))
	{
		return;
	}

	wil::com_ptr_nothrow<IDataObject> pDataObject;
	PCUITEMID_CHILD pidlChildFolder = ILFindLastID(pidlDirectory.get());
	hr =
		GetUIObjectOf(pShellFolder.get(), nullptr, 1, &pidlChildFolder, IID_PPV_ARGS(&pDataObject));

	if (FAILED(hr))
	{
		return;
	}

	auto serviceProvider = winrt::make_self<ServiceProvider>();

	auto newMenuClient = winrt::make<NewMenuClient>(this);
	serviceProvider->RegisterService(IID_INewMenuClient, newMenuClient.get());

	ContextMenuManager cmm(ContextMenuManager::ContextMenuType::Background, pidlDirectory.get(),
		pDataObject.get(), serviceProvider.get(), BLACKLISTED_BACKGROUND_MENU_CLSID_ENTRIES);

	cmm.ShowMenu(m_hContainer, menu, IDM_FILE_COPYFOLDERPATH, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID,
		*pCursorPos, *m_pStatusBar);
}

wil::unique_hmenu Explorerplusplus::InitializeRightClickMenu()
{
	wil::unique_hmenu parentMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_MAINMENU_RCLICK)));

	for (auto viewMode : VIEW_MODES)
	{
		std::wstring text =
			ResourceHelper::LoadString(m_hLanguageModule, GetViewModeMenuStringId(viewMode));
		MenuHelper::AddStringItem(parentMenu.get(), GetViewModeMenuId(viewMode), text,
			IDM_RCLICK_VIEW_PLACEHOLDER, FALSE);
	}

	DeleteMenu(parentMenu.get(), IDM_RCLICK_VIEW_PLACEHOLDER, MF_BYCOMMAND);

	SortMenuBuilder sortMenuBuilder(m_hLanguageModule);
	auto [sortByMenu, groupByMenu] = sortMenuBuilder.BuildMenus(m_tabContainer->GetSelectedTab());

	MenuHelper::AttachSubMenu(parentMenu.get(), std::move(sortByMenu), IDM_POPUP_SORTBY, FALSE);
	MenuHelper::AttachSubMenu(parentMenu.get(), std::move(groupByMenu), IDM_POPUP_GROUPBY, FALSE);

	ViewMode viewMode = m_pActiveShellBrowser->GetViewMode();

	if (viewMode == +ViewMode::List)
	{
		MenuHelper::EnableItem(parentMenu.get(), IDM_POPUP_GROUPBY, FALSE);
	}
	else
	{
		MenuHelper::EnableItem(parentMenu.get(), IDM_POPUP_GROUPBY, TRUE);
	}

	return parentMenu;
}

void Explorerplusplus::OnListViewItemRClick(POINT *pCursorPos)
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if (nSelected > 0)
	{
		std::vector<unique_pidl_child> pidlPtrs;
		std::vector<PCITEMID_CHILD> pidlItems;
		int iItem = -1;

		while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
		{
			auto pidlPtr = m_pActiveShellBrowser->GetItemChildIdl(iItem);

			pidlItems.push_back(pidlPtr.get());
			pidlPtrs.push_back(std::move(pidlPtr));
		}

		auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

		FileContextMenuManager fcmm(m_hActiveListView, pidlDirectory.get(), pidlItems);

		FileContextMenuInfo fcmi;
		fcmi.uFrom = FROM_LISTVIEW;

		StatusBar statusBar(m_hStatusBar);

		fcmm.ShowMenu(this, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID, pCursorPos, &statusBar, nullptr,
			reinterpret_cast<DWORD_PTR>(&fcmi), TRUE, IsKeyDown(VK_SHIFT));
	}
}

void Explorerplusplus::OnListViewDoubleClick(NMHDR *nmhdr)
{
	if (nmhdr->hwndFrom == m_hActiveListView)
	{
		LV_HITTESTINFO ht;
		DWORD dwPos;
		POINT mousePos;

		dwPos = GetMessagePos();
		mousePos.x = GET_X_LPARAM(dwPos);
		mousePos.y = GET_Y_LPARAM(dwPos);
		ScreenToClient(m_hActiveListView, &mousePos);

		ht.pt = mousePos;
		ListView_HitTest(m_hActiveListView, &ht);

		if (ht.flags != LVHT_NOWHERE && ht.iItem != -1)
		{
			if (IsKeyDown(VK_MENU))
			{
				auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();
				auto pidl = m_pActiveShellBrowser->GetItemChildIdl(ht.iItem);
				ShowMultipleFileProperties(pidlDirectory.get(), { pidl.get() }, m_hContainer);
			}
			else
			{
				OpenListViewItem(ht.iItem,
					DetermineOpenDisposition(IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT)));
			}
		}
	}
}

void Explorerplusplus::OnListViewCopyItemPath() const
{
	if (ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::wstring strItemPaths;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iItem);

		strItemPaths += fullFilename + std::wstring(_T("\r\n"));
	}

	strItemPaths = strItemPaths.substr(0, strItemPaths.size() - 2);

	BulkClipboardWriter clipboardWriter;
	clipboardWriter.WriteText(strItemPaths);
}

void Explorerplusplus::OnListViewCopyUniversalPaths() const
{
	if (ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::wstring strUniversalPaths;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iItem);

		TCHAR szBuffer[1024];

		DWORD dwBufferSize = SIZEOF_ARRAY(szBuffer);
		auto *puni = reinterpret_cast<UNIVERSAL_NAME_INFO *>(&szBuffer);
		DWORD dwRet = WNetGetUniversalName(fullFilename.c_str(), UNIVERSAL_NAME_INFO_LEVEL,
			reinterpret_cast<LPVOID>(puni), &dwBufferSize);

		if (dwRet == NO_ERROR)
		{
			strUniversalPaths += puni->lpUniversalName + std::wstring(_T("\r\n"));
		}
		else
		{
			strUniversalPaths += fullFilename + std::wstring(_T("\r\n"));
		}
	}

	strUniversalPaths = strUniversalPaths.substr(0, strUniversalPaths.size() - 2);

	BulkClipboardWriter clipboardWriter;
	clipboardWriter.WriteText(strUniversalPaths);
}

void Explorerplusplus::OnListViewSetFileAttributes() const
{
	const Tab &selectedTab = m_tabContainer->GetSelectedTab();
	selectedTab.GetShellBrowser()->SetFileAttributesForSelection();
}

void Explorerplusplus::OnListViewPaste()
{
	wil::com_ptr_nothrow<IDataObject> clipboardObject;
	HRESULT hr = OleGetClipboard(&clipboardObject);

	if (FAILED(hr))
	{
		return;
	}

	const auto &selectedTab = m_tabContainer->GetSelectedTab();
	auto directory = selectedTab.GetShellBrowser()->GetDirectoryIdl();

	if (CanShellPasteDataObject(directory.get(), clipboardObject.get(),
			DROPEFFECT_COPY | DROPEFFECT_MOVE))
	{
		auto serviceProvider = winrt::make_self<ServiceProvider>();

		auto folderView = winrt::make<FolderView>(selectedTab.GetShellBrowserWeak());
		serviceProvider->RegisterService(IID_IFolderView, folderView.get());

		ExecuteActionFromContextMenu(directory.get(), {},
			selectedTab.GetShellBrowser()->GetListView(), L"paste", 0, serviceProvider.get());
	}
	else
	{
		TCHAR szDestination[MAX_PATH + 1];

		/* DO NOT use the internal current directory string.
		 Files are copied asynchronously, so a change of directory
		 will cause the destination directory to change in the
		 middle of the copy operation. */
		StringCchCopy(szDestination, SIZEOF_ARRAY(szDestination),
			selectedTab.GetShellBrowser()->GetDirectory().c_str());

		/* Also, the string must be double NULL terminated. */
		szDestination[lstrlen(szDestination) + 1] = '\0';

		DropHandler *pDropHandler = DropHandler::CreateNew();
		auto *dropFilesCallback = new DropFilesCallback(this);
		pDropHandler->CopyClipboardData(clipboardObject.get(), m_hContainer, szDestination,
			dropFilesCallback);
		pDropHandler->Release();
	}
}

int Explorerplusplus::HighlightSimilarFiles(HWND ListView) const
{
	BOOL bSimilarTypes;
	int iSelected;
	int nItems;
	int nSimilar = 0;
	int i = 0;

	iSelected = ListView_GetNextItem(ListView, -1, LVNI_SELECTED);

	if (iSelected == -1)
		return -1;

	std::wstring testFile = m_pActiveShellBrowser->GetItemFullName(iSelected);

	nItems = ListView_GetItemCount(ListView);

	for (i = 0; i < nItems; i++)
	{
		std::wstring fullFileName = m_pActiveShellBrowser->GetItemFullName(i);

		bSimilarTypes = CompareFileTypes(fullFileName.c_str(), testFile.c_str());

		if (bSimilarTypes)
		{
			ListViewHelper::SelectItem(ListView, i, TRUE);
			nSimilar++;
		}
		else
		{
			ListViewHelper::SelectItem(ListView, i, FALSE);
		}
	}

	return nSimilar;
}

void Explorerplusplus::OpenAllSelectedItems(OpenFolderDisposition openFolderDisposition)
{
	BOOL bSeenDirectory = FALSE;
	DWORD dwAttributes;
	int iItem = -1;
	int iFolderItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVIS_SELECTED)) != -1)
	{
		dwAttributes = m_pActiveShellBrowser->GetItemFileFindData(iItem).dwFileAttributes;

		if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			bSeenDirectory = TRUE;
			iFolderItem = iItem;
		}
		else
		{
			OpenListViewItem(iItem);
		}
	}

	if (bSeenDirectory)
	{
		OpenListViewItem(iFolderItem, openFolderDisposition);
	}
}

void Explorerplusplus::OpenListViewItem(int index, OpenFolderDisposition openFolderDisposition)
{
	auto pidlComplete = m_pActiveShellBrowser->GetItemCompleteIdl(index);
	OpenItem(pidlComplete.get(), openFolderDisposition);
}

OpenFolderDisposition Explorerplusplus::DetermineOpenDisposition(bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	if (isCtrlKeyDown && !isShiftKeyDown)
	{
		if (m_config->openTabsInForeground)
		{
			return OpenFolderDisposition::ForegroundTab;
		}
		else
		{
			return OpenFolderDisposition::BackgroundTab;
		}
	}
	else if (!isCtrlKeyDown && isShiftKeyDown)
	{
		return OpenFolderDisposition::NewWindow;
	}
	else if (isCtrlKeyDown && isShiftKeyDown)
	{
		// Ctrl + Shift inverts the usual behavior.
		if (m_config->openTabsInForeground)
		{
			return OpenFolderDisposition::BackgroundTab;
		}
		else
		{
			return OpenFolderDisposition::ForegroundTab;
		}
	}
	else
	{
		return OpenFolderDisposition::CurrentTab;
	}
}
