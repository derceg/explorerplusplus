// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "../Helper/StringHelper.h"
#include "../ThirdParty/Sol/forward.hpp"
#include <optional>

__interface IExplorerplusplus;
class Navigation;
class ShellBrowser;
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
			ViewMode viewMode;
			bool sortAscending;
			bool showInGroups;
			bool showHidden;
			bool autoArrange;

			FolderSettings(const ShellBrowser &shellBrowser);
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

		TabsApi(IExplorerplusplus *expp, TabContainer *tabContainer, Navigation *navigation);

		std::vector<Tab> getAll();
		std::optional<Tab> get(int tabId);
		int create(sol::table createProperties);
		void update(int tabId, sol::table properties);
		void refresh(int tabId);
		int move(int tabId, int newIndex);
		bool close(int tabId);

	private:

		void extractTabPropertiesForCreation(sol::table createProperties, TabSettings &tabSettings);
		void extractFolderSettingsForCreation(sol::table folderSettingsTable, ::FolderSettings &folderSettings);

		IExplorerplusplus *m_expp;
		TabContainer *m_tabContainer;
		Navigation *m_navigation;
	};
}