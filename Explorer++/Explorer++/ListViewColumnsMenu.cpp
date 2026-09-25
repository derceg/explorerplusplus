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

		UINT id = m_idCounter++;
		m_rootMenuView->AppendItem(this, id, m_resourceLoader->LoadString(column.nameStringId));

		m_idToColumnMap.insert({ id, columnId });
	}
}

bool ListViewColumnsMenu::IsItemEnabled(UINT id) const
{
	// The primary column can't be removed.
	return !m_columnModel->IsPrimaryColumnId(GetColumnIdForItem(id));
}

bool ListViewColumnsMenu::IsItemChecked(UINT id) const
{
	const auto &column = m_columnModel->GetColumnById(GetColumnIdForItem(id));
	return column.visible;
}

void ListViewColumnsMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	auto columnId = GetColumnIdForItem(id);
	m_columnModel->SetColumnVisible(columnId, !m_columnModel->IsColumnVisible(columnId));
}

ListViewColumnId ListViewColumnsMenu::GetColumnIdForItem(UINT id) const
{
	auto itr = m_idToColumnMap.find(id);
	CHECK(itr != m_idToColumnMap.end());
	return itr->second;
}
