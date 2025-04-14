// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRuleListView.h"
#include "ColorRuleEditorDialog.h"
#include "ColorRuleModel.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"

ColorRuleListView::ColorRuleListView(HWND listView, const ResourceLoader *resourceLoader,
	HINSTANCE resourceInstance, ThemeManager *themeManager, ColorRuleModel *model) :
	m_listView(listView),
	m_resourceLoader(resourceLoader),
	m_resourceInstance(resourceInstance),
	m_themeManager(themeManager),
	m_model(model)
{
	ListView_SetExtendedListViewStyleEx(listView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

	InsertColumns();
	InsertColorRules();

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(listView),
		std::bind_front(&ColorRuleListView::ParentWndProc, this)));

	m_connections.push_back(
		model->AddItemAddedObserver(std::bind_front(&ColorRuleListView::OnColorRuleAdded, this)));
	m_connections.push_back(model->AddItemUpdatedObserver(
		std::bind_front(&ColorRuleListView::OnColorRuleUpdated, this)));
	m_connections.push_back(
		model->AddItemMovedObserver(std::bind_front(&ColorRuleListView::OnColorRuleMoved, this)));
	m_connections.push_back(model->AddItemRemovedObserver(
		std::bind_front(&ColorRuleListView::OnColorRuleRemoved, this)));
	m_connections.push_back(model->AddAllItemsRemovedObserver(
		std::bind_front(&ColorRuleListView::OnAllColorRulesRemoved, this)));
}

LRESULT CALLBACK ColorRuleListView::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_listView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_DBLCLK:
				OnDoubleClick(reinterpret_cast<NMITEMACTIVATE *>(lParam));
				break;

			case LVN_ITEMCHANGED:
			{
				auto info = reinterpret_cast<NMLISTVIEW *>(lParam);

				if (WI_IsFlagClear(info->uOldState, LVIS_SELECTED)
					&& WI_IsFlagSet(info->uNewState, LVIS_SELECTED))
				{
					colorRuleSelectedSignal.m_signal();
				}
				else if (WI_IsFlagSet(info->uOldState, LVIS_SELECTED)
					&& WI_IsFlagClear(info->uNewState, LVIS_SELECTED))
				{
					colorRuleDeselectedSignal.m_signal();
				}
			}
			break;
			}
		}
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ColorRuleListView::OnDoubleClick(const NMITEMACTIVATE *itemActivate)
{
	if (itemActivate->iItem == -1)
	{
		return;
	}

	auto *colorRule = m_model->GetItemAtIndex(itemActivate->iItem);

	ColorRuleEditorDialog editorDialog(m_resourceLoader, m_resourceInstance, GetParent(m_listView),
		m_themeManager, m_model, ColorRuleEditorDialog::EditDetails::EditColorRule(colorRule));
	editorDialog.ShowModalDialog();
}

void ColorRuleListView::InsertColumns()
{
	std::wstring text = m_resourceLoader->LoadString(IDS_CUSTOMIZE_COLORS_COLUMN_DESCRIPTION);
	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = text.data();
	ListView_InsertColumn(m_listView, 0, &lvColumn);

	text = m_resourceLoader->LoadString(IDS_CUSTOMIZE_COLORS_COLUMN_FILENAME_PATTERN);
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = text.data();
	ListView_InsertColumn(m_listView, 1, &lvColumn);

	text = m_resourceLoader->LoadString(IDS_CUSTOMIZE_COLORS_COLUMN_ATTRIBUTES);
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = text.data();
	ListView_InsertColumn(m_listView, 2, &lvColumn);

	RECT clientRect;
	[[maybe_unused]] auto res = GetClientRect(m_listView, &clientRect);
	assert(res);

	SendMessage(m_listView, LVM_SETCOLUMNWIDTH, 0, GetRectWidth(&clientRect) / 3);
	SendMessage(m_listView, LVM_SETCOLUMNWIDTH, 1, GetRectWidth(&clientRect) / 3);
	SendMessage(m_listView, LVM_SETCOLUMNWIDTH, 2, GetRectWidth(&clientRect) / 3);
}

void ColorRuleListView::InsertColorRules()
{
	size_t index = 0;

	for (const auto &colorRule : m_model->GetItems())
	{
		InsertColorRule(colorRule.get(), index);

		index++;
	}
}

void ColorRuleListView::InsertColorRule(const ColorRule *colorRule, size_t index)
{
	LVITEM lvItem = {};
	lvItem.iItem = static_cast<int>(index);
	lvItem.iSubItem = 0;
	[[maybe_unused]] int finalIndex = ListView_InsertItem(m_listView, &lvItem);
	assert(finalIndex == static_cast<int>(index));

	UpdateColumnsForItem(colorRule, index);
}

void ColorRuleListView::UpdateColumnsForItem(const ColorRule *colorRule, size_t index)
{
	auto description = colorRule->GetDescription();
	ListView_SetItemText(m_listView, index, 0, description.data());

	auto pattern = colorRule->GetFilterPattern();
	ListView_SetItemText(m_listView, index, 1, pattern.data());

	auto attributesString = BuildFileAttributesString(colorRule->GetFilterAttributes());
	ListView_SetItemText(m_listView, index, 2, attributesString.data());
}

void ColorRuleListView::OnColorRuleAdded(ColorRule *colorRule, size_t index)
{
	InsertColorRule(colorRule, index);
}

void ColorRuleListView::OnColorRuleUpdated(ColorRule *colorRule)
{
	auto index = m_model->GetItemIndex(colorRule);
	UpdateColumnsForItem(colorRule, index);
}

void ColorRuleListView::OnColorRuleMoved(ColorRule *colorRule, size_t oldIndex, size_t newIndex)
{
	bool selected = false;
	int selectedIndex = ListView_GetNextItem(m_listView, -1, LVNI_SELECTED);

	if (selectedIndex == static_cast<int>(oldIndex))
	{
		selected = true;
	}

	ListView_DeleteItem(m_listView, oldIndex);
	InsertColorRule(colorRule, newIndex);

	if (selected)
	{
		ListViewHelper::SelectItem(m_listView, static_cast<int>(newIndex), true);
	}
}

void ColorRuleListView::OnColorRuleRemoved(const ColorRule *colorRule, size_t oldIndex)
{
	UNREFERENCED_PARAMETER(colorRule);

	ListView_DeleteItem(m_listView, oldIndex);
}

void ColorRuleListView::OnAllColorRulesRemoved()
{
	ListView_DeleteAllItems(m_listView);
}

ColorRule *ColorRuleListView::MaybeGetSelectedColorRule()
{
	int selectedIndex = ListView_GetNextItem(m_listView, -1, LVNI_SELECTED);

	if (selectedIndex == -1)
	{
		return nullptr;
	}

	return m_model->GetItemAtIndex(selectedIndex);
}
