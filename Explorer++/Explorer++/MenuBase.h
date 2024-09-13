// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <vector>

class MenuView;

// This class, along with MenuView, is used to implement an MVP menu system. That is, the view is
// considered passive and only contains general display logic. A class that derives from this class
// functions as a presenter and is responsible for retrieving model data and adding it to the view.
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
