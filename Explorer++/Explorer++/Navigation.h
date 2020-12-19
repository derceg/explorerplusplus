// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

__interface IExplorerplusplus;
class TabContainer;
class Tab;

class Navigation
{
public:
	Navigation(IExplorerplusplus *expp);

	void OnNavigateUp();

	HRESULT BrowseFolderInCurrentTab(const TCHAR *szPath);
	HRESULT BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory);

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);

private:
	struct SelectionInfo
	{
		int folderId;
		std::wstring filename;
	};

	void OnNavigationCompleted(const Tab &tab);
	void OnNavigationFailed(const Tab &tab);

	IExplorerplusplus *m_expp;
	TabContainer *m_tabContainer;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::unordered_map<int, SelectionInfo> m_selectionInfoMap;
};