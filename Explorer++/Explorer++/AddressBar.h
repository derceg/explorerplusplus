// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "ShellBrowser/HistoryEntry.h"
#include "SignalWrapper.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include "../Helper/WindowSubclass.h"
#include <concurrencpp/concurrencpp.h>
#include <memory>

class App;
class AsyncIconFetcher;
class BrowserWindow;
class CoreInterface;
struct NavigateParams;
class Runtime;
class Tab;

class AddressBar : public BaseWindow
{
public:
	static AddressBar *Create(HWND parent, App *app, BrowserWindow *browserWindow,
		CoreInterface *coreInterface);

	// Signals
	SignalWrapper<AddressBar, void()> sizeUpdatedSignal;

private:
	AddressBar(HWND parent, App *app, BrowserWindow *browserWindow, CoreInterface *coreInterface);
	~AddressBar() = default;

	static HWND CreateAddressBar(HWND parent);

	LRESULT ComboBoxSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void OnEnterPressed();
	void OnEscapePressed();
	void OnBeginDrag();
	void OnTabSelected(const Tab &tab);
	void OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams);
	void UpdateTextAndIcon(const Tab &tab);
	static concurrencpp::null_result RetrieveUpdatedIcon(WeakPtr<AddressBar> self,
		PidlAbsolute pidl, std::shared_ptr<AsyncIconFetcher> iconFetcher, Runtime *runtime,
		std::stop_token stopToken);
	void UpdateTextAndIconInUI(std::wstring *text, int iconIndex);
	void RevertTextInUI();
	void OnFontOrDpiUpdated();

	App *const m_app;
	BrowserWindow *const m_browserWindow;
	CoreInterface *const m_coreInterface;

	MainFontSetter m_fontSetter;

	std::wstring m_currentText;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	std::unique_ptr<ScopedStopSource> m_scopedStopSource;

	WeakPtrFactory<AddressBar> m_weakPtrFactory;
};
