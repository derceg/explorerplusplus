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
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/Macros.h"
#include <cassert>
#include <list>

void ShellBrowser::QueueColumnTask(int itemInternalIndex, int columnIndex)
{
	auto columnID = GetColumnIdByIndex(columnIndex);

	if (!columnID)
	{
		return;
	}

	int columnResultID = m_columnResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(itemInternalIndex);
	GlobalFolderSettings globalFolderSettings = m_config->globalFolderSettings;

	auto result = m_columnThreadPool.push([this, columnResultID, columnID, itemInternalIndex, basicItemInfo, globalFolderSettings](int id) {
		UNREFERENCED_PARAMETER(id);

		return GetColumnTextAsync(m_hListView, columnResultID, *columnID, itemInternalIndex, basicItemInfo, globalFolderSettings);
	});

	// The function call above might finish before this line runs,
	// but that doesn't matter, as the results won't be processed
	// until a message posted to the main thread has been handled
	// (which can only occur after this function has returned).
	m_columnResults.insert({ columnResultID, std::move(result) });
}

ShellBrowser::ColumnResult_t ShellBrowser::GetColumnTextAsync(HWND listView, int columnResultId, unsigned int ColumnID,
	int InternalIndex, const BasicItemInfo_t &basicItemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	std::wstring columnText = GetColumnText(ColumnID, basicItemInfo, globalFolderSettings);

	// This message may be delivered before this function has returned.
	// That doesn't actually matter, since the message handler will
	// simply wait for the result to be returned.
	PostMessage(listView, WM_APP_COLUMN_RESULT_READY, columnResultId, 0);

	ColumnResult_t result;
	result.itemInternalIndex = InternalIndex;
	result.columnID = ColumnID;
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

	auto columnIndex = GetColumnIndexById(result.columnID);

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

std::optional<int> ShellBrowser::GetColumnIndexById(unsigned int id) const
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

		if (static_cast<unsigned int>(hdItem.lParam) == id)
		{
			return i;
		}
	}

	return std::nullopt;
}

std::optional<unsigned int> ShellBrowser::GetColumnIdByIndex(int index) const
{
	HWND hHeader = ListView_GetHeader(m_hListView);

	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	BOOL res = Header_GetItem(hHeader, index, &hdItem);

	if (!res)
	{
		return std::nullopt;
	}

	return static_cast<unsigned int>(hdItem.lParam);
}

void ShellBrowser::PlaceColumns()
{
	int							iColumnIndex = 0;
	int							i = 0;

	m_nActiveColumns = 0;

	if(m_pActiveColumns != nullptr)
	{
		for(auto itr = m_pActiveColumns->begin();itr != m_pActiveColumns->end();itr++)
		{
			if(itr->bChecked)
			{
				InsertColumn(itr->id,iColumnIndex,itr->iWidth);

				/* Do NOT set column widths here. For some reason, this causes list mode to
				break. (If this code is active, and the listview starts of in details mode
				and is then switched to list mode, no items will be shown; they appear to
				be placed off the left edge of the listview). */
				//ListView_SetColumnWidth(m_hListView,iColumnIndex,LVSCW_AUTOSIZE_USEHEADER);

				iColumnIndex++;
				m_nActiveColumns++;
			}
		}

		for(i = m_nCurrentColumns + m_nActiveColumns;i >= m_nActiveColumns;i--)
		{
			ListView_DeleteColumn(m_hListView,i);
		}

		m_nCurrentColumns = m_nActiveColumns;
	}
}

void ShellBrowser::InsertColumn(unsigned int ColumnId,int iColumnIndex,int iWidth)
{
	HWND		hHeader;
	HDITEM		hdItem;
	LV_COLUMN	lvColumn;
	TCHAR		szText[64];
	int			iActualColumnIndex;
	int			iStringIndex;

	iStringIndex = LookupColumnNameStringIndex(ColumnId);

	LoadString(m_hResourceModule,iStringIndex,
		szText,SIZEOF_ARRAY(szText));

	lvColumn.mask		= LVCF_TEXT|LVCF_WIDTH;
	lvColumn.pszText	= szText;
	lvColumn.cx			= iWidth;

	if(ColumnId == CM_SIZE || ColumnId == CM_REALSIZE || 
		ColumnId == CM_TOTALSIZE || ColumnId == CM_FREESPACE)
	{
		lvColumn.mask	|= LVCF_FMT;
		lvColumn.fmt	= LVCFMT_RIGHT;
	}

	iActualColumnIndex = ListView_InsertColumn(m_hListView,iColumnIndex,&lvColumn);

	hHeader = ListView_GetHeader(m_hListView);

	/* Store the column's ID with the column itself. */
	hdItem.mask		= HDI_LPARAM;
	hdItem.lParam	= ColumnId;

	Header_SetItem(hHeader,iActualColumnIndex,&hdItem);
}

void ShellBrowser::SetActiveColumnSet()
{
	std::vector<Column_t> *pActiveColumns = nullptr;

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		pActiveColumns = &m_folderColumns.controlPanelColumns;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		pActiveColumns = &m_folderColumns.myComputerColumns;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		pActiveColumns = &m_folderColumns.recycleBinColumns;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		pActiveColumns = &m_folderColumns.printersColumns;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		pActiveColumns = &m_folderColumns.networkConnectionsColumns;
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
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
	if(m_pActiveColumns != pActiveColumns)
	{
		m_pActiveColumns = pActiveColumns;
		m_bColumnsPlaced = FALSE;
	}
}

SortMode ShellBrowser::DetermineColumnSortMode(int iColumnId)
{
	switch(iColumnId)
	{
		case CM_NAME:
			return SortMode::Name;

		case CM_TYPE:
			return SortMode::Type;

		case CM_SIZE:
			return SortMode::Size;

		case CM_DATEMODIFIED:
			return SortMode::DateModified;

		case CM_ATTRIBUTES:
			return SortMode::Attributes;

		case CM_REALSIZE:
			return SortMode::RealSize;

		case CM_SHORTNAME:
			return SortMode::ShortName;

		case CM_OWNER:
			return SortMode::Owner;

		case CM_PRODUCTNAME:
			return SortMode::ProductName;

		case CM_COMPANY:
			return SortMode::Company;

		case CM_DESCRIPTION:
			return SortMode::Description;

		case CM_FILEVERSION:
			return SortMode::FileVersion;

		case CM_PRODUCTVERSION:
			return SortMode::ProductVersion;

		case CM_SHORTCUTTO:
			return SortMode::ShortcutTo;

		case CM_HARDLINKS:
			return SortMode::HardLinks;

		case CM_EXTENSION:
			return SortMode::Extension;

		case CM_CREATED:
			return SortMode::Created;

		case CM_ACCESSED:
			return SortMode::Accessed;

		case CM_TITLE:
			return SortMode::Title;

		case CM_SUBJECT:
			return SortMode::Subject;

		case CM_AUTHORS:
			return SortMode::Authors;

		case CM_KEYWORDS:
			return SortMode::Keywords;

		case CM_COMMENT:
			return SortMode::Comments;

		case CM_CAMERAMODEL:
			return SortMode::CameraModel;

		case CM_DATETAKEN:
			return SortMode::DateTaken;

		case CM_WIDTH:
			return SortMode::Width;

		case CM_HEIGHT:
			return SortMode::Height;

		case CM_VIRTUALCOMMENTS:
			return SortMode::VirtualComments;

		case CM_TOTALSIZE:
			return SortMode::TotalSize;

		case CM_FREESPACE:
			return SortMode::FreeSpace;

		case CM_FILESYSTEM:
			return SortMode::FileSystem;

		case CM_ORIGINALLOCATION:
			return SortMode::OriginalLocation;

		case CM_DATEDELETED:
			return SortMode::DateDeleted;

		case CM_NUMPRINTERDOCUMENTS:
			return SortMode::NumPrinterDocuments;

		case CM_PRINTERSTATUS:
			return SortMode::PrinterStatus;

		case CM_PRINTERCOMMENTS:
			return SortMode::PrinterComments;

		case CM_PRINTERLOCATION:
			return SortMode::PrinterLocation;

		case CM_NETWORKADAPTER_STATUS:
			return SortMode::NetworkAdapterStatus;

		case CM_MEDIA_BITRATE:
			return SortMode::MediaBitrate;

		case CM_MEDIA_COPYRIGHT:
			return SortMode::MediaCopyright;

		case CM_MEDIA_DURATION:
			return SortMode::MediaDuration;

		case CM_MEDIA_PROTECTED:
			return SortMode::MediaProtected;

		case CM_MEDIA_RATING:
			return SortMode::MediaRating;

		case CM_MEDIA_ALBUMARTIST:
			return SortMode::MediaAlbumArtist;

		case CM_MEDIA_ALBUM:
			return SortMode::MediaAlbum;

		case CM_MEDIA_BEATSPERMINUTE:
			return SortMode::MediaBeatsPerMinute;

		case CM_MEDIA_COMPOSER:
			return SortMode::MediaComposer;

		case CM_MEDIA_CONDUCTOR:
			return SortMode::MediaConductor;

		case CM_MEDIA_DIRECTOR:
			return SortMode::MediaDirector;

		case CM_MEDIA_GENRE:
			return SortMode::MediaGenre;

		case CM_MEDIA_LANGUAGE:
			return SortMode::MediaLanguage;

		case CM_MEDIA_BROADCASTDATE:
			return SortMode::MediaBroadcastDate;

		case CM_MEDIA_CHANNEL:
			return SortMode::MediaChannel;

		case CM_MEDIA_STATIONNAME:
			return SortMode::MediaStationName;

		case CM_MEDIA_MOOD:
			return SortMode::MediaMood;

		case CM_MEDIA_PARENTALRATING:
			return SortMode::MediaParentalRating;

		case CM_MEDIA_PARENTALRATINGREASON:
			return SortMode::MediaParentalRatingReason;

		case CM_MEDIA_PERIOD:
			return SortMode::MediaPeriod;

		case CM_MEDIA_PRODUCER:
			return SortMode::MediaProducer;

		case CM_MEDIA_PUBLISHER:
			return SortMode::MediaPublisher;

		case CM_MEDIA_WRITER:
			return SortMode::MediaWriter;

		case CM_MEDIA_YEAR:
			return SortMode::MediaYear;

		default:
			assert(false);
			break;
	}

	return SortMode::Name;
}

int ShellBrowser::LookupColumnNameStringIndex(int iColumnId)
{
	switch (iColumnId)
	{
	case CM_NAME:
		return IDS_COLUMN_NAME_NAME;

	case CM_TYPE:
		return IDS_COLUMN_NAME_TYPE;

	case CM_SIZE:
		return IDS_COLUMN_NAME_SIZE;

	case CM_DATEMODIFIED:
		return IDS_COLUMN_NAME_DATEMODIFIED;

	case CM_ATTRIBUTES:
		return IDS_COLUMN_NAME_ATTRIBUTES;

	case CM_REALSIZE:
		return IDS_COLUMN_NAME_REALSIZE;

	case CM_SHORTNAME:
		return IDS_COLUMN_NAME_SHORTNAME;

	case CM_OWNER:
		return IDS_COLUMN_NAME_OWNER;

	case CM_PRODUCTNAME:
		return IDS_COLUMN_NAME_PRODUCTNAME;

	case CM_COMPANY:
		return IDS_COLUMN_NAME_COMPANY;

	case CM_DESCRIPTION:
		return IDS_COLUMN_NAME_DESCRIPTION;

	case CM_FILEVERSION:
		return IDS_COLUMN_NAME_FILEVERSION;

	case CM_PRODUCTVERSION:
		return IDS_COLUMN_NAME_PRODUCTVERSION;

	case CM_SHORTCUTTO:
		return IDS_COLUMN_NAME_SHORTCUTTO;

	case CM_HARDLINKS:
		return IDS_COLUMN_NAME_HARDLINKS;

	case CM_EXTENSION:
		return IDS_COLUMN_NAME_EXTENSION;

	case CM_CREATED:
		return IDS_COLUMN_NAME_CREATED;

	case CM_ACCESSED:
		return IDS_COLUMN_NAME_ACCESSED;

	case CM_TITLE:
		return IDS_COLUMN_NAME_TITLE;

	case CM_SUBJECT:
		return IDS_COLUMN_NAME_SUBJECT;

	case CM_AUTHORS:
		return IDS_COLUMN_NAME_AUTHORS;

	case CM_KEYWORDS:
		return IDS_COLUMN_NAME_KEYWORDS;

	case CM_COMMENT:
		return IDS_COLUMN_NAME_COMMENT;

	case CM_CAMERAMODEL:
		return IDS_COLUMN_NAME_CAMERAMODEL;

	case CM_DATETAKEN:
		return IDS_COLUMN_NAME_DATETAKEN;

	case CM_WIDTH:
		return IDS_COLUMN_NAME_WIDTH;

	case CM_HEIGHT:
		return IDS_COLUMN_NAME_HEIGHT;

	case CM_VIRTUALCOMMENTS:
		return IDS_COLUMN_NAME_VIRTUALCOMMENTS;

	case CM_TOTALSIZE:
		return IDS_COLUMN_NAME_TOTALSIZE;

	case CM_FREESPACE:
		return IDS_COLUMN_NAME_FREESPACE;

	case CM_FILESYSTEM:
		return IDS_COLUMN_NAME_FILESYSTEM;

	case CM_ORIGINALLOCATION:
		return IDS_COLUMN_NAME_ORIGINALLOCATION;

	case CM_DATEDELETED:
		return IDS_COLUMN_NAME_DATEDELETED;

	case CM_NUMPRINTERDOCUMENTS:
		return IDS_COLUMN_NAME_NUMPRINTERDOCUMENTS;

	case CM_PRINTERSTATUS:
		return IDS_COLUMN_NAME_PRINTERSTATUS;

	case CM_PRINTERCOMMENTS:
		return IDS_COLUMN_NAME_PRINTERCOMMENTS;

	case CM_PRINTERLOCATION:
		return IDS_COLUMN_NAME_PRINTERLOCATION;

	case CM_PRINTERMODEL:
		return IDS_COLUMN_NAME_PRINTERMODEL;

	case CM_NETWORKADAPTER_STATUS:
		return IDS_COLUMN_NAME_NETWORKADAPTER_STATUS;

	case CM_MEDIA_BITRATE:
		return IDS_COLUMN_NAME_BITRATE;

	case CM_MEDIA_COPYRIGHT:
		return IDS_COLUMN_NAME_COPYRIGHT;

	case CM_MEDIA_DURATION:
		return IDS_COLUMN_NAME_DURATION;

	case CM_MEDIA_PROTECTED:
		return IDS_COLUMN_NAME_PROTECTED;

	case CM_MEDIA_RATING:
		return IDS_COLUMN_NAME_RATING;

	case CM_MEDIA_ALBUMARTIST:
		return IDS_COLUMN_NAME_ALBUMARTIST;

	case CM_MEDIA_ALBUM:
		return IDS_COLUMN_NAME_ALBUM;

	case CM_MEDIA_BEATSPERMINUTE:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;

	case CM_MEDIA_COMPOSER:
		return IDS_COLUMN_NAME_COMPOSER;

	case CM_MEDIA_CONDUCTOR:
		return IDS_COLUMN_NAME_CONDUCTOR;

	case CM_MEDIA_DIRECTOR:
		return IDS_COLUMN_NAME_DIRECTOR;

	case CM_MEDIA_GENRE:
		return IDS_COLUMN_NAME_GENRE;

	case CM_MEDIA_LANGUAGE:
		return IDS_COLUMN_NAME_LANGUAGE;

	case CM_MEDIA_BROADCASTDATE:
		return IDS_COLUMN_NAME_BROADCASTDATE;

	case CM_MEDIA_CHANNEL:
		return IDS_COLUMN_NAME_CHANNEL;

	case CM_MEDIA_STATIONNAME:
		return IDS_COLUMN_NAME_STATIONNAME;

	case CM_MEDIA_MOOD:
		return IDS_COLUMN_NAME_MOOD;

	case CM_MEDIA_PARENTALRATING:
		return IDS_COLUMN_NAME_PARENTALRATING;

	case CM_MEDIA_PARENTALRATINGREASON:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;

	case CM_MEDIA_PERIOD:
		return IDS_COLUMN_NAME_PERIOD;

	case CM_MEDIA_PRODUCER:
		return IDS_COLUMN_NAME_PRODUCER;

	case CM_MEDIA_PUBLISHER:
		return IDS_COLUMN_NAME_PUBLISHER;

	case CM_MEDIA_WRITER:
		return IDS_COLUMN_NAME_WRITER;

	case CM_MEDIA_YEAR:
		return IDS_COLUMN_NAME_YEAR;

	default:
		assert(false);
		break;
	}

	return 0;
}

int ShellBrowser::LookupColumnDescriptionStringIndex(int iColumnId)
{
	switch (iColumnId)
	{
	case CM_NAME:
		return IDS_COLUMN_DESCRIPTION_NAME;

	case CM_TYPE:
		return IDS_COLUMN_DESCRIPTION_TYPE;

	case CM_SIZE:
		return IDS_COLUMN_DESCRIPTION_SIZE;

	case CM_DATEMODIFIED:
		return IDS_COLUMN_DESCRIPTION_MODIFIED;

	case CM_ATTRIBUTES:
		return IDS_COLUMN_DESCRIPTION_ATTRIBUTES;

	case CM_REALSIZE:
		return IDS_COLUMN_DESCRIPTION_REALSIZE;

	case CM_SHORTNAME:
		return IDS_COLUMN_DESCRIPTION_SHORTNAME;

	case CM_OWNER:
		return IDS_COLUMN_DESCRIPTION_OWNER;

	case CM_PRODUCTNAME:
		return IDS_COLUMN_DESCRIPTION_PRODUCTNAME;

	case CM_COMPANY:
		return IDS_COLUMN_DESCRIPTION_COMPANY;

	case CM_DESCRIPTION:
		return IDS_COLUMN_DESCRIPTION_DESCRIPTION;

	case CM_FILEVERSION:
		return IDS_COLUMN_DESCRIPTION_FILEVERSION;

	case CM_PRODUCTVERSION:
		return IDS_COLUMN_DESCRIPTION_PRODUCTVERSION;

	case CM_SHORTCUTTO:
		return IDS_COLUMN_DESCRIPTION_SHORTCUTTO;

	case CM_HARDLINKS:
		return IDS_COLUMN_DESCRIPTION_HARDLINKS;

	case CM_EXTENSION:
		return IDS_COLUMN_DESCRIPTION_EXTENSION;

	case CM_CREATED:
		return IDS_COLUMN_DESCRIPTION_CREATED;

	case CM_ACCESSED:
		return IDS_COLUMN_DESCRIPTION_ACCESSED;

	case CM_TITLE:
		return IDS_COLUMN_DESCRIPTION_TITLE;

	case CM_SUBJECT:
		return IDS_COLUMN_DESCRIPTION_SUBJECT;

	case CM_AUTHORS:
		return IDS_COLUMN_DESCRIPTION_AUTHORS;

	case CM_KEYWORDS:
		return IDS_COLUMN_DESCRIPTION_KEYWORDS;

	case CM_COMMENT:
		return IDS_COLUMN_DESCRIPTION_COMMENT;

	case CM_CAMERAMODEL:
		return IDS_COLUMN_DESCRIPTION_CAMERAMODEL;

	case CM_DATETAKEN:
		return IDS_COLUMN_DESCRIPTION_DATETAKEN;

	case CM_WIDTH:
		return IDS_COLUMN_DESCRIPTION_WIDTH;

	case CM_HEIGHT:
		return IDS_COLUMN_DESCRIPTION_HEIGHT;

	case CM_VIRTUALCOMMENTS:
		return IDS_COLUMN_DESCRIPTION_COMMENT;

	case CM_TOTALSIZE:
		return IDS_COLUMN_DESCRIPTION_TOTALSIZE;

	case CM_FREESPACE:
		return IDS_COLUMN_DESCRIPTION_FREESPACE;

	case CM_FILESYSTEM:
		return IDS_COLUMN_DESCRIPTION_FILESYSTEM;

	case CM_NUMPRINTERDOCUMENTS:
		return IDS_COLUMN_DESCRIPTION_NUMPRINTERDOCUMENTS;

	case CM_PRINTERCOMMENTS:
		return IDS_COLUMN_DESCRIPTION_PRINTERCOMMENTS;

	case CM_PRINTERLOCATION:
		return IDS_COLUMN_DESCRIPTION_PRINTERLOCATION;

	case CM_NETWORKADAPTER_STATUS:
		return IDS_COLUMN_DESCRIPTION_NETWORKADAPTER_STATUS;

	case CM_MEDIA_BITRATE:
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
	UINT iColumnId = 0;

	for(auto itr = m_pActiveColumns->begin();itr != m_pActiveColumns->end();itr++)
	{
		/* Only increment if this column is actually been shown. */
		if(itr->bChecked)
		{
			if(iCurrentColumn == iClickedColumn)
			{
				sortMode = DetermineColumnSortMode(itr->id);
				iColumnId = itr->id;
				break;
			}

			iCurrentColumn++;
		}
	}

	/* Same column was clicked. Toggle the
	ascending/descending sort state. Use unique
	column ID, not index, as columns may be
	inserted/deleted. */
	if(m_iPreviousSortedColumnId == iColumnId)
	{
		m_folderSettings.sortAscending = !m_folderSettings.sortAscending;
	}

	SortFolder(sortMode);
}

void ShellBrowser::ApplyHeaderSortArrow()
{
	HWND hHeader;
	HDITEM hdItem;
	BOOL previousColumnFound = FALSE;
	int iColumn = 0;
	int iPreviousSortedColumn = 0;
	int iColumnId = -1;

	hHeader = ListView_GetHeader(m_hListView);

	if (m_PreviousSortColumnExists)
	{
		/* Search through the currently active columns to find the column that previously
		had the up/down arrow. */
		for (auto itr = m_pActiveColumns->begin(); itr != m_pActiveColumns->end(); itr++)
		{
			/* Only increment if this column is actually been shown. */
			if (itr->bChecked)
			{
				if (m_iPreviousSortedColumnId == itr->id)
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
	for(auto itr = m_pActiveColumns->begin();itr != m_pActiveColumns->end();itr++)
	{
		if(itr->bChecked)
		{
			if(DetermineColumnSortMode(itr->id) == m_folderSettings.sortMode)
			{
				iColumnId = itr->id;
				break;
			}

			iColumn++;
		}
	}

	hdItem.mask = HDI_FORMAT;
	Header_GetItem(hHeader,iColumn,&hdItem);

	if(!m_folderSettings.sortAscending)
		hdItem.fmt |= HDF_SORTDOWN;
	else
		hdItem.fmt |= HDF_SORTUP;

	/* Add the up/down arrow to the column by which
	items are now sorted. */
	Header_SetItem(hHeader,iColumn,&hdItem);

	m_iPreviousSortedColumnId = iColumnId;
	m_PreviousSortColumnExists = true;
}

size_t ShellBrowser::GetNumActiveColumns(void) const
{
	return m_pActiveColumns->size();
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

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		pActiveColumns = &m_folderColumns.controlPanelColumns;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		pActiveColumns = &m_folderColumns.myComputerColumns;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		pActiveColumns = &m_folderColumns.recycleBinColumns;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		pActiveColumns = &m_folderColumns.printersColumns;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		pActiveColumns = &m_folderColumns.networkConnectionsColumns;
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
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
	if(m_folderSettings.viewMode == +ViewMode::Details)
	{
		for(auto itr = pActiveColumns->begin();itr != pActiveColumns->end();itr++)
		{
			if(itr->bChecked)
			{
				itr->iWidth = ListView_GetColumnWidth(m_hListView,iColumn);

				iColumn++;
			}
		}
	}
}

std::vector<Column_t> ShellBrowser::ExportCurrentColumns()
{
	std::vector<Column_t> columns;
	int iColumn = 0;

	for (auto itr = m_pActiveColumns->begin(); itr != m_pActiveColumns->end(); itr++)
	{
		if (m_folderSettings.viewMode == +ViewMode::Details && itr->bChecked)
		{
			itr->iWidth = ListView_GetColumnWidth(m_hListView, iColumn);

			iColumn++;
		}

		Column_t Column;
		Column.id = itr->id;
		Column.bChecked = itr->bChecked;
		Column.iWidth = itr->iWidth;
		columns.push_back(Column);
	}

	return columns;
}

void ShellBrowser::ImportColumns(const std::vector<Column_t> &columns)
{
	Column_t ci;
	BOOL bResortFolder = FALSE;
	int iColumn = 0;
	int i = 0;

	for (auto itr = columns.begin(); itr != columns.end(); itr++)
	{
		/* Check if this column represents the current sorting mode.
		If it does, and it is been removed, set the sort mode back
		to the first checked column. */
		if (!itr->bChecked && DetermineColumnSortMode(itr->id) == m_folderSettings.sortMode)
		{
			/* Find the first checked column. */
			for (auto itr2 = columns.begin(); itr2 != columns.end(); itr2++)
			{
				if (itr2->bChecked)
				{
					m_folderSettings.sortMode = DetermineColumnSortMode(itr2->id);

					bResortFolder = TRUE;
					break;
				}
			}
		}

		GetColumnInternal(itr->id, &ci);

		if (itr->bChecked)
		{
			if (m_folderSettings.viewMode == +ViewMode::Details)
			{
				for (auto itr2 = m_pActiveColumns->begin(); itr2 != m_pActiveColumns->end(); itr2++)
				{
					if (itr2->id == itr->id &&
						!itr2->bChecked)
					{
						InsertColumn(itr->id, iColumn, itr->iWidth);

						for (i = 0; i < m_nTotalItems; i++)
						{
							LVITEM lvItem;
							lvItem.mask = LVIF_PARAM;
							lvItem.iItem = i;
							lvItem.iSubItem = 0;
							BOOL res = ListView_GetItem(m_hListView, &lvItem);

							if (res)
							{
								QueueColumnTask(static_cast<int>(lvItem.lParam), itr->id);
							}
						}

						break;
					}
				}
			}

			iColumn++;
		}
		else
		{
			for (auto itr2 = m_pActiveColumns->begin(); itr2 != m_pActiveColumns->end(); itr2++)
			{
				if (itr2->id == itr->id &&
					itr2->bChecked)

				{
					ListView_DeleteColumn(m_hListView, iColumn);
					break;
				}
			}
		}
	}

	/* Copy the new columns. */
	*m_pActiveColumns = columns;

	/* The folder will need to be resorted if the
	sorting column was removed. */
	if (bResortFolder)
	{
		SortFolder(m_folderSettings.sortMode);
	}

	m_bColumnsPlaced = FALSE;
}

void ShellBrowser::GetColumnInternal(unsigned int id, Column_t *pci) const
{
	for (auto itr = m_pActiveColumns->begin(); itr != m_pActiveColumns->end(); itr++)
	{
		if (itr->id == id)
		{
			*pci = *itr;
			return;
		}
	}
}