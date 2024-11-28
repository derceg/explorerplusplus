// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "ShellBrowser/HistoryEntry.h"
#include "SignalWrapper.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/WindowSubclass.h"
#include <wil/resource.h>
#include <optional>

class BrowserWindow;
class CoreInterface;
struct NavigateParams;
class Tab;

class AddressBar : public BaseWindow
{
public:
	static AddressBar *Create(HWND parent, BrowserWindow *browserWindow,
		CoreInterface *coreInterface);

	// Signals
	SignalWrapper<AddressBar, void()> sizeUpdatedSignal;

private:
	AddressBar(HWND parent, BrowserWindow *browserWindow, CoreInterface *coreInterface);
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
	void UpdateTextAndIconInUI(std::wstring *text, int iconIndex);
	void RevertTextInUI();
	void OnHistoryEntryUpdated(const HistoryEntry &entry, HistoryEntry::PropertyType propertyType);
	void OnFontOrDpiUpdated();

	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;

	MainFontSetter m_fontSetter;

	boost::signals2::scoped_connection m_historyEntryUpdatedConnection;
	int m_defaultFolderIconIndex;

	std::wstring m_currentText;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
