// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "RebarBandView.h"
#include "ViewBase.h"
#include <optional>
#include <string>

class AddressBarViewDelegate;
struct Config;

class AddressBarView : public ViewBase, public RebarBandView
{
public:
	AddressBarView(HWND parent, const Config *config);

	void SetDelegate(AddressBarViewDelegate *delegate);

	// RebarBandView
	HWND GetHWND() const override;
	UINT GetHeight() const override;

	void Focus();
	std::wstring GetText() const;
	bool IsTextModified() const;
	void SelectAllText();
	void UpdateTextAndIcon(const std::optional<std::wstring> &optionalText, int iconIndex);
	void RevertText();
	void ShowDropDown();

	AddressBarViewDelegate *GetDelegateForTesting();
	void SetTextForTesting(const std::wstring &text);

private:
	static HWND CreateAddressBar(HWND parent);

	LRESULT ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND GetEditControl() const;
	void OnFontOrDpiUpdated();

	AddressBarViewDelegate *m_delegate = nullptr;
	MainFontSetter m_fontSetter;
	std::wstring m_currentText;
};
