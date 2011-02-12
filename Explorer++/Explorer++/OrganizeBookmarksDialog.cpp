/******************************************************************
 *
 * Project: Explorer++
 * File: OrganizeBookmarksDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the bookmarking of tabs.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"


#define DEFAULT_NAMECOLUMN_WIDTH		100
#define DEFAULT_LOCATIONCOLUMN_WIDTH	220
#define DEFAULT_DESCRIPTIONCOLUMN_WIDTH	180

/* This will be set to TRUE when a bookmarks
properties are updated, and wil cause the
bookmarks menu and toolbar to be correspondingly
updated. */
BOOL g_bBookmarksAltered = FALSE;

HMENU	g_hBookmarkMenu;
int		g_iMenuItem;

INT_PTR CALLBACK OrganizeBookmarksStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				pContainer = (Explorerplusplus *)lParam;
			}
			break;
	}

	return pContainer->OrganizeBookmarks(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::OrganizeBookmarks(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnOrganizeBookmarksInit(hDlg);
			return FALSE;
			break;

		case WM_INITMENU:
			OnOrganizeBookmarksInitMenu(hDlg,wParam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_ORGANIZE_MOVEUP:
					OrganizeBookmarksMove(hDlg,TRUE);
					break;

				case IDC_ORGANIZE_MOVEDOWN:
					OrganizeBookmarksMove(hDlg,FALSE);
					break;

				case IDM_BOOKMARK_OPEN:
					OnOrganizeBookmarksOpen(hDlg,FALSE);
					break;

				case IDM_BOOKMARK_OPENINNEWTAB:
					OnOrganizeBookmarksOpen(hDlg,TRUE);
					break;

				case IDM_BOOKMARK_DELETE:
				case IDC_ORGANIZE_DELETE:
					OnOrganizeBookmarksDelete(hDlg);
					break;

				case IDM_BOOKMARK_PROPERTIES:
				case IDC_ORGANIZE_PROPERTIES:
					OnOrganizeBookmarksProperties(hDlg);
					break;

				case IDM_BOOKMARK_SHOWONTOOLBAR:
					OnOrganizeBookmarksShowOnToolbar(hDlg);
					break;

				case IDOK:
					OnOrganizeBookmarksOk(hDlg);
					break;

				case IDCANCEL:
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
			case NM_DBLCLK:
				OnOrganizeBookmarksDoubleClick(hDlg,lParam);
				break;

			case NM_RCLICK:
				OnOrganizeBookmarksRightClick(hDlg,lParam);
				break;

			case TVN_SELCHANGED:
				OnOrganizeBookmarksTvnSelChanged(hDlg,lParam);
				break;
			}
			break;

		case WM_CLOSE:
			OrganizeBookmarksSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return FALSE;
}

void Explorerplusplus::OnOrganizeBookmarksInit(HWND hDlg)
{
	HWND						hListView;
	LVCOLUMN					col;
	TCHAR						szTemp[32];

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	ListView_SetExtendedListViewStyleEx(hListView,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hListView,LVS_EX_LABELTIP,LVS_EX_LABELTIP);

	LoadString(g_hLanguageModule,IDS_BOOKMARKDLG_NAME,szTemp,SIZEOF_ARRAY(szTemp));

	col.mask	= LVCF_TEXT;
	col.pszText	= szTemp;
	ListView_InsertColumn(hListView,0,&col);

	LoadString(g_hLanguageModule,IDS_BOOKMARKDLG_LOCATION,szTemp,SIZEOF_ARRAY(szTemp));

	col.mask	= LVCF_TEXT;
	col.pszText	= szTemp;
	ListView_InsertColumn(hListView,1,&col);

	LoadString(g_hLanguageModule,IDS_BOOKMARKDLG_DESCRIPTION,szTemp,SIZEOF_ARRAY(szTemp));

	col.mask	= LVCF_TEXT;
	col.pszText	= szTemp;
	ListView_InsertColumn(hListView,2,&col);

	
	HWND			hFolders;
	HIMAGELIST		himl;
	HBITMAP			hb;

	hFolders = GetDlgItem(hDlg,IDC_ORGANIZE_FOLDERS);

	himl = ImageList_Create(16,16,ILC_COLOR32 | ILC_MASK,0,1);
	hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hb,NULL);
	DeleteObject(hb);
	SendMessage(hFolders,TVM_SETIMAGELIST,TVSIL_NORMAL,(LPARAM)himl);

	Bookmark_t RootBookmark;

	m_Bookmark.GetRoot(&RootBookmark);

	InsertBookmarkFolderItemsIntoTreeView(hFolders,NULL,&RootBookmark);

	/* Expand the root node in the tree. */
	SendMessage(hFolders,TVM_EXPAND,TVE_EXPAND,(LPARAM)TreeView_GetRoot(hFolders));


	/* Set the default widths of the bookmark columns. (Can also
	use LVSCW_AUTOSIZE_USEHEADER to size according to item text). */
	ListView_SetColumnWidth(hListView,0,DEFAULT_NAMECOLUMN_WIDTH);
	ListView_SetColumnWidth(hListView,1,DEFAULT_LOCATIONCOLUMN_WIDTH);
	ListView_SetColumnWidth(hListView,2,DEFAULT_DESCRIPTIONCOLUMN_WIDTH);

	TreeView_SelectItem(hFolders,TreeView_GetRoot(hFolders));

	/* Select the first item. */
	ListView_SelectItem(hListView,0,TRUE);

	SetFocus(hListView);

	/* Set the focus to the first item. Setting the focus will allow
	the selection to be changed immediately by pressing up/down etc. */
	ListView_SetItemState(hListView,0,LVIS_FOCUSED,LVIS_FOCUSED);

	ListView_SetImageList(hListView,himl,LVSIL_SMALL);

	if(m_bOrganizeBookmarksDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptOrganizeBookmarks.x,
			m_ptOrganizeBookmarks.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}
}

void Explorerplusplus::OrganizeBookmarksRefreshItem(HWND hDlg,int iItem)
{
	HWND		hListView;
	Bookmark_t	Bookmark;
	LVITEM		lvItem;

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(hListView,&lvItem);

	m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&Bookmark);

	ListView_SetItemText(hListView,iItem,0,Bookmark.szItemName);

	if(Bookmark.Type == BOOKMARK_TYPE_BOOKMARK)
	{
		ListView_SetItemText(hListView,iItem,1,Bookmark.szLocation);
	}

	ListView_SetItemText(hListView,iItem,2,Bookmark.szItemDescription);
}

void Explorerplusplus::MoveColumnItem(HWND hDlg,BOOL bUp)
{
	HWND	hListView;
	int		iSelected;

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		if(bUp)
			ListView_SwapItems(hListView,iSelected,iSelected - 1);
		else
			ListView_SwapItems(hListView,iSelected,iSelected + 1);

		SetFocus(hListView);
	}
}

void Explorerplusplus::OrganizeBookmarksMove(HWND hDlg,BOOL bUp)
{
	HWND		hListView;
	LVITEM		lvItem;
	Bookmark_t	BookmarkSelected;
	Bookmark_t	BookmarkSwap;
	int			iSelected;
	int			iSwap;

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= iSelected;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&BookmarkSelected);

		if(bUp)
		{
			if(iSelected == 0)
				return;

			iSwap = iSelected - 1;
		}
		else
		{
			if(iSelected == (ListView_GetItemCount(hListView) - 1))
				return;

			iSwap = iSelected + 1;
		}

		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= iSwap;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&BookmarkSwap);

		m_Bookmark.SwapBookmarks(&BookmarkSelected,&BookmarkSwap);

		ListView_SwapItemsNolParam(hListView,iSelected,iSwap);
	}
}

void Explorerplusplus::OnOrganizeBookmarksOk(HWND hDlg)
{
	/* Replace the bookmarks menu (in case any of the
	bookmarks have had their properties changed). */
	InsertBookmarksIntoMenu();

	/* Loop through each of the buttons currently on the
	toolbar. Update it with current information. Once gone
	through all buttons on toolbar, add new buttons. */
	/*Bookmark_t RootBookmark;

	m_Bookmark.GetRoot(&RootBookmark);
	UpdateToolbar(&RootBookmark);*/

	OrganizeBookmarksSaveState(hDlg);

	EndDialog(hDlg,1);
}

void Explorerplusplus::OnOrganizeBookmarksProperties(HWND hDlg)
{
	HWND	hListView;
	int		iSelected;

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
		ShowBookmarkProperties(hDlg,hListView,iSelected);
}

void Explorerplusplus::ShowBookmarkProperties(HWND hDlg,HWND hListView,int iItem)
{
	LVITEM						lvItem;
	Bookmark_t					Bookmark;
	BookmarkPropertiesInfo_t	bpi;
	INT_PTR						nResult = 0;
	BOOL						bOnToolbar;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(hListView,&lvItem);

	m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&Bookmark);

	bpi.pContainer		= this;
	bpi.pBookmarkHandle	= (void *)lvItem.lParam;

	bOnToolbar = Bookmark.bShowOnToolbar;

	/* Which dialog is shown depends on whether
	this item is a (bookmark) folder or a bookmark. */
	if(Bookmark.Type == BOOKMARK_TYPE_FOLDER)
	{
		nResult = DialogBoxParam(g_hLanguageModule,
			MAKEINTRESOURCE(IDD_BOOKMARKFOLDER_PROPERTIES),
			hDlg,BookmarkFolderPropertiesProcStub,
			(LPARAM)&bpi);
	}
	else
	{
		nResult = DialogBoxParam(g_hLanguageModule,
			MAKEINTRESOURCE(IDD_BOOKMARK_PROPERTIES),
			hDlg,BookmarkPropertiesProcStub,
			(LPARAM)&bpi);
	}

	/* Ok was pressed. Need to update the information
	for this bookmark. */
	if(nResult == 1)
	{
		OrganizeBookmarksRefreshItem(hDlg,iItem);

		g_bBookmarksAltered = TRUE;
	}

	SetFocus(hListView);
}

void Explorerplusplus::OnOrganizeBookmarksDoubleClick(HWND hDlg,LPARAM lParam)
{
	HWND			hBookmarks;
	NMITEMACTIVATE	*pnmItem = NULL;
	LVITEM			lvItem;
	Bookmark_t		Bookmark;
	Bookmark_t		Child;
	HRESULT			hr;

	hBookmarks = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	pnmItem = (NMITEMACTIVATE *)lParam;

	/* If this is a folder, open it; if it is a bookmark,
	open the directory within the main window. */
	if(pnmItem->iItem != -1)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= pnmItem->iItem;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hBookmarks,&lvItem);

		m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&Bookmark);

		if(Bookmark.Type == BOOKMARK_TYPE_FOLDER)
		{
			ListView_DeleteAllItems(hBookmarks);

			hr = m_Bookmark.GetChild(&Bookmark,&Child);

			/* Insert any child items into the listview. */
			if(SUCCEEDED(hr))
				InsertBookmarksIntoListView(hBookmarks,&Child);

			/* ...now, track the item in the treeview. */
			OrganizeBookmarksTrackInTreeView(hDlg,Bookmark.pHandle);
		}
		else if(Bookmark.Type == BOOKMARK_TYPE_BOOKMARK)
		{
			BrowseFolder(Bookmark.szLocation,SBSP_SAMEBROWSER);

			EndDialog(hDlg,1);
		}
	}
}

/* The current selection in the treeview is either
the current item, or is the parent of the upcoming
item.
Therefore, get the first child of the current item,
and keep stepping through each child item, until
the current item is found. */
void Explorerplusplus::OrganizeBookmarksTrackInTreeView(HWND hDlg,void *pBookmarkHandle)
{
	HWND		hTreeViewFolders;
	HTREEITEM	hItemCurrent;
	HTREEITEM	hItemChild;
	TVITEM		tvItem;

	hTreeViewFolders = GetDlgItem(hDlg,IDC_ORGANIZE_FOLDERS);

	hItemCurrent = TreeView_GetSelection(hTreeViewFolders);

	hItemChild = TreeView_GetNextItem(hTreeViewFolders,hItemCurrent,TVGN_CHILD);

	while(hItemChild != NULL)
	{
		tvItem.mask		= TVIF_PARAM;
		tvItem.hItem	= hItemChild;
		TreeView_GetItem(hTreeViewFolders,&tvItem);

		if((void *)tvItem.lParam == pBookmarkHandle)
		{
			TreeView_SelectItem(hTreeViewFolders,hItemChild);
			break;
		}

		hItemChild = TreeView_GetNextItem(hTreeViewFolders,
			hItemChild,TVGN_NEXT);
	}
}

void Explorerplusplus::OnOrganizeBookmarksRightClick(HWND hDlg,LPARAM lParam)
{
	HWND			hBookmarks;
	NMITEMACTIVATE	*pnmItem = NULL;
	LV_HITTESTINFO	lvhti;
	POINT			ptCursor;
	HMENU			hMenu;
	DWORD			dwPos;

	hBookmarks = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	pnmItem = (NMITEMACTIVATE *)lParam;

	dwPos = GetMessagePos();
	ptCursor.x = GET_X_LPARAM(dwPos);
	ptCursor.y = GET_Y_LPARAM(dwPos);

	lvhti.pt.x = ptCursor.x;
	lvhti.pt.y = ptCursor.y;
	ScreenToClient(hBookmarks,&lvhti.pt);

	ListView_HitTest(hBookmarks,&lvhti);

	if(!((lvhti.flags & LVHT_NOWHERE) == LVHT_NOWHERE) && lvhti.iItem != -1)
	{
		hMenu = LoadMenu(GetModuleHandle(0),MAKEINTRESOURCE(IDR_BOOKMARK_MENU));
		g_hBookmarkMenu = GetSubMenu(hMenu,0);

		g_iMenuItem = lvhti.iItem;

		TrackPopupMenu(g_hBookmarkMenu,TPM_LEFTALIGN,ptCursor.x,ptCursor.y,0,hDlg,NULL);

		DestroyMenu(hMenu);
	}
}

void Explorerplusplus::OnOrganizeBookmarksTvnSelChanged(HWND hDlg,LPARAM lParam)
{
	HWND		hBookmarks;
	HWND		hFolders;
	Bookmark_t	Parent;
	Bookmark_t	Child;
	HRESULT		hr;
	NMTREEVIEW	*pnmtv = NULL;

	pnmtv = (NMTREEVIEW *)lParam;

	hFolders = GetDlgItem(hDlg,IDC_ORGANIZE_FOLDERS);
	hBookmarks = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	ListView_DeleteAllItems(hBookmarks);

	m_Bookmark.RetrieveBookmark((void *)pnmtv->itemNew.lParam,&Parent);

	hr = m_Bookmark.GetChild(&Parent,&Child);

	if(SUCCEEDED(hr))
		InsertBookmarksIntoListView(hBookmarks,&Child);
}

void Explorerplusplus::OnOrganizeBookmarksDelete(HWND hDlg)
{
	HWND		hListView;
	LVITEM		lvItem;
	Bookmark_t	Bookmark;
	int			nItems;
	BOOL		bDeleted;
	int			iSelected;

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	nItems = ListView_GetItemCount(hListView);
	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= iSelected;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&Bookmark);

		bDeleted = DeleteBookmarkSafe(hDlg,(void *)lvItem.lParam);

		/* If the bookmark was actually deleted, remove it
		from the toolbar and rebuild the bookmarks menu. The
		bookmark may not have actually been deleted if the
		confirmation dialog was shown, and the user clicked
		'no'. */
		if(bDeleted)
		{
			/* If the bookmark is on the toolbar,
			remove it. */
			if(Bookmark.bShowOnToolbar)
				RemoveItemFromBookmarksToolbar(Bookmark.pHandle);

			g_bBookmarksAltered = TRUE;
			ListView_DeleteItem(hListView,iSelected);

			/* Rebuild the bookmarks menu. */
			InsertBookmarksIntoMenu();

			if(iSelected == (nItems - 1))
				iSelected--;

			ListView_SelectItem(hListView,iSelected,TRUE);

			SetFocus(hListView);
		}
	}
}

void Explorerplusplus::OnOrganizeBookmarksInitMenu(HWND hDlg,WPARAM wParam)
{
	HWND		hListView;
	LVITEM		lvItem;
	Bookmark_t	Bookmark;
	BOOL		bRes;

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	if((HMENU)wParam == g_hBookmarkMenu)
	{
		if(g_iMenuItem != -1)
		{
			lvItem.mask		= LVIF_PARAM;
			lvItem.iItem	= g_iMenuItem;
			lvItem.iSubItem	= 0;
			bRes = ListView_GetItem(hListView,&lvItem);

			if(bRes)
			{
				m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&Bookmark);

				lCheckMenuItem(g_hBookmarkMenu,IDM_BOOKMARK_SHOWONTOOLBAR,
					Bookmark.bShowOnToolbar);
			}
		}
	}
}

void Explorerplusplus::OnOrganizeBookmarksShowOnToolbar(HWND hDlg)
{
	HWND		hListView;
	LVITEM		lvItem;
	Bookmark_t	Bookmark;
	BOOL		bRes;

	hListView = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	if(g_iMenuItem != -1)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= g_iMenuItem;
		lvItem.iSubItem	= 0;
		bRes = ListView_GetItem(hListView,&lvItem);

		if(bRes)
		{
			m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&Bookmark);

			Bookmark.bShowOnToolbar = !Bookmark.bShowOnToolbar;

			if(Bookmark.bShowOnToolbar)
			{
				/* Insert the bookmark onto the toolbar. */
				InsertBookmarkIntoToolbar(&Bookmark,GenerateUniqueBookmarkToolbarId());
			}
			else
			{
				/* Find the item and remove it. */
				RemoveItemFromBookmarksToolbar(Bookmark.pHandle);
			}

			m_Bookmark.UpdateBookmark(Bookmark.pHandle,&Bookmark);
		}
	}
}

void Explorerplusplus::OnOrganizeBookmarksOpen(HWND hDlg,BOOL bOpenInNewTab)
{
	HWND			hBookmarks;
	LVITEM			lvItem;
	Bookmark_t		Bookmark;
	Bookmark_t		Child;
	HRESULT			hr;

	hBookmarks = GetDlgItem(hDlg,IDC_ORGANIZE_BOOKMARKS);

	/* If this is a folder, open it; if it is a bookmark,
	open the directory within the main window. */
	if(g_iMenuItem != -1)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= g_iMenuItem;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hBookmarks,&lvItem);

		m_Bookmark.RetrieveBookmark((void *)lvItem.lParam,&Bookmark);

		if(Bookmark.Type == BOOKMARK_TYPE_FOLDER)
		{
			ListView_DeleteAllItems(hBookmarks);

			hr = m_Bookmark.GetChild(&Bookmark,&Child);

			if(SUCCEEDED(hr))
				InsertBookmarksIntoListView(hBookmarks,&Child);
		}
		else if(Bookmark.Type == BOOKMARK_TYPE_BOOKMARK)
		{
			if(bOpenInNewTab)
				BrowseFolder(Bookmark.szLocation,SBSP_SAMEBROWSER,TRUE,TRUE,FALSE);
			else
				BrowseFolder(Bookmark.szLocation,SBSP_SAMEBROWSER);

			EndDialog(hDlg,1);
		}
	}
}

void Explorerplusplus::OrganizeBookmarksSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptOrganizeBookmarks.x = rcTemp.left;
	m_ptOrganizeBookmarks.y = rcTemp.top;

	m_bOrganizeBookmarksDlgStateSaved = TRUE;
}