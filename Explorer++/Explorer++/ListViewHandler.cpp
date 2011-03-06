/******************************************************************
 *
 * Project: Explorer++
 * File: ListViewHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles messages asscoiated with the main
 * listview controls.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "SetFileAttributesDialog.h"
#include "MassRenameDialog.h"
#include "../Helper/DropHandler.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/ContextMenuManager.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/FileActionHandler.h"


LRESULT CALLBACK	ListViewSubclassProcStub(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK	ListViewEditProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

LRESULT	(CALLBACK *DefaultListViewProc)(HWND,UINT,WPARAM,LPARAM);

/*
 * Creates and then subclasses a new listview control.
 */
HWND Explorerplusplus::CreateAndSubclassListView(HWND hParent,DWORD Style)
{
	HWND hListView;
	DWORD dwExtendedStyle;
	IImageList *pImageList = NULL;

	hListView = CreateListView(hParent,Style);

	SHGetImageList(SHIL_SMALL,IID_IImageList,(void **)&pImageList);
	ListView_SetImageList(hListView,(HIMAGELIST)pImageList,LVSIL_SMALL);
	pImageList->Release();

	dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	/* If the user has selected to turn on full row
	select, add the style to the listview. */
	if(m_bUseFullRowSelect)
	{
		dwExtendedStyle |= LVS_EX_FULLROWSELECT;
	}

	if(m_bCheckBoxSelection)
	{
		dwExtendedStyle |= LVS_EX_CHECKBOXES;
	}

	ListView_SetExtendedListViewStyle(hListView,
		dwExtendedStyle);

	/* Set the listview to the Windows Explorer theme
	used in Windows Vista. */
	SetWindowTheme(hListView,L"Explorer",NULL);

	return hListView;
}

LRESULT CALLBACK ListViewSubclassProcStub(HWND ListView,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	ListViewInfo_t	*plvi = NULL;
	Explorerplusplus		*pContainer = NULL;

	plvi = (ListViewInfo_t *)GetWindowLongPtr(ListView,GWLP_USERDATA);

	pContainer = (Explorerplusplus *)plvi->pContainer;

	/* Jump across to the member window function (will handle all requests). */
	return pContainer->ListViewSubclassProc(ListView,msg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::ListViewSubclassProc(HWND ListView,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_MENUSELECT:
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;

		case WM_SETFOCUS:
			m_hLastActiveWindow = ListView;
			HandleToolbarItemStates();
			break;

		case WM_LBUTTONDOWN:
			return OnListViewLButtonDown(wParam,lParam);
			break;

		case WM_LBUTTONDBLCLK:
			{
				LV_HITTESTINFO	ht;
				DWORD			dwPos;
				POINT			MousePos;

				dwPos = GetMessagePos();
				MousePos.x = GET_X_LPARAM(dwPos);
				MousePos.y = GET_Y_LPARAM(dwPos);
				ScreenToClient(m_hActiveListView,&MousePos);

				ht.pt = MousePos;
				ListView_HitTest(ListView,&ht);

				/* NM_DBLCLK for the listview is sent both on double clicks
				(by default), as well as in the situation when LVS_EX_ONECLICKACTIVATE
				is active (in which case it is sent on a single mouse click).
				Therefore, because we only want to navigate up one folder on
				a DOUBLE click, we'll handle the event here. */
				if(ht.flags == LVHT_NOWHERE)
				{
					/* The user has double clicked in the whitepsace
					area for this tab, so go up one folder... */
					OnNavigateUp();
					return 0;
				}
			}
			break;

		case WM_RBUTTONDOWN:
			if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
				&& !(wParam & MK_MBUTTON))
			{
				LV_HITTESTINFO lvhti;

				lvhti.pt.x	= LOWORD(lParam);
				lvhti.pt.y	= HIWORD(lParam);

				/* Test to see if the mouse click was
				on an item or not. */
				ListView_HitTest(m_hActiveListView,&lvhti);

				HDC hdc;
				TCHAR szText[MAX_PATH];
				RECT rc;
				SIZE sz;
				UINT uViewMode;
				m_pFolderView[m_iObjectIndex]->GetCurrentViewMode(&uViewMode);
				if(uViewMode == VM_LIST)
				{
					if(!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
					{
						ListView_GetItemRect(m_hActiveListView,lvhti.iItem,&rc,LVIR_LABEL);
						ListView_GetItemText(m_hActiveListView,lvhti.iItem,0,szText,MAX_PATH);

						hdc = GetDC(m_hActiveListView);
						GetTextExtentPoint32(hdc,szText,lstrlen(szText),&sz);
						ReleaseDC(m_hActiveListView,hdc);

						rc.right = rc.left + sz.cx;

						if(!PtInRect(&rc,lvhti.pt))
						{
							m_bBlockNext = TRUE;
						}
					}
				}

				if(!(lvhti.flags & LVHT_NOWHERE))
				{
					m_bDragAllowed = TRUE;
				}
			}
			break;

		case WM_RBUTTONUP:
			m_bDragCancelled = FALSE;
			m_bDragAllowed = FALSE;

			m_bBlockNext = FALSE;
			break;

		case WM_MBUTTONDOWN:
			OnListViewMButtonDown(wParam,lParam);
			break;

		case WM_MBUTTONUP:
			OnListViewMButtonUp(wParam,lParam);
			break;

		/* If no item is currently been dragged, and the last drag
		has not just finished (i.e. item was dragged, but was cancelled
		with escape, but mouse button is still down), and when the right
		mouse button was clicked, it was over an item, start dragging. */
		case WM_MOUSEMOVE:
			{
				m_bBlockNext = FALSE;
				if(!m_bDragging && !m_bDragCancelled && m_bDragAllowed)
				{
					if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
						&& !(wParam & MK_MBUTTON))
					{
						NMLISTVIEW nmlv;
						POINT pt;
						DWORD dwPos;
						HRESULT hr;

						dwPos = GetMessagePos();
						pt.x = GET_X_LPARAM(dwPos);
						pt.y = GET_Y_LPARAM(dwPos);
						MapWindowPoints(HWND_DESKTOP,m_hActiveListView,&pt,1);

						LV_HITTESTINFO lvhti;

						lvhti.pt = pt;

						/* Test to see if the mouse click was
						on an item or not. */
						ListView_HitTest(m_hActiveListView,&lvhti);

						if(!(lvhti.flags & LVHT_NOWHERE) && ListView_GetSelectedCount(m_hActiveListView) > 0)
						{
							nmlv.iItem = 0;
							nmlv.ptAction = pt;

							hr = OnListViewBeginDrag((LPARAM)&nmlv,DRAG_TYPE_RIGHTCLICK);

							if(hr == DRAGDROP_S_CANCEL)
							{
								m_bDragCancelled = TRUE;
							}
						}
					}
				}
			}
			break;

		case WM_MOUSEWHEEL:
			if(OnMouseWheel(wParam,lParam))
				return 0;
			break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
			case HDN_BEGINDRAG:
				return FALSE;
				break;

			case HDN_ENDDRAG:
				{
					/* When the drag ends, the dragged item
					is shifted into position, and all later
					items are shifted down one. Therefore,
					take the item out of its position in the
					list, and move it into its new position. */
					NMHEADER *pnmHeader = NULL;
					list<Column_t> ActiveColumnList;
					list<Column_t>::iterator itr;
					Column_t Column;
					int i = 0;

					pnmHeader = (NMHEADER *)lParam;

					m_pActiveShellBrowser->ExportCurrentColumns(&ActiveColumnList);

					i = 0;
					itr = ActiveColumnList.begin();
					while(i < (pnmHeader->iItem + 1) && itr != ActiveColumnList.end())
					{
						if(itr->bChecked)
						{
							i++;
						}

						itr++;
					}

					if(itr != ActiveColumnList.begin())
						itr--;

					Column = *itr;
					ActiveColumnList.erase(itr);

					i = 0;
					itr = ActiveColumnList.begin();
					while(i < (pnmHeader->pitem->iOrder + 1) && itr != ActiveColumnList.end())
					{
						if(itr->bChecked)
						{
							i++;
						}

						itr++;
					}

					if(itr != ActiveColumnList.begin())
						itr--;

					ActiveColumnList.insert(itr,Column);

					m_pActiveShellBrowser->ImportColumns(&ActiveColumnList,TRUE);

					RefreshTab(m_iObjectIndex);

					return TRUE;
				}
				break;
			}
			break;
	}

	return CallWindowProc(DefaultListViewProc,ListView,msg,wParam,lParam);
}

LRESULT Explorerplusplus::OnListViewLButtonDown(WPARAM wParam,LPARAM lParam)
{
	LV_HITTESTINFO HitTestInfo;

	HitTestInfo.pt.x	= LOWORD(lParam);
	HitTestInfo.pt.y	= HIWORD(lParam);

	/* Test to see if the mouse click was
	on an item or not. */
	ListView_HitTest(m_hActiveListView,&HitTestInfo);

	/* If the mouse click was not on an item,
	then assume we are counting down the number
	of items selected. */
	if(HitTestInfo.flags == LVHT_NOWHERE)
	{
		m_bSelectionFromNowhere = TRUE;

		if(!(wParam & MK_CONTROL) && m_nSelected > 1)
			m_bCountingDown = TRUE;
	}
	else
	{
		m_bSelectionFromNowhere = FALSE;
	}

	return CallWindowProc(DefaultListViewProc,m_hActiveListView,WM_LBUTTONDOWN,wParam,lParam);
}

void Explorerplusplus::OnListViewMButtonDown(WPARAM wParam,LPARAM lParam)
{
	LV_HITTESTINFO ht;

	ht.pt.x = LOWORD(lParam);
	ht.pt.y = HIWORD(lParam);

	ListView_HitTest(m_hActiveListView,&ht);

	if(ht.flags != LVHT_NOWHERE && ht.iItem != -1)
	{
		m_ListViewMButtonItem = ht.iItem;

		ListView_SetItemState(m_hActiveListView,ht.iItem,LVIS_FOCUSED,LVIS_FOCUSED);
	}
	else
	{
		m_ListViewMButtonItem = -1;
	}
}

void Explorerplusplus::OnListViewMButtonUp(WPARAM wParam,LPARAM lParam)
{
	LV_HITTESTINFO	ht;

	ht.pt.x = LOWORD(lParam);
	ht.pt.y = HIWORD(lParam);

	ListView_HitTest(m_hActiveListView,&ht);

	if(ht.flags != LVHT_NOWHERE)
	{
		/* Only open an item if it was the one
		on which the middle mouse button was
		initially clicked on. */
		if(ht.iItem == m_ListViewMButtonItem)
		{
			IShellFolder *pDesktopFolder	= NULL;
			IShellFolder *pShellFolder		= NULL;
			LPITEMIDLIST pidl				= NULL;
			LPITEMIDLIST ridl				= NULL;
			SFGAOF uAttributes				= SFGAO_FOLDER | SFGAO_STREAM;
			TCHAR szParsingPath[MAX_PATH];
			STRRET str;
			HRESULT hr;

			hr = SHGetDesktopFolder(&pDesktopFolder);

			if(SUCCEEDED(hr))
			{
				pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
				hr = pDesktopFolder->BindToObject(pidl,NULL,IID_IShellFolder,(LPVOID *)&pShellFolder);

				if(!SUCCEEDED(hr))
				{
					hr = SHGetDesktopFolder(&pShellFolder);
				}

				if(SUCCEEDED(hr))
				{
					ridl = m_pActiveShellBrowser->QueryItemRelativeIdl(ht.iItem);

					hr = pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&ridl,&uAttributes);

					if(SUCCEEDED(hr))
					{
						if((uAttributes & SFGAO_FOLDER) &&
							!(uAttributes & SFGAO_STREAM))
						{
							/* Folder item. */
							pShellFolder->GetDisplayNameOf(ridl,SHGDN_FORPARSING,&str);
							StrRetToBuf(&str,ridl,szParsingPath,MAX_PATH);

							BrowseFolder(szParsingPath,SBSP_ABSOLUTE,TRUE,FALSE,FALSE);
						}
					}

					CoTaskMemFree(ridl);
					pShellFolder->Release();
				}
				CoTaskMemFree(pidl);
				pDesktopFolder->Release();
			}
		}
	}
}

LRESULT Explorerplusplus::OnListViewKeyDown(LPARAM lParam)
{
	LV_KEYDOWN	*lv_key = NULL;

	lv_key = (LV_KEYDOWN *)lParam;

	switch(lv_key->wVKey)
	{
		case VK_RETURN:
			if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
			{
				/* Key press: Ctrl+Enter
				Action: Open item in background tab. */
				OpenAllSelectedItems(TRUE);
			}
			else
			{
				OpenAllSelectedItems(FALSE);
			}
			break;

		case VK_DELETE:
			if(GetKeyState(VK_SHIFT) & 0x80)
				OnListViewFileDelete(TRUE);
			else
				OnListViewFileDelete(FALSE);
			break;

		case VK_BACK:
			if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
			{
				LPITEMIDLIST pidl = NULL;
				TCHAR szRoot[MAX_PATH];

				pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

				GetDisplayName(pidl,szRoot,SHGDN_FORPARSING);
				PathStripToRoot(szRoot);

				/* Go to the root of this directory. */
				BrowseFolder(szRoot,
					SBSP_ABSOLUTE|SBSP_SAMEBROWSER);

				CoTaskMemFree(pidl);
			}
			else
			{
				OnNavigateUp();
			}
			break;

		case 'A':
			if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
			{
				m_bCountingUp = TRUE;
				ListView_SelectAllItems(m_hActiveListView);
				SetFocus(m_hActiveListView);
			}
			break;

		case 'C':
			if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
				OnListViewCopy(TRUE);
			break;

		case 'I':
			if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
			{
				m_bInverted = TRUE;
				m_nSelectedOnInvert = m_nSelected;
				ListView_InvertSelection(m_hActiveListView);
				SetFocus(m_hActiveListView);
			}
			break;

		case 'V':
			if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
				OnListViewPaste();
			break;

		case 'X':
			if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
				OnListViewCopy(FALSE);
			break;
	}

	return 0;
}

void Explorerplusplus::OnListViewItemChanged(LPARAM lParam)
{
	NMLISTVIEW	*ItemChanged = NULL;
	int			iObjectIndex;
	BOOL		Selected;

	ItemChanged = (NM_LISTVIEW FAR *)lParam;

	iObjectIndex = DetermineListViewObjectIndex(ItemChanged->hdr.hwndFrom);

	if(iObjectIndex == -1)
		return;

	if(m_pShellBrowser[iObjectIndex]->QueryDragging())
		return;

	if(ItemChanged->uChanged == LVIF_STATE &&
		((LVIS_STATEIMAGEMASK & ItemChanged->uNewState) >> 12) != 0 &&
		((LVIS_STATEIMAGEMASK & ItemChanged->uOldState) >> 12) != 0)
	{
		if(ListView_GetCheckState(m_hListView[iObjectIndex],ItemChanged->iItem))
		{
			ListView_SelectItem(m_hListView[iObjectIndex],ItemChanged->iItem,TRUE);
		}
		else
		{
			ListView_SelectItem(m_hListView[iObjectIndex],ItemChanged->iItem,FALSE);
		}

		return;
	}

	if((ItemChanged->uNewState & LVIS_SELECTED) &&
	(ItemChanged->uOldState & LVIS_SELECTED))
		return;

	/* Only proceed if an item was selected or deselected. */
	if(ItemChanged->uNewState & LVIS_SELECTED)
		Selected = TRUE;
	else if(ItemChanged->uOldState & LVIS_SELECTED)
		Selected  = FALSE;
	else
		return;

	if(Selected)
	{
		if(ListView_GetCheckState(m_hListView[iObjectIndex],ItemChanged->iItem) == 0)
			ListView_SetCheckState(m_hListView[iObjectIndex],ItemChanged->iItem,TRUE);
	}
	else
	{
		if(ListView_GetCheckState(m_hListView[iObjectIndex],ItemChanged->iItem) != 0)
			ListView_SetCheckState(m_hListView[iObjectIndex],ItemChanged->iItem,FALSE);
	}

	/* The selection for this tab has changed, so invalidate any
	folder size calculations that are occurring for this tab
	(applies only to folder sizes that will be shown in the display
	window). */
	list<DWFolderSize_t>::iterator itr;

	for(itr = m_DWFolderSizes.begin();itr != m_DWFolderSizes.end();itr++)
	{
		if(itr->iTabId == iObjectIndex)
		{
			itr->bValid = FALSE;
		}
	}

	/* Only update internal selection info
	if the listview that sent the change
	notification is active. */
	if(iObjectIndex == m_iObjectIndex)
	{
		if(Selected)
		{
			m_nSelected++;

			if(m_nSelected == ListView_GetItemCount(m_hActiveListView))
				m_bCountingUp = FALSE;
		}
		else
		{
			m_nSelected--;

			if(m_nSelected <= 1)
				m_bCountingDown = FALSE;
		}
	}

	m_pShellBrowser[iObjectIndex]->UpdateFileSelectionInfo(
	(int)ItemChanged->lParam,Selected);

	if((ListView_GetItemCount(m_hActiveListView) - m_nSelected) == m_nSelectedOnInvert)
		m_bInverted = FALSE;

	if(m_bCountingUp || m_bCountingDown || m_bInverted)
		return;

	HandleFileSelectionDisplay();
	HandleStatusText();
	HandleToolbarItemStates();
}

int Explorerplusplus::DetermineListViewObjectIndex(HWND hListView)
{
	ListViewInfo_t	*plvi = NULL;

	plvi = (ListViewInfo_t *)GetWindowLongPtr(hListView,GWLP_USERDATA);

	if(plvi->iObjectIndex < MAX_TABS &&
		m_uTabMap[plvi->iObjectIndex] == 1)
		return plvi->iObjectIndex;

	return -1;
}

LRESULT CALLBACK ListViewEditProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->ListViewEditProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::ListViewEditProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	/* Can't pick up tab and return keys properly
	(blocked by edit control using WM_GETDLGMESSAGE).
	So, send custom message instead. */
	case WM_USER_KEYDOWN:
		m_bListViewBeginRename = FALSE;
		switch(wParam)
		{
		case VK_F2:
			{
				/* Cycle between:
				- Selecting filename (no extension)
				- Selecting whole name
				- Selecting only extension */

				TCHAR szFileName[MAX_PATH];
				DWORD dwAttributes;
				BOOL bExtensionFound = FALSE;
				BOOL bSelectionSet = FALSE;
				int i;

				SendMessage(hwnd,WM_GETTEXT,
					(WPARAM)SIZEOF_ARRAY(szFileName),
					(LPARAM)szFileName);

				dwAttributes = m_pActiveShellBrowser->QueryFileAttributes(m_iItemEditing);

				if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
					FILE_ATTRIBUTE_DIRECTORY)
				{
					for(i = lstrlen(szFileName) - 1;i >= 0;i--)
					{
						if(szFileName[i] == '.')
						{
							bExtensionFound = TRUE;
							break;
						}
					}

					if(bExtensionFound)
					{
						if(m_ListViewEditingStage == LISTVIEW_RENAME_FILENAME)
						{
							SendMessage(hwnd,EM_SETSEL,i + 1,lstrlen(szFileName));
							m_ListViewEditingStage = LISTVIEW_RENAME_EXTENSION;
						}
						else if(m_ListViewEditingStage == LISTVIEW_RENAME_EXTENSION)
						{
							m_ListViewEditingStage = LISTVIEW_RENAME_ENTIRE;
							SendMessage(hwnd,EM_SETSEL,0,-1);
						}
						else if(m_ListViewEditingStage == LISTVIEW_RENAME_ENTIRE)
						{
							m_ListViewEditingStage = LISTVIEW_RENAME_FILENAME;

							SendMessage(hwnd,EM_SETSEL,0,i);
						}
					}


					if(m_ListViewEditingStage == LISTVIEW_RENAME_ENTIRE || !bSelectionSet)
					{

					}
				}
			}
			break;

		case VK_TAB:
			{
				int iSel;
				int iNewSel;
				int nItems;

				iSel = ListView_GetNextItem(GetParent(hwnd),-1,LVNI_ALL|LVNI_SELECTED);
				ListView_SelectItem(GetParent(hwnd),iSel,FALSE);

				nItems = ListView_GetItemCount(GetParent(hwnd));

				if(iSel == (nItems - 1))
					iNewSel = 0;
				else
					iNewSel = iSel + 1;

				ListView_EditLabel(GetParent(hwnd),iNewSel);

				return 0;
			}
			break;
		}
		break;

	case EM_SETSEL:
		{
			if(m_bListViewBeginRename)
			{
				TCHAR	szFileName[MAX_PATH];
				DWORD	dwAttributes;
				BOOL	bExtensionFound = FALSE;
				int		i;

				SendMessage(hwnd,WM_GETTEXT,
					(WPARAM)SIZEOF_ARRAY(szFileName),
					(LPARAM)szFileName);

				dwAttributes = m_pActiveShellBrowser->QueryFileAttributes(m_iItemEditing);

				if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
					FILE_ATTRIBUTE_DIRECTORY)
				{
					for(i = lstrlen(szFileName) - 1;i >= 0;i--)
					{
						if(szFileName[i] == '.')
						{
							bExtensionFound = TRUE;
							break;
						}
					}

					/* Select all text up to the '.' character
					before the extension. */
					/* wParam represents the lower bound; lParam
					the upper bound. Modify them so that all the
					items text is selected, except for any extension. */

					if(bExtensionFound)
					{
						wParam = 0;
						lParam = i;
					}
				}

				//m_bListViewBeginRename = FALSE;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

BOOL Explorerplusplus::OnListViewBeginLabelEdit(LPARAM lParam)
{
	HWND			hEdit;
	NMLVDISPINFO	*pnmdi = NULL;

	if(!IsRenamePossible())
		return TRUE;

	pnmdi = (NMLVDISPINFO *)lParam;

	hEdit = ListView_GetEditControl(m_hActiveListView);

	/* Subclass the edit window. The only reason this is
	done is so that when the listview item is put into
	edit mode, any extension the file has will not be
	selected along with the rest of the text. Although
	selection works directly from here in Windows Vista,
	it does not work in Windows XP. */
	SetWindowSubclass(hEdit,ListViewEditProcStub,0,(DWORD_PTR)this);

	m_ListViewEditingStage = LISTVIEW_RENAME_FILENAME;
	m_iItemEditing = pnmdi->item.iItem;

	m_bListViewRenaming = TRUE;
	m_bListViewBeginRename = TRUE;

	return FALSE;
}

BOOL Explorerplusplus::OnListViewEndLabelEdit(LPARAM lParam)
{
	NMLVDISPINFO	*pdi = NULL;
	LVITEM			*pItem = NULL;
	TCHAR			NewFileName[MAX_PATH + 1];
	TCHAR			OldFileName[MAX_PATH + 1];
	TCHAR			CurrentDirectory[MAX_PATH];
	TCHAR			OldName[MAX_PATH];
	TCHAR			szTemp[128];
	TCHAR			szError[256];
	TCHAR			szTitle[256];
	DWORD			dwAttributes;
	int				ret;

	pdi = (NMLVDISPINFO *) lParam;
	pItem = &pdi->item;

	m_bListViewRenaming = FALSE;

	/* Did the user cancel the editing? */
	if(pItem->pszText == NULL)
		return FALSE;

	/* Is the new filename empty? */
	if(lstrcmp(pItem->pszText,EMPTY_STRING) == 0)
		return FALSE;

	/*
	Deny file names ending with a dot, as they are just
	synonyms for the same file without any dot(s).
	For example:
	C:\Hello.txt
	C:\Hello.txt....
	refer to exactly the same file.

	Also, do not allow filenames to end with a space.
	
	Taken from the web site referenced below:
	"Do not end a file or directory name with a trailing
	space or a period. Although the underlying file system
	may support such names, the operating system does not.
	However, it is acceptable to start a name with a period."	
	*/
	if(pItem->pszText[lstrlen(pItem->pszText) - 1] == '.' ||
		pItem->pszText[lstrlen(pItem->pszText) - 1] == ' ')
		return FALSE;

	/*
	The following characters are NOT allowed
	within a file name:
	\/:*?"<>|

	See: http://msdn.microsoft.com/en-us/library/aa365247.aspx
	*/
	if(StrChr(pItem->pszText,'\\') != NULL ||
		StrChr(pItem->pszText,'/') != NULL ||
		StrChr(pItem->pszText,':') != NULL ||
		StrChr(pItem->pszText,'*') != NULL ||
		StrChr(pItem->pszText,'?') != NULL ||
		StrChr(pItem->pszText,'"') != NULL ||
		StrChr(pItem->pszText,'<') != NULL ||
		StrChr(pItem->pszText,'>') != NULL ||
		StrChr(pItem->pszText,'|') != NULL)
	{
		LoadString(g_hLanguageModule,IDS_ERR_FILENAMEINVALID,
			szError,SIZEOF_ARRAY(szError));
		LoadString(g_hLanguageModule,IDS_ERR_FILENAMEINVALID_MSGTITLE,
			szTitle,SIZEOF_ARRAY(szTitle));

		MessageBox(m_hContainer,szError,szTitle,MB_ICONERROR);

		return 0;
	}

	m_pActiveShellBrowser->QueryCurrentDirectory(MAX_PATH,CurrentDirectory);
	StringCchCopy(NewFileName,SIZEOF_ARRAY(NewFileName),CurrentDirectory);
	StringCchCopy(OldFileName,SIZEOF_ARRAY(OldFileName),CurrentDirectory);

	m_pActiveShellBrowser->QueryName(pItem->iItem,
	OldName);
	PathAppend(OldFileName,OldName);

	PathAppend(NewFileName,pItem->pszText);

	dwAttributes = m_pActiveShellBrowser->QueryFileAttributes(pItem->iItem);

	if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
	{
		BOOL bExtensionHidden = FALSE;

		bExtensionHidden = (!m_bShowExtensionsGlobal) ||
			(m_bHideLinkExtensionGlobal && lstrcmpi(PathFindExtension(OldName),_T(".lnk")) == 0);

		/* If file extensions are turned off, the new filename
		will be incorrect (i.e. it will be missing the extension).
		Therefore, append the extension manually if it is turned
		off. */
		if(bExtensionHidden)
		{
			TCHAR	*szExt = NULL;

			szExt = PathFindExtension(OldName);

			if(*szExt == '.')
				StringCchCat(NewFileName,SIZEOF_ARRAY(NewFileName),szExt);
		}
	}

	CFileActionHandler::RenamedItem_t RenamedItem;
	RenamedItem.strOldFilename = OldFileName;
	RenamedItem.strNewFilename = NewFileName;

	std::list<CFileActionHandler::RenamedItem_t> RenamedItemList;
	RenamedItemList.push_back(RenamedItem);
	ret = m_FileActionHandler.RenameFiles(RenamedItemList);

	/* If the file was not renamed, show an error message. */
	if(!ret)
	{
		LoadString(g_hLanguageModule,IDS_FILERENAMEERROR,szTemp,
		SIZEOF_ARRAY(szTemp));

		MessageBox(m_hContainer,szTemp,NExplorerplusplus::WINDOW_NAME,
			MB_ICONWARNING|MB_OK);
	}

	return ret;
}

/*
 * Called when information (icon number, text) is required
 * for an item in one of the listview controls.
 */
void Explorerplusplus::OnListViewGetDisplayInfo(LPARAM lParam)
{
	NMLVDISPINFO	*pnmv = NULL;
	LVITEM			*plvItem = NULL;
	NMHDR			*nmhdr = NULL;
	int				iIndex = 0;
	TCITEM			tcItem;
	int				nTabsProcessed = 0;
	int				nTabs;

	pnmv = (NMLVDISPINFO *)lParam;

	plvItem = &pnmv->item;
	nmhdr = &pnmv->hdr;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	/* Find the tab asscoiated with this call. */
	while((nmhdr->hwndFrom != m_hListView[iIndex])  && nTabsProcessed < nTabs)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,nTabsProcessed,&tcItem);

		iIndex = (int)tcItem.lParam;

		nTabsProcessed++;
	}

	m_pShellBrowser[iIndex]->OnListViewGetDisplayInfo(lParam);

	return;
}

/*
 * Called when a column is clicked in the main listview.
 */
void Explorerplusplus::OnListViewColumnClick(LPARAM lParam)
{
	NMLISTVIEW *pnmlv = NULL;

	pnmlv = (NMLISTVIEW *)lParam;

	m_pActiveShellBrowser->ColumnClicked(pnmlv->iSubItem);

	return;
}

/*
 * Called when info tip text is required for an item
 * in the main listview control.
 */
void Explorerplusplus::OnListViewGetInfoTip(LPARAM lParam)
{
	LPNMLVGETINFOTIP	pGetInfoTip	= NULL;
	TCHAR				szInfoTip[512];

	pGetInfoTip = (LPNMLVGETINFOTIP)lParam;
	
	/* The pszText member of pGetInfoTip will contain the text of the
	item if its name is truncated in the listview. Always concatenate
	the rest of the info tip onto the name if it is there. */
	if(m_bShowInfoTips)
	{
		CreateFileInfoTip(pGetInfoTip->iItem,szInfoTip,SIZEOF_ARRAY(szInfoTip));

		if(lstrlen(pGetInfoTip->pszText) > 0)
			StringCchCat(pGetInfoTip->pszText,pGetInfoTip->cchTextMax,_T("\n"));

		StringCchCat(pGetInfoTip->pszText,pGetInfoTip->cchTextMax,
			szInfoTip);
	}
	else
	{
		StringCchCopy(pGetInfoTip->pszText,pGetInfoTip->cchTextMax,
			EMPTY_STRING);
	}
}

void Explorerplusplus::CreateFileInfoTip(int iItem,TCHAR *szInfoTip,UINT cchMax)
{
	HRESULT	hr;

	/* Use Explorer infotips if the option is selected, or this is a
	virtual folder. Otherwise, show the modified date. */
	if((m_InfoTipType == INFOTIP_SYSTEM) || m_pActiveShellBrowser->InVirtualFolder())
	{
		LPITEMIDLIST	pidlDirectory = NULL;
		LPITEMIDLIST	pridlItem = NULL;

		pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
		pridlItem = m_pActiveShellBrowser->QueryItemRelativeIdl(iItem);

		hr = GetFileInfoTip(m_hContainer,pidlDirectory,const_cast<LPCITEMIDLIST *>(&pridlItem),
			szInfoTip,cchMax);

		if(!SUCCEEDED(hr))
			StringCchCopy(szInfoTip,cchMax,EMPTY_STRING);

		CoTaskMemFree(pidlDirectory);
		CoTaskMemFree(pridlItem);
	}
	else
	{
		WIN32_FIND_DATA	*pwfd = NULL;
		TCHAR			szDate[256];
		TCHAR			szDateModified[256];

		pwfd = m_pActiveShellBrowser->QueryFileFindData(iItem);

		CreateFileTimeString(&pwfd->ftLastWriteTime,
			szDateModified,SIZEOF_ARRAY(szDateModified),m_bShowFriendlyDatesGlobal);

		LoadString(g_hLanguageModule,IDS_GENERAL_DATEMODIFIED,szDate,
				SIZEOF_ARRAY(szDate));

		StringCchPrintf(szInfoTip,cchMax,_T("%s: %s"),
			szDate,szDateModified);
	}
}

void Explorerplusplus::OnListViewRClick(HWND hParent,POINT *pCursorPos)
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
	menu inccorectly).
	Due to the possibility of unforseen problems,
	this function should NOT access the current
	shell browser once the menu has been destroyed. */

	SetForegroundWindow(m_hContainer);

	if((GetKeyState(VK_SHIFT) & 0x80) &&
		!(GetKeyState(VK_CONTROL) & 0x80) &&
		!(GetKeyState(VK_MENU) & 0x80))
	{
		LVHITTESTINFO lvhti;

		lvhti.pt = *pCursorPos;
		ScreenToClient(m_hActiveListView,&lvhti.pt);
		ListView_HitTest(m_hActiveListView,&lvhti);

		if(!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
		{
			if(ListView_GetItemState(m_hActiveListView,lvhti.iItem,LVIS_SELECTED) !=
				LVIS_SELECTED)
			{
				ListView_DeselectAllItems(m_hActiveListView);
				ListView_SelectItem(m_hActiveListView,lvhti.iItem,TRUE);
			}
		}
	}

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
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
	HMENU hMenu = InitializeRightClickMenu();
	LPITEMIDLIST pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	CContextMenuManager ccm(CMT_DIRECTORY_BACKGROUND_HANDLERS,pidlDirectory,
		NULL,reinterpret_cast<IUnknown *>(this));

	ccm.AddMenuEntries(hMenu,11,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID);

	int iCmd = TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RIGHTBUTTON|
		TPM_VERTICAL|TPM_RETURNCMD,pCursorPos->x,pCursorPos->y,0,
		m_hContainer,NULL);

	if(iCmd >= MIN_SHELL_MENU_ID && iCmd <= MAX_SHELL_MENU_ID)
	{
		ccm.HandleMenuEntry(m_hContainer,iCmd);
	}
	else
	{
		SendMessage(m_hContainer,WM_COMMAND,MAKEWPARAM(iCmd,0),NULL);
	}

	CoTaskMemFree(pidlDirectory);
	DestroyMenu(hMenu);
}

/* TODO: */
HMENU Explorerplusplus::InitializeRightClickMenu(void)
{
	HMENU hMenu;
	MENUITEMINFO mii;

	hMenu = GetSubMenu(LoadMenu(g_hLanguageModule,
		MAKEINTRESOURCE(IDR_MAINMENU_RCLICK)),0);

	mii.cbSize	= sizeof(mii);
	mii.fMask	= MIIM_SUBMENU;
	mii.hSubMenu	= m_hArrangeSubMenu;
	SetMenuItemInfo(hMenu,IDM_POPUP_SORTBY,FALSE,&mii);

	mii.cbSize	= sizeof(mii);
	mii.fMask	= MIIM_SUBMENU;
	mii.hSubMenu	= m_hGroupBySubMenu;
	SetMenuItemInfo(hMenu,IDM_POPUP_GROUPBY,FALSE,&mii);

	/*if(uViewMode == VM_DETAILS)
	{
		lEnableMenuItem(hMenu,IDM_POPUP_GROUPBY,TRUE);
	}
	else if(uViewMode == VM_LIST)
	{
		lEnableMenuItem(hMenu,IDM_POPUP_GROUPBY,FALSE);
	}
	else
	{
		lEnableMenuItem(hMenu,IDM_POPUP_GROUPBY,TRUE);
	}*/

	//// Initialization.cpp - InitializeMenus().
	//InsertMenuItem(m_hRightClickMenu,IDM_VIEW_PLACEHOLDER,FALSE,&mii);
	//DeleteMenu(m_hRightClickMenu,IDM_VIEW_PLACEHOLDER,MF_BYCOMMAND);

	//SetMenuItemBitmap(m_hRightClickMenu,IDM_VIEW_REFRESH,SHELLIMAGES_REFRESH);
	//SetMenuItemBitmap(m_hRightClickMenu,IDM_EDIT_PASTE,SHELLIMAGES_PASTE);
	//SetMenuItemBitmap(m_hRightClickMenu,IDM_EDIT_PASTESHORTCUT,SHELLIMAGES_PASTESHORTCUT);
	//SetMenuItemBitmap(m_hRightClickMenu,IDM_BOOKMARKS_BOOKMARKTHISTAB,SHELLIMAGES_ADDFAV);
	//SetMenuItemBitmap(m_hRightClickMenu,IDM_RCLICK_PROPERTIES,SHELLIMAGES_PROPERTIES);

	return hMenu;
}

void Explorerplusplus::OnListViewItemRClick(POINT *pCursorPos)
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected > 0)
	{
		list<LPITEMIDLIST> pidlList;
		int iItem = -1;

		while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
		{
			pidlList.push_back(m_pActiveShellBrowser->QueryItemRelativeIdl(iItem));
		}

		LPITEMIDLIST pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

		CFileContextMenuManager fcmm(m_hActiveListView,pidlDirectory,
			pidlList);

		FileContextMenuInfo_t fcmi;
		fcmi.uFrom = FROM_LISTVIEW;

		CStatusBar StatusBar(m_hStatusBar);

		fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,pCursorPos,&StatusBar,
			reinterpret_cast<DWORD_PTR>(&fcmi),TRUE,GetKeyState(VK_SHIFT) & 0x80);

		CoTaskMemFree(pidlDirectory);

		for each(auto pidl in pidlList)
		{
			CoTaskMemFree(pidl);
		}
	}
}

void Explorerplusplus::OnListViewHeaderRClick(POINT *pCursorPos)
{
	HMENU						hHeaderPopupMenu;
	HMENU						hMenu;
	MENUITEMINFO				mii;
	list<Column_t>				m_pActiveColumnList;
	list<Column_t>::iterator	itr;
	TCHAR						szColumnText[256];
	unsigned int				*pHeaderList = NULL;
	int							nItems = 0;
	int							iItem = 0;
	int							i = 0;

	hHeaderPopupMenu = LoadMenu(g_hLanguageModule,MAKEINTRESOURCE(IDR_HEADER_MENU));

	hMenu = GetSubMenu(hHeaderPopupMenu,0);

	m_pActiveShellBrowser->ExportCurrentColumns(&m_pActiveColumnList);

	nItems = GetColumnHeaderMenuList(&pHeaderList);

	for(i = 0;i < nItems;i++)
	{
		for(itr = m_pActiveColumnList.begin();itr != m_pActiveColumnList.end();itr++)
		{
			if(itr->id == pHeaderList[i])
			{
				LoadString(g_hLanguageModule,LookupColumnNameStringIndex(itr->id),
					szColumnText,SIZEOF_ARRAY(szColumnText));

				if(itr->bChecked)
					mii.fState	= MFS_CHECKED;
				else
					mii.fState	= MFS_ENABLED;

				/* Build a list of the current columns, and add them
				to the menu. */
				mii.cbSize			= sizeof(mii);
				mii.fMask			= MIIM_STRING|MIIM_STATE|MIIM_ID;
				mii.dwTypeData		= szColumnText;
				mii.wID				= MENU_HEADER_STARTID + iItem;
				InsertMenuItem(hMenu,iItem,TRUE,&mii);

				iItem++;
				break;
			}
		}
	}

	SetMenuOwnerDraw(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RIGHTBUTTON|TPM_VERTICAL,
		pCursorPos->x,pCursorPos->y,0,m_hContainer,NULL);

	mii.cbSize	= sizeof(mii);
	mii.fMask	= MIIM_DATA;
	GetMenuItemInfo(hHeaderPopupMenu,0,TRUE,&mii);

	free((CustomMenuInfo_t *)mii.dwItemData);

	DestroyMenu(hHeaderPopupMenu);
}

int Explorerplusplus::GetColumnHeaderMenuList(unsigned int **pHeaderList)
{
	int nItems;

	if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		*pHeaderList = g_MyComputerHeaderList;
		nItems = sizeof(g_MyComputerHeaderList) / sizeof(g_MyComputerHeaderList[0]);
	}
	else if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		*pHeaderList = g_ControlPanelHeaderList;
		nItems = sizeof(g_ControlPanelHeaderList) / sizeof(g_ControlPanelHeaderList[0]);
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		*pHeaderList = g_RecycleBinHeaderList;
		nItems = sizeof(g_RecycleBinHeaderList) / sizeof(g_RecycleBinHeaderList[0]);
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		*pHeaderList = g_NetworkConnectionsHeaderList;
		nItems = sizeof(g_NetworkConnectionsHeaderList) / sizeof(g_NetworkConnectionsHeaderList[0]);
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
	{
		*pHeaderList = g_NetworkHeaderList;
		nItems = sizeof(g_NetworkHeaderList) / sizeof(g_NetworkHeaderList[0]);
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		*pHeaderList = g_PrintersHeaderList;
		nItems = sizeof(g_PrintersHeaderList) / sizeof(g_PrintersHeaderList[0]);
	}
	else
	{
		*pHeaderList = g_RealFolderHeaderList;
		nItems = sizeof(g_RealFolderHeaderList) / sizeof(g_RealFolderHeaderList[0]);
	}

	return nItems;
}

HRESULT Explorerplusplus::OnListViewBeginDrag(LPARAM lParam,DragTypes_t DragType)
{
	IDropSource			*pDropSource = NULL;
	IDragSourceHelper	*pDragSourceHelper = NULL;
	NMLISTVIEW			*pnmlv = NULL;
	POINT				pt = {0,0};
	HRESULT				hr;
	int					iDragStartObjectIndex;

	pnmlv = (NMLISTVIEW *)lParam;

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return E_FAIL;
	}

	LPITEMIDLIST pidlDirectory = NULL;
	list<LPITEMIDLIST> ItemList;
	list<std::wstring> FilenameList;
	int iItem = -1;

	/* Store the pidl of the current folder, as well as the relative
	pidl's of the dragged items. */
	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		ItemList.push_back(m_pActiveShellBrowser->QueryItemRelativeIdl(iItem));

		TCHAR szFullFilename[MAX_PATH];

		m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename);

		std::wstring stringFilename(szFullFilename);

		FilenameList.push_back(stringFilename);
	}

	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_ALL,
		IID_IDragSourceHelper,(LPVOID *)&pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = CreateDropSource(&pDropSource,DragType);

		if(SUCCEEDED(hr))
		{
			FORMATETC ftc[2];
			STGMEDIUM stg[2];

			/* We'll export two formats:
			CF_HDROP
			CFSTR_SHELLIDLIST */
			BuildHDropList(&ftc[0],&stg[0],FilenameList);
			BuildShellIDList(&ftc[1],&stg[1],pidlDirectory,ItemList);

			IDataObject *pDataObject = NULL;
			IAsyncOperation *pAsyncOperation = NULL;
			
			hr = CreateDataObject(ftc,stg,&pDataObject,2);
			pDataObject->QueryInterface(IID_IAsyncOperation,(void **)&pAsyncOperation);

			assert(pAsyncOperation != NULL);

			/* Docs mention setting the argument to VARIANT_TRUE/VARIANT_FALSE.
			But the argument is a BOOL, so we'll go with regular TRUE/FALSE. */
			pAsyncOperation->SetAsyncMode(TRUE);

			hr = pDragSourceHelper->InitializeFromWindow(m_hActiveListView,&pt,pDataObject);

			m_pActiveShellBrowser->DragStarted(pnmlv->iItem,&pnmlv->ptAction);
			m_bDragging = TRUE;

			/* Need to remember which tab started the drag (as
			it may be different from the tab in which the drag
			finishes). */
			iDragStartObjectIndex = m_iObjectIndex;

			DWORD dwEffect;

			hr = DoDragDrop(pDataObject,pDropSource,DROPEFFECT_COPY|DROPEFFECT_MOVE|
				DROPEFFECT_LINK,&dwEffect);

			m_bDragging = FALSE;

			/* The object that starts any drag may NOT be the
			object that stops it (i.e. when a file is dragged
			between tabs). Therefore, need to tell the object
			that STARTED dragging that dragging has stopped. */
			m_pShellBrowser[iDragStartObjectIndex]->DragStopped();

			BOOL bInAsyncOp;

			hr = pAsyncOperation->InOperation(&bInAsyncOp);

			if(!SUCCEEDED(hr) ||
				bInAsyncOp == FALSE)
			{
				/* Data extraction has finished synchronously.
				SetAsyncMode() calls AddRef(), so we'll need to
				balance that out here. */
				pAsyncOperation->Release();
			}

			pAsyncOperation->Release();
			pDataObject->Release();
			pDropSource->Release();
		}

		pDragSourceHelper->Release();
	}

	for each(auto pidl in ItemList)
	{
		CoTaskMemFree(pidl);
	}

	CoTaskMemFree(pidlDirectory);

	return hr;
}

void Explorerplusplus::OnListViewFileDelete(BOOL bPermanent)
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected == 0)
	{
		return;
	}

	std::list<std::wstring> FullFilenameList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename);
		FullFilenameList.push_back(szFullFilename);
	}

	m_FileActionHandler.DeleteFiles(m_hContainer,FullFilenameList,bPermanent);
}

void Explorerplusplus::OnListViewDoubleClick(NMHDR *nmhdr)
{
	if(nmhdr->hwndFrom == m_hActiveListView)
	{
		LV_HITTESTINFO	ht;
		DWORD			dwPos;
		POINT			MousePos;

		dwPos = GetMessagePos();
		MousePos.x = GET_X_LPARAM(dwPos);
		MousePos.y = GET_Y_LPARAM(dwPos);
		ScreenToClient(m_hActiveListView,&MousePos);

		ht.pt = MousePos;
		ListView_HitTest(m_hActiveListView,&ht);

		if(ht.flags != LVHT_NOWHERE && ht.iItem != -1)
		{
			short AltKey = GetKeyState(VK_MENU);
			short ControlKey = GetKeyState(VK_CONTROL);
			short ShiftKey = GetKeyState(VK_SHIFT);

			if(AltKey & 0x8000)
			{
				LPITEMIDLIST pidlDirectory = NULL;
				LPITEMIDLIST pidl = NULL;

				pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
				pidl = m_pActiveShellBrowser->QueryItemRelativeIdl(ht.iItem);

				ShowMultipleFileProperties(pidlDirectory,(LPCITEMIDLIST *)&pidl,1);

				CoTaskMemFree(pidl);
				CoTaskMemFree(pidlDirectory);
			}
			else if(ControlKey & 0x8000)
			{
				/* Open the item in a new tab. */
				OpenListViewItem(ht.iItem,TRUE,FALSE);
			}
			else if(ShiftKey & 0x8000)
			{
				/* Open the item in a new window. */
				OpenListViewItem(ht.iItem,FALSE,TRUE);
			}
			else
			{
				OpenListViewItem(ht.iItem,FALSE,FALSE);
			}
		}
	}
}

void Explorerplusplus::OnListViewFileRename(void)
{
	if(m_pActiveShellBrowser->InVirtualFolder())
	{
		return;
	}

	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	/* If there is only item selected, start editing
	it in-place. If multiple items are selected,
	show the mass rename dialog. */
	if(nSelected == 1)
	{
		int iSelected = ListView_GetNextItem(m_hActiveListView,
			-1,LVNI_SELECTED|LVNI_FOCUSED);

		if(iSelected != -1)
		{
			/* Start editing the label for this item. */
			ListView_EditLabel(m_hActiveListView,iSelected);
		}
	}
	else if(nSelected > 1)
	{
		std::list<std::wstring>	FullFilenameList;
		TCHAR szFullFilename[MAX_PATH];
		int iIndex = -1;

		for(int i = 0;i < nSelected;i++)
		{
			iIndex = ListView_GetNextItem(m_hActiveListView,
				iIndex,LVNI_SELECTED);

			if(iIndex != -1)
			{
				m_pActiveShellBrowser->QueryFullItemName(iIndex,szFullFilename);
				FullFilenameList.push_back(szFullFilename);
			}
		}

		CMassRenameDialog CMassRenameDialog(g_hLanguageModule,IDD_MASSRENAME,
			m_hContainer,FullFilenameList,&m_FileActionHandler);
		CMassRenameDialog.ShowModalDialog();
	}
}

void Explorerplusplus::OnListViewShowFileProperties(void)
{
	LPITEMIDLIST	*ppidl = NULL;
	LPITEMIDLIST	pidlDirectory = NULL;
	int				iItem;
	int				nSelected;
	int				i = 0;

	nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected == 0)
	{
		ppidl = NULL;
	}
	else
	{
		ppidl = (LPITEMIDLIST *)malloc(nSelected * sizeof(LPCITEMIDLIST));

		iItem = -1;

		while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
		{
			ppidl[i] = m_pActiveShellBrowser->QueryItemRelativeIdl(iItem);

			i++;
		}
	}

	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
	ShowMultipleFileProperties(pidlDirectory,(LPCITEMIDLIST *)ppidl,nSelected);
	CoTaskMemFree(pidlDirectory);

	for(i = 0;i < nSelected;i++)
	{
		CoTaskMemFree(ppidl[i]);
	}

	free(ppidl);
}

void Explorerplusplus::OnListViewCopyItemPath(void)
{
	IBufferManager	*pBufferManager = NULL;
	TCHAR			FullFileName[MAX_PATH];
	TCHAR			*szFullFileNameText = NULL;
	DWORD			dwBufferSize;
	int				iSelected = -1;

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
		return;

	iSelected = ListView_GetNextItem(m_hActiveListView,-1,LVNI_SELECTED);

	pBufferManager = new CBufferManager();

	while(iSelected != -1)
	{
		m_pActiveShellBrowser->QueryFullItemName(iSelected,FullFileName);

		pBufferManager->WriteLine(FullFileName);

		iSelected = ListView_GetNextItem(m_hActiveListView,iSelected,LVNI_SELECTED);
	}

	pBufferManager->QueryBufferSize(&dwBufferSize);

	szFullFileNameText = (TCHAR *)malloc(dwBufferSize * sizeof(TCHAR));

	if(szFullFileNameText != NULL)
	{
		pBufferManager->QueryBuffer(szFullFileNameText,dwBufferSize);

		CopyTextToClipboard(szFullFileNameText);
	}

	pBufferManager->Release();
}

void Explorerplusplus::OnListViewCopyUniversalPaths(void)
{
	IBufferManager	*pBufferManager = NULL;
	TCHAR			FullFileName[MAX_PATH];
	TCHAR			*szFullFileNameText = NULL;
	DWORD			dwBufferSize;
	DWORD			dwRet;
	int				iSelected = -1;

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
		return;

	iSelected = ListView_GetNextItem(m_hActiveListView,-1,LVNI_SELECTED);

	pBufferManager = new CBufferManager();

	while(iSelected != -1)
	{
		m_pActiveShellBrowser->QueryFullItemName(iSelected,FullFileName);

		UNIVERSAL_NAME_INFO	*puni;
		TCHAR szBuffer[1024];

		dwBufferSize = SIZEOF_ARRAY(szBuffer);
		puni = (UNIVERSAL_NAME_INFO *)&szBuffer;
		dwRet = WNetGetUniversalName(FullFileName,UNIVERSAL_NAME_INFO_LEVEL,
			(LPVOID)puni,&dwBufferSize);

		if(dwRet == NO_ERROR)
			pBufferManager->WriteLine(puni->lpUniversalName);
		else
			pBufferManager->WriteLine(FullFileName);

		iSelected = ListView_GetNextItem(m_hActiveListView,iSelected,LVNI_SELECTED);
	}

	pBufferManager->QueryBufferSize(&dwBufferSize);

	szFullFileNameText = (TCHAR *)malloc(dwBufferSize * sizeof(TCHAR));

	if(szFullFileNameText != NULL)
	{
		pBufferManager->QueryBuffer(szFullFileNameText,dwBufferSize);

		CopyTextToClipboard(szFullFileNameText);
	}

	pBufferManager->Release();
}

HRESULT Explorerplusplus::OnListViewCopy(BOOL bCopy)
{
	IDataObject		*pClipboardDataObject = NULL;
	int				iItem = -1;
	HRESULT			hr;

	if(!CanCutOrCopySelection())
		return E_FAIL;

	SetCursor(LoadCursor(NULL,IDC_WAIT));

	std::list<std::wstring> FileNameList;

	BuildListViewFileSelectionList(m_hActiveListView,&FileNameList);

	if(bCopy)
	{
		hr = CopyFiles(FileNameList,&pClipboardDataObject);

		if(SUCCEEDED(hr))
		{
			m_pClipboardDataObject = pClipboardDataObject;
		}
	}
	else
	{
		hr = CutFiles(FileNameList,&pClipboardDataObject);

		if(SUCCEEDED(hr))
		{
			m_pClipboardDataObject = pClipboardDataObject;
			m_iCutTabInternal = m_iObjectIndex;

			TCHAR szFilename[MAX_PATH];

			/* 'Ghost' each of the cut items. */
			while((iItem = ListView_GetNextItem(m_hActiveListView,
				iItem,LVNI_SELECTED)) != -1)
			{
				m_pActiveShellBrowser->QueryDisplayName(iItem,SIZEOF_ARRAY(szFilename),
					szFilename);
				m_CutFileNameList.push_back(szFilename);

				m_pActiveShellBrowser->GhostItem(iItem);
			}
		}
	}

	SetCursor(LoadCursor(NULL,IDC_ARROW));

	return hr;
}

void Explorerplusplus::OnListViewSetFileAttributes(void)
{
	if(ListView_GetSelectedCount(m_hActiveListView) > 0)
	{
		int iSel = -1;

		std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t> sfaiList;

		while((iSel = ListView_GetNextItem(m_hActiveListView,
			iSel,LVNI_SELECTED)) != -1)
		{
			NSetFileAttributesDialogExternal::SetFileAttributesInfo_t sfai;

			m_pActiveShellBrowser->QueryFullItemName(iSel,sfai.szFullFileName);

			WIN32_FIND_DATA *pwfd = m_pActiveShellBrowser->QueryFileFindData(iSel);
			sfai.wfd = *pwfd;

			sfaiList.push_back(sfai);
		}

		CSetFileAttributesDialog SetFileAttributesDialog(g_hLanguageModule,
			IDD_SETFILEATTRIBUTES,m_hContainer,sfaiList);

		SetFileAttributesDialog.ShowModalDialog();
	}
}

void Explorerplusplus::OnListViewPaste(void)
{
	IDataObject *pClipboardObject = NULL;
	HRESULT hr;

	hr = OleGetClipboard(&pClipboardObject);

	if(hr == S_OK)
	{
		IClipboardHandler *pClipboardHandler = new CDropHandler();

		TCHAR szDestination[MAX_PATH + 1];

		/* DO NOT use the internal current directory string.
		Files are copied asynchronously, so a change of directory
		will cause the destination directory to change in the
		middle of the copy operation. */
		StringCchCopy(szDestination,SIZEOF_ARRAY(szDestination),
			m_CurrentDirectory);

		/* Also, the string must be double NULL terminated. */
		szDestination[lstrlen(szDestination) + 1] = '\0';

		pClipboardHandler->CopyClipboardData(pClipboardObject,
			m_hContainer,szDestination,this,!m_bOverwriteExistingFilesConfirmation);

		pClipboardHandler->Release();
		pClipboardObject->Release();
	}
}

void Explorerplusplus::OnDropFile(list<PastedFile_t> *ppfl,POINT *ppt)
{
	if(m_pActiveShellBrowser->QueryNumSelected() == 0)
	{
		m_pActiveShellBrowser->SelectItems(ppfl);
	}
}

void Explorerplusplus::BuildListViewFileSelectionList(HWND hListView,
	std::list<std::wstring> *pFileSelectionList)
{
	if(pFileSelectionList == NULL)
	{
		return;
	}

	std::list<std::wstring> FileSelectionList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(hListView,
		iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFileName[MAX_PATH];

		m_pActiveShellBrowser->QueryFullItemName(iItem,
			szFullFileName);

		std::wstring stringFileName(szFullFileName);
		FileSelectionList.push_back(stringFileName);
	}

	pFileSelectionList->assign(FileSelectionList.begin(),
		FileSelectionList.end());
}