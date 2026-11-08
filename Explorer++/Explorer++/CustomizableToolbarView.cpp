// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomizableToolbarView.h"
#include "TestHelper.h"
#include <algorithm>

CustomizableToolbarView::CustomizableToolbarView(ViewBase *parent, DWORD style, DWORD extendedStyle,
	MenuHelpTextHost *menuHelpTextHost, const Config *config) :
	ToolbarViewBase(parent, style, extendedStyle, menuHelpTextHost, config)
{
}

void CustomizableToolbarView::InitializeAvailableButtons(
	std::vector<std::unique_ptr<ToolbarButton>> buttons)
{
	CHECK(!m_availableButtonsInitialized);

	int idCounter = INITIAL_ID;

	for (auto &button : buttons)
	{
		button->SetParent(this);
		m_availableButtons.push_back({ idCounter++, std::move(button) });
	}

	m_availableButtonsInitialized = true;
}

void CustomizableToolbarView::SetDefaultItems(const std::vector<ToolbarItem> &items)
{
	CHECK(m_availableButtonsInitialized);

	m_defaultItems = items;
}

void CustomizableToolbarView::SetItems(const std::vector<ToolbarItem> &items)
{
	CHECK(m_availableButtonsInitialized);

	ClearExistingButtons();

	size_t index = 0;

	for (const auto &item : items)
	{
		if (item.IsButton())
		{
			auto *button = item.GetButton();
			AddButton(GetButtonId(button), button, index);
		}
		else
		{
			AddSeparator(index);
		}

		index++;
	}
}

void CustomizableToolbarView::StartCustomization()
{
	SendMessage(GetHWND(), TB_CUSTOMIZE, 0, 0);
}

std::optional<LRESULT> CustomizableToolbarView::MaybeHandleNotification(NMHDR *notification)
{
	switch (notification->code)
	{
	case TBN_QUERYINSERT:
		return true;

	case TBN_QUERYDELETE:
		return true;

	case TBN_GETBUTTONINFO:
		return OnGetButtonInfo(reinterpret_cast<NMTOOLBAR *>(notification));

	case TBN_RESET:
		OnReset();
		return 0;

	case TBN_TOOLBARCHANGE:
		OnButtonsChanged();
		return 0;

	case TBN_INITCUSTOMIZE:
		return TBNRF_HIDEHELP;
	}

	return std::nullopt;
}

bool CustomizableToolbarView::OnGetButtonInfo(NMTOOLBAR *data)
{
	if (data->iItem < 0 || static_cast<size_t>(data->iItem) >= m_availableButtons.size())
	{
		return false;
	}

	const auto &entry = m_availableButtons[data->iItem];
	const auto *button = entry.button.get();

	StringCchCopy(data->pszText, data->cchText, button->GetText().c_str());

	data->tbButton = BuildNativeButtonStruct(entry.id, button);
	data->tbButton.iString = reinterpret_cast<INT_PTR>(data->pszText);

	return true;
}

void CustomizableToolbarView::OnReset()
{
	SetItems(m_defaultItems);
}

void CustomizableToolbarView::OnButtonsChanged()
{
	NotifySizeRequirementsChanged();
}

void CustomizableToolbarView::ClearExistingButtons()
{
	for (size_t count = GetNumButtons(); count > 0; count--)
	{
		auto res = SendMessage(GetHWND(), TB_DELETEBUTTON, count - 1, 0);
		CHECK(res);
	}

	NotifySizeRequirementsChanged();
}

int CustomizableToolbarView::GetButtonId(const ToolbarButton *button) const
{
	auto itr = std::ranges::find(m_availableButtons, button,
		[](const auto &entry) { return entry.button.get(); });
	CHECK(itr != m_availableButtons.end());
	return itr->id;
}

ToolbarButton *CustomizableToolbarView::GetButtonById(int id) const
{
	auto itr = std::ranges::find(m_availableButtons, id, &ButtonEntry::id);
	CHECK(itr != m_availableButtons.end());
	return itr->button.get();
}
