// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"

class ListViewColumnModel;
class ResourceLoader;

// Shows a menu containing the columns provided by the ListViewColumnModel instance. Each column can
// be toggled on/off.
class ListViewColumnsMenu : public MenuBase
{
public:
	ListViewColumnsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		ListViewColumnModel *columnModel, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();

	ListViewColumnModel *const m_columnModel;
	const ResourceLoader *const m_resourceLoader;
};
