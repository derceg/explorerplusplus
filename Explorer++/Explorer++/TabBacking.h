// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <memory>
#include <vector>

class BrowserWindow;
struct Config;
class CoreInterface;
class MainFontSetter;
class ResourceLoader;
class TabEvents;
class WindowSubclass;

class TabBacking
{
public:
	static TabBacking *Create(HWND parent, BrowserWindow *browser, CoreInterface *coreInterface,
		const ResourceLoader *resourceLoader, const Config *config, TabEvents *tabEvents);

	HWND GetHWND() const;

private:
	static constexpr int CLOSE_BUTTON_ID = 1;

	TabBacking(HWND parent, BrowserWindow *browser, CoreInterface *coreInterface,
		const ResourceLoader *resourceLoader, const Config *config, TabEvents *tabEvents);

	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateToolbar();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void OnCloseButtonClicked();
	void UpdateLayout();
	void OnNcDestroy();

	const HWND m_hwnd;
	CoreInterface *const m_coreInterface;
	HWND m_toolbar = nullptr;
	wil::unique_himagelist m_toolbarImageList;
	std::unique_ptr<MainFontSetter> m_toolbarTooltipFontSetter;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
