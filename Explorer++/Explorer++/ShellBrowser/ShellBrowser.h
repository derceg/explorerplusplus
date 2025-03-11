// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>
#include <vector>

struct FolderSettings;
class NavigationManager;
class NavigationRequest;
class ShellBrowserHelperBase;
class ShellNavigationController;
class Tab;

class ShellBrowser
{
public:
	ShellBrowser();
	virtual ~ShellBrowser() = default;

	virtual FolderSettings GetFolderSettings() const = 0;
	virtual ShellNavigationController *GetNavigationController() const = 0;

	int GetId() const;

	const Tab *GetTab() const;
	void SetTab(const Tab *tab);

	void AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper);
	const NavigationRequest *MaybeGetLatestActiveNavigation() const;

protected:
	virtual NavigationManager *GetNavigationManager() = 0;
	virtual const NavigationManager *GetNavigationManager() const = 0;

private:
	static inline int idCounter = 1;
	const int m_id;

	std::vector<std::unique_ptr<ShellBrowserHelperBase>> m_helpers;
	const Tab *m_tab = nullptr;
};
