// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "../Helper/Logging.h"

void ShellTreeView::StartDirectoryMonitoringForItem(ItemInfo &item)
{
	// There shouldn't be more than one call to monitor a directory.
	assert(item.shChangeNotifyId == 0);

	SHChangeNotifyEntry shcne;
	shcne.pidl = item.pidl.get();
	shcne.fRecursive = false;
	item.shChangeNotifyId = SHChangeNotifyRegister(m_hTreeView,
		SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
		SHCNE_ATTRIBUTES | SHCNE_MKDIR | SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_UPDATEDIR
			| SHCNE_UPDATEITEM,
		WM_APP_SHELL_NOTIFY, 1, &shcne);

	if (item.shChangeNotifyId == 0)
	{
		std::wstring path;
		HRESULT hr = GetDisplayName(item.pidl.get(), SHGDN_FORPARSING, path);

		if (SUCCEEDED(hr))
		{
			LOG(warning) << L"Couldn't monitor directory \"" << path << L"\" for changes.";
		}
	}
}

void ShellTreeView::StopDirectoryMonitoringForItem(ItemInfo &item)
{
	if (item.shChangeNotifyId == 0)
	{
		return;
	}

	[[maybe_unused]] auto res = SHChangeNotifyDeregister(item.shChangeNotifyId);
	assert(res);

	item.shChangeNotifyId = 0;
}
