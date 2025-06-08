// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "../Helper/SignalWrapper.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class AddressBarViewDelegate;
struct Config;
class WindowSubclass;

class AddressBarView
{
public:
	// Signals
	SignalWrapper<AddressBarView, void()> sizeUpdatedSignal;
	SignalWrapper<AddressBarView, void()> windowDestroyedSignal;

	static AddressBarView *Create(HWND parent, const Config *config);

	void SetDelegate(AddressBarViewDelegate *delegate);
	HWND GetHWND() const;
	std::wstring GetText() const;
	bool IsTextModified() const;
	void SelectAllText();
	void UpdateTextAndIcon(const std::optional<std::wstring> &optionalText, int iconIndex);
	void RevertText();

	AddressBarViewDelegate *GetDelegateForTesting();
	void SetTextForTesting(const std::wstring &text);

private:
	AddressBarView(HWND parent, const Config *config);

	static HWND CreateAddressBar(HWND parent);

	LRESULT ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND GetEditControl() const;
	void OnFontOrDpiUpdated();
	void OnNcDestroy();

	const HWND m_hwnd;
	AddressBarViewDelegate *m_delegate = nullptr;
	MainFontSetter m_fontSetter;
	std::wstring m_currentText;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
