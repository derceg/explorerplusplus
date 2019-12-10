// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "MainToolbar.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <optional>

class AddressBar : public CBaseWindow
{
public:

	static AddressBar *Create(HWND parent, IExplorerplusplus *expp, MainToolbar *mainToolbar);

private:

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	AddressBar(HWND parent, IExplorerplusplus *expp, MainToolbar *mainToolbar);
	~AddressBar() = default;

	static HWND CreateAddressBar(HWND parent);

	static LRESULT CALLBACK EditSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void OnGo();
	void OnBeginDrag();
	void OnTabSelected(const Tab &tab);
	void OnNavigationCompleted(const Tab &tab);
	void UpdateTextAndIcon(const Tab &tab);
	void UpdateTextAndIconInUI(std::wstring *text, int iconIndex);
	void OnHistoryEntryUpdated(const HistoryEntry &entry, HistoryEntry::PropertyType propertyType);

	IExplorerplusplus *m_expp;
	MainToolbar *m_mainToolbar;

	boost::signals2::scoped_connection m_historyEntryUpdatedConnection;
	int m_defaultFolderIconIndex;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};