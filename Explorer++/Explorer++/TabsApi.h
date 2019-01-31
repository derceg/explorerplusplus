// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Explorer++_internal.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"

namespace Plugins
{
	class TabsApi
	{
	public:
		struct Tab
		{
			std::wstring location;
			ViewMode viewMode;
			bool locked;
			bool addressLocked;

			Tab(const TabInfo_t &tabInternal)
			{
				TCHAR path[MAX_PATH];
				tabInternal.shellBrower->QueryCurrentDirectory(SIZEOF_ARRAY(path), path);

				location = path;
				viewMode = tabInternal.shellBrower->GetCurrentViewMode();
				locked = tabInternal.bLocked;
				addressLocked = tabInternal.bAddressLocked;
			}

			std::wstring toString()
			{
				return _T("location = ") + location
					+ _T(", viewMode = ") + std::to_wstring(viewMode)
					+ _T(", locked = ") + std::to_wstring(locked)
					+ _T(", addressLocked = ") + std::to_wstring(addressLocked);
			}
		};

		TabsApi(IExplorerplusplus *pexpp);
		~TabsApi();

		boost::optional<Tab> get(int tabId);
		void create(std::wstring path);
		bool close(int tabId);

	private:

		IExplorerplusplus *m_pexpp;
	};
}