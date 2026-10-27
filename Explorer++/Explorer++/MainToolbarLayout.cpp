// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbarLayout.h"
#include <algorithm>

MainToolbarLayout::MainToolbarLayout(const std::vector<MainToolbarButtonId> &buttons)
{
	for (auto button : buttons)
	{
		AddButton(button);
	}
}

void MainToolbarLayout::AddButton(MainToolbarButtonId button)
{
	if ((button != MainToolbarButtonId::Separator)
		&& (std::ranges::find(m_buttons, button) != m_buttons.end()))
	{
		return;
	}

	m_buttons.push_back(button);
}

const std::vector<MainToolbarButtonId> &MainToolbarLayout::GetButtons() const
{
	return m_buttons;
}
