// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "IDropFilesCallback.h"
#include "ListViewEdit.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "Navigation.h"
#include "ResourceHelper.h"
#include "SetFileAttributesDialog.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "SortMenuBuilder.h"
#include "TabContainer.h"
#include "ViewModeHelper.h"
#include "iServiceProvider.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/ContextMenuManager.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/iDataObject.h"
#include "../Helper/iDropSource.h"
#include <wil/com.h>

LRESULT CALLBACK Explorerplusplus::ListViewProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pexpp = reinterpret_cast<Explorerplusplus *>(dwRefData);

	return pexpp->ListViewSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::ListViewSubclassProc(
	HWND ListView, UINT msg, WPARAM wParam, LPARAM lParam)
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
					ListView_GetItemText(
						m_hActiveListView, lvhti.iItem, 0, szText, SIZEOF_ARRAY(szText));

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

			if (!(lvhti.flags & LVHT_NOWHERE))
			{
				m_bDragAllowed = true;
			}
		}
		break;

	case WM_RBUTTONUP:
		m_bDragCancelled = false;
		m_bDragAllowed = false;

		m_blockNextListViewSelection = false;
		break;

	/* If no item is currently been dragged, and the last drag
	has not just finished (i.e. item was dragged, but was cancelled
	with escape, but mouse button is still down), and when the right
	mouse button was clicked, it was over an item, start dragging. */
	case WM_MOUSEMOVE:
	{
		m_blockNextListViewSelection = false;
		if (!m_bDragging && !m_bDragCancelled && m_bDragAllowed)
		{
			if ((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON) && !(wParam & MK_MBUTTON))
			{
				NMLISTVIEW nmlv;
				POINT pt;
				DWORD dwPos;
				HRESULT hr;

				dwPos = GetMessagePos();
				pt.x = GET_X_LPARAM(dwPos);
				pt.y = GET_Y_LPARAM(dwPos);
				MapWindowPoints(HWND_DESKTOP, m_hActiveListView, &pt, 1);

				LV_HITTESTINFO lvhti;

				lvhti.pt = pt;

				/* Test to see if the mouse click was
				on an item or not. */
				ListView_HitTest(m_hActiveListView, &lvhti);

				if (!(lvhti.flags & LVHT_NOWHERE)
					&& ListView_GetSelectedCount(m_hActiveListView) > 0)
				{
					nmlv.iItem = 0;
					nmlv.ptAction = pt;

					hr = OnListViewBeginDrag((LPARAM) &nmlv, DragType::RightClick);

					if (hr == DRAGDROP_S_CANCEL)
					{
						m_bDragCancelled = true;
					}
				}
			}
		}
	}
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

BOOL Explorerplusplus::OnListViewBeginLabelEdit(const NMLVDISPINFO *dispInfo)
{
	if (!CanRename())
	{
		return TRUE;
	}

	HWND editControl = ListView_GetEditControl(m_hActiveListView);

	if (editControl == nullptr)
	{
		return TRUE;
	}

	auto fileData = m_pActiveShellBrowser->GetItemFileFindData(dispInfo->item.iItem);
	std::wstring editingName = m_pActiveShellBrowser->GetItemEditingName(dispInfo->item.iItem);

	bool useEditingName = true;

	// The editing name may differ from the display name. For example, the display name of the C:\
	// drive item will be something like "Local Disk (C:)", while its editing name will be "Local
	// Disk". Since the editing name is affected by the file name extensions setting in Explorer, it
	// won't be used if:
	//
	// - Extensions are hidden in Explorer, but shown in Explorer++ (since the editing name would
	//   contain no extension)
	// - Extensions are shown in Explorer, but hidden in Explorer++ (since the editing name would
	//   contain an extension). Note that this case is handled when editing is finished - if
	//   extensions are hidden, the extension will be manually re-added when renaming an item.
	if (!WI_IsFlagSet(fileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		std::wstring displayName = m_pActiveShellBrowser->GetItemDisplayName(dispInfo->item.iItem);

		if (m_config->globalFolderSettings.showExtensions
			|| m_config->globalFolderSettings.hideLinkExtension)
		{
			auto *extension = PathFindExtension(displayName.c_str());

			if (*extension != '\0'
				&& lstrcmp((editingName + extension).c_str(), displayName.c_str()) == 0)
			{
				useEditingName = false;
			}
		}
		else
		{
			auto *extension = PathFindExtension(editingName.c_str());

			if (*extension != '\0'
				&& lstrcmp((displayName + extension).c_str(), editingName.c_str()) == 0)
			{
				useEditingName = false;
			}
		}
	}

	// Note that the necessary text is set in the edit control, rather than the listview. This is
	// for the following two reasons:
	//
	// 1. Setting the listview item text after the edit control has already been created won't
	// change the text in the control
	// 2. Even if setting the listview item text did change the edit control text, the text would
	// need to be reverted if the user canceled editing. Setting the edit control text means there's
	// nothing that needs to be changed if editing is canceled.
	if (useEditingName)
	{
		SetWindowText(editControl, editingName.c_str());
	}

	ListViewEdit::CreateNew(editControl, dispInfo->item.iItem, this);

	m_bListViewRenaming = true;

	return FALSE;
}

BOOL Explorerplusplus::OnListViewEndLabelEdit(const NMLVDISPINFO *dispInfo)
{
	m_bListViewRenaming = false;

	// Did the user cancel editing?
	if (dispInfo->item.pszText == nullptr)
	{
		return FALSE;
	}

	std::wstring newFilename = dispInfo->item.pszText;

	if (newFilename.empty())
	{
		return FALSE;
	}

	std::wstring editingName = m_pActiveShellBrowser->GetItemEditingName(dispInfo->item.iItem);

	if (newFilename == editingName)
	{
		return FALSE;
	}

	auto fileData = m_pActiveShellBrowser->GetItemFileFindData(dispInfo->item.iItem);

	if (!WI_IsFlagSet(fileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		std::wstring filename = m_pActiveShellBrowser->GetItemName(dispInfo->item.iItem);
		auto *extension = PathFindExtension(filename.c_str());

		bool extensionHidden = !m_config->globalFolderSettings.showExtensions
			|| (m_config->globalFolderSettings.hideLinkExtension
				&& lstrcmpi(extension, _T(".lnk")) == 0);

		// If file extensions are turned off, the new filename will be incorrect (i.e. it will be
		// missing the extension). Therefore, append the extension manually if it is turned off.
		if (extensionHidden && *extension != '\0')
		{
			newFilename += extension;
		}
	}

	auto pidl = m_pActiveShellBrowser->GetItemCompleteIdl(dispInfo->item.iItem);

	wil::com_ptr_nothrow<IShellFolder> parent;
	PCITEMID_CHILD child;
	HRESULT hr = SHBindToParent(pidl.get(), IID_PPV_ARGS(&parent), &child);

	if (FAILED(hr))
	{
		return FALSE;
	}

	SHGDNF flags = SHGDN_INFOLDER;

	// As with GetDisplayNameOf(), the behavior of SetNameOf() is influenced by whether or not file
	// extensions are displayed in Explorer. If extensions are displayed and the SHGDN_INFOLDER name
	// is set, then the name should contain an extension. On the other hand, if extensions aren't
	// displayed and the SHGDN_INFOLDER name is set, then the name shouldn't contain an extension.
	// Given that extensions can be independently hidden and shown in Explorer++, this behavior is
	// undesirable and incompatible.
	// For example, if extensions are hidden in Explorer, but shown in Explorer++, then it wouldn't
	// be possible to change a file's extension. When setting the SHGDN_INFOLDER name, the original
	// extension would always be re-added by the shell.
	// Therefore, if a file is being edited, the parsing name (which will always contain an
	// extension) will be updated.
	if (!m_pActiveShellBrowser->InVirtualFolder()
		&& !WI_IsFlagSet(fileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		flags |= SHGDN_FORPARSING;
	}

	unique_pidl_child newChild;
	hr = parent->SetNameOf(m_pActiveShellBrowser->GetListView(), child, newFilename.c_str(), flags,
		wil::out_param(newChild));

	if (FAILED(hr))
	{
		return FALSE;
	}

	hr = parent->CompareIDs(0, child, newChild.get());

	// It's possible for the rename operation to succeed, but for the item name to remain unchanged.
	// For example, if one or more '.' characters are appended to the end of the item name, the
	// rename operation will succeed, but the name won't actually change. In those sorts of cases,
	// the name the user entered should be removed.
	if (HRESULT_CODE(hr) == 0)
	{
		return FALSE;
	}

	return TRUE;
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

	ServiceProvider serviceProvider(this);
	ContextMenuManager cmm(ContextMenuManager::ContextMenuType::Background, pidlDirectory.get(),
		pDataObject.get(), &serviceProvider, BLACKLISTED_BACKGROUND_MENU_CLSID_ENTRIES);

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

		fcmm.ShowMenu(this, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID, pCursorPos, &statusBar,
			reinterpret_cast<DWORD_PTR>(&fcmi), TRUE, IsKeyDown(VK_SHIFT));
	}
}

HRESULT Explorerplusplus::OnListViewBeginDrag(LPARAM lParam, DragType dragType)
{
	IDropSource *pDropSource = nullptr;
	IDragSourceHelper *pDragSourceHelper = nullptr;
	NMLISTVIEW *pnmlv = nullptr;
	POINT pt = { 0, 0 };
	HRESULT hr;
	int iDragStartObjectIndex;

	pnmlv = reinterpret_cast<NMLISTVIEW *>(lParam);

	if (ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return E_FAIL;
	}

	std::vector<unique_pidl_child> pidls;
	std::vector<PCITEMID_CHILD> rawPidls;
	std::vector<std::wstring> filenameList;

	int item = -1;

	/* Store the pidl of the current folder, as well as the relative
	pidl's of the dragged items. */
	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	while ((item = ListView_GetNextItem(m_hActiveListView, item, LVNI_SELECTED)) != -1)
	{
		auto pidl = m_pActiveShellBrowser->GetItemChildIdl(item);

		rawPidls.push_back(pidl.get());
		pidls.push_back(std::move(pidl));

		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(item);
		filenameList.push_back(fullFilename);
	}

	hr = CoCreateInstance(
		CLSID_DragDropHelper, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDragSourceHelper));

	if (SUCCEEDED(hr))
	{
		hr = CreateDropSource(&pDropSource, dragType);

		if (SUCCEEDED(hr))
		{
			FORMATETC ftc[2];
			STGMEDIUM stg[2];

			/* We'll export two formats:
			CF_HDROP
			CFSTR_SHELLIDLIST */
			BuildHDropList(&ftc[0], &stg[0], filenameList);
			BuildShellIDList(&ftc[1], &stg[1], pidlDirectory.get(), rawPidls);

			IDataObject *pDataObject = CreateDataObject(ftc, stg, 2);

			IDataObjectAsyncCapability *pAsyncCapability = nullptr;
			pDataObject->QueryInterface(IID_PPV_ARGS(&pAsyncCapability));

			assert(pAsyncCapability != nullptr);

			/* Docs mention setting the argument to VARIANT_TRUE/VARIANT_FALSE.
			But the argument is a BOOL, so we'll go with regular TRUE/FALSE. */
			pAsyncCapability->SetAsyncMode(TRUE);

			hr = pDragSourceHelper->InitializeFromWindow(m_hActiveListView, &pt, pDataObject);

			m_pActiveShellBrowser->DragStarted(pnmlv->iItem, &pnmlv->ptAction);
			m_bDragging = true;

			/* Need to remember which tab started the drag (as
			it may be different from the tab in which the drag
			finishes). */
			iDragStartObjectIndex = m_tabContainer->GetSelectedTab().GetId();

			DWORD dwEffect;

			hr = DoDragDrop(pDataObject, pDropSource,
				DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK, &dwEffect);

			m_bDragging = false;

			/* The object that starts any drag may NOT be the
			object that stops it (i.e. when a file is dragged
			between tabs). Therefore, need to tell the object
			that STARTED dragging that dragging has stopped. */
			m_tabContainer->GetTab(iDragStartObjectIndex).GetShellBrowser()->DragStopped();

			BOOL bInAsyncOp;

			hr = pAsyncCapability->InOperation(&bInAsyncOp);

			pAsyncCapability->Release();
			pDataObject->Release();
			pDropSource->Release();
		}

		pDragSourceHelper->Release();
	}

	return hr;
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
				std::vector<PCITEMID_CHILD> items = { pidl.get() };

				ShowMultipleFileProperties(pidlDirectory.get(), items.data(), m_hContainer, 1);
			}
			else
			{
				OpenListViewItem(
					ht.iItem, DetermineOpenDisposition(IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT)));
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
	IDataObject *pClipboardObject = nullptr;
	HRESULT hr;

	hr = OleGetClipboard(&pClipboardObject);

	if (hr == S_OK)
	{
		TCHAR szDestination[MAX_PATH + 1];

		/* DO NOT use the internal current directory string.
		Files are copied asynchronously, so a change of directory
		will cause the destination directory to change in the
		middle of the copy operation. */
		StringCchCopy(szDestination, SIZEOF_ARRAY(szDestination),
			m_pActiveShellBrowser->GetDirectory().c_str());

		/* Also, the string must be double NULL terminated. */
		szDestination[lstrlen(szDestination) + 1] = '\0';

		DropHandler *pDropHandler = DropHandler::CreateNew();
		auto *dropFilesCallback = new DropFilesCallback(this);
		pDropHandler->CopyClipboardData(pClipboardObject, m_hContainer, szDestination,
			dropFilesCallback, !m_config->overwriteExistingFilesConfirmation);
		pDropHandler->Release();

		pClipboardObject->Release();
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

OpenFolderDisposition Explorerplusplus::DetermineOpenDisposition(
	bool isCtrlKeyDown, bool isShiftKeyDown)
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