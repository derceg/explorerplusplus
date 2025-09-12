// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "FilterDialog.h"
#include "MainResource.h"
#include "../Helper/ListViewHelper.h"

std::wstring ShellBrowserImpl::GetFilterText() const
{
	return m_folderSettings.filter;
}

void ShellBrowserImpl::SetFilterText(const std::wstring &filter)
{
	if (filter == m_folderSettings.filter)
	{
		return;
	}

	m_folderSettings.filter = filter;

	if (m_folderSettings.filterEnabled)
	{
		UnfilterAllItems();
		UpdateFiltering();
	}
}

bool ShellBrowserImpl::IsFilterCaseSensitive() const
{
	return m_folderSettings.filterCaseSensitive;
}

void ShellBrowserImpl::SetFilterCaseSensitive(bool caseSensitive)
{
	if (caseSensitive == m_folderSettings.filterCaseSensitive)
	{
		return;
	}

	m_folderSettings.filterCaseSensitive = caseSensitive;

	if (m_folderSettings.filterEnabled)
	{
		UnfilterAllItems();
		UpdateFiltering();
	}
}

bool ShellBrowserImpl::IsFilterEnabled() const
{
	return m_folderSettings.filterEnabled;
}

void ShellBrowserImpl::SetFilterEnabled(bool enabled)
{
	if (enabled == m_folderSettings.filterEnabled)
	{
		return;
	}

	m_folderSettings.filterEnabled = enabled;

	UpdateFiltering();
}

void ShellBrowserImpl::UpdateFiltering()
{
	if (m_folderSettings.filterEnabled)
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
	if (!m_folderSettings.filterEnabled)
	{
		return;
	}

	int nItems = ListView_GetItemCount(m_listView);

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

	m_app->GetShellBrowserEvents()->NotifyItemsChanged(this);
}

void ShellBrowserImpl::RemoveFilteredItem(int iItem, int iItemInternal)
{
	ULARGE_INTEGER ulFileSize;

	const auto &item = m_itemInfoMap.at(iItemInternal);

	if (ListView_GetItemState(m_listView, iItem, LVIS_SELECTED) == LVIS_SELECTED)
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
	ListView_DeleteItem(m_listView, iItem);

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
	m_app->GetShellBrowserEvents()->NotifyItemsChanged(this);
}

void ShellBrowserImpl::UnfilterItem(int internalIndex)
{
	assert(m_directoryState.filteredItemsList.count(internalIndex) == 1);

	RestoreFilteredItem(internalIndex);
	m_directoryState.filteredItemsList.erase(internalIndex);
	m_app->GetShellBrowserEvents()->NotifyItemsChanged(this);
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

void ShellBrowserImpl::EditFilterSettings()
{
	auto *filterDialog = FilterDialog::Create(m_app->GetResourceLoader(), m_owner, this);
	filterDialog->ShowModalDialog();
}
