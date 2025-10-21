// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "FolderView.h"
#include "ServiceProvider.h"
#include "ViewModes.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WinRTBaseWrapper.h"

/* Scroll definitions. */
#define MIN_X_POS 20
#define MIN_Y_POS 20
#define X_SCROLL_AMOUNT 10
#define Y_SCROLL_AMOUNT 10

int ShellBrowserImpl::GetDropTargetItem(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_listView, &ptClient);

	if (!res)
	{
		return -1;
	}

	LVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = ptClient;
	int index = ListView_HitTest(m_listView, &hitTestInfo);

	if (index == -1)
	{
		return -1;
	}

	auto &item = GetItemByIndex(index);

	// Folders always act as drop targets. If a folder can't actually accept a drop, the drop should
	// be marked as blocked. On the other hand, a file may accept a drop (e.g. it may be possible to
	// drop an item on an executable). But if the file can't accept the drop, then the drop target
	// should revert to the parent.
	if (WI_IsFlagClear(item.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)
		&& !GetDropTargetForPidl(item.pidlComplete.Raw()))
	{
		return -1;
	}

	// The target item is either a folder (which may or may not accept the drop), or a file that can
	// accept a drop.
	return index;
}

unique_pidl_absolute ShellBrowserImpl::GetPidlForTargetItem(int targetItem)
{
	if (targetItem != -1)
	{
		auto &item = GetItemByIndex(targetItem);
		return unique_pidl_absolute(ILCloneFull(item.pidlComplete.Raw()));
	}

	return unique_pidl_absolute(ILCloneFull(m_directoryState.pidlDirectory.Raw()));
}

IUnknown *ShellBrowserImpl::GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl)
{
	// It's important to restrict this to the current folder. Otherwise, there can be situations
	// where an item is dropped in a subfolder, and an item with the same name is selected in the
	// parent folder (which is a bug that affects Windows Explorer).
	if (!ArePidlsEquivalent(targetItemPidl, m_directoryState.pidlDirectory.Raw()))
	{
		return nullptr;
	}

	if (!m_dropServiceProvider)
	{
		m_dropServiceProvider = winrt::make_self<ServiceProvider>();
		m_dropServiceProvider->RegisterService(IID_IFolderView,
			winrt::make<FolderView>(m_weakPtrFactory.GetWeakPtr()));
	}

	return m_dropServiceProvider.get();
}

bool ShellBrowserImpl::IsTargetSourceOfDrop(int targetItem)
{
	if (m_performingDrag && targetItem == -1)
	{
		return true;
	}

	return false;
}

void ShellBrowserImpl::UpdateUiForDrop(int targetItem, const POINT &pt)
{
	ListView_SetItemState(m_listView, -1, 0, LVIS_DROPHILITED);

	if (targetItem != -1)
	{
		ListView_SetItemState(m_listView, targetItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
	}

	ScrollListViewForDrop(pt);
}

/* TODO: This isn't declared. */
int CALLBACK SortTemporaryStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	auto *pShellBrowser = reinterpret_cast<ShellBrowserImpl *>(lParamSort);
	return pShellBrowser->SortTemporary(lParam1, lParam2);
}

int CALLBACK ShellBrowserImpl::SortTemporary(LPARAM lParam1, LPARAM lParam2)
{
	return m_itemInfoMap.at(static_cast<int>(lParam1)).iRelativeSort
		- m_itemInfoMap.at(static_cast<int>(lParam2)).iRelativeSort;
}

void ShellBrowserImpl::RepositionLocalFiles(const POINT *ppt)
{
	POINT pt;
	POINT ptOrigin;

	pt = *ppt;
	ScreenToClient(m_listView, &pt);

	/* The auto arrange style must be off for the items
	to be moved. Therefore, if the style is on, turn it
	off, move the items, and the turn it back on. */
	if (m_folderSettings.autoArrangeEnabled)
	{
		ListViewHelper::SetAutoArrange(m_listView, false);
	}

	for (const auto &pidl : m_draggedItems)
	{
		auto index = GetItemIndexForPidl(pidl.Raw());

		if (!index)
		{
			continue;
		}

		if (m_folderSettings.viewMode == +ViewMode::Details)
		{
			LVITEM lvItem;
			POINT ptItem;
			BOOL bBelowPreviousItem = TRUE;
			BOOL bRes;
			int iInsert = 0;
			int iSort = 0;
			int nItems;
			int i = 0;

			nItems = ListView_GetItemCount(m_listView);

			/* Find the closest item to the dropped item. */
			for (i = 0; i < nItems; i++)
			{
				ListView_GetItemPosition(m_listView, i, &ptItem);

				if (bBelowPreviousItem && (pt.y - ptItem.y) < 0)
				{
					iInsert = i - 1;
					break;
				}

				/* If the dropped item is below the last item,
				it will be moved to the last position. */
				if (i == (nItems - 1))
				{
					iInsert = nItems;
				}

				bBelowPreviousItem = (pt.y - ptItem.y) > 0;
			}

			for (i = 0; i < nItems; i++)
			{
				lvItem.mask = LVIF_PARAM;
				lvItem.iItem = i;
				lvItem.iSubItem = 0;
				bRes = ListView_GetItem(m_listView, &lvItem);

				if (bRes)
				{
					if (i == *index)
					{
						m_itemInfoMap.at((int) lvItem.lParam).iRelativeSort = iInsert;
					}
					else
					{
						if (iSort == iInsert)
						{
							iSort++;
						}

						m_itemInfoMap.at((int) lvItem.lParam).iRelativeSort = iSort;
					}
				}

				iSort++;
			}

			ListView_SortItems(m_listView, SortTemporaryStub, (LPARAM) this);
		}
		else
		{
			/* If auto arrange is on, the dropped item(s) will
			have to be 'snapped' to the nearest item position.
			Otherwise, they may simply be placed where they are
			dropped. */
			if (m_folderSettings.autoArrangeEnabled)
			{
				LVFINDINFO lvfi;
				LVHITTESTINFO lvhti;
				RECT rcItem;
				POINT ptNext;
				BOOL bRowEnd = FALSE;
				BOOL bRowStart = FALSE;
				int iNext;
				int iHitItem;
				int nItems;

				lvhti.pt = pt;
				iHitItem = ListView_HitTest(m_listView, &lvhti);

				/* Based on ListView_PositionInsertMark() code. */
				if (iHitItem != -1 && lvhti.flags & LVHT_ONITEM)
				{
					ListView_GetItemRect(m_listView, lvhti.iItem, &rcItem, LVIR_BOUNDS);

					if ((pt.x - rcItem.left) > ((rcItem.right - rcItem.left) / 2))
					{
						iNext = iHitItem;
					}
					else
					{
						/* Can just insert the item _after_ the item to the
						left, unless this is the start of a row. */
						iNext = ListView_GetNextItem(m_listView, iHitItem, LVNI_TOLEFT);

						if (iNext == -1)
						{
							iNext = iHitItem;
						}

						bRowStart = (ListView_GetNextItem(m_listView, iNext, LVNI_TOLEFT) == -1);
					}
				}
				else
				{
					lvfi.flags = LVFI_NEARESTXY;
					lvfi.pt = pt;
					lvfi.vkDirection = VK_UP;
					iNext = ListView_FindItem(m_listView, -1, &lvfi);

					if (iNext == -1)
					{
						lvfi.flags = LVFI_NEARESTXY;
						lvfi.pt = pt;
						lvfi.vkDirection = VK_LEFT;
						iNext = ListView_FindItem(m_listView, -1, &lvfi);
					}

					ListView_GetItemRect(m_listView, iNext, &rcItem, LVIR_BOUNDS);

					if (pt.x > rcItem.left + ((rcItem.right - rcItem.left) / 2))
					{
						if (pt.y > rcItem.bottom)
						{
							int iBelow;

							iBelow = ListView_GetNextItem(m_listView, iNext, LVNI_BELOW);

							if (iBelow != -1)
							{
								iNext = iBelow;
							}
						}

						bRowEnd = TRUE;
					}

					nItems = ListView_GetItemCount(m_listView);

					ListView_GetItemRect(m_listView, nItems - 1, &rcItem, LVIR_BOUNDS);

					if ((pt.x > rcItem.left + ((rcItem.right - rcItem.left) / 2))
						&& pt.x < rcItem.right + ((rcItem.right - rcItem.left) / 2) + 2
						&& pt.y > rcItem.top)
					{
						iNext = nItems - 1;

						bRowEnd = TRUE;
					}

					if (!bRowEnd)
					{
						int iLeft;

						iLeft = ListView_GetNextItem(m_listView, iNext, LVNI_TOLEFT);

						if (iLeft != -1)
						{
							iNext = iLeft;
						}
						else
						{
							bRowStart = TRUE;
						}
					}
				}

				ListView_GetItemPosition(m_listView, iNext, &ptNext);

				/* Offset by 1 pixel in the x-direction. This ensures that
				the dropped item will always be placed AFTER iNext. */
				if (bRowStart)
				{
					/* If at the start of a row, simply place at x = 0
					so that dropped item will be placed before first
					item... */
					ListView_SetItemPosition32(m_listView, *index, 0, ptNext.y);
				}
				else
				{
					ListView_SetItemPosition32(m_listView, *index, ptNext.x + 1, ptNext.y);
				}
			}
			else
			{
				ListView_GetOrigin(m_listView, &ptOrigin);

				/* ListView may be scrolled horizontally or vertically. */
				ListView_SetItemPosition32(m_listView, *index,
					ptOrigin.x + pt.x - m_ptDraggedOffset.x,
					ptOrigin.y + pt.y - m_ptDraggedOffset.y);
			}
		}
	}

	if (m_folderSettings.autoArrangeEnabled)
	{
		ListViewHelper::SetAutoArrange(m_listView, true);
	}

	m_performingDrag = false;
}

void ShellBrowserImpl::ScrollListViewForDrop(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_listView, &ptClient);

	if (!res)
	{
		return;
	}

	RECT rc;
	res = GetClientRect(m_listView, &rc);

	if (!res)
	{
		return;
	}

	LONG_PTR fStyle;

	fStyle = GetWindowLongPtr(m_listView, GWL_STYLE);

	/* The listview can be scrolled only if there
	is a scrollbar present. */
	if ((fStyle & WS_HSCROLL) == WS_HSCROLL)
	{
		if (ptClient.x < MIN_X_POS)
		{
			ListView_Scroll(m_listView, -X_SCROLL_AMOUNT, 0);
		}
		else if (ptClient.x > (rc.right - MIN_X_POS))
		{
			ListView_Scroll(m_listView, X_SCROLL_AMOUNT, 0);
		}
	}

	if ((fStyle & WS_VSCROLL) == WS_VSCROLL)
	{
		if (ptClient.y < MIN_Y_POS)
		{
			ListView_Scroll(m_listView, 0, -Y_SCROLL_AMOUNT);
		}
		else if (ptClient.y > (rc.bottom - MIN_Y_POS))
		{
			ListView_Scroll(m_listView, 0, Y_SCROLL_AMOUNT);
		}
	}
}

void ShellBrowserImpl::ResetDropUiState()
{
	ListViewHelper::PositionInsertMark(m_listView, nullptr);

	ListView_SetItemState(m_listView, -1, 0, LVIS_DROPHILITED);
}
