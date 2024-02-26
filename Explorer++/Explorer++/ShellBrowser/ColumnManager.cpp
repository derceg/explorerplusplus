// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "ColumnDataRetrieval.h"
#include "Columns.h"
#include "Config.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SortModes.h"
#include "ViewModes.h"
#include <cassert>
#include <list>

void ShellBrowser::QueueColumnTask(int itemInternalIndex, ColumnType columnType)
{
	int columnResultID = m_columnResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(itemInternalIndex);
	GlobalFolderSettings globalFolderSettings = m_config->globalFolderSettings;

	auto result = m_columnThreadPool.push(
		[listView = m_hListView, columnResultID, columnType, itemInternalIndex, basicItemInfo,
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

ShellBrowser::ColumnResult_t ShellBrowser::GetColumnTextAsync(HWND listView, int columnResultId,
	ColumnType columnType, int internalIndex, const BasicItemInfo_t &basicItemInfo,
	const GlobalFolderSettings &globalFolderSettings)
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

void ShellBrowser::ProcessColumnResult(int columnResultId)
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
	ListView_SetItemText(m_hListView, *index, *columnIndex, columnText.get());

	m_columnResults.erase(itr);
}

std::optional<int> ShellBrowser::GetColumnIndexByType(ColumnType columnType) const
{
	HWND header = ListView_GetHeader(m_hListView);

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

std::optional<ColumnType> ShellBrowser::GetColumnTypeByIndex(int index) const
{
	HWND hHeader = ListView_GetHeader(m_hListView);

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

void ShellBrowser::AddFirstColumn()
{
	Column_t firstCheckedColumn = GetFirstCheckedColumn();
	InsertColumn(firstCheckedColumn.type, 0, firstCheckedColumn.width);
}

void ShellBrowser::SetUpListViewColumns()
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
		ListView_DeleteColumn(m_hListView, i);
	}

	m_nCurrentColumns = m_nActiveColumns;
}

void ShellBrowser::InsertColumn(ColumnType columnType, int columnIndex, int width)
{
	std::wstring columnText =
		ResourceHelper::LoadString(m_resourceInstance, LookupColumnNameStringIndex(columnType));

	LV_COLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.pszText = columnText.data();
	lvColumn.cx = width;

	if (columnType == +ColumnType::Size || columnType == +ColumnType::RealSize
		|| columnType == +ColumnType::TotalSize || columnType == +ColumnType::FreeSpace)
	{
		lvColumn.mask |= LVCF_FMT;
		lvColumn.fmt = LVCFMT_RIGHT;
	}

	int actualColumnIndex = ListView_InsertColumn(m_hListView, columnIndex, &lvColumn);

	HWND header = ListView_GetHeader(m_hListView);

	// Store the column's ID with the column itself.
	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	hdItem.lParam = static_cast<LPARAM>(columnType);
	Header_SetItem(header, actualColumnIndex, &hdItem);
}

void ShellBrowser::DeleteAllColumns()
{
	HWND header = ListView_GetHeader(m_hListView);
	int numColumns = Header_GetItemCount(header);

	if (numColumns == -1)
	{
		return;
	}

	for (int i = numColumns - 1; i >= 0; i--)
	{
		ListView_DeleteColumn(m_hListView, i);
	}

	m_PreviousSortColumnExists = false;
}

void ShellBrowser::SetActiveColumnSet()
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

SortMode ShellBrowser::DetermineColumnSortMode(ColumnType columnType)
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

int ShellBrowser::LookupColumnNameStringIndex(ColumnType columnType)
{
	switch (columnType)
	{
	case ColumnType::Name:
		return IDS_COLUMN_NAME_NAME;

	case ColumnType::Type:
		return IDS_COLUMN_NAME_TYPE;

	case ColumnType::Size:
		return IDS_COLUMN_NAME_SIZE;

	case ColumnType::DateModified:
		return IDS_COLUMN_NAME_DATEMODIFIED;

	case ColumnType::Attributes:
		return IDS_COLUMN_NAME_ATTRIBUTES;

	case ColumnType::RealSize:
		return IDS_COLUMN_NAME_REALSIZE;

	case ColumnType::ShortName:
		return IDS_COLUMN_NAME_SHORTNAME;

	case ColumnType::Owner:
		return IDS_COLUMN_NAME_OWNER;

	case ColumnType::ProductName:
		return IDS_COLUMN_NAME_PRODUCTNAME;

	case ColumnType::Company:
		return IDS_COLUMN_NAME_COMPANY;

	case ColumnType::Description:
		return IDS_COLUMN_NAME_DESCRIPTION;

	case ColumnType::FileVersion:
		return IDS_COLUMN_NAME_FILEVERSION;

	case ColumnType::ProductVersion:
		return IDS_COLUMN_NAME_PRODUCTVERSION;

	case ColumnType::ShortcutTo:
		return IDS_COLUMN_NAME_SHORTCUTTO;

	case ColumnType::HardLinks:
		return IDS_COLUMN_NAME_HARDLINKS;

	case ColumnType::Extension:
		return IDS_COLUMN_NAME_EXTENSION;

	case ColumnType::Created:
		return IDS_COLUMN_NAME_CREATED;

	case ColumnType::Accessed:
		return IDS_COLUMN_NAME_ACCESSED;

	case ColumnType::Title:
		return IDS_COLUMN_NAME_TITLE;

	case ColumnType::Subject:
		return IDS_COLUMN_NAME_SUBJECT;

	case ColumnType::Authors:
		return IDS_COLUMN_NAME_AUTHORS;

	case ColumnType::Keywords:
		return IDS_COLUMN_NAME_KEYWORDS;

	case ColumnType::Comment:
		return IDS_COLUMN_NAME_COMMENT;

	case ColumnType::CameraModel:
		return IDS_COLUMN_NAME_CAMERAMODEL;

	case ColumnType::DateTaken:
		return IDS_COLUMN_NAME_DATETAKEN;

	case ColumnType::Width:
		return IDS_COLUMN_NAME_WIDTH;

	case ColumnType::Height:
		return IDS_COLUMN_NAME_HEIGHT;

	case ColumnType::VirtualComments:
		return IDS_COLUMN_NAME_VIRTUALCOMMENTS;

	case ColumnType::TotalSize:
		return IDS_COLUMN_NAME_TOTALSIZE;

	case ColumnType::FreeSpace:
		return IDS_COLUMN_NAME_FREESPACE;

	case ColumnType::FileSystem:
		return IDS_COLUMN_NAME_FILESYSTEM;

	case ColumnType::OriginalLocation:
		return IDS_COLUMN_NAME_ORIGINALLOCATION;

	case ColumnType::DateDeleted:
		return IDS_COLUMN_NAME_DATEDELETED;

	case ColumnType::PrinterNumDocuments:
		return IDS_COLUMN_NAME_NUMPRINTERDOCUMENTS;

	case ColumnType::PrinterStatus:
		return IDS_COLUMN_NAME_PRINTERSTATUS;

	case ColumnType::PrinterComments:
		return IDS_COLUMN_NAME_PRINTERCOMMENTS;

	case ColumnType::PrinterLocation:
		return IDS_COLUMN_NAME_PRINTERLOCATION;

	case ColumnType::PrinterModel:
		return IDS_COLUMN_NAME_PRINTERMODEL;

	case ColumnType::NetworkAdaptorStatus:
		return IDS_COLUMN_NAME_NETWORKADAPTER_STATUS;

	case ColumnType::MediaBitrate:
		return IDS_COLUMN_NAME_BITRATE;

	case ColumnType::MediaCopyright:
		return IDS_COLUMN_NAME_COPYRIGHT;

	case ColumnType::MediaDuration:
		return IDS_COLUMN_NAME_DURATION;

	case ColumnType::MediaProtected:
		return IDS_COLUMN_NAME_PROTECTED;

	case ColumnType::MediaRating:
		return IDS_COLUMN_NAME_RATING;

	case ColumnType::MediaAlbumArtist:
		return IDS_COLUMN_NAME_ALBUMARTIST;

	case ColumnType::MediaAlbum:
		return IDS_COLUMN_NAME_ALBUM;

	case ColumnType::MediaBeatsPerMinute:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;

	case ColumnType::MediaComposer:
		return IDS_COLUMN_NAME_COMPOSER;

	case ColumnType::MediaConductor:
		return IDS_COLUMN_NAME_CONDUCTOR;

	case ColumnType::MediaDirector:
		return IDS_COLUMN_NAME_DIRECTOR;

	case ColumnType::MediaGenre:
		return IDS_COLUMN_NAME_GENRE;

	case ColumnType::MediaLanguage:
		return IDS_COLUMN_NAME_LANGUAGE;

	case ColumnType::MediaBroadcastDate:
		return IDS_COLUMN_NAME_BROADCASTDATE;

	case ColumnType::MediaChannel:
		return IDS_COLUMN_NAME_CHANNEL;

	case ColumnType::MediaStationName:
		return IDS_COLUMN_NAME_STATIONNAME;

	case ColumnType::MediaMood:
		return IDS_COLUMN_NAME_MOOD;

	case ColumnType::MediaParentalRating:
		return IDS_COLUMN_NAME_PARENTALRATING;

	case ColumnType::MediaParentalRatingReason:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;

	case ColumnType::MediaPeriod:
		return IDS_COLUMN_NAME_PERIOD;

	case ColumnType::MediaProducer:
		return IDS_COLUMN_NAME_PRODUCER;

	case ColumnType::MediaPublisher:
		return IDS_COLUMN_NAME_PUBLISHER;

	case ColumnType::MediaWriter:
		return IDS_COLUMN_NAME_WRITER;

	case ColumnType::MediaYear:
		return IDS_COLUMN_NAME_YEAR;

	default:
		assert(false);
		break;
	}

	return 0;
}

int ShellBrowser::LookupColumnDescriptionStringIndex(ColumnType columnType)
{
	switch (columnType)
	{
	case ColumnType::Name:
		return IDS_COLUMN_DESCRIPTION_NAME;

	case ColumnType::Type:
		return IDS_COLUMN_DESCRIPTION_TYPE;

	case ColumnType::Size:
		return IDS_COLUMN_DESCRIPTION_SIZE;

	case ColumnType::DateModified:
		return IDS_COLUMN_DESCRIPTION_MODIFIED;

	case ColumnType::Attributes:
		return IDS_COLUMN_DESCRIPTION_ATTRIBUTES;

	case ColumnType::RealSize:
		return IDS_COLUMN_DESCRIPTION_REALSIZE;

	case ColumnType::ShortName:
		return IDS_COLUMN_DESCRIPTION_SHORTNAME;

	case ColumnType::Owner:
		return IDS_COLUMN_DESCRIPTION_OWNER;

	case ColumnType::ProductName:
		return IDS_COLUMN_DESCRIPTION_PRODUCTNAME;

	case ColumnType::Company:
		return IDS_COLUMN_DESCRIPTION_COMPANY;

	case ColumnType::Description:
		return IDS_COLUMN_DESCRIPTION_DESCRIPTION;

	case ColumnType::FileVersion:
		return IDS_COLUMN_DESCRIPTION_FILEVERSION;

	case ColumnType::ProductVersion:
		return IDS_COLUMN_DESCRIPTION_PRODUCTVERSION;

	case ColumnType::ShortcutTo:
		return IDS_COLUMN_DESCRIPTION_SHORTCUTTO;

	case ColumnType::HardLinks:
		return IDS_COLUMN_DESCRIPTION_HARDLINKS;

	case ColumnType::Extension:
		return IDS_COLUMN_DESCRIPTION_EXTENSION;

	case ColumnType::Created:
		return IDS_COLUMN_DESCRIPTION_CREATED;

	case ColumnType::Accessed:
		return IDS_COLUMN_DESCRIPTION_ACCESSED;

	case ColumnType::Title:
		return IDS_COLUMN_DESCRIPTION_TITLE;

	case ColumnType::Subject:
		return IDS_COLUMN_DESCRIPTION_SUBJECT;

	case ColumnType::Authors:
		return IDS_COLUMN_DESCRIPTION_AUTHORS;

	case ColumnType::Keywords:
		return IDS_COLUMN_DESCRIPTION_KEYWORDS;

	case ColumnType::Comment:
		return IDS_COLUMN_DESCRIPTION_COMMENT;

	case ColumnType::CameraModel:
		return IDS_COLUMN_DESCRIPTION_CAMERAMODEL;

	case ColumnType::DateTaken:
		return IDS_COLUMN_DESCRIPTION_DATETAKEN;

	case ColumnType::Width:
		return IDS_COLUMN_DESCRIPTION_WIDTH;

	case ColumnType::Height:
		return IDS_COLUMN_DESCRIPTION_HEIGHT;

	case ColumnType::VirtualComments:
		return IDS_COLUMN_DESCRIPTION_COMMENT;

	case ColumnType::TotalSize:
		return IDS_COLUMN_DESCRIPTION_TOTALSIZE;

	case ColumnType::FreeSpace:
		return IDS_COLUMN_DESCRIPTION_FREESPACE;

	case ColumnType::FileSystem:
		return IDS_COLUMN_DESCRIPTION_FILESYSTEM;

	case ColumnType::PrinterNumDocuments:
		return IDS_COLUMN_DESCRIPTION_NUMPRINTERDOCUMENTS;

	case ColumnType::PrinterComments:
		return IDS_COLUMN_DESCRIPTION_PRINTERCOMMENTS;

	case ColumnType::PrinterLocation:
		return IDS_COLUMN_DESCRIPTION_PRINTERLOCATION;

	case ColumnType::NetworkAdaptorStatus:
		return IDS_COLUMN_DESCRIPTION_NETWORKADAPTER_STATUS;

	case ColumnType::MediaBitrate:
		return IDS_COLUMN_DESCRIPTION_BITRATE;

	default:
		assert(false);
		break;
	}

	return 0;
}

void ShellBrowser::ColumnClicked(int iClickedColumn)
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

void ShellBrowser::ApplyHeaderSortArrow()
{
	HWND hHeader;
	HDITEM hdItem;
	BOOL previousColumnFound = FALSE;
	int iColumn = 0;
	int iPreviousSortedColumn = 0;

	hHeader = ListView_GetHeader(m_hListView);

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

void ShellBrowser::ImportAllColumns(const FolderColumns &folderColumns)
{
	m_folderColumns = folderColumns;
}

FolderColumns ShellBrowser::ExportAllColumns()
{
	SaveColumnWidths();

	return m_folderColumns;
}

void ShellBrowser::SaveColumnWidths()
{
	std::vector<Column_t> *pActiveColumns = nullptr;
	int iColumn = 0;

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

	/* Only save column widths if the listview is currently in
	details view. If it's not currently in details view, then
	column widths have already been saved when the view changed. */
	if (m_folderSettings.viewMode == +ViewMode::Details)
	{
		for (auto itr = pActiveColumns->begin(); itr != pActiveColumns->end(); itr++)
		{
			if (itr->checked)
			{
				itr->width = ListView_GetColumnWidth(m_hListView, iColumn);

				iColumn++;
			}
		}
	}
}

std::vector<Column_t> ShellBrowser::GetCurrentColumns()
{
	if (m_folderSettings.viewMode == +ViewMode::Details)
	{
		SaveColumnWidths();
	}

	return *m_pActiveColumns;
}

void ShellBrowser::SetCurrentColumns(const std::vector<Column_t> &columns)
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
			assert(firstChecked != columns.end());

			m_folderSettings.sortMode = DetermineColumnSortMode(firstChecked->type);
			sortFolder = true;
		}

		if (m_folderSettings.viewMode != +ViewMode::Details)
		{
			continue;
		}

		auto existingColumn = std::find_if(m_pActiveColumns->begin(), m_pActiveColumns->end(),
			[column](const Column_t &currentColumn) { return currentColumn.type == column.type; });
		assert(existingColumn != m_pActiveColumns->end());

		if (column.checked && !existingColumn->checked)
		{
			InsertColumn(column.type, columnIndex, column.width);
		}
		else if (!column.checked && existingColumn->checked)
		{
			ListView_DeleteColumn(m_hListView, columnIndex);
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

	columnsChanged.m_signal();
}

void ShellBrowser::GetColumnInternal(ColumnType columnType, Column_t *pci) const
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

Column_t ShellBrowser::GetFirstCheckedColumn()
{
	auto itr = std::find_if(m_pActiveColumns->begin(), m_pActiveColumns->end(),
		[](const Column_t &column) { return column.checked; });

	// There should always be at least one checked column.
	assert(itr != m_pActiveColumns->end());

	return *itr;
}
