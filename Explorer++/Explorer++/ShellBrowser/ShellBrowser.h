// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <memory>
#include <vector>

struct FolderSettings;
class NavigationManager;
class NavigationRequest;
class ShellNavigationController;
class Tab;

class ShellBrowser
{
public:
	ShellBrowser();
	virtual ~ShellBrowser() = default;

	int GetId() const;

	const Tab *GetTab() const;
	void SetTab(const Tab *tab);

	const PidlAbsolute &GetDirectory() const;
	const NavigationRequest *MaybeGetLatestActiveNavigation() const;

	virtual FolderSettings GetFolderSettings() const = 0;
	virtual ShellNavigationController *GetNavigationController() const = 0;
	virtual bool CanCreateNewFolder() const = 0;
	virtual void CreateNewFolder() = 0;
	virtual bool CanSplitFile() const = 0;
	virtual void SplitFile() = 0;

protected:
	virtual NavigationManager *GetNavigationManager() = 0;
	virtual const NavigationManager *GetNavigationManager() const = 0;

private:
	static inline int idCounter = 1;
	const int m_id;

	const Tab *m_tab = nullptr;
};
