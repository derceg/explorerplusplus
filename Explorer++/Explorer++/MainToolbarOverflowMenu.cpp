// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbarOverflowMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "ResourceIconModel.h"
#include "../Helper/EnumHelper.h"

MainToolbarOverflowMenu::MainToolbarOverflowMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager,
	const std::vector<MainToolbar::OverflowItem> &overflowItems, BrowserWindow *browser,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_browser(browser)
{
	BuildMenu(overflowItems, resourceLoader);
}

void MainToolbarOverflowMenu::BuildMenu(const std::vector<MainToolbar::OverflowItem> &overflowItems,
	const ResourceLoader *resourceLoader)
{
	for (const auto &item : overflowItems)
	{
		AddOverflowItem(item, resourceLoader);
	}
}

void MainToolbarOverflowMenu::AddOverflowItem(const MainToolbar::OverflowItem &item,
	const ResourceLoader *resourceLoader)
{
	if (item.button == MainToolbarButton::Separator)
	{
		m_rootMenuView->AppendSeparator();
		return;
	}

	std::unique_ptr<IconModel> iconModel;

	if (item.icon)
	{
		iconModel =
			std::make_unique<ResourceIconModel>(*item.icon, IconSize::Small, resourceLoader);
	}

	if (item.subMenuFactory)
	{
		auto *subMenuView = m_rootMenuView->AppendSubMenu(
			SubMenuItemParams(item.text).SetIconModel(std::move(iconModel)));
		m_childMenus.push_back(item.subMenuFactory(subMenuView));
	}
	else
	{
		m_rootMenuView->AppendItem(MenuItemParams(item.text)
				.SetIconModel(std::move(iconModel))
				.SetEnabledCallback([enabled = item.enabled] { return enabled; })
				.SetCheckedCallback([checked = item.checked] { return checked; })
				.SetSelectedCallback([this, button = item.button]
					{ SendCommandToBrowser(static_cast<WORD>(ToUnderlying(button))); }));
	}
}

void MainToolbarOverflowMenu::SendCommandToBrowser(WORD id)
{
	SendMessage(m_browser->GetHWND(), WM_COMMAND, MAKEWPARAM(id, 0), 0);
}
