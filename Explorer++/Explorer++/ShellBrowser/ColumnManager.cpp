// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "ColumnDataRetrieval.h"
#include "ColumnHelper.h"
#include "Columns.h"
#include "Config.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SortModes.h"
#include "ViewModes.h"
#include <cassert>
#include <list>

void ShellBrowserImpl::QueueColumnTask(int itemInternalIndex, ColumnType columnType)
{
	int columnResultID = m_columnResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(itemInternalIndex);
	GlobalFolderSettings globalFolderSettings = m_config->globalFolderSettings;

	auto result = m_columnThreadPool.push(
		[listView = m_listView, columnResultID, columnType, itemInternalIndex, basicItemInfo,
			globalFolderSettings](int id)
		{
			UNREFERENCED_PARAMETER(id);

			return GetColumnTextAsync(listView, columnResultID, columnType, itemInternalIndex,
				basicItemInfo, globalFolderSettings);
		});

	// The function call above might finish before this line runs,
	// but that doesn't matter, as the results won't be processed
	// until a message posted to the main thread has been handled
	// (which can only occur after this function has returned).
	m_columnResults.insert({ columnResultID, std::move(result) });
}

ShellBrowserImpl::ColumnResult_t ShellBrowserImpl::GetColumnTextAsync(HWND listView,
	int columnResultId, ColumnType columnType, int internalIndex,
	const BasicItemInfo_t &basicItemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	std::wstring columnText = GetColumnText(columnType, basicItemInfo, globalFolderSettings);

	// This message may be delivered before this function has returned.
	// That doesn't actually matter, since the message handler will
	// simply wait for the result to be returned.
	PostMessage(listView, WM_APP_COLUMN_RESULT_READY, columnResultId, 0);

	ColumnResult_t result;
	result.itemInternalIndex = internalIndex;
	result.columnType = columnType;
	result.columnText = columnText;

	return result;
}

void ShellBrowserImpl::ProcessColumnResult(int columnResultId)
{
	auto itr = m_columnResults.find(columnResultId);

	if (itr == m_columnResults.end())
	{
		// This result is for a previous folder. It can be ignored.
		return;
	}

	if (m_folderSettings.viewMode != +ViewMode::Details)
	{
		return;
	}

	auto result = itr->second.get();

	auto index = LocateItemByInternalIndex(result.itemInternalIndex);

	if (!index)
	{
		// This is a valid state. The item may simply have been deleted.
		return;
	}

	auto columnIndex = GetColumnIndexByType(result.columnType);

	if (!columnIndex)
	{
		// This is also a valid state. The column may have been removed.
		return;
	}

	auto columnText = std::make_unique<TCHAR[]>(result.columnText.size() + 1);
	StringCchCopy(columnText.get(), result.columnText.size() + 1, result.columnText.c_str());
	ListView_SetItemText(m_listView, *index, *columnIndex, columnText.get());

	m_columnResults.erase(itr);
}

std::optional<int> ShellBrowserImpl::GetColumnIndexByType(ColumnType columnType) const
{
	HWND header = ListView_GetHeader(m_listView);

	int numItems = Header_GetItemCount(header);

	for (int i = 0; i < numItems; i++)
	{
		HDITEM hdItem;
		hdItem.mask = HDI_LPARAM;
		BOOL res = Header_GetItem(header, i, &hdItem);

		if (!res)
		{
			continue;
		}

		if (static_cast<ColumnType::_integral>(hdItem.lParam) == columnType._to_integral())
		{
			return i;
		}
	}

	return std::nullopt;
}

std::optional<ColumnType> ShellBrowserImpl::GetColumnTypeByIndex(int index) const
{
	HWND hHeader = ListView_GetHeader(m_listView);

	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	BOOL res = Header_GetItem(hHeader, index, &hdItem);

	if (!res)
	{
		return std::nullopt;
	}

	auto columnType =
		ColumnType::_from_integral_nothrow(static_cast<ColumnType::_integral>(hdItem.lParam));
	CHECK(columnType);

	return *columnType;
}

void ShellBrowserImpl::AddFirstColumn()
{
	Column_t firstCheckedColumn = GetFirstCheckedColumn();
	InsertColumn(firstCheckedColumn.type, 0, firstCheckedColumn.width);
}

void ShellBrowserImpl::SetUpListViewColumns()
{
	m_nActiveColumns = 0;

	int currentIndex = 0;

	for (const Column_t &column : *m_pActiveColumns)
	{
		if (!column.checked)
		{
			continue;
		}

		InsertColumn(column.type, currentIndex, column.width);

		/* Do NOT set column widths here. For some reason, this causes list mode to
		break. (If this code is active, and the listview starts of in details mode
		and is then switched to list mode, no items will be shown; they appear to
		be placed off the left edge of the listview). */
		// ListView_SetColumnWidth(m_hListView,iColumnIndex,LVSCW_AUTOSIZE_USEHEADER);

		currentIndex++;
		m_nActiveColumns++;
	}

	for (int i = m_nCurrentColumns + m_nActiveColumns; i >= m_nActiveColumns; i--)
	{
		ListView_DeleteColumn(m_listView, i);
	}

	m_nCurrentColumns = m_nActiveColumns;
}

void ShellBrowserImpl::InsertColumn(ColumnType columnType, int columnIndex, int width)
{
	std::wstring columnName = GetColumnName(m_app->GetResourceLoader(), columnType);

	LV_COLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.pszText = columnName.data();
	lvColumn.cx = width;

	if (columnType == +ColumnType::Size || columnType == +ColumnType::RealSize
		|| columnType == +ColumnType::TotalSize || columnType == +ColumnType::FreeSpace)
	{
		lvColumn.mask |= LVCF_FMT;
		lvColumn.fmt = LVCFMT_RIGHT;
	}

	int actualColumnIndex = ListView_InsertColumn(m_listView, columnIndex, &lvColumn);

	HWND header = ListView_GetHeader(m_listView);

	// Store the column's ID with the column itself.
	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	hdItem.lParam = static_cast<LPARAM>(columnType);
	Header_SetItem(header, actualColumnIndex, &hdItem);
}

void ShellBrowserImpl::DeleteAllColumns()
{
	HWND header = ListView_GetHeader(m_listView);
	int numColumns = Header_GetItemCount(header);

	if (numColumns == -1)
	{
		return;
	}

	for (int i = numColumns - 1; i >= 0; i--)
	{
		ListView_DeleteColumn(m_listView, i);
	}

	m_PreviousSortColumnExists = false;
}

void ShellBrowserImpl::SetActiveColumnSet()
{
	std::vector<Column_t> *pActiveColumns = nullptr;

	if (CompareVirtualFolders(CSIDL_CONTROLS))
	{
		pActiveColumns = &m_folderColumns.controlPanelColumns;
	}
	else if (CompareVirtualFolders(CSIDL_DRIVES))
	{
		pActiveColumns = &m_folderColumns.myComputerColumns;
	}
	else if (CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		pActiveColumns = &m_folderColumns.recycleBinColumns;
	}
	else if (CompareVirtualFolders(CSIDL_PRINTERS))
	{
		pActiveColumns = &m_folderColumns.printersColumns;
	}
	else if (CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		pActiveColumns = &m_folderColumns.networkConnectionsColumns;
	}
	else if (CompareVirtualFolders(CSIDL_NETWORK))
	{
		pActiveColumns = &m_folderColumns.myNetworkPlacesColumns;
	}
	else
	{
		pActiveColumns = &m_folderColumns.realFolderColumns;
	}

	/* If the current set of columns are different
	from the previous set of columns (i.e. the
	current folder and previous folder are of a
	different 'type'), set the new columns, and
	place them (else do nothing). */
	if (m_pActiveColumns != pActiveColumns)
	{
		m_pActiveColumns = pActiveColumns;
	}
}

SortMode ShellBrowserImpl::DetermineColumnSortMode(ColumnType columnType)
{
	switch (columnType)
	{
	case ColumnType::Name:
		return SortMode::Name;

	case ColumnType::Type:
		return SortMode::Type;

	case ColumnType::Size:
		return SortMode::Size;

	case ColumnType::DateModified:
		return SortMode::DateModified;

	case ColumnType::Attributes:
		return SortMode::Attributes;

	case ColumnType::RealSize:
		return SortMode::RealSize;

	case ColumnType::ShortName:
		return SortMode::ShortName;

	case ColumnType::Owner:
		return SortMode::Owner;

	case ColumnType::ProductName:
		return SortMode::ProductName;

	case ColumnType::Company:
		return SortMode::Company;

	case ColumnType::Description:
		return SortMode::Description;

	case ColumnType::FileVersion:
		return SortMode::FileVersion;

	case ColumnType::ProductVersion:
		return SortMode::ProductVersion;

	case ColumnType::ShortcutTo:
		return SortMode::ShortcutTo;

	case ColumnType::HardLinks:
		return SortMode::HardLinks;

	case ColumnType::Extension:
		return SortMode::Extension;

	case ColumnType::Created:
		return SortMode::Created;

	case ColumnType::Accessed:
		return SortMode::Accessed;

	case ColumnType::Title:
		return SortMode::Title;

	case ColumnType::Subject:
		return SortMode::Subject;

	case ColumnType::Authors:
		return SortMode::Authors;

	case ColumnType::Keywords:
		return SortMode::Keywords;

	case ColumnType::Comment:
		return SortMode::Comments;

	case ColumnType::CameraModel:
		return SortMode::CameraModel;

	case ColumnType::DateTaken:
		return SortMode::DateTaken;

	case ColumnType::Width:
		return SortMode::Width;

	case ColumnType::Height:
		return SortMode::Height;

	case ColumnType::VirtualComments:
		return SortMode::VirtualComments;

	case ColumnType::TotalSize:
		return SortMode::TotalSize;

	case ColumnType::FreeSpace:
		return SortMode::FreeSpace;

	case ColumnType::FileSystem:
		return SortMode::FileSystem;

	case ColumnType::OriginalLocation:
		return SortMode::OriginalLocation;

	case ColumnType::DateDeleted:
		return SortMode::DateDeleted;

	case ColumnType::PrinterNumDocuments:
		return SortMode::NumPrinterDocuments;

	case ColumnType::PrinterStatus:
		return SortMode::PrinterStatus;

	case ColumnType::PrinterComments:
		return SortMode::PrinterComments;

	case ColumnType::PrinterLocation:
		return SortMode::PrinterLocation;

	case ColumnType::NetworkAdaptorStatus:
		return SortMode::NetworkAdapterStatus;

	case ColumnType::MediaBitrate:
		return SortMode::MediaBitrate;

	case ColumnType::MediaCopyright:
		return SortMode::MediaCopyright;

	case ColumnType::MediaDuration:
		return SortMode::MediaDuration;

	case ColumnType::MediaProtected:
		return SortMode::MediaProtected;

	case ColumnType::MediaRating:
		return SortMode::MediaRating;

	case ColumnType::MediaAlbumArtist:
		return SortMode::MediaAlbumArtist;

	case ColumnType::MediaAlbum:
		return SortMode::MediaAlbum;

	case ColumnType::MediaBeatsPerMinute:
		return SortMode::MediaBeatsPerMinute;

	case ColumnType::MediaComposer:
		return SortMode::MediaComposer;

	case ColumnType::MediaConductor:
		return SortMode::MediaConductor;

	case ColumnType::MediaDirector:
		return SortMode::MediaDirector;

	case ColumnType::MediaGenre:
		return SortMode::MediaGenre;

	case ColumnType::MediaLanguage:
		return SortMode::MediaLanguage;

	case ColumnType::MediaBroadcastDate:
		return SortMode::MediaBroadcastDate;

	case ColumnType::MediaChannel:
		return SortMode::MediaChannel;

	case ColumnType::MediaStationName:
		return SortMode::MediaStationName;

	case ColumnType::MediaMood:
		return SortMode::MediaMood;

	case ColumnType::MediaParentalRating:
		return SortMode::MediaParentalRating;

	case ColumnType::MediaParentalRatingReason:
		return SortMode::MediaParentalRatingReason;

	case ColumnType::MediaPeriod:
		return SortMode::MediaPeriod;

	case ColumnType::MediaProducer:
		return SortMode::MediaProducer;

	case ColumnType::MediaPublisher:
		return SortMode::MediaPublisher;

	case ColumnType::MediaWriter:
		return SortMode::MediaWriter;

	case ColumnType::MediaYear:
		return SortMode::MediaYear;

	default:
		assert(false);
		break;
	}

	return SortMode::Name;
}

void ShellBrowserImpl::ColumnClicked(int iClickedColumn)
{
	int iCurrentColumn = 0;
	SortMode sortMode = SortMode::Name;
	ColumnType columnType;

	for (auto itr = m_pActiveColumns->begin(); itr != m_pActiveColumns->end(); itr++)
	{
		/* Only increment if this column is actually been shown. */
		if (itr->checked)
		{
			if (iCurrentColumn == iClickedColumn)
			{
				sortMode = DetermineColumnSortMode(itr->type);
				columnType = itr->type;

				if (m_previousSortColumn == columnType)
				{
					m_folderSettings.sortDirection =
						InvertSortDirection(m_folderSettings.sortDirection);
				}
				else
				{
					m_folderSettings.sortMode = sortMode;
				}

				SortFolder();

				break;
			}

			iCurrentColumn++;
		}
	}
}

void ShellBrowserImpl::ApplyHeaderSortArrow()
{
	HWND hHeader;
	HDITEM hdItem;
	BOOL previousColumnFound = FALSE;
	int iColumn = 0;
	int iPreviousSortedColumn = 0;

	hHeader = ListView_GetHeader(m_listView);

	if (m_PreviousSortColumnExists)
	{
		/* Search through the currently active columns to find the column that previously
		had the up/down arrow. */
		for (auto itr = m_pActiveColumns->begin(); itr != m_pActiveColumns->end(); itr++)
		{
			/* Only increment if this column is actually been shown. */
			if (itr->checked)
			{
				if (m_previousSortColumn == itr->type)
				{
					previousColumnFound = TRUE;
					break;
				}

				iPreviousSortedColumn++;
			}
		}

		if (previousColumnFound)
		{
			hdItem.mask = HDI_FORMAT;
			Header_GetItem(hHeader, iPreviousSortedColumn, &hdItem);

			if (hdItem.fmt & HDF_SORTUP)
			{
				hdItem.fmt &= ~HDF_SORTUP;
			}
			else if (hdItem.fmt & HDF_SORTDOWN)
			{
				hdItem.fmt &= ~HDF_SORTDOWN;
			}

			/* Remove the up/down arrow from the column by which
			results were previously sorted. */
			Header_SetItem(hHeader, iPreviousSortedColumn, &hdItem);
		}
	}

	/* Find the index of the column representing the current sort mode. */
	for (auto itr = m_pActiveColumns->begin(); itr != m_pActiveColumns->end(); itr++)
	{
		if (itr->checked)
		{
			if (DetermineColumnSortMode(itr->type) == m_folderSettings.sortMode)
			{
				m_previousSortColumn = itr->type;
				m_PreviousSortColumnExists = true;
				break;
			}

			iColumn++;
		}
	}

	hdItem.mask = HDI_FORMAT;
	Header_GetItem(hHeader, iColumn, &hdItem);

	if (m_folderSettings.sortDirection == +SortDirection::Ascending)
	{
		hdItem.fmt |= HDF_SORTUP;
	}
	else
	{
		hdItem.fmt |= HDF_SORTDOWN;
	}

	/* Add the up/down arrow to the column by which
	items are now sorted. */
	Header_SetItem(hHeader, iColumn, &hdItem);
}

void ShellBrowserImpl::SetAllColumnSets(const FolderColumns &folderColumns)
{
	m_folderColumns = folderColumns;
}

const FolderColumns &ShellBrowserImpl::GetAllColumnSets() const
{
	return m_folderColumns;
}

const std::vector<Column_t> &ShellBrowserImpl::GetCurrentColumnSet() const
{
	return *m_pActiveColumns;
}

void ShellBrowserImpl::SetCurrentColumnSet(const std::vector<Column_t> &columns)
{
	bool sortFolder = false;
	int columnIndex = 0;

	for (auto &column : columns)
	{
		// Check if this column represents the current sorting mode. If it does, and it is being
		// removed, set the sort mode back to the first checked column.
		if (!column.checked && DetermineColumnSortMode(column.type) == m_folderSettings.sortMode)
		{
			auto firstChecked = std::find_if(columns.begin(), columns.end(),
				[](const Column_t &currentColumn) { return currentColumn.checked; });
			CHECK(firstChecked != columns.end());

			m_folderSettings.sortMode = DetermineColumnSortMode(firstChecked->type);
			sortFolder = true;
		}

		if (m_folderSettings.viewMode != +ViewMode::Details)
		{
			continue;
		}

		auto existingColumn = std::find_if(m_pActiveColumns->begin(), m_pActiveColumns->end(),
			[column](const Column_t &currentColumn) { return currentColumn.type == column.type; });
		CHECK(existingColumn != m_pActiveColumns->end());

		if (column.checked && !existingColumn->checked)
		{
			InsertColumn(column.type, columnIndex, column.width);
		}
		else if (!column.checked && existingColumn->checked)
		{
			ListView_DeleteColumn(m_listView, columnIndex);
		}

		if (column.checked)
		{
			columnIndex++;
		}
	}

	*m_pActiveColumns = columns;

	// The folder will need to be re-sorted if the sorting column was removed.
	if (sortFolder)
	{
		SortFolder();
	}
}

void ShellBrowserImpl::GetColumnInternal(ColumnType columnType, Column_t *pci) const
{
	for (auto itr = m_pActiveColumns->begin(); itr != m_pActiveColumns->end(); itr++)
	{
		if (itr->type == columnType)
		{
			*pci = *itr;
			return;
		}
	}
}

Column_t ShellBrowserImpl::GetFirstCheckedColumn()
{
	auto itr = std::find_if(m_pActiveColumns->begin(), m_pActiveColumns->end(),
		[](const Column_t &column) { return column.checked; });

	// There should always be at least one checked column.
	CHECK(itr != m_pActiveColumns->end());

	return *itr;
}
