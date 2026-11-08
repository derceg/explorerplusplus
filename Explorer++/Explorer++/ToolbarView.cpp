// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarView.h"
#include <algorithm>

ToolbarView::ToolbarView(ViewBase *parent, DWORD style, DWORD extendedStyle,
	MenuHelpTextHost *menuHelpTextHost, const Config *config) :
	ToolbarViewBase(parent, style, extendedStyle, menuHelpTextHost, config)
{
}

void ToolbarView::AddButton(std::unique_ptr<ToolbarButton> button, size_t index)
{
	CHECK(index <= m_buttons.size());

	int id = m_idCounter++;

	button->SetParent(this);

	auto *rawButton = button.get();
	m_buttons.insert(m_buttons.begin() + index, { id, std::move(button) });

	ToolbarViewBase::AddButton(id, rawButton, index);
}

ToolbarButton *ToolbarView::GetButtonAtIndex(size_t index) const
{
	CHECK_LT(index, m_buttons.size());
	return m_buttons[index].button.get();
}

void ToolbarView::RemoveButton(const ToolbarButton *button)
{
	auto itr =
		std::ranges::find(m_buttons, button, [](const auto &entry) { return entry.button.get(); });
	CHECK(itr != m_buttons.end());

	auto index = std::distance(m_buttons.begin(), itr);

	auto res = SendMessage(GetHWND(), TB_DELETEBUTTON, index, 0);
	CHECK(res);

	m_buttons.erase(itr);

	NotifySizeRequirementsChanged();
}

ToolbarButton *ToolbarView::GetButtonById(int id) const
{
	auto itr = std::ranges::find(m_buttons, id, &ButtonEntry::id);
	CHECK(itr != m_buttons.end());
	return itr->button.get();
}
