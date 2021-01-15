// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "MainResource.h"
#include "../Helper/ListViewHelper.h"

std::wstring ShellBrowser::GetFilter() const
{
	return m_folderSettings.filter;
}

void ShellBrowser::SetFilter(std::wstring_view filter)
{
	m_folderSettings.filter = filter;

	if (m_folderSettings.applyFilter)
	{
		UnfilterAllItems();
		UpdateFiltering();
	}
}

void ShellBrowser::SetFilterStatus(BOOL bFilter)
{
	m_folderSettings.applyFilter = bFilter;

	UpdateFiltering();
}

BOOL ShellBrowser::GetFilterStatus() const
{
	return m_folderSettings.applyFilter;
}

void ShellBrowser::SetFilterCaseSensitive(BOOL filterCaseSensitive)
{
	m_folderSettings.filterCaseSensitive = filterCaseSensitive;
}

BOOL ShellBrowser::GetFilterCaseSensitive() const
{
	return m_folderSettings.filterCaseSensitive;
}

void ShellBrowser::UpdateFiltering()
{
	if (m_folderSettings.applyFilter)
	{
		RemoveFilteredItems();

		ApplyFilteringBackgroundImage(true);
	}
	else
	{
		UnfilterAllItems();

		if (m_directoryState.numItems == 0)
		{
			ApplyFolderEmptyBackgroundImage(true);
		}
		else
		{
			ApplyFilteringBackgroundImage(false);
		}
	}
}

void ShellBrowser::RemoveFilteredItems()
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

void ShellBrowser::RemoveFilteredItem(int iItem, int iItemInternal)
{
	ULARGE_INTEGER ulFileSize;

	const auto &item = m_itemInfoMap.at(iItemInternal);

	if (ListView_GetItemState(m_hListView, iItem, LVIS_SELECTED) == LVIS_SELECTED)
	{
		ulFileSize.LowPart = item.wfd.nFileSizeLow;
		ulFileSize.HighPart = item.wfd.nFileSizeHigh;

		m_directoryState.fileSelectionSize.QuadPart -= ulFileSize.QuadPart;
	}

	/* Take the file size of the removed file away from the total
	directory size. */
	ulFileSize.LowPart = item.wfd.nFileSizeLow;
	ulFileSize.HighPart = item.wfd.nFileSizeHigh;

	m_directoryState.totalDirSize.QuadPart -= ulFileSize.QuadPart;

	/* Remove the item from the m_hListView. */
	ListView_DeleteItem(m_hListView, iItem);

	m_directoryState.numItems--;

	assert(m_directoryState.filteredItemsList.count(iItemInternal) == 0);
	m_directoryState.filteredItemsList.insert(iItemInternal);
}

BOOL ShellBrowser::IsFilenameFiltered(const TCHAR *FileName) const
{
	if (CheckWildcardMatch(
			m_folderSettings.filter.c_str(), FileName, m_folderSettings.filterCaseSensitive))
	{
		return FALSE;
	}

	return TRUE;
}

void ShellBrowser::UnfilterAllItems()
{
	for (int internalIndex : m_directoryState.filteredItemsList)
	{
		RestoreFilteredItem(internalIndex);
	}

	m_directoryState.filteredItemsList.clear();
	SendMessage(m_hOwner, WM_USER_UPDATEWINDOWS, 0, 0);
}

void ShellBrowser::UnfilterItem(int internalIndex)
{
	assert(m_directoryState.filteredItemsList.count(internalIndex) == 1);

	RestoreFilteredItem(internalIndex);
	m_directoryState.filteredItemsList.erase(internalIndex);
	SendMessage(m_hOwner, WM_USER_UPDATEWINDOWS, 0, 0);
}

void ShellBrowser::RestoreFilteredItem(int internalIndex)
{
	int sortedPosition = DetermineItemSortedPosition(internalIndex);

	AwaitingAdd_t awaitingAdd;
	awaitingAdd.iItem = sortedPosition;
	awaitingAdd.bPosition = TRUE;
	awaitingAdd.iAfter = sortedPosition - 1;
	awaitingAdd.iItemInternal = internalIndex;
	m_directoryState.awaitingAddList.push_back(awaitingAdd);

	InsertAwaitingItems(m_folderSettings.showInGroups);
}

void ShellBrowser::ApplyFilteringBackgroundImage(bool apply)
{
	if (apply)
	{
		ListViewHelper::SetBackgroundImage(m_hListView, IDB_FILTERINGAPPLIED);
	}
	else
	{
		ListViewHelper::SetBackgroundImage(m_hListView, NULL);
	}
}