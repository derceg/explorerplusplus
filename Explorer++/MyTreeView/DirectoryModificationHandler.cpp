// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MyTreeView.h"
#include "MyTreeViewInternal.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"

/* TODO: Will have to change this. If AddItem() fails for some
reason, then add the item to the tracking list. */
void CMyTreeView::DirectoryAlteredAddFile(const TCHAR *szFullFileName)
{
	LPITEMIDLIST pidlComplete = NULL;
	HRESULT hr;

	/* We'll use this as a litmus test to check whether or not
	the file actually exists. */
	hr = GetIdlFromParsingName(szFullFileName,&pidlComplete);

	if(SUCCEEDED(hr))
	{
		AddItem(szFullFileName);
		CoTaskMemFree(pidlComplete);
	}
}

void CMyTreeView::DirectoryAlteredRemoveFile(const TCHAR *szFullFileName)
{
	TCHAR szParent[MAX_PATH];
	HTREEITEM hItem;
	HTREEITEM hDeskItem;
	HTREEITEM hDeskParent;

	StringCchCopy(szParent,SIZEOF_ARRAY(szParent),szFullFileName);
	PathRemoveFileSpec(szParent);

	/* If the item is on the desktop, we need to remove the item 
	   from the desktop tree branch as well. */
	hDeskParent = LocateItemOnDesktopTree(szParent);  
	hDeskItem	= LocateItemOnDesktopTree(szFullFileName); 

	if(hDeskItem != NULL)
	{
		RemoveItem(hDeskItem);
	}

	hItem = LocateDeletedItem(szFullFileName);
	if(hItem != NULL)
	{
		RemoveItem(szFullFileName);
	}

	/* The parent item should be updated (if it is in the tree), regardless of whether the
	   actual item was found. For example, the number of children may need to be set to 0
	   to remove the "+" sign from the tree. The update should be done after the removal. */
	UpdateParent(hDeskParent);
	UpdateParent(szParent);
}

void CMyTreeView::DirectoryAlteredRenameFile(const TCHAR *szFullFileName, const TCHAR *szOldFileName)
{
	HTREEITEM hItem;
	HTREEITEM hDeskItem;

	/* If the item is on the desktop, it is a special case.
	   We need to update the treeview in two places:
	   1) the root item
	   2) the user's desktop folder if the tree is expanded to that folder
		  (i.e. c:\users\'username'\Desktop) */
	hDeskItem = LocateItemOnDesktopTree(szOldFileName);

	if(hDeskItem != NULL)
	{
		// Update root item
		RenameItem(hDeskItem,szFullFileName);
	}

	/* Check if the file currently exists in the treeview. */
	hItem = LocateItemByPath(szOldFileName,FALSE);

	if(hItem != NULL)
	{
		RenameItem(hItem,szFullFileName);

		/* Notify the parent that the selected item (or one of its ancestors)
		has been renamed. */
		HTREEITEM hSelection = TreeView_GetSelection(m_hTreeView);
		HTREEITEM hAncestor = hSelection;

		while(hAncestor != hItem && hAncestor != NULL) 
		{
			hAncestor = TreeView_GetParent(m_hTreeView,hAncestor);
		} 

		if(hAncestor == hItem)
		{
			TVITEM tvSelected;

			tvSelected.mask		= TVIF_PARAM;
			tvSelected.hItem	= hSelection;
			BOOL bRes = TreeView_GetItem(m_hTreeView,&tvSelected);

			HWND hParent = GetParent(m_hTreeView);

			if(bRes && hParent != NULL)
			{
				NMTREEVIEW nmtv;

				nmtv.hdr.code	= TVN_SELCHANGED;
				nmtv.action		= TVC_UNKNOWN;
				nmtv.itemNew	= tvSelected;

				/* TODO: Switch to custom notification. */
				SendMessage(hParent,WM_NOTIFY,0,reinterpret_cast<LPARAM>(&nmtv));
			}
		}
	}
}

void CMyTreeView::AddDrive(const TCHAR *szDrive)
{
	LPITEMIDLIST	pidlMyComputer = NULL;
	HTREEITEM		hMyComputer;
	HRESULT			hr;

	hr = SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidlMyComputer);

	if(hr == S_OK)
	{
		hMyComputer = LocateExistingItem(pidlMyComputer);

		if(hMyComputer != NULL)
		{
			AddItemInternal(hMyComputer,szDrive);
		}

		CoTaskMemFree(pidlMyComputer);
	}
}

void CMyTreeView::AddItem(const TCHAR *szFullFileName)
{
	TCHAR			szDirectory[MAX_PATH];
	HTREEITEM		hParent;
	HTREEITEM		hDeskParent;

	/* If the specified item is a drive, it
	will need to be handled differently,
	as it is a child of my computer (and
	as such is not a regular file). */
	if(PathIsRoot(szFullFileName))
	{
		AddDrive(szFullFileName);
	}
	else
	{
		StringCchCopy(szDirectory, SIZEOF_ARRAY(szDirectory), szFullFileName);
		PathRemoveFileSpec(szDirectory);

		// Check if it is a desktop (sub)child
		hDeskParent = LocateItemOnDesktopTree(szDirectory);

		hParent = LocateExistingItem(szDirectory);

		/* If this items' parent isn't currently shown on the
		treeview and the item is not on the desktop, exit without
		doing anything further. */
		if(hParent == NULL && hDeskParent == NULL)
			return;

		AddItemInternal(hParent,szFullFileName);

		if(hDeskParent != NULL)
		{
			/* If the item is on the desktop, it is a special
			case. We need to update the treeview also starting
			from the root item. */
			AddItemInternal(hDeskParent,szFullFileName);
		}
	}
}

void CMyTreeView::AddItemInternal(HTREEITEM hParent,const TCHAR *szFullFileName)
{
	IShellFolder	*pShellFolder = NULL;
	LPITEMIDLIST	pidlComplete = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	HTREEITEM		hItem;
	TVITEMEX		tvItem;
	TVINSERTSTRUCT	tvis;
	SHFILEINFO		shfi;
	SFGAOF			Attributes;
	TCHAR			szDisplayName[MAX_PATH];
	HRESULT			hr;
	BOOL			res;
	int				iItemId;
	int				nChildren = 0;

	hr = GetIdlFromParsingName(szFullFileName,&pidlComplete);

	if(!SUCCEEDED(hr))
		return;

	tvItem.mask		= TVIF_CHILDREN | TVIF_STATE;
	tvItem.hItem	= hParent;
	res = TreeView_GetItem(m_hTreeView,&tvItem);

	if(res)
	{
		/* If the parent node is currently collapsed,
		simply indicate that it has children (i.e. a
		plus sign will be shown next to the parent node). */
		if((tvItem.cChildren == 0) ||
			((tvItem.state & TVIS_EXPANDED) != TVIS_EXPANDED))
		{
			tvItem.mask			= TVIF_CHILDREN;
			tvItem.hItem		= hParent;
			tvItem.cChildren	= 1;
			TreeView_SetItem(m_hTreeView,&tvItem);
		}
		else
		{
			SHGetFileInfo(szFullFileName,NULL,&shfi,
				sizeof(shfi),SHGFI_SYSICONINDEX);

			hr = SHBindToParent(pidlComplete, IID_PPV_ARGS(&pShellFolder), (LPCITEMIDLIST *)&pidlRelative);

			if(SUCCEEDED(hr))
			{
				Attributes = SFGAO_HASSUBFOLDER;

				/* Only retrieve the attributes for this item. */
				hr = pShellFolder->GetAttributesOf(1,
					(LPCITEMIDLIST *)&pidlRelative,&Attributes);

				if(SUCCEEDED(hr))
				{
					if((Attributes & SFGAO_HASSUBFOLDER) != SFGAO_HASSUBFOLDER)
						nChildren = 0;
					else
						nChildren = 1;

					iItemId = GenerateUniqueItemId();

					m_pItemInfo[iItemId].pidl = ILClone(pidlComplete);
					m_pItemInfo[iItemId].pridl = ILClone(pidlRelative);

					GetDisplayName(szFullFileName,szDisplayName,SIZEOF_ARRAY(szDisplayName),SHGDN_NORMAL);

					tvItem.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_CHILDREN;
					tvItem.pszText			= szDisplayName;
					tvItem.iImage			= shfi.iIcon;
					tvItem.iSelectedImage	= shfi.iIcon;
					tvItem.lParam			= (LPARAM)iItemId;
					tvItem.cChildren		= nChildren;

					if(hParent != NULL)
					{
						tvis.hParent			= hParent;
						tvis.hInsertAfter		= DetermineItemSortedPosition(hParent,szFullFileName);
						tvis.itemex				= tvItem;

						hItem = TreeView_InsertItem(m_hTreeView,&tvis);
					}
				}

				pShellFolder->Release();
			}
		}
	}

	CoTaskMemFree(pidlComplete);
}

/* Renames an item in the treeview. This essentially involves changing
the items display text, and updating it's pidl. Note that the children of
this item MUST ALL BE UPDATED as well, since their pidl's will also
change. */
void CMyTreeView::RenameItem(HTREEITEM hItem, const TCHAR *szFullFileName)
{
	TVITEMEX	tvItem;
	ItemInfo_t	*pItemInfo = NULL;
	LPITEMIDLIST	pidlParent;
	SHFILEINFO	shfi;
	TCHAR		szFileName[MAX_PATH];
	HRESULT		hr;
	BOOL		res;

	if(hItem == NULL)
		return;

	tvItem.mask		= TVIF_PARAM;
	tvItem.hItem	= hItem;
	res = TreeView_GetItem(m_hTreeView,&tvItem);

	if(res)
	{
		pItemInfo = &m_pItemInfo[(int)tvItem.lParam];

		CoTaskMemFree(pItemInfo->pidl);

		StringCchCopy(szFileName, SIZEOF_ARRAY(szFileName), szFullFileName);
		PathStripPath(szFileName);

		hr = GetIdlFromParsingName(szFullFileName,&pItemInfo->pidl);

		pidlParent = pItemInfo->pidl;

		if(SUCCEEDED(hr))
		{
			SHGetFileInfo(szFullFileName,NULL,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

			tvItem.mask				= TVIF_HANDLE|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
			tvItem.hItem			= hItem;
			tvItem.pszText			= szFileName;
			tvItem.iImage			= shfi.iIcon;
			tvItem.iSelectedImage	= shfi.iIcon;
			TreeView_SetItem(m_hTreeView,&tvItem);

			/* Now recursively go through each of this items children and
			update their pidl's. */
			UpdateChildren(hItem,pidlParent);
		}
	}
}

void CMyTreeView::UpdateChildren(HTREEITEM hParent,LPITEMIDLIST pidlParent)
{
	HTREEITEM hChild;
	TVITEMEX tvItem;
	LPITEMIDLIST pidl = NULL;
	BOOL bRes;

	hChild = TreeView_GetChild(m_hTreeView,hParent);

	if(hChild != NULL)
	{
		tvItem.mask		= TVIF_PARAM;
		tvItem.hItem	= hChild;
		bRes = TreeView_GetItem(m_hTreeView,&tvItem);

		if(bRes)
		{
			pidl = UpdateItemInfo(pidlParent,(int)tvItem.lParam);

			UpdateChildren(hChild,pidl);

			while((hChild = TreeView_GetNextItem(m_hTreeView,hChild,TVGN_NEXT)) != NULL)
			{
				tvItem.mask		= TVIF_PARAM;
				tvItem.hItem	= hChild;
				bRes = TreeView_GetItem(m_hTreeView,&tvItem);

				if(bRes)
				{
					pidl = UpdateItemInfo(pidlParent,(int)tvItem.lParam);

					UpdateChildren(hChild,pidl);
				}
			}
		}
	}
}

/* Updates an items pidl, returning a pointer to the new idl. */
LPITEMIDLIST CMyTreeView::UpdateItemInfo(LPITEMIDLIST pidlParent,int iItemId)
{
	ItemInfo_t *pItemInfo = NULL;

	pItemInfo = &m_pItemInfo[iItemId];

	CoTaskMemFree(pItemInfo->pidl);

	m_pItemInfo[iItemId].pidl = ILCombine(pidlParent,m_pItemInfo[iItemId].pridl);

	return m_pItemInfo[iItemId].pidl;
}

void CMyTreeView::RemoveItem(const TCHAR *szFullFileName)
{
	HTREEITEM hItem;

	hItem = LocateDeletedItem(szFullFileName);

	if(hItem != NULL)
	{
		RemoveItem(hItem);
	}
}

void CMyTreeView::RemoveItem(HTREEITEM hItem)
{
	EraseItems(hItem);
	TreeView_DeleteItem(m_hTreeView,hItem);
}

void CMyTreeView::UpdateParent(const TCHAR *szParent)
{
	HTREEITEM hParent;

	hParent = LocateExistingItem(szParent);

	UpdateParent(hParent);
}

void CMyTreeView::UpdateParent(HTREEITEM hParent)
{
	if(hParent != NULL)
	{
		TVITEM tvItem;
		SFGAOF Attributes = SFGAO_HASSUBFOLDER;
		BOOL bRes;
		HRESULT hr;

		tvItem.mask		= TVIF_PARAM|TVIF_HANDLE;
		tvItem.hItem	= hParent;
		bRes = TreeView_GetItem(m_hTreeView,&tvItem);

		if(bRes)
		{
			hr = GetItemAttributes(m_pItemInfo[static_cast<int>(tvItem.lParam)].pidl,
				&Attributes);

			if(SUCCEEDED(hr))
			{
				/* If the parent folder no longer has any sub-folders,
				set its number of children to 0. */
				if((Attributes & SFGAO_HASSUBFOLDER) != SFGAO_HASSUBFOLDER)
				{
					tvItem.cChildren = 0;
					TreeView_Expand(m_hTreeView,hParent,TVE_COLLAPSE);
				}
				else
				{
					tvItem.cChildren = 1;
				}

				tvItem.mask		= TVIF_CHILDREN;
				tvItem.hItem	= hParent;
				TreeView_SetItem(m_hTreeView,&tvItem);
			}
		}
	}
}