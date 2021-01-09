// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

const UINT DIRECTORYMODIFIED_TIMER_ID = 0;
const UINT DIRECTORYMODIFIED_TIMER_ELAPSE = 500;

void ShellTreeView::DirectoryAltered()
{
	EnterCriticalSection(&m_cs);

	KillTimer(m_hTreeView,0);

	/* Three basic situations to watch out for:
	 - File is created, then renamed. Both notifications
	 come in at the same time.
	 - File is renamed twice in a row. Both notifications
	 come in at the same time.
	 - File created or renamed, and then deleted. Both
	 notifications come in at the same time.
	These situations may also happen multiple times
	in one run. e.g. 5 files are created and renamed
	before the first notification comes in. Therefore,
	need to keep track of ALL events.
	Notifications:
	Created
	Created
	Renamed
	Renamed
	In this case, need to remember that the first two
	notifications referred to files that didn't exist. */
	for(const auto &af : m_AlteredList)
	{
		switch(af.dwAction)
		{
			case FILE_ACTION_ADDED:
				DirectoryAlteredAddFile(af.szFileName);
				break;

			/* The modified notification does not need
			to be handled in any way. File size/date/attribute
			changes have no effect within the treeview. */
			case FILE_ACTION_MODIFIED:
				break;

			case FILE_ACTION_REMOVED:
				DirectoryAlteredRemoveFile(af.szFileName);
				break;

			case FILE_ACTION_RENAMED_OLD_NAME:
				/* Store the old name. Since
				FILE_ACTION_RENAMED_OLD_NAME/FILE_ACTION_RENAMED_NEW_NAME
				always come in in paris, we only need to be able to store
				one old filename. */
				StringCchCopy(m_szAlteredOldFileName,
					SIZEOF_ARRAY(m_szAlteredOldFileName),
					af.szFileName);
				break;

			case FILE_ACTION_RENAMED_NEW_NAME:
				DirectoryAlteredRenameFile(af.szFileName);
				break;
		}
	}

	m_AlteredList.clear();

	LeaveCriticalSection(&m_cs);
}

/* TODO: Will have to change this. If AddItem() fails for some
reason, then add the item to the tracking list. */
void ShellTreeView::DirectoryAlteredAddFile(const TCHAR *szFullFileName)
{
	/* We'll use this as a litmus test to check whether or not
	the file actually exists. */
	PIDLIST_ABSOLUTE pidlComplete = nullptr;
	HRESULT hr = SHParseDisplayName(szFullFileName, nullptr, &pidlComplete, 0, nullptr);

	if(SUCCEEDED(hr))
	{
		AddItem(szFullFileName);
		CoTaskMemFree(pidlComplete);
	}
	else
	{
		/* The file doesn't exist, so keep a record of it. */
		AlteredFile_t af;

		StringCchCopy(af.szFileName,SIZEOF_ARRAY(af.szFileName),szFullFileName);
		af.dwAction = FILE_ACTION_ADDED;

		m_AlteredTrackingList.push_back(af);
	}
}

void ShellTreeView::DirectoryAlteredRemoveFile(const TCHAR *szFullFileName)
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

	if(hDeskItem != nullptr)
	{
		RemoveItem(hDeskItem);
	}

	hItem = LocateDeletedItem(szFullFileName);
	if(hItem != nullptr)
	{
		RemoveItem(szFullFileName);
	}

	/* The parent item should be updated (if it is in the tree), regardless of whether the
	   actual item was found. For example, the number of children may need to be set to 0
	   to remove the "+" sign from the tree. The update should be done after the removal. */
	UpdateParent(hDeskParent);
	UpdateParent(szParent);

	if(hDeskItem == nullptr && hItem == nullptr)
	{
		/* The file does not currently exist within the treeview.
		   It should appear in the tracking list however. */
		for(auto itr = m_AlteredTrackingList.begin();itr != m_AlteredTrackingList.end();itr++)
		{
			if(lstrcmp(szFullFileName,itr->szFileName) == 0)
			{
				/* Found the deleted item. Remove it from the tracking list. */
				/* TODO: May need to do this multiple times.
				e.g.
				Created
				Renamed
				Removed

				Both the created and renamed notifications need to
				be removed from the queue. */
				m_AlteredTrackingList.erase(itr);
				break;
			}
		}
	}
}

void ShellTreeView::DirectoryAlteredRenameFile(const TCHAR *szFullFileName)
{
	HTREEITEM hItem;
	HTREEITEM hDeskItem;

	/* If the item is on the desktop, it is a special case.
	   We need to update the treeview in two places:
	   1) the root item
	   2) the user's desktop folder if the tree is expanded to that folder
		  (i.e. c:\users\'username'\Desktop) */
	hDeskItem = LocateItemOnDesktopTree(m_szAlteredOldFileName);  

	if(hDeskItem != nullptr)
	{
		// Update root item
		RenameItem(hDeskItem,szFullFileName);
	}

	/* Check if the file currently exists in the treeview. */
	hItem = LocateItemByPath(m_szAlteredOldFileName,FALSE);

	if(hItem != nullptr)
	{
		RenameItem(hItem,szFullFileName);

		/* Notify the parent that the selected item (or one of its ancestors)
		has been renamed. */
		auto hSelection = TreeView_GetSelection(m_hTreeView);
		HTREEITEM hAncestor = hSelection;

		while(hAncestor != hItem && hAncestor != nullptr) 
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

			if(bRes && hParent != nullptr)
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

	if(!hDeskItem && !hItem)
	{
		BOOL bFound = FALSE;

		/* Search through the queue of file additions
		and modifications for this file. */
		for(auto itr = m_AlteredTrackingList.begin();itr != m_AlteredTrackingList.end();itr++)
		{
			if(lstrcmp(m_szAlteredOldFileName,itr->szFileName) == 0)
			{
				/* Item has been found. Change the name on the
				notification and push it back into the list.
				Note: The item must be pushed directly after the current item. */
				StringCchCopy(itr->szFileName,SIZEOF_ARRAY(itr->szFileName),
					szFullFileName);

				for(auto itrAltered = m_AlteredList.begin();itrAltered != m_AlteredList.end();itrAltered++)
				{
					if(itrAltered->dwAction == FILE_ACTION_RENAMED_NEW_NAME &&
						lstrcmp(itrAltered->szFileName,szFullFileName) == 0)
					{
						m_AlteredList.insert(++itrAltered,*itr);
						break;
					}
				}

				m_AlteredTrackingList.erase(itr);

				bFound = TRUE;
				break;
			}
		}

		/* This may happen if a file has been
		renamed twice in a row, or renamed then deleted.
		e.g.
		1.txt (Existing file) -> 2.txt
		2.txt -> 3.txt
		
		Notifications:
		old name/new name - 1.txt/2.txt
		old name/new name - 2.txt/3.txt
		Need to remember the original filename.
		e.g. Need to be able to go from 1.txt to 3.txt. */
		if(!bFound)
		{
			/* TODO: Change the filename on the renamed action and push it
			back into the queue. */
		}
	}
}

void ShellTreeView::DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction, void *pData)
{
	DirectoryAltered_t	*pDirectoryAltered = nullptr;
	ShellTreeView		*shellTreeView = nullptr;
	TCHAR				szFullFileName[MAX_PATH];

	pDirectoryAltered = (DirectoryAltered_t *)pData;

	shellTreeView = (ShellTreeView *)pDirectoryAltered->shellTreeView;

	StringCchCopy(szFullFileName, SIZEOF_ARRAY(szFullFileName), pDirectoryAltered->szPath);
	if(!PathAppend(szFullFileName, szFileName))
	{
		return;
	}

	shellTreeView->DirectoryModified(dwAction,szFullFileName);
}

void ShellTreeView::DirectoryModified(DWORD dwAction, const TCHAR *szFullFileName)
{
	EnterCriticalSection(&m_cs);

	SetTimer(m_hTreeView,DIRECTORYMODIFIED_TIMER_ID,
		DIRECTORYMODIFIED_TIMER_ELAPSE, nullptr);

	AlteredFile_t af;

	StringCchCopy(af.szFileName,SIZEOF_ARRAY(af.szFileName),szFullFileName);
	af.dwAction = dwAction;

	m_AlteredList.push_back(af);

	LeaveCriticalSection(&m_cs);
}

void ShellTreeView::AddDrive(const TCHAR *szDrive)
{
	PIDLIST_ABSOLUTE pidlMyComputer = nullptr;
	HRESULT hr = SHGetFolderLocation(nullptr,CSIDL_DRIVES, nullptr,0,&pidlMyComputer);

	if(hr == S_OK)
	{
		HTREEITEM hMyComputer = LocateExistingItem(pidlMyComputer);

		if(hMyComputer != nullptr)
		{
			AddItemInternal(hMyComputer,szDrive);
		}

		CoTaskMemFree(pidlMyComputer);
	}
}

void ShellTreeView::AddItem(const TCHAR *szFullFileName)
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
		if (hParent == nullptr && hDeskParent == nullptr)
		{
			return;
		}

		AddItemInternal(hParent,szFullFileName);

		if(hDeskParent != nullptr)
		{
			/* If the item is on the desktop, it is a special
			case. We need to update the treeview also starting
			from the root item. */
			AddItemInternal(hDeskParent,szFullFileName);
		}
	}
}

void ShellTreeView::AddItemInternal(HTREEITEM hParent,const TCHAR *szFullFileName)
{
	IShellFolder	*pShellFolder = nullptr;
	PIDLIST_ABSOLUTE	pidlComplete = nullptr;
	PCITEMID_CHILD	pidlRelative = nullptr;
	HTREEITEM		hItem;
	TVITEMEX		tvItem;
	TVINSERTSTRUCT	tvis;
	SHFILEINFO		shfi;
	SFGAOF			attributes;
	HRESULT			hr;
	BOOL			res;
	int				iItemId;
	int				nChildren = 0;

	hr = SHParseDisplayName(szFullFileName, nullptr, &pidlComplete, 0, nullptr);

	if (!SUCCEEDED(hr))
	{
		return;
	}

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
			SHGetFileInfo(szFullFileName,0,&shfi,
				sizeof(shfi),SHGFI_SYSICONINDEX);

			hr = SHBindToParent(pidlComplete, IID_PPV_ARGS(&pShellFolder), &pidlRelative);

			if(SUCCEEDED(hr))
			{
				attributes = SFGAO_HASSUBFOLDER;

				/* Only retrieve the attributes for this item. */
				hr = pShellFolder->GetAttributesOf(1,&pidlRelative,&attributes);

				if(SUCCEEDED(hr))
				{
					if ((attributes & SFGAO_HASSUBFOLDER) != SFGAO_HASSUBFOLDER)
					{
						nChildren = 0;
					}
					else
					{
						nChildren = 1;
					}

					iItemId = GenerateUniqueItemId();

					m_itemInfoMap[iItemId].pidl.reset(ILCloneFull(pidlComplete));
					m_itemInfoMap[iItemId].pridl.reset(ILCloneChild(pidlRelative));

					std::wstring displayName;
					GetDisplayName(szFullFileName, SHGDN_NORMAL, displayName);

					tvItem.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_CHILDREN;
					tvItem.pszText			= displayName.data();
					tvItem.iImage			= shfi.iIcon;
					tvItem.iSelectedImage	= shfi.iIcon;
					tvItem.lParam			= iItemId;
					tvItem.cChildren		= nChildren;

					if(hParent != nullptr)
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
void ShellTreeView::RenameItem(HTREEITEM hItem, const TCHAR *szFullFileName)
{
	TVITEMEX	tvItem;
	PIDLIST_ABSOLUTE	pidlParent;
	SHFILEINFO	shfi;
	TCHAR		szFileName[MAX_PATH];
	HRESULT		hr;
	BOOL		res;

	if (hItem == nullptr)
	{
		return;
	}

	tvItem.mask		= TVIF_PARAM;
	tvItem.hItem	= hItem;
	res = TreeView_GetItem(m_hTreeView,&tvItem);

	if(res)
	{
		StringCchCopy(szFileName, SIZEOF_ARRAY(szFileName), szFullFileName);
		PathStripPath(szFileName);

		ItemInfo_t &iteminfo = m_itemInfoMap.at(static_cast<int>(tvItem.lParam));
		hr = SHParseDisplayName(szFullFileName, nullptr, wil::out_param(iteminfo.pidl), 0, nullptr);

		pidlParent = iteminfo.pidl.get();

		if(SUCCEEDED(hr))
		{
			SHGetFileInfo(szFullFileName,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

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

void ShellTreeView::UpdateChildren(HTREEITEM hParent, PCIDLIST_ABSOLUTE pidlParent)
{
	HTREEITEM hChild;
	TVITEMEX tvItem;
	PCIDLIST_ABSOLUTE pidl = nullptr;
	BOOL bRes;

	hChild = TreeView_GetChild(m_hTreeView,hParent);

	if(hChild != nullptr)
	{
		tvItem.mask		= TVIF_PARAM;
		tvItem.hItem	= hChild;
		bRes = TreeView_GetItem(m_hTreeView,&tvItem);

		if(bRes)
		{
			pidl = UpdateItemInfo(pidlParent,(int)tvItem.lParam);

			UpdateChildren(hChild,pidl);

			while((hChild = TreeView_GetNextItem(m_hTreeView,hChild,TVGN_NEXT)) != nullptr)
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

PCIDLIST_ABSOLUTE ShellTreeView::UpdateItemInfo(PCIDLIST_ABSOLUTE pidlParent, int iItemId)
{
	ItemInfo_t &itemInfo = m_itemInfoMap.at(iItemId);
	itemInfo.pidl.reset(ILCombine(pidlParent, itemInfo.pridl.get()));

	return itemInfo.pidl.get();
}

void ShellTreeView::RemoveItem(const TCHAR *szFullFileName)
{
	HTREEITEM hItem;

	hItem = LocateDeletedItem(szFullFileName);

	if(hItem != nullptr)
	{
		RemoveItem(hItem);
	}
}

void ShellTreeView::RemoveItem(HTREEITEM hItem)
{
	EraseItems(hItem);
	TreeView_DeleteItem(m_hTreeView,hItem);
}

void ShellTreeView::UpdateParent(const TCHAR *szParent)
{
	HTREEITEM hParent;

	hParent = LocateExistingItem(szParent);

	UpdateParent(hParent);
}

void ShellTreeView::UpdateParent(HTREEITEM hParent)
{
	if(hParent != nullptr)
	{
		TVITEM tvItem;
		SFGAOF attributes = SFGAO_HASSUBFOLDER;
		BOOL bRes;
		HRESULT hr;

		tvItem.mask		= TVIF_PARAM|TVIF_HANDLE;
		tvItem.hItem	= hParent;
		bRes = TreeView_GetItem(m_hTreeView,&tvItem);

		if(bRes)
		{
			hr = GetItemAttributes(m_itemInfoMap.at(static_cast<int>(tvItem.lParam)).pidl.get(),
				&attributes);

			if(SUCCEEDED(hr))
			{
				/* If the parent folder no longer has any sub-folders,
				set its number of children to 0. */
				if((attributes & SFGAO_HASSUBFOLDER) != SFGAO_HASSUBFOLDER)
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