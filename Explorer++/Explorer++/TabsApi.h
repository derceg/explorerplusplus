// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "TabContainerInterface.h"
#include "TabInterface.h"
#include "../ThirdParty/Sol/sol.hpp"
#include <boost/signals2.hpp>
#include <unordered_map>

namespace Plugins
{
	class TabsApi
	{
	public:
		struct FolderSettings
		{
			ViewMode viewMode;
			bool sortAscending;
			bool showInGroups;
			bool showHidden;
			bool autoArrange;
			bool showFolderSizes;
			bool hideSystemFiles;
			bool hideLinkExtension;

			FolderSettings(const CShellBrowser &shellBrowser)
			{
				viewMode = shellBrowser.GetCurrentViewMode();
				sortAscending = shellBrowser.GetSortAscending();
				showInGroups = shellBrowser.IsGroupViewEnabled();
				showHidden = shellBrowser.GetShowHidden();
				autoArrange = shellBrowser.GetAutoArrange();
				showFolderSizes = shellBrowser.GetShowFolderSizes();
				hideSystemFiles = shellBrowser.GetHideSystemFiles();
				hideLinkExtension = shellBrowser.GetHideLinkExtension();
			}

			std::wstring toString()
			{
				return _T("viewMode = ") + std::to_wstring(viewMode)
					+ _T(", sortAscending = ") + std::to_wstring(sortAscending)
					+ _T(", showInGroups = ") + std::to_wstring(showInGroups)
					+ _T(", showHidden = ") + std::to_wstring(showHidden)
					+ _T(", autoArrange = ") + std::to_wstring(autoArrange)
					+ _T(", showFolderSizes = ") + std::to_wstring(showFolderSizes)
					+ _T(", hideSystemFiles = ") + std::to_wstring(hideSystemFiles);
					+ _T(", hideLinkExtension = ") + std::to_wstring(hideLinkExtension);
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
				folderSettings(*tabInternal.shellBrowser)
			{
				TCHAR path[MAX_PATH];
				tabInternal.shellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(path), path);

				id = tabInternal.id;
				location = path;
				name = tabInternal.szName;
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
		void create(std::wstring path);
		void update(int tabId, sol::table properties);
		void refresh(int tabId);
		int move(int tabId, int newIndex);
		bool close(int tabId);

		/* Events. */
		int addTabCreatedObserver(sol::protected_function observer);
		void removeTabCreatedObserver(int id);

		int addTabRemovedObserver(sol::protected_function observer);
		void removeTabRemovedObserver(int id);

	private:

		void onTabCreated(int tabId, sol::protected_function observer);

		TabContainerInterface *m_tabContainer;
		TabInterface *m_tabInterface;

		int m_tabCreatedIdCounter;
		std::unordered_map<int, boost::signals2::connection> m_tabCreatedConnections;

		int m_tabRemovedIdCounter;
		std::unordered_map<int, boost::signals2::connection> m_tabRemovedConnections;
	};
}