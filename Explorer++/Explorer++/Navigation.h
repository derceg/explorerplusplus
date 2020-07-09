// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

__interface IExplorerplusplus;
class TabContainer;

class Navigation
{
public:
	Navigation(IExplorerplusplus *expp);

	void OnNavigateUp();

	HRESULT BrowseFolderInCurrentTab(const TCHAR *szPath);
	HRESULT BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory);

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);

private:
	IExplorerplusplus *m_expp;
	TabContainer *m_tabContainer;
};