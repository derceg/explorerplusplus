// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "Navigation.h"
#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/StringHelper.h"
#include "../ThirdParty/Sol/forward.hpp"

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

			FolderSettings(const ShellBrowser &shellBrowser)
			{
				sortMode = shellBrowser.GetSortMode();
				viewMode = shellBrowser.GetViewMode();
				sortAscending = shellBrowser.GetSortAscending();
				showInGroups = shellBrowser.GetShowInGroups();
				showHidden = shellBrowser.GetShowHidden();
				autoArrange = shellBrowser.GetAutoArrange();
			}

			std::wstring toString()
			{
				return _T("sortMode = ") + strToWstr(sortMode._to_string())
					+ _T(", viewMode = ") + strToWstr(viewMode._to_string())
					+ _T(", sortAscending = ") + std::to_wstring(sortAscending)
					+ _T(", showInGroups = ") + std::to_wstring(showInGroups)
					+ _T(", showHidden = ") + std::to_wstring(showHidden)
					+ _T(", autoArrange = ") + std::to_wstring(autoArrange);
			}
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

			Tab(const ::Tab &tabInternal) :
				folderSettings(*tabInternal.GetShellBrowser())
			{
				id = tabInternal.GetId();
				location = tabInternal.GetShellBrowser()->GetDirectory();
				name = tabInternal.GetName();
				locked = (tabInternal.GetLockState() == ::Tab::LockState::Locked);
				addressLocked = (tabInternal.GetLockState() == ::Tab::LockState::AddressLocked);
			}

			std::wstring toString()
			{
				return _T("id = ") + std::to_wstring(id)
					+ _T(", location = ") + location
					+ _T(", name = ") + name
					+ _T(", locked = ") + std::to_wstring(locked)
					+ _T(", addressLocked = ") + std::to_wstring(addressLocked)
					+ _T(", folderSettings = {") + folderSettings.toString() + _T("}");
			}
		};

		TabsApi(IExplorerplusplus *expp, TabContainer *tabContainer, Navigation *navigation);

		std::vector<Tab> getAll();
		boost::optional<Tab> get(int tabId);
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