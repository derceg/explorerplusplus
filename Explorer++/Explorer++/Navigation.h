// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include "CoreInterface.h"
#include "SignalWrapper.h"
#include "TabContainer.h"

class Navigation
{
public:

	Navigation(std::shared_ptr<Config> config, IExplorerplusplus *expp);

	void OnNavigateHome();
	void OnNavigateUp();

	HRESULT BrowseFolderInCurrentTab(const TCHAR *szPath);
	HRESULT BrowseFolder(Tab &tab, const TCHAR *szPath);
	HRESULT BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory);
	HRESULT BrowseFolder(Tab &tab, PCIDLIST_ABSOLUTE pidlDirectory);

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);

private:

	std::shared_ptr<Config> m_config;
	IExplorerplusplus *m_expp;
	TabContainer *m_tabContainer;
};