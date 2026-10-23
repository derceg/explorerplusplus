// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AddressBarView.h"
#include "AddressBarViewDelegate.h"
#include "BrowserCommandTarget.h"
#include "RebarBand.h"
#include "ScopedBrowserCommandTarget.h"
#include "../Helper/Pidl.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>
#include <vector>

class AsyncIconFetcher;
class BrowserWindow;
class NavigationEvents;
class NavigationRequest;
class Runtime;
class ShellBrowser;
class ShellBrowserEvents;
class Tab;
class TabEvents;

class AddressBar : private AddressBarViewDelegate, public BrowserCommandTarget, public RebarBand
{
public:
	AddressBar(AddressBarView *view, BrowserWindow *browser, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		const Runtime *runtime, AsyncIconFetcher *iconFetcher);

	// RebarBand
	AddressBarView *GetView() override;

	// BrowserCommandTarget
	bool IsCommandEnabled(int command) const override;
	void ExecuteCommand(int command) override;

private:
	enum class IconUpdateType
	{
		// Indicates that the icon will be fetched only if there is no cached icon available.
		FetchIfNotCached,

		// Always fetches the icon, regardless of whether a cached icon is available or not.
		AlwaysFetch
	};

	void Initialize(TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
		NavigationEvents *navigationEvents);

	// AddressBarViewDelegate
	bool OnKeyPressed(UINT key) override;
	void OnBeginDrag() override;
	void OnFocused() override;

	void OnEnterPressed();
	void OnEscapePressed();
	void OnTabSelected(const Tab &tab);
	void OnNavigationCommitted(const NavigationRequest *request);
	void OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser);
	void UpdateTextAndIcon(const ShellBrowser *shellBrowser,
		IconUpdateType iconUpdateType = IconUpdateType::FetchIfNotCached);
	static concurrencpp::null_result RetrieveUpdatedIcon(WeakPtr<AddressBar> weakSelf,
		PidlAbsolute pidl);

	AddressBarView *const m_view;
	BrowserWindow *const m_browser;
	const Runtime *const m_runtime;
	AsyncIconFetcher *const m_iconFetcher;
	ScopedBrowserCommandTarget m_commandTarget;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::unique_ptr<ScopedStopSource> m_scopedStopSource;

	WeakPtrFactory<AddressBar> m_weakPtrFactory;
};
