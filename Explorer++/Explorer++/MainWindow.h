// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "Tab.h"
#include "../Helper/BaseWindow.h"

struct Config;

class MainWindow : BaseWindow
{
public:

	static MainWindow *Create(HWND hwnd, std::shared_ptr<Config> config, HINSTANCE instance,
		IExplorerplusplus *expp);

private:

	MainWindow(HWND hwnd, std::shared_ptr<Config> config, HINSTANCE instance,
		IExplorerplusplus *expp);
	~MainWindow() = default;

	void OnNavigationCompleted(const Tab &tab);
	void OnTabSelected(const Tab &tab);

	void OnShowFullTitlePathUpdated(BOOL newValue);
	void OnShowUserNameInTitleBarUpdated(BOOL newValue);
	void OnShowPrivilegeLevelInTitleBarUpdated(BOOL newValue);

	void UpdateWindowText();

	HWND m_hwnd;
	std::shared_ptr<Config> m_config;
	HINSTANCE m_instance;
	IExplorerplusplus *m_expp;

	std::vector<boost::signals2::scoped_connection> m_connections;
};