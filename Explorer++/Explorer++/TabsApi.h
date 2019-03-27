// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "TabContainerInterface.h"
#include "TabInterface.h"
#include "../ThirdParty/Sol/sol.hpp"
#include <codecvt>

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

			FolderSettings(const CShellBrowser &shellBrowser)
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
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

				return _T("sortMode = ") + converter.from_bytes(sortMode._to_string())
					+ _T(", viewMode = ") + converter.from_bytes(viewMode._to_string())
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
			bool locked;
			bool addressLocked;

			FolderSettings folderSettings;

			Tab(const ::Tab &tabInternal) :
				folderSettings(*tabInternal.GetShellBrowser())
			{
				TCHAR path[MAX_PATH];
				tabInternal.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(path), path);

				id = tabInternal.GetId();
				location = path;
				name = tabInternal.GetName();
				locked = tabInternal.GetLocked();
				addressLocked = tabInternal.GetAddressLocked();
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

		TabsApi(TabContainerInterface *tabContainer, TabInterface *tabInterface);
		~TabsApi();

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

		TabContainerInterface *m_tabContainer;
		TabInterface *m_tabInterface;
	};
}