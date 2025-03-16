// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include <sol/forward.hpp>
#include <optional>

class CoreInterface;
struct FolderSettings;
class ShellBrowserImpl;
class TabContainer;
struct TabSettings;

namespace Plugins
{
class TabsApi
{
public:
	struct FolderSettings
	{
		SortMode sortMode;
		SortMode groupMode;
		ViewMode viewMode;
		bool autoArrange;
		SortDirection sortDirection;
		SortDirection groupSortDirection;
		bool showInGroups;
		bool showHidden;

		FolderSettings(const ShellBrowserImpl &shellBrowser);
		std::wstring toString();
	};

	struct Tab
	{
		int id;
		std::wstring location;
		std::wstring name;

		// TODO: Use the Tab::LockState enum instead of these values.
		bool locked;
		bool addressLocked;

		FolderSettings folderSettings;

		Tab(const ::Tab &tabInternal);
		std::wstring toString();
	};

	TabsApi(CoreInterface *coreInterface, TabContainer *tabContainer);

	std::vector<Tab> getAll();
	std::optional<Tab> get(int tabId);
	int create(sol::table createProperties);
	void update(int tabId, sol::table properties);
	void refresh(int tabId);
	int move(int tabId, int newIndex);
	bool close(int tabId);

private:
	void extractTabPropertiesForCreation(sol::table createProperties, TabSettings &tabSettings);
	void extractFolderSettingsForCreation(sol::table folderSettingsTable,
		::FolderSettings &folderSettings);

	CoreInterface *m_coreInterface;
	TabContainer *m_tabContainer;
};
}
