// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"

void ShellBrowser::InsertTileViewColumns()
{
	LVTILEVIEWINFO lvtvi;
	LVCOLUMN lvColumn;

	/* Name. */
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = EMPTY_STRING;
	ListView_InsertColumn(m_hListView, 1, &lvColumn);

	/* Type. */
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = EMPTY_STRING;
	ListView_InsertColumn(m_hListView, 2, &lvColumn);

	/* File size. */
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = EMPTY_STRING;
	ListView_InsertColumn(m_hListView, 3, &lvColumn);

	lvtvi.cbSize = sizeof(lvtvi);
	lvtvi.dwMask = LVTVIM_COLUMNS;
	lvtvi.dwFlags = LVTVIF_AUTOSIZE;
	lvtvi.cLines = 2;
	ListView_SetTileViewInfo(m_hListView, &lvtvi);
}

void ShellBrowser::DeleteTileViewColumns()
{
	ListView_DeleteColumn(m_hListView, 3);
	ListView_DeleteColumn(m_hListView, 2);
	ListView_DeleteColumn(m_hListView, 1);
}

void ShellBrowser::SetTileViewInfo()
{
	LVITEM lvItem;
	BOOL bRes;
	int nItems;
	int i = 0;

	nItems = ListView_GetItemCount(m_hListView);

	for (i = 0; i < nItems; i++)
	{
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		bRes = ListView_GetItem(m_hListView, &lvItem);

		if (bRes)
		{
			SetTileViewItemInfo(i, (int) lvItem.lParam);
		}
	}
}

/* TODO: Make this function configurable. */
void ShellBrowser::SetTileViewItemInfo(int iItem, int iItemInternal)
{
	SHFILEINFO shfi;
	LVTILEINFO lvti;
	UINT uColumns[2] = { 1, 2 };
	int columnFormats[2] = { LVCFMT_LEFT, LVCFMT_LEFT };
	TCHAR fullFileName[MAX_PATH];

	lvti.cbSize = sizeof(lvti);
	lvti.iItem = iItem;
	lvti.cColumns = 2;
	lvti.puColumns = uColumns;
	lvti.piColFmt = columnFormats;
	ListView_SetTileInfo(m_hListView, &lvti);

	GetItemFullName(iItem, fullFileName, SIZEOF_ARRAY(fullFileName));

	SHGetFileInfo(fullFileName, 0, &shfi, sizeof(SHFILEINFO), SHGFI_TYPENAME);

	ListView_SetItemText(m_hListView, iItem, 1, shfi.szTypeName);

	if ((m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		!= FILE_ATTRIBUTE_DIRECTORY)
	{
		TCHAR lpszFileSize[32];
		ULARGE_INTEGER lFileSize;

		lFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
		lFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

		FormatSizeString(lFileSize, lpszFileSize, SIZEOF_ARRAY(lpszFileSize),
			m_config->globalFolderSettings.forceSize,
			m_config->globalFolderSettings.sizeDisplayFormat);

		ListView_SetItemText(m_hListView, iItem, 2, lpszFileSize);
	}
}