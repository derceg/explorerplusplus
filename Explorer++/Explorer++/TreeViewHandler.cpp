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
	m_pMyTreeView = new CMyTreeView(m_hTreeView,m_hContainer,m_pDirMon,m_hTreeViewIconThread);

	/* Now, subclass the treeview again. This is needed for messages
	such as WM_MOUSEWHEEL, which need to be intercepted before they
	reach the window procedure provided by CMyTreeView. */
	SetWindowSubclass(m_hTreeView,TreeViewSubclassStub,1,(DWORD_PTR)this);

	LoadString(m_hLanguageModule,IDS_HIDEFOLDERSPANE,szTemp,SIZEOF_ARRAY(szTemp));
	m_hFoldersToolbar = CreateTabToolbar(m_hHolder,FOLDERS_TOOLBAR_CLOSE,szTemp);

	m_tabContainer->tabCreatedSignal.AddObserver([this] (int tabId, BOOL switchToNewTab) {
		UNREFERENCED_PARAMETER(tabId);
		UNREFERENCED_PARAMETER(switchToNewTab);

		UpdateTreeViewSelection();
	});

	m_navigation->navigationCompletedSignal.AddObserver([this] (const Tab &tab) {
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
			LPITEMIDLIST pidl = NULL;

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
					pidl = m_pMyTreeView->BuildPath(tvhi.hItem);
					m_tabContainer->CreateNewTab(pidl);

					CoTaskMemFree(pidl);
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
	LPITEMIDLIST	pidl = NULL;
	DWORD			fMask = 0;
	HRESULT			hr;

	hItem		= TreeView_GetSelection(m_hTreeView);
	hParentItem = TreeView_GetParent(m_hTreeView,hItem); 

	// Select the parent item to release the lock and allow deletion
	TreeView_Select(m_hTreeView,hParentItem,TVGN_CARET);

	if(hItem != NULL)
	{
		pidl = m_pMyTreeView->BuildPath(hItem);

		if(bPermanent)
		{
			fMask = CMIC_MASK_SHIFT_DOWN;
		}

		hr = ExecuteActionFromContextMenu(pidl,NULL,m_hContainer,0,_T("delete"),fMask);

		CoTaskMemFree(pidl);
	}
}

void Explorerplusplus::OnTreeViewRightClick(WPARAM wParam,LPARAM lParam)
{
	LPITEMIDLIST pidl = NULL;
	POINT *ppt = NULL;
	HTREEITEM hItem;
	HTREEITEM hPrevItem;
	IShellFolder *pShellParentFolder = NULL;
	LPITEMIDLIST pidlRelative = NULL;
	HRESULT hr;

	hItem	= (HTREEITEM)wParam;
	ppt		= (POINT *)lParam;

	m_bTreeViewRightClick = TRUE;

	hPrevItem = TreeView_GetSelection(m_hTreeView);
	TreeView_SelectItem(m_hTreeView,hItem);
	pidl = m_pMyTreeView->BuildPath(hItem);

	hr = SHBindToParent(pidl, IID_PPV_ARGS(&pShellParentFolder),
	(LPCITEMIDLIST *)&pidlRelative);

	if(SUCCEEDED(hr))
	{
		HTREEITEM hParent;
		LPITEMIDLIST pidlParent	= NULL;

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
			pidlParent = m_pMyTreeView->BuildPath(hParent);

			if(pidlParent != NULL)
			{
				m_bTreeViewOpenInNewTab = FALSE;

				std::list<LPITEMIDLIST> pidlList;

				pidlList.push_back(pidlRelative);

				CFileContextMenuManager fcmm(m_hContainer,pidlParent,
					pidlList);

				FileContextMenuInfo_t fcmi;
				fcmi.uFrom = FROM_TREEVIEW;

				CStatusBar StatusBar(m_hStatusBar);

				fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,ppt,&StatusBar,
					reinterpret_cast<DWORD_PTR>(&fcmi),TRUE,IsKeyDown(VK_SHIFT));

				CoTaskMemFree(pidlParent);
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

	CoTaskMemFree(pidl);
}

/*
 * Shows the properties dialog for the currently
 * selected treeview item.
 */
void Explorerplusplus::OnTreeViewShowFileProperties(void) const
{
	LPITEMIDLIST	pidlDirectory = NULL;
	HTREEITEM		hItem;

	hItem = TreeView_GetSelection(m_hTreeView);

	/* Get the path of the currently selected item. */
	pidlDirectory = m_pMyTreeView->BuildPath(hItem);

	ShowMultipleFileProperties(pidlDirectory,NULL, m_hContainer,0);

	CoTaskMemFree(pidlDirectory);
}

BOOL Explorerplusplus::OnTreeViewItemExpanding(LPARAM lParam)
{
	NMTREEVIEW *pnmtv;
	TVITEM *tvItem;
	HTREEITEM *pItem;
	NMHDR *nmhdr;

	nmhdr = (NMHDR *)lParam;

	pnmtv = (LPNMTREEVIEW)lParam;

	tvItem = &pnmtv->itemNew;

	pItem = &tvItem->hItem;

	if(TreeView_GetParent(nmhdr->hwndFrom,*pItem) == NULL)
	{
		return FALSE;
	}

	if(pnmtv->action == TVE_EXPAND)
	{
		LPITEMIDLIST pidl	= NULL;

		pidl = m_pMyTreeView->BuildPath(tvItem->hItem);
		m_pMyTreeView->AddDirectory(tvItem->hItem,pidl);

		CoTaskMemFree(pidl);
	}
	else
	{
		HTREEITEM hSelection = TreeView_GetSelection(m_hTreeView);

		if(hSelection != NULL)
		{
			/* We may collapse multiple levels (not just the parent folder), so we need 
			to search up the tree for the parent item. */
			HTREEITEM hItem = hSelection;

			do 
			{
				hItem = TreeView_GetParent(m_hTreeView,hItem);
			} while (hItem != tvItem->hItem && hItem != NULL);

			if(hItem == tvItem->hItem)
			{
				LPITEMIDLIST pidl	= NULL;

				pidl = m_pMyTreeView->BuildPath(tvItem->hItem);
				m_navigation->BrowseFolderInCurrentTab(pidl,0);

				CoTaskMemFree(pidl);
			}
		}

		m_pMyTreeView->EraseItems(tvItem->hItem);

		SendMessage(nmhdr->hwndFrom,TVM_EXPAND,
		(WPARAM)TVE_COLLAPSE|TVE_COLLAPSERESET,
		(LPARAM)tvItem->hItem);
	}

	return FALSE;
}

void Explorerplusplus::OnTreeViewCopyItemPath(void) const
{
	HTREEITEM		hItem;
	LPITEMIDLIST	pidl;
	TCHAR			szFullFileName[MAX_PATH];

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		pidl = m_pMyTreeView->BuildPath(hItem);

		GetDisplayName(pidl,szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

		CopyTextToClipboard(szFullFileName);

		CoTaskMemFree(pidl);
	}
}

void Explorerplusplus::OnTreeViewCopyUniversalPaths(void) const
{
	HTREEITEM		hItem;
	LPITEMIDLIST	pidl;
	TCHAR			szFullFileName[MAX_PATH];
	UNIVERSAL_NAME_INFO	uni;
	DWORD			dwBufferSize;
	DWORD			dwRet;

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		pidl = m_pMyTreeView->BuildPath(hItem);

		GetDisplayName(pidl,szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

		dwBufferSize = sizeof(uni);
		dwRet = WNetGetUniversalName(szFullFileName,UNIVERSAL_NAME_INFO_LEVEL,
			(void **)&uni,&dwBufferSize);

		if(dwRet == NO_ERROR)
			CopyTextToClipboard(uni.lpUniversalName);
		else
			CopyTextToClipboard(szFullFileName);

		CoTaskMemFree(pidl);
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
		LPITEMIDLIST pidl = m_pMyTreeView->BuildPath(hItem);

		std::list<std::wstring> FileNameList;
		TCHAR szFullFileName[MAX_PATH];

		GetDisplayName(pidl,szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

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

		CoTaskMemFree(pidl);
	}
}

void Explorerplusplus::OnTreeViewHolderWindowTimer(void)
{
	LPITEMIDLIST	pidlDirectory = NULL;
	LPITEMIDLIST	pidlCurrentDirectory = NULL;

	pidlDirectory = m_pMyTreeView->BuildPath(g_NewSelectionItem);
	pidlCurrentDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	if(!m_bSelectingTreeViewDirectory && !m_bTreeViewRightClick &&
		!CompareIdls(pidlDirectory,pidlCurrentDirectory))
	{
		m_navigation->BrowseFolderInCurrentTab(pidlDirectory,0);

		if(m_config->treeViewAutoExpandSelected)
		{
			TreeView_Expand(m_hTreeView,g_NewSelectionItem,TVE_EXPAND);
		}
	}

	CoTaskMemFree(pidlCurrentDirectory);
	CoTaskMemFree(pidlDirectory);

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
	NMTVDISPINFO *pdi	= NULL;
	LPITEMIDLIST pidl	= NULL;

	pdi = (NMTVDISPINFO *)lParam;

	pidl = m_pMyTreeView->BuildPath(pdi->item.hItem);

	/* Save the old filename, in the case that the file
	needs to be renamed. */
	GetDisplayName(pidl,m_OldTreeViewFileName,SIZEOF_ARRAY(m_OldTreeViewFileName),SHGDN_FORPARSING);

	CoTaskMemFree(pidl);

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
	case TVN_ITEMEXPANDING:
		return OnTreeViewItemExpanding(lParam);
		break;

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

	case TVN_BEGINDRAG:
		/* Forward the message to the treeview for it to handle. */
		SendMessage(m_hTreeView,WM_NOTIFY,0,lParam);
		break;

	case TVN_GETDISPINFO:
		SendMessage(m_hTreeView,WM_NOTIFY,0,lParam);
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

	LPITEMIDLIST pidlItem = m_pMyTreeView->BuildPath(hItem);
	HRESULT hr = GetDisplayName(pidlItem,sfai.szFullFileName,SIZEOF_ARRAY(sfai.szFullFileName),SHGDN_FORPARSING);
	CoTaskMemFree(pidlItem);

	if(hr == S_OK)
	{
		HANDLE hFindFile = FindFirstFile(sfai.szFullFileName,&sfai.wfd);

		if(hFindFile != INVALID_HANDLE_VALUE)
		{
			FindClose(hFindFile);

			sfaiList.push_back(sfai);

			CSetFileAttributesDialog SetFileAttributesDialog(m_hLanguageModule,
				IDD_SETFILEATTRIBUTES,m_hContainer,sfaiList);

			SetFileAttributesDialog.ShowModalDialog();
		}
	}
}

void Explorerplusplus::OnTreeViewPaste(void)
{
	HTREEITEM hItem;
	LPITEMIDLIST pidl = NULL;
	TCHAR szFullFileName[MAX_PATH + 1];

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		IDataObject *pClipboardObject = NULL;

		HRESULT hr = OleGetClipboard(&pClipboardObject);

		if(hr == S_OK)
		{
			CDropHandler *pDropHandler = CDropHandler::CreateNew();

			pidl = m_pMyTreeView->BuildPath(hItem);

			assert(pidl != NULL);

			GetDisplayName(pidl,szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);

			/* Name must be double NULL terminated. */
			szFullFileName[lstrlen(szFullFileName) + 1] = '\0';

			pDropHandler->CopyClipboardData(pClipboardObject,
				m_hTreeView,szFullFileName,NULL,
				!m_config->overwriteExistingFilesConfirmation);

			CoTaskMemFree(pidl);

			pDropHandler->Release();
			pClipboardObject->Release();
		}
	}
}

void Explorerplusplus::UpdateTreeViewSelection(void)
{
	HTREEITEM		hItem;
	LPITEMIDLIST	pidlDirectory = NULL;
	TCHAR			szDirectory[MAX_PATH];
	TCHAR			szRoot[MAX_PATH];
	UINT			uDriveType;
	BOOL			bNetworkPath = FALSE;

	if(!m_config->synchronizeTreeview)
	{
		return;
	}

	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	GetDisplayName(pidlDirectory,szDirectory,SIZEOF_ARRAY(szDirectory),SHGDN_FORPARSING);

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
		hItem = m_pMyTreeView->LocateItem(pidlDirectory);

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

	CoTaskMemFree(pidlDirectory);
}