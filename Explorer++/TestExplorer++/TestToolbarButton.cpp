// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TestToolbarButton.h"

TestToolbarButton::TestToolbarButton(MouseEventCallback clickedCallback /*= {}*/) :
	ToolbarButton(std::move(clickedCallback)),
	m_index(m_indexCounter++)
{
}

std::wstring TestToolbarButton::GetText() const
{
	return L"Test button " + std::to_wstring(m_index);
}

std::wstring TestToolbarButton::GetTooltipText() const
{
	return L"Help text " + std::to_wstring(m_index);
}

std::optional<int> TestToolbarButton::GetImageIndex() const
{
	return std::nullopt;
}

bool TestToolbarButton::IsEnabled() const
{
	return m_enabled;
}

bool TestToolbarButton::IsChecked() const
{
	return m_checked;
}

void TestToolbarButton::SetEnabled(bool enabled)
{
	m_enabled = enabled;
}

void TestToolbarButton::SetChecked(bool checked)
{
	m_checked = checked;
}
