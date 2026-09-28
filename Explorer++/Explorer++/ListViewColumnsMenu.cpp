// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListViewColumnsMenu.h"
#include "ListViewColumn.h"
#include "ListViewColumnModel.h"
#include "MenuView.h"
#include "ResourceLoader.h"

ListViewColumnsMenu::ListViewColumnsMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, ListViewColumnModel *columnModel,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_columnModel(columnModel),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();
}

void ListViewColumnsMenu::BuildMenu()
{
	for (auto columnId : m_columnModel->GetAllColumnIds())
	{
		const auto &column = m_columnModel->GetColumnById(columnId);

		m_rootMenuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(column.nameStringId))
				.SetEnabledCallback(
					[this, columnId] { return !m_columnModel->IsPrimaryColumnId(columnId); })
				.SetCheckedCallback(
					[this, columnId] { return m_columnModel->GetColumnById(columnId).visible; })
				.SetSelectedCallback(
					[this, columnId] { m_columnModel->ToggleColumnVisibility(columnId); }));
	}
}
