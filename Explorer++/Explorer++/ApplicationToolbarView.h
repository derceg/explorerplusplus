// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationContextMenu.h"
#include "ToolbarView.h"
#include <boost/signals2.hpp>
#include <vector>

class CoreInterface;

namespace Applications
{

class Application;
class ApplicationModel;

class ApplicationToolbarView : public ToolbarView
{
public:
	static ApplicationToolbarView *Create(HWND parent, CoreInterface *coreInterface,
		ApplicationModel *model);

	void ShowContextMenu(Application *application, const POINT &ptClient);

private:
	ApplicationToolbarView(HWND parent, CoreInterface *coreInterface, ApplicationModel *model);

	void OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt);

	HMODULE m_resourceModule;
	std::vector<boost::signals2::scoped_connection> m_connections;

	ApplicationContextMenu m_contextMenu;
};

}
