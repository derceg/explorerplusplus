/******************************************************************
 *
 * Project: Explorer++
 * File: EvenSwitcher.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Switches events based on the currently selected window
 * (principally the listview and treeview).
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"

void CContainer::OnCopyItemPath(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewCopyItemPath();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewCopyItemPath();
	}
}

void CContainer::OnCopyUniversalPaths(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewCopyUniversalPaths();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewCopyUniversalPaths();
	}
}

void CContainer::OnCopy(BOOL bCopy)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewCopy(bCopy);
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewCopy(bCopy);
	}
}

void CContainer::OnFileRename(void)
{
	HWND	hFocus;

	if(m_bListViewRenaming)
	{
		SendMessage(ListView_GetEditControl(m_hActiveListView),
			WM_USER_KEYDOWN,VK_F2,0);
	}
	else
	{
		hFocus = GetFocus();

		if(hFocus == m_hActiveListView)
		{
			OnListViewFileRename();
		}
		else if(hFocus == m_hTreeView)
		{
			OnTreeViewFileRename();
		}
	}
}

void CContainer::OnFileDelete(BOOL bPermanent)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewFileDelete(bPermanent);
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewFileDelete(bPermanent);
	}
}

void CContainer::OnSetFileAttributes(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewSetFileAttributes();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewSetFileAttributes();
	}
}

void CContainer::OnShowFileProperties(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewShowFileProperties();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewShowFileProperties();
	}
}

void CContainer::OnRightClick(NMHDR *nmhdr)
{
	if(nmhdr->hwndFrom == m_hActiveListView)
	{
		POINT CursorPos;
		DWORD dwPos;

		dwPos = GetMessagePos();
		CursorPos.x = GET_X_LPARAM(dwPos);
		CursorPos.y = GET_Y_LPARAM(dwPos);

		OnListViewRClick(m_hActiveListView,&CursorPos);
	}
	else if(nmhdr->hwndFrom == ListView_GetHeader(m_hActiveListView))
	{
		/* The header on the active listview was right-clicked. */
		POINT CursorPos;
		DWORD dwPos;

		dwPos = GetMessagePos();
		CursorPos.x = GET_X_LPARAM(dwPos);
		CursorPos.y = GET_Y_LPARAM(dwPos);

		OnListViewHeaderRClick(&CursorPos);
	}
	else if(nmhdr->hwndFrom == m_hMainToolbar)
	{
		OnMainToolbarRClick();
	}
}

void CContainer::OnPaste(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewPaste();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewPaste();
	}
}

/* This will be called once files have been pasted from the background
thread (provided the paste actually succeeded). */
void PasteFilesCallback(void *pData,list<PastedFile_t> *pPastedFileList)
{
	CContainer *pContainer = NULL;

	pContainer = (CContainer *)pData;

	pContainer->PasteFilesCallbackInternal(pPastedFileList);
}

void CContainer::PasteFilesCallbackInternal(list<PastedFile_t> *pPastedFileList)
{
	/* If the files were pasted successfully, and the tab currently
	does not have any selected files, then select the pasted files. */
	if(m_pActiveShellBrowser->QueryNumSelected() == 0)
	{
		m_pActiveShellBrowser->SelectItems(pPastedFileList);
	}
}