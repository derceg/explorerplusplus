// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuBase.h"
#include "MenuView.h"

MenuBase::MenuBase(MenuView *menuView) : m_menuView(menuView)
{
	m_connections.push_back(
		menuView->AddViewDestroyedObserver(std::bind_front(&MenuBase::OnViewDestroyed, this)));
}

void MenuBase::OnViewDestroyed()
{
	// The view should always outlive the associated menu controller, so this method should never be
	// triggered.
	CHECK(false);
}
