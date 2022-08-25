// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class CoreInterface;
class TabContainer;

class Navigation
{
public:
	Navigation(CoreInterface *coreInterface);

	void OnNavigateUp();

	HRESULT BrowseFolderInCurrentTab(const TCHAR *szPath);
	HRESULT BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory);

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);

private:
	CoreInterface *m_coreInterface;
	TabContainer *m_tabContainer;
};
