// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "HolderWindow.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "Navigation.h"
#include "SetFileAttributesDialog.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Controls.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../MyTreeView/MyTreeView.h"

#define TREEVIEW_FOLDER_OPEN_DELAY	500
#define FOLDERS_TOOLBAR_CLOSE		6000

LRESULT CALLBACK TreeViewHolderProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
LRESULT CALLBACK TreeViewSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

/* Used to keep track of which item was selected in
the treeview control. */
HTREEITEM	g_NewSelectionItem;

void Explorerplusplus::CreateFolderControls(void)
{
	TCHAR szTemp[32];
	UINT uStyle = WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

	if(m_config->showFolders)
		uStyle |= WS_VISIBLE;

	LoadString(m_hLanguageModule,IDS_FOLDERS_WINDOW_TEXT,szTemp,SIZEOF_ARRAY(szTemp));
	m_hHolder = CreateHolderWindow(m_hContainer,szTemp,uStyle);
	SetWindowSubclass(m_hHolder,TreeViewHolderProcStub,0,(DWORD_PTR)this);

	m_hTreeView = CreateTreeView(m_hHolder,WS_CHILD|WS_VISIBLE|TVS_SHOWSELALWAYS|
		TVS_HASBUTTONS|TVS_EDITLABELS|TVS_HASLINES|TVS_TRACKSELECT);

	SetWindowTheme(m_hTreeView,L"Explorer",NULL);

	SetWindowLongPtr(m_hTreeView,GWL_EXSTYLE,WS_EX_CLIENTEDGE);
	m_pMyTreeView = new CMyTreeView(m_hTreeView, m_hHolder, m_pDirMon, &m_cachedIcons);

	/* Now, subclass the treeview again. This is needed for messages
	such as WM_MOUSEWHEEL, which need to be intercepted before they
	reach the window procedure provided by CMyTreeView. */
	SetWindowSubclass(m_hTreeView,TreeViewSubclassStub,1,(DWORD_PTR)this);

	LoadString(m_hLanguageModule,IDS_HIDEFOLDERSPANE,szTemp,SIZEOF_ARRAY(szTemp));
	m_hFoldersToolbar = CreateTabToolbar(m_hHolder,FOLDERS_TOOLBAR_CLOSE,szTemp);

	m_InitializationFinished.addObserver([this] (bool newValue) {
		if (newValue)
		{
			// Updating the treeview selection is relatively expensive, so it's
			// not done at all during startup. Therefore, the selection will be
			// set a single time, once the application initialization is
			// complete and all tabs have been restored.
			UpdateTreeViewSelection();
		}
	});

	m_tabContainer->tabCreatedSignal.AddObserver([this] (int tabId, BOOL switchToNewTab) {
		UNREFERENCED_PARAMETER(tabId);
		UNREFERENCED_PARAMETER(switchToNewTab);

		UpdateTreeViewSelection();
	});

	m_tabContainer->tabNavigationCompletedSignal.AddObserver([this] (const Tab &tab) {
		UNREFERENCED_PARAMETER(tab);

		UpdateTreeViewSelection();
	});

	m_tabContainer->tabSelectedSignal.AddObserver([this] (const Tab &tab) {
		UNREFERENCED_PARAMETER(tab);

		UpdateTreeViewSelection();
	});

	m_tabContainer->tabRemovedSignal.AddObserver([this] (int tabId) {
		UNREFERENCED_PARAMETER(tabId);

		UpdateTreeViewSelection();
	});
}

LRESULT CALLBACK TreeViewSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TreeViewSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewSubclass(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_SETFOCUS:
		m_mainToolbar->UpdateToolbarButtonStates();
		m_hLastActiveWindow = m_hTreeView;
		break;

	case WM_MBUTTONDOWN:
		{
			TVHITTESTINFO tvhi;

			tvhi.pt.x = LOWORD(lParam);
			tvhi.pt.y = HIWORD(lParam);

			TreeView_HitTest(m_hTreeView,&tvhi);

			if(tvhi.flags != LVHT_NOWHERE && tvhi.hItem != NULL)
			{
				m_hTVMButtonItem = tvhi.hItem;
			}
			else
			{
				m_hTVMButtonItem = NULL;
			}
		}
		break;

	case WM_MBUTTONUP:
		{
			TVHITTESTINFO tvhi;
			tvhi.pt.x = LOWORD(lParam);
			tvhi.pt.y = HIWORD(lParam);

			TreeView_HitTest(m_hTreeView,&tvhi);

			if(tvhi.flags != LVHT_NOWHERE && tvhi.hItem != NULL)
			{
				/* Only open an item if it was the one
				on which the middle mouse button was
				initially clicked on. */
				if(tvhi.hItem == m_hTVMButtonItem)
				{
					auto pidl = m_pMyTreeView->GetItemPidl(tvhi.hItem);
					m_tabContainer->CreateNewTab(pidl.get());
				}
			}
		}
		break;

	case WM_MOUSEWHEEL:
		if(OnMouseWheel(MOUSEWHEEL_SOURCE_TREEVIEW,wParam,lParam))
		{
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void Explorerplusplus::OnTreeViewFileRename(void)
{
	HTREEITEM hItem;

	SetFocus(m_hTreeView);
	hItem = TreeView_GetSelection(m_hTreeView);
	TreeView_EditLabel(m_hTreeView,hItem);
}

void Explorerplusplus::OnTreeViewFileDelete(BOOL bPermanent)
{
	HTREEITEM		hItem, hParentItem;
	DWORD			fMask = 0;
	HRESULT			hr;

	hItem		= TreeView_GetSelection(m_hTreeView);
	hParentItem = TreeView_GetParent(m_hTreeView,hItem); 

	// Select the parent item to release the lock and allow deletion
	TreeView_Select(m_hTreeView,hParentItem,TVGN_CARET);

	if(hItem != NULL)
	{
		auto pidl = m_pMyTreeView->GetItemPidl(hItem);

		if(bPermanent)
		{
			fMask = CMIC_MASK_SHIFT_DOWN;
		}

		hr = ExecuteActionFromContextMenu(pidl.get(),NULL,m_hContainer,0,_T("delete"),fMask);
	}
}

void Explorerplusplus::OnTreeViewRightClick(WPARAM wParam,LPARAM lParam)
{
	POINT *ppt = NULL;
	HTREEITEM hItem;
	HTREEITEM hPrevItem;
	IShellFolder *pShellParentFolder = NULL;
	PCITEMID_CHILD pidlRelative = NULL;
	HRESULT hr;

	hItem	= (HTREEITEM)wParam;
	ppt		= (POINT *)lParam;

	m_bTreeViewRightClick = TRUE;

	hPrevItem = TreeView_GetSelection(m_hTreeView);
	TreeView_SelectItem(m_hTreeView,hItem);
	auto pidl = m_pMyTreeView->GetItemPidl(hItem);

	hr = SHBindToParent(pidl.get(), IID_PPV_ARGS(&pShellParentFolder), &pidlRelative);

	if(SUCCEEDED(hr))
	{
		HTREEITEM hParent;

		hParent = TreeView_GetParent(m_hTreeView,hItem);

		/* If we right-click on the "Desktop" item in the treeview, there is no parent.
		   In such case, use "Desktop" as parent item as well, to allow the context menu
		   to be shown. */
		if(hParent == NULL)
		{
			hParent = hItem;
		}

		if(hParent != NULL)
		{
			auto pidlParent = m_pMyTreeView->GetItemPidl(hParent);

			if(pidlParent)
			{
				m_bTreeViewOpenInNewTab = FALSE;

				std::vector<PCITEMID_CHILD> pidlItems;
				pidlItems.push_back(pidlRelative);

				CFileContextMenuManager fcmm(m_hContainer, pidlParent.get(), pidlItems);

				FileContextMenuInfo_t fcmi;
				fcmi.uFrom = FROM_TREEVIEW;

				CStatusBar StatusBar(m_hStatusBar);

				fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,ppt,&StatusBar,
					reinterpret_cast<DWORD_PTR>(&fcmi),TRUE,IsKeyDown(VK_SHIFT));
			}
		}

		pShellParentFolder->Release();
	}

	/* Don't switch back to the previous folder if
	the folder that was right-clicked was opened in
	a new tab (i.e. can just keep the selection the
	same). */
	if(!m_bTreeViewOpenInNewTab)
		TreeView_SelectItem(m_hTreeView,hPrevItem);

	m_bTreeViewRightClick = FALSE;
}

/*
 * Shows the properties dialog for the currently
 * selected treeview item.
 */
void Explorerplusplus::OnTreeViewShowFileProperties(void) const
{
	HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);

	/* Get the path of the currently selected item. */
	auto pidlDirectory = m_pMyTreeView->GetItemPidl(hItem);
	ShowMultipleFileProperties(pidlDirectory.get(), NULL, m_hContainer, 0);
}

void Explorerplusplus::OnTreeViewCopyItemPath(void) const
{
	HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		auto pidl = m_pMyTreeView->GetItemPidl(hItem);

		TCHAR szFullFileName[MAX_PATH];
		GetDisplayName(pidl.get(),szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

		BulkClipboardWriter clipboardWriter;
		clipboardWriter.WriteText(szFullFileName);
	}
}

void Explorerplusplus::OnTreeViewCopyUniversalPaths(void) const
{
	HTREEITEM		hItem;
	TCHAR			szFullFileName[MAX_PATH];
	UNIVERSAL_NAME_INFO	uni;
	DWORD			dwBufferSize;
	DWORD			dwRet;

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		auto pidl = m_pMyTreeView->GetItemPidl(hItem);

		GetDisplayName(pidl.get(),szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

		dwBufferSize = sizeof(uni);
		dwRet = WNetGetUniversalName(szFullFileName,UNIVERSAL_NAME_INFO_LEVEL,
			(void **)&uni,&dwBufferSize);

		BulkClipboardWriter clipboardWriter;

		if (dwRet == NO_ERROR)
		{
			clipboardWriter.WriteText(uni.lpUniversalName);
		}
		else
		{
			clipboardWriter.WriteText(szFullFileName);
		}
	}
}

void Explorerplusplus::OnTreeViewCopy(BOOL bCopy)
{
	IDataObject		*pClipboardDataObject = NULL;
	HTREEITEM		hItem;
	TVITEM			tvItem;
	HRESULT			hr;

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		auto pidl = m_pMyTreeView->GetItemPidl(hItem);

		std::list<std::wstring> FileNameList;
		TCHAR szFullFileName[MAX_PATH];

		GetDisplayName(pidl.get(),szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

		std::wstring stringFileName(szFullFileName);
		FileNameList.push_back(stringFileName);

		if(bCopy)
		{
			hr = CopyFiles(FileNameList,&pClipboardDataObject);
		}
		else
		{
			hr = CutFiles(FileNameList,&pClipboardDataObject);

			if(SUCCEEDED(hr))
			{
				m_hCutTreeViewItem = hItem;
				m_iCutTabInternal = m_tabContainer->GetSelectedTab().GetId();

				tvItem.mask			= TVIF_HANDLE|TVIF_STATE;
				tvItem.hItem		= hItem;
				tvItem.state		= TVIS_CUT;
				tvItem.stateMask	= TVIS_CUT;
				TreeView_SetItem(m_hTreeView,&tvItem);
			}
		}

		if(SUCCEEDED(hr))
		{
			m_pClipboardDataObject = pClipboardDataObject;
		}
	}
}

void Explorerplusplus::OnTreeViewHolderWindowTimer(void)
{
	auto pidlDirectory = m_pMyTreeView->GetItemPidl(g_NewSelectionItem);
	auto pidlCurrentDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	if(!m_bSelectingTreeViewDirectory && !m_bTreeViewRightClick &&
		!CompareIdls(pidlDirectory.get(),pidlCurrentDirectory.get()))
	{
		Tab &selectedTab = m_tabContainer->GetSelectedTab();
		selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(pidlDirectory.get());

		if(m_config->treeViewAutoExpandSelected)
		{
			TreeView_Expand(m_hTreeView,g_NewSelectionItem,TVE_EXPAND);
		}
	}

	KillTimer(m_hHolder,0);
}

void Explorerplusplus::OnTreeViewSelChanged(LPARAM lParam)
{
	NMTREEVIEW	*pnmtv = NULL;
	TVITEM		*tvItem = NULL;

	/* Check whether the selection was changed because a new directory
	was browsed to, or if the treeview control is involved in a
	drag and drop operation. */
	if(!m_bSelectingTreeViewDirectory && !m_bTreeViewRightClick &&
		!m_pMyTreeView->QueryDragging())
	{
			pnmtv = (LPNMTREEVIEW)lParam;

			tvItem = &pnmtv->itemNew;

			g_NewSelectionItem = tvItem->hItem;

			if(m_config->treeViewDelayEnabled)
			{
				/* Schedule a folder change. This adds enough
				of a delay for the treeview selection to be changed
				without the current folder been changed immediately. */
				SetTimer(m_hHolder,0,TREEVIEW_FOLDER_OPEN_DELAY,NULL);
			}
			else
			{
				/* The treeview delay is disabled. For simplicity, just
				set a timer of length 0. */
				SetTimer(m_hHolder,0,0,NULL);
			}
	}
	else
	{
		m_bSelectingTreeViewDirectory = FALSE;
	}
}

int Explorerplusplus::OnTreeViewBeginLabelEdit(LPARAM lParam)
{
	NMTVDISPINFO *pdi = reinterpret_cast<NMTVDISPINFO *>(lParam);

	auto pidl = m_pMyTreeView->GetItemPidl(pdi->item.hItem);

	/* Save the old filename, in the case that the file
	needs to be renamed. */
	GetDisplayName(pidl.get(),m_OldTreeViewFileName,SIZEOF_ARRAY(m_OldTreeViewFileName),SHGDN_FORPARSING);

	return FALSE;
}

int Explorerplusplus::OnTreeViewEndLabelEdit(LPARAM lParam)
{
	NMTVDISPINFO	*pdi = NULL;
	TCHAR			NewFileName[MAX_PATH];

	pdi = (NMTVDISPINFO *)lParam;

	/* No text was entered, so simply notify
	the control to revert to the previous text. */
	if(pdi->item.pszText == NULL)
		return FALSE;

	/* Build the new filename from the text entered
	and the parent directory component of the old
	filename. */
	StringCchCopy(NewFileName,SIZEOF_ARRAY(NewFileName),m_OldTreeViewFileName);
	PathRemoveFileSpec(NewFileName);
	BOOL bRes = PathAppend(NewFileName,pdi->item.pszText);

	if(!bRes)
	{
		return FALSE;
	}

	CFileActionHandler::RenamedItem_t RenamedItem;
	RenamedItem.strOldFilename = m_OldTreeViewFileName;
	RenamedItem.strNewFilename = NewFileName;

	TrimStringRight(RenamedItem.strNewFilename,_T(" "));

	std::list<CFileActionHandler::RenamedItem_t> RenamedItemList;
	RenamedItemList.push_back(RenamedItem);
	m_FileActionHandler.RenameFiles(RenamedItemList);

	return TRUE;
}

LRESULT Explorerplusplus::OnTreeViewKeyDown(LPARAM lParam)
{
	NMTVKEYDOWN	*nmtvkd = NULL;

	nmtvkd = (NMTVKEYDOWN *)lParam;

	switch(nmtvkd->wVKey)
	{
	case VK_DELETE:
		if(IsKeyDown(VK_SHIFT))
		{
			OnTreeViewFileDelete(TRUE);
		}
		else
		{
			OnTreeViewFileDelete(FALSE);
		}
		break;

	case 'C':
		if(IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			OnTreeViewCopy(TRUE);
		}
		break;

	case 'V':
		if(IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			OnTreeViewPaste();
		}
		break;

	case 'X':
		if(IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			OnTreeViewCopy(FALSE);
		}
		break;
	}

	/* If the ctrl key is down, this key sequence
	is likely a modifier. Stop any other pressed
	key from been used in an incremental search. */
	if(IsKeyDown(VK_CONTROL))
	{
		return 1;
	}

	return 0;
}

LRESULT CALLBACK TreeViewHolderProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TreeViewHolderProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderProc(HWND hwnd,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_NOTIFY:
		return TreeViewHolderWindowNotifyHandler(hwnd, msg, wParam, lParam);
		break;

	case WM_COMMAND:
		return TreeViewHolderWindowCommandHandler(wParam);
		break;

	case WM_TIMER:
		OnTreeViewHolderWindowTimer();
		break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderWindowNotifyHandler(HWND hwnd,
	UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(((LPNMHDR)lParam)->code)
	{
	case TVN_SELCHANGED:
		OnTreeViewSelChanged(lParam);
		break;

	case TVN_BEGINLABELEDIT:
		OnTreeViewBeginLabelEdit(lParam);
		break;

	case TVN_ENDLABELEDIT:
		/* TODO: Should return the value from this function. Can't do it
		at the moment, since the treeview looks items up by their label
		when a directory modification event is received (meaning that if
		the label changes, the lookup for the old file name will fail). */
		OnTreeViewEndLabelEdit(lParam);
		break;

	case TVN_KEYDOWN:
		return OnTreeViewKeyDown(lParam);
		break;

	case NM_RCLICK:
		{
			NMHDR *nmhdr = NULL;
			POINT ptCursor;
			DWORD dwPos;
			TVHITTESTINFO	tvht;

			nmhdr = (NMHDR *)lParam;

			if(nmhdr->hwndFrom == m_hTreeView)
			{
				dwPos = GetMessagePos();
				ptCursor.x = GET_X_LPARAM(dwPos);
				ptCursor.y = GET_Y_LPARAM(dwPos);

				tvht.pt = ptCursor;

				ScreenToClient(m_hTreeView,&tvht.pt);

				TreeView_HitTest(m_hTreeView,&tvht);

				if((tvht.flags & TVHT_NOWHERE) == 0)
				{
					OnTreeViewRightClick((WPARAM)tvht.hItem,(LPARAM)&ptCursor);
				}
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderWindowCommandHandler(WPARAM wParam)
{
	switch(LOWORD(wParam))
	{
	case FOLDERS_TOOLBAR_CLOSE:
		ToggleFolders();
		break;
	}

	return 1;
}

void Explorerplusplus::OnTreeViewSetFileAttributes(void) const
{
	HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem == NULL)
	{
		return;
	}

	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t> sfaiList;
	NSetFileAttributesDialogExternal::SetFileAttributesInfo_t sfai;

	auto pidlItem = m_pMyTreeView->GetItemPidl(hItem);
	HRESULT hr = GetDisplayName(pidlItem.get(),sfai.szFullFileName,SIZEOF_ARRAY(sfai.szFullFileName),SHGDN_FORPARSING);

	if(hr == S_OK)
	{
		HANDLE hFindFile = FindFirstFile(sfai.szFullFileName,&sfai.wfd);

		if(hFindFile != INVALID_HANDLE_VALUE)
		{
			FindClose(hFindFile);

			sfaiList.push_back(sfai);

			CSetFileAttributesDialog SetFileAttributesDialog(m_hLanguageModule, m_hContainer, sfaiList);

			SetFileAttributesDialog.ShowModalDialog();
		}
	}
}

void Explorerplusplus::OnTreeViewPaste(void)
{
	HTREEITEM hItem;
	TCHAR szFullFileName[MAX_PATH + 1];

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		IDataObject *pClipboardObject = NULL;

		HRESULT hr = OleGetClipboard(&pClipboardObject);

		if(hr == S_OK)
		{
			CDropHandler *pDropHandler = CDropHandler::CreateNew();

			auto pidl = m_pMyTreeView->GetItemPidl(hItem);

			GetDisplayName(pidl.get(),szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

			/* Name must be double NULL terminated. */
			szFullFileName[lstrlen(szFullFileName) + 1] = '\0';

			pDropHandler->CopyClipboardData(pClipboardObject,
				m_hTreeView,szFullFileName,NULL,
				!m_config->overwriteExistingFilesConfirmation);

			pDropHandler->Release();
			pClipboardObject->Release();
		}
	}
}

void Explorerplusplus::UpdateTreeViewSelection()
{
	HTREEITEM		hItem;
	TCHAR			szDirectory[MAX_PATH];
	TCHAR			szRoot[MAX_PATH];
	UINT			uDriveType;
	BOOL			bNetworkPath = FALSE;

	if (!m_InitializationFinished.get() || !m_config->synchronizeTreeview || !m_config->showFolders)
	{
		return;
	}

	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	GetDisplayName(pidlDirectory.get(),szDirectory,SIZEOF_ARRAY(szDirectory),SHGDN_FORPARSING);

	if(PathIsUNC(szDirectory))
	{
		bNetworkPath = TRUE;
	}
	else
	{
		StringCchCopy(szRoot,SIZEOF_ARRAY(szRoot),szDirectory);
		PathStripToRoot(szRoot);
		uDriveType = GetDriveType(szRoot);

		bNetworkPath = (uDriveType == DRIVE_REMOTE);
	}

	/* To improve performance, do not automatically sync the
	treeview with network or UNC paths. */
	if(!bNetworkPath)
	{
		hItem = m_pMyTreeView->LocateItem(pidlDirectory.get());

		if(hItem != NULL)
		{
			/* TVN_SELCHANGED is NOT sent when the new selected
			item is the same as the old selected item. It is only
			sent when the two are different.
			Therefore, the only case to handle is when the treeview
			selection is changed by browsing using the listview. */
			if(TreeView_GetSelection(m_hTreeView) != hItem)
				m_bSelectingTreeViewDirectory = TRUE;

			SendMessage(m_hTreeView,TVM_SELECTITEM,(WPARAM)TVGN_CARET,(LPARAM)hItem);
		}
	}
}