// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabStorageTestHelper.h"
#include "PidlTestHelper.h"
#include "TabStorage.h"

TabStorageData CreateTabStorageFromDirectory(const std::wstring &directory,
	TestStorageType storageType)
{
	TabStorageData tab;

	// When using the registry to persist data, only the pidl field is used. When using the config
	// XML file, only the directory field is used. To ensure that comparing two TabStorageData
	// instances works as expected, it's important that only the required field is set.
	// For example, if the directory field was set when saving registry data, it would be ignored.
	// It wouldn't be set when loading data, however, which would cause the comparison between the
	// data being saved and the data being loaded to fail.
	if (storageType == TestStorageType::Registry)
	{
		tab.pidl = CreateSimplePidlForTest(directory);
	}
	else
	{
		tab.directory = directory;
	}

	return tab;
}

void BuildTabStorageLoadSaveReference(std::vector<TabStorageData> &outputTabs,
	TestStorageType storageType)
{
	auto tab1 = CreateTabStorageFromDirectory(L"C:\\", storageType);

	tab1.tabSettings.name = L"C drive";

	tab1.folderSettings.sortMode = SortMode::Size;
	tab1.folderSettings.groupMode = SortMode::Name;
	tab1.folderSettings.viewMode = ViewMode::Icons;
	tab1.folderSettings.autoArrangeEnabled = true;
	tab1.folderSettings.sortDirection = SortDirection::Descending;
	tab1.folderSettings.groupSortDirection = SortDirection::Ascending;
	tab1.folderSettings.showInGroups = false;
	tab1.folderSettings.showHidden = true;
	tab1.folderSettings.filterEnabled = false;
	tab1.folderSettings.filterCaseSensitive = false;

	tab1.columns.realFolderColumns = { { ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Type, FALSE, 200 } };
	tab1.columns.myComputerColumns = { { ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::TotalSize, TRUE, 800 } };
	tab1.columns.controlPanelColumns = { { ColumnType::VirtualComments, TRUE, 685 },
		{ ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH } };
	tab1.columns.recycleBinColumns = { { ColumnType::Size, FALSE, 363 },
		{ ColumnType::DateModified, TRUE, 212 } };
	tab1.columns.printersColumns = { { ColumnType::PrinterComments, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::PrinterLocation, FALSE, DEFAULT_COLUMN_WIDTH } };
	tab1.columns.networkConnectionsColumns = { { ColumnType::NetworkAdaptorStatus, TRUE, 88 },
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH } };
	tab1.columns.myNetworkPlacesColumns = { { ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::VirtualComments, FALSE, DEFAULT_COLUMN_WIDTH } };

	outputTabs.push_back(tab1);

	// Note that since no columns appear in the saved data for this tab and the third tab, there's
	// no need to set any columns here. If there are no columns in the saved data, the default
	// columns will be used.
	auto tab2 = CreateTabStorageFromDirectory(L"C:\\Users", storageType);

	tab2.tabSettings.lockState = Tab::LockState::Locked;

	tab2.folderSettings.sortMode = SortMode::DateModified;
	tab2.folderSettings.groupMode = SortMode::Size;
	tab2.folderSettings.viewMode = ViewMode::SmallIcons;
	tab2.folderSettings.autoArrangeEnabled = false;
	tab2.folderSettings.sortDirection = SortDirection::Ascending;
	tab2.folderSettings.groupSortDirection = SortDirection::Descending;
	tab2.folderSettings.showInGroups = true;
	tab2.folderSettings.showHidden = false;
	tab2.folderSettings.filterEnabled = true;
	tab2.folderSettings.filterCaseSensitive = false;
	tab2.folderSettings.filter = L"*.exe";

	outputTabs.push_back(tab2);

	auto tab3 = CreateTabStorageFromDirectory(L"C:\\Users\\Default", storageType);

	tab3.tabSettings.lockState = Tab::LockState::AddressLocked;

	tab3.folderSettings.sortMode = SortMode::Created;
	tab3.folderSettings.groupMode = SortMode::Name;
	tab3.folderSettings.viewMode = ViewMode::Details;
	tab3.folderSettings.autoArrangeEnabled = true;
	tab3.folderSettings.sortDirection = SortDirection::Descending;
	tab3.folderSettings.groupSortDirection = SortDirection::Ascending;
	tab3.folderSettings.showInGroups = false;
	tab3.folderSettings.showHidden = true;
	tab3.folderSettings.filterEnabled = true;
	tab3.folderSettings.filterCaseSensitive = true;
	tab3.folderSettings.filter = L"*.txt";

	outputTabs.push_back(tab3);
}
