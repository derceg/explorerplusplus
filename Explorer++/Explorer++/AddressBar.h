// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AddressBarDelegate.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>
#include <vector>

class AddressBarView;
class App;
class AsyncIconFetcher;
class BrowserWindow;
class NavigationRequest;
class Runtime;
class ShellBrowser;
class Tab;

class AddressBar : private AddressBarDelegate
{
public:
	static AddressBar *Create(AddressBarView *view, App *app, BrowserWindow *browser);

	AddressBarView *GetView() const;

private:
	enum class IconUpdateType
	{
		// Indicates that the icon will be fetched only if there is no cached icon available.
		FetchIfNotCached,

		// Always fetches the icon, regardless of whether a cached icon is available or not.
		AlwaysFetch
	};

	AddressBar(AddressBarView *view, App *app, BrowserWindow *browser);
	~AddressBar() = default;

	void Initialize();

	// AddressBarDelegate
	bool OnKeyPressed(UINT key) override;
	void OnBeginDrag() override;

	void OnEnterPressed();
	void OnEscapePressed();
	void OnTabSelected(const Tab &tab);
	void OnNavigationCommitted(const NavigationRequest *request);
	void OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser);
	void UpdateTextAndIcon(const Tab &tab,
		IconUpdateType iconUpdateType = IconUpdateType::FetchIfNotCached);
	static concurrencpp::null_result RetrieveUpdatedIcon(WeakPtr<AddressBar> self,
		PidlAbsolute pidl, std::shared_ptr<AsyncIconFetcher> iconFetcher, Runtime *runtime,
		std::stop_token stopToken);
	void OnWindowDestroyed();

	AddressBarView *const m_view;
	App *const m_app;
	BrowserWindow *const m_browser;

	std::vector<boost::signals2::scoped_connection> m_connections;

	std::unique_ptr<ScopedStopSource> m_scopedStopSource;

	WeakPtrFactory<AddressBar> m_weakPtrFactory;
};
