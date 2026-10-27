// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "CoreInterface.h"
#include "MainToolbarButtonId.h"
#include "Tab.h"
#include "ToolbarBase.h"
#include <boost/bimap.hpp>
#include <wil/resource.h>
#include <memory>
#include <optional>
#include <unordered_map>

class AppServices;
class MainToolbarLayout;
class MainToolbarView;
class MenuBase;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class ShellIconLoader;
class ToolbarButton;
class ToolbarItem;

class MainToolbar : public ToolbarBase
{
public:
	MainToolbar(MainToolbarView *view, const std::optional<MainToolbarLayout> &initialLayout,
		BrowserWindow *browser, AppServices *appServices, CoreInterface *coreInterface,
		ShellIconLoader *shellIconLoader);

	void UpdateToolbarButtonStates();

	MainToolbarLayout GetLayoutForStorage() const;

	ToolbarButton *GetButtonForIdForTesting(MainToolbarButtonId buttonId) const;

private:
	void Initialize(const std::optional<MainToolbarLayout> &initialLayout);
	void CreateImageLists();
	void SetToolbarImageList();
	void InitializeAvailableButtons();
	std::unique_ptr<ToolbarButton> CreateButtonForId(MainToolbarButtonId buttonId);
	std::vector<ToolbarItem> ToToolbarItems(const std::vector<MainToolbarButtonId> &buttonIds);
	std::vector<MainToolbarButtonId> GetDefaultButtons() const;

	void OnTabSelected(const Tab &tab);
	void OnNavigationCommitted(const NavigationRequest *request);
	void OnBrowserCommandTargetChanged();
	void OnBrowserLifecycleStateChanged(BrowserWindow::LifecycleState updatedState);
	void OnUseLargeToolbarIconsUpdated(BOOL newValue);
	void OnClipboardUpdate();
	void EnableButton(MainToolbarButtonId buttonId, bool enable);

	ToolbarButton *GetButtonForId(MainToolbarButtonId buttonId) const;
	MainToolbarButtonId GetIdForButton(ToolbarButton *button) const;

	MainToolbarView *const m_view;
	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	CoreInterface *const m_coreInterface;
	ShellIconLoader *const m_shellIconLoader;
	const Config *const m_config;
	const ResourceLoader *const m_resourceLoader;

	boost::bimap<MainToolbarButtonId, ToolbarButton *> m_idToButtonMap;
	wil::unique_himagelist m_imageListSmall;
	wil::unique_himagelist m_imageListLarge;
	std::unordered_map<MainToolbarButtonId, int> m_idToImageIndexMap;

	std::vector<boost::signals2::scoped_connection> m_connections;

	std::optional<int> m_middleButtonItem;
};
