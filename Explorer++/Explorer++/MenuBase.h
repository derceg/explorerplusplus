// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <vector>

class MenuView;

class MenuBase
{
public:
	MenuBase(MenuView *menuView);

	virtual ~MenuBase() = default;

protected:
	MenuView *const m_menuView;

private:
	void OnViewDestroyed();

	std::vector<boost::signals2::scoped_connection> m_connections;
};
