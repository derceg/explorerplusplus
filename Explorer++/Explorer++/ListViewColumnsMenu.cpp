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

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind(&ListViewColumnsMenu::OnMenuItemSelected, this, std::placeholders::_1)));
}

void ListViewColumnsMenu::BuildMenu()
{
	for (auto columnId : m_columnModel->GetAllColumnIds())
	{
		const auto &column = m_columnModel->GetColumnById(columnId);

		UINT id = m_idCounter++;
		m_menuView->AppendItem(id, m_resourceLoader->LoadString(column.nameStringId));
		m_menuView->CheckItem(id, column.visible);

		// The primary column can't be removed.
		m_menuView->EnableItem(id, !m_columnModel->IsPrimaryColumnId(columnId));

		m_idToColumnMap.insert({ id, columnId });
	}
}

void ListViewColumnsMenu::OnMenuItemSelected(UINT menuItemId)
{
	auto itr = m_idToColumnMap.find(menuItemId);
	CHECK(itr != m_idToColumnMap.end());
	m_columnModel->SetColumnVisible(itr->second, !m_columnModel->IsColumnVisible(itr->second));
}
