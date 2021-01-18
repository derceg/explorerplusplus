// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/HistoryEntry.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>
#include <optional>

__interface IExplorerplusplus;
class Tab;

class AddressBar : public BaseWindow
{
public:
	static AddressBar *Create(HWND parent, IExplorerplusplus *expp);

private:
	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	// This is the same background color as used in the Explorer address bar.
	static inline constexpr COLORREF DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	AddressBar(HWND parent, IExplorerplusplus *expp);
	~AddressBar() = default;

	static HWND CreateAddressBar(HWND parent);

	LRESULT ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK EditSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	std::optional<LRESULT> OnComboBoxExCtlColorEdit(HWND hwnd, HDC hdc);
	void OnGo();
	void OnBeginDrag();
	static std::optional<wil::unique_stg_medium> GenerateShortcutDescriptorStgMedium(
		PCIDLIST_ABSOLUTE pidl);
	static std::optional<FILEGROUPDESCRIPTOR> GenerateShortcutDescriptor(PCIDLIST_ABSOLUTE pidl);
	static std::optional<wil::unique_stg_medium> GenerateShortcutContentsStgMedium(
		PCIDLIST_ABSOLUTE pidl);
	void OnTabSelected(const Tab &tab);
	void OnNavigationCommitted(const Tab &tab, PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry);
	void UpdateTextAndIcon(const Tab &tab);
	void UpdateTextAndIconInUI(std::wstring *text, int iconIndex);
	void OnHistoryEntryUpdated(const HistoryEntry &entry, HistoryEntry::PropertyType propertyType);

	IExplorerplusplus *m_expp;
	wil::unique_hbrush m_backgroundBrush;

	boost::signals2::scoped_connection m_historyEntryUpdatedConnection;
	int m_defaultFolderIconIndex;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};