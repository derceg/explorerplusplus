// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "MainResource.h"
#include "../Helper/ListViewHelper.h"

std::wstring ShellBrowserImpl::GetFilterText() const
{
	return m_folderSettings.filter;
}

void ShellBrowserImpl::SetFilterText(std::wstring_view filter)
{
	m_folderSettings.filter = filter;

	if (m_folderSettings.applyFilter)
	{
		UnfilterAllItems();
		UpdateFiltering();
	}
}

void ShellBrowserImpl::SetFilterApplied(bool filter)
{
	m_folderSettings.applyFilter = filter;

	UpdateFiltering();
}

bool ShellBrowserImpl::IsFilterApplied() const
{
	return m_folderSettings.applyFilter;
}

void ShellBrowserImpl::SetFilterCaseSensitive(bool filterCaseSensitive)
{
	m_folderSettings.filterCaseSensitive = filterCaseSensitive;
}

bool ShellBrowserImpl::GetFilterCaseSensitive() const
{
	return m_folderSettings.filterCaseSensitive;
}

void ShellBrowserImpl::UpdateFiltering()
{
	if (m_folderSettings.applyFilter)
	{
		RemoveFilteredItems();
	}
	else
	{
		UnfilterAllItems();
	}
}

void ShellBrowserImpl::RemoveFilteredItems()
{
	if (!m_folderSettings.applyFilter)
	{
		return;
	}

	int nItems = ListView_GetItemCount(m_hListView);

	for (int i = nItems - 1; i >= 0; i--)
	{
		int internalIndex = GetItemInternalIndex(i);

		if (!((m_itemInfoMap.at(internalIndex).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				== FILE_ATTRIBUTE_DIRECTORY))
		{
			if (IsFilenameFiltered(m_itemInfoMap.at(internalIndex).displayName.c_str()))
			{
				RemoveFilteredItem(i, internalIndex);
			}
		}
	}

	SendMessage(m_hOwner, WM_USER_UPDATEWINDOWS, 0, 0);
}

void ShellBrowserImpl::RemoveFilteredItem(int iItem, int iItemInternal)
{
	ULARGE_INTEGER ulFileSize;

	const auto &item = m_itemInfoMap.at(iItemInternal);

	if (ListView_GetItemState(m_hListView, iItem, LVIS_SELECTED) == LVIS_SELECTED)
	{
		ulFileSize.LowPart = item.wfd.nFileSizeLow;
		ulFileSize.HighPart = item.wfd.nFileSizeHigh;

		m_directoryState.fileSelectionSize -= ulFileSize.QuadPart;
	}

	/* Take the file size of the removed file away from the total
	directory size. */
	ulFileSize.LowPart = item.wfd.nFileSizeLow;
	ulFileSize.HighPart = item.wfd.nFileSizeHigh;

	m_directoryState.totalDirSize -= ulFileSize.QuadPart;

	/* Remove the item from the m_hListView. */
	ListView_DeleteItem(m_hListView, iItem);

	m_directoryState.numItems--;

	assert(m_directoryState.filteredItemsList.count(iItemInternal) == 0);
	m_directoryState.filteredItemsList.insert(iItemInternal);
}

BOOL ShellBrowserImpl::IsFilenameFiltered(const TCHAR *FileName) const
{
	if (CheckWildcardMatch(m_folderSettings.filter.c_str(), FileName,
			m_folderSettings.filterCaseSensitive))
	{
		return FALSE;
	}

	return TRUE;
}

void ShellBrowserImpl::UnfilterAllItems()
{
	for (int internalIndex : m_directoryState.filteredItemsList)
	{
		RestoreFilteredItem(internalIndex);
	}

	m_directoryState.filteredItemsList.clear();
	SendMessage(m_hOwner, WM_USER_UPDATEWINDOWS, 0, 0);
}

void ShellBrowserImpl::UnfilterItem(int internalIndex)
{
	assert(m_directoryState.filteredItemsList.count(internalIndex) == 1);

	RestoreFilteredItem(internalIndex);
	m_directoryState.filteredItemsList.erase(internalIndex);
	SendMessage(m_hOwner, WM_USER_UPDATEWINDOWS, 0, 0);
}

void ShellBrowserImpl::RestoreFilteredItem(int internalIndex)
{
	int sortedPosition = DetermineItemSortedPosition(internalIndex);

	AwaitingAdd_t awaitingAdd;
	awaitingAdd.iItem = sortedPosition;
	awaitingAdd.bPosition = TRUE;
	awaitingAdd.iAfter = sortedPosition - 1;
	awaitingAdd.iItemInternal = internalIndex;
	m_directoryState.awaitingAddList.push_back(awaitingAdd);

	InsertAwaitingItems();
}
