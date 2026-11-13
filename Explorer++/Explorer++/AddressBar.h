// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AddressBarView.h"
#include "AddressBarViewDelegate.h"
#include "BrowserCommandTarget.h"
#include "RebarBand.h"
#include "ScopedBrowserCommandTarget.h"
#include "../Helper/DropTargetDelegate.h"
#include "../Helper/Pidl.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>
#include <variant>
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

class AddressBar :
	public RebarBand,
	public BrowserCommandTarget,
	private AddressBarViewDelegate,
	private DropTargetDelegate
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

	DWORD SimulateDropForTest(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect);

private:
	using DroppedData = std::variant<std::monostate, PidlAbsolute, std::wstring>;

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

	// DropTargetDelegate
	DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;
	DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) override;
	void DragLeave() override;
	DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;

	static DroppedData MaybeExtractDropData(IDataObject *dataObject);
	static HRESULT MaybeExtractFirstPidlFromDataObject(IDataObject *dataObject,
		PidlAbsolute &outputPidl);
	DWORD GetDropEffect(DWORD allowedEffects) const;

	AddressBarView *const m_view;
	BrowserWindow *const m_browser;
	const Runtime *const m_runtime;
	AsyncIconFetcher *const m_iconFetcher;
	ScopedBrowserCommandTarget m_commandTarget;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::unique_ptr<ScopedStopSource> m_scopedStopSource;

	// This will only contain a non-empty value while a drop is in progress.
	DroppedData m_droppedData;

	WeakPtrFactory<AddressBar> m_weakPtrFactory;
};
