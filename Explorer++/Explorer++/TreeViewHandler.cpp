/******************************************************************
 *
 * Project: Explorer++
 * File: TreeViewHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles messages asscoiated with the main
 * treeview control.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "SetFileAttributesDialog.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FileContextMenuManager.h"


#define TREEVIEW_FOLDER_OPEN_DELAY	500

/* Used to keep track of which item was selected in
the treeview control. */
HTREEITEM	g_NewSelectionItem;

LRESULT CALLBACK TreeViewSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TreeViewSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewSubclass(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_SETFOCUS:
		HandleToolbarItemStates();
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
					BrowseFolder(pidl,SBSP_ABSOLUTE,TRUE,FALSE,FALSE);

					CoTaskMemFree(pidl);
				}
			}
		}
		break;

	case WM_MOUSEWHEEL:
		if(OnMouseWheel(wParam,lParam))
			return 0;
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

		hr = ExecuteActionFromContextMenu(pidl,NULL,0,_T("delete"),fMask);

		CoTaskMemFree(pidl);
	}
}

void Explorerplusplus::OnTreeViewFileDeletePermanent(void)
{
	HTREEITEM hItem;
	LPITEMIDLIST pidl	= NULL;
	TCHAR szPath[MAX_PATH + 1];

	hItem = TreeView_GetSelection(m_hTreeView);

	pidl = m_pMyTreeView->BuildPath(hItem);

	GetDisplayName(pidl,szPath,SHGDN_FORPARSING);

	szPath[lstrlen(szPath) + 1] = '\0';

	/* TODO: */
	//DeleteFilesPermanently(m_hTreeView,szPath);

	CoTaskMemFree(pidl);
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

	hr = SHBindToParent(pidl,IID_IShellFolder,(void **)&pShellParentFolder,
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

				list<LPITEMIDLIST> pidlList;

				pidlList.push_back(pidlRelative);

				CFileContextMenuManager fcmm(m_hContainer,pidlParent,
					pidlList);

				FileContextMenuInfo_t fcmi;
				fcmi.uFrom = FROM_TREEVIEW;

				CStatusBar StatusBar(m_hStatusBar);

				fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,ppt,&StatusBar,
					reinterpret_cast<DWORD_PTR>(&fcmi),TRUE,GetKeyState(VK_SHIFT) & 0x80);

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
void Explorerplusplus::OnTreeViewShowFileProperties(void)
{
	LPITEMIDLIST	pidlDirectory = NULL;
	HTREEITEM		hItem;

	hItem = TreeView_GetSelection(m_hTreeView);

	/* Get the path of the currently selected item. */
	pidlDirectory = m_pMyTreeView->BuildPath(hItem);

	ShowMultipleFileProperties(pidlDirectory,NULL,0);

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
				BrowseFolder(pidl,SBSP_SAMEBROWSER);

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

void Explorerplusplus::OnTreeViewCopyItemPath(void)
{
	HTREEITEM		hItem;
	LPITEMIDLIST	pidl;
	TCHAR			szFullFileName[MAX_PATH];

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		pidl = m_pMyTreeView->BuildPath(hItem);

		GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);

		CopyTextToClipboard(szFullFileName);

		CoTaskMemFree(pidl);
	}
}

void Explorerplusplus::OnTreeViewCopyUniversalPaths(void)
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

		GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);

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

		GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);

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
				m_iCutTabInternal = m_iObjectIndex;

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
		BrowseFolder(pidlDirectory,SBSP_SAMEBROWSER);

		if(m_bTVAutoExpandSelected)
		{
			TreeView_Expand(m_hTreeView,g_NewSelectionItem,TVE_EXPAND);
		}
	}

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

			if(m_bTreeViewDelayEnabled)
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
	GetDisplayName(pidl,m_OldTreeViewFileName,SHGDN_FORPARSING);

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
	PathAppend(NewFileName,pdi->item.pszText);

	CFileActionHandler::RenamedItem_t RenamedItem;
	RenamedItem.strOldFilename = m_OldTreeViewFileName;
	RenamedItem.strNewFilename = NewFileName;

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
		if(GetKeyState(VK_SHIFT) & 0x80)
			OnTreeViewFileDelete(TRUE);
		else
			OnTreeViewFileDelete(FALSE);
		break;

	case 'C':
		if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
			OnTreeViewCopy(TRUE);
		break;

	case 'V':
		if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
			OnTreeViewPaste();
		break;

	case 'X':
		if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
			OnTreeViewCopy(FALSE);
		break;
	}

	/* If the ctrl key is down, this key sequence
	is likely a modifier. Stop any other pressed
	key from been used in an incremental search. */
	if(GetKeyState(VK_CONTROL) & 0x80)
		return 1;

	return 0;
}

LRESULT CALLBACK TreeViewHolderProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TreeViewHolderProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderProc(HWND hwnd,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_NOTIFY:
		return TreeViewHolderWindowNotifyHandler(lParam);
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

LRESULT CALLBACK Explorerplusplus::TreeViewHolderWindowNotifyHandler(LPARAM lParam)
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

	return 0;
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

void Explorerplusplus::OnTreeViewSetFileAttributes(void)
{
	HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem == NULL)
	{
		return;
	}

	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t> sfaiList;
	NSetFileAttributesDialogExternal::SetFileAttributesInfo_t sfai;

	LPITEMIDLIST pidlItem = m_pMyTreeView->BuildPath(hItem);
	HRESULT hr = GetDisplayName(pidlItem,sfai.szFullFileName,SHGDN_FORPARSING);
	CoTaskMemFree(pidlItem);

	if(hr == S_OK)
	{
		HANDLE hFindFile = FindFirstFile(sfai.szFullFileName,&sfai.wfd);

		if(hFindFile != INVALID_HANDLE_VALUE)
		{
			FindClose(hFindFile);

			sfaiList.push_back(sfai);

			CSetFileAttributesDialog SetFileAttributesDialog(g_hLanguageModule,
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
			IClipboardHandler *pClipboardHandler = NULL;

			pClipboardHandler = new CDropHandler();

			pidl = m_pMyTreeView->BuildPath(hItem);

			assert(pidl != NULL);

			GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);

			/* Name must be double NULL terminated. */
			szFullFileName[lstrlen(szFullFileName) + 1] = '\0';

			pClipboardHandler->CopyClipboardData(pClipboardObject,
				m_hTreeView,szFullFileName,NULL,
				!m_bOverwriteExistingFilesConfirmation);

			CoTaskMemFree(pidl);

			pClipboardHandler->Release();
			pClipboardObject->Release();
		}
	}
}