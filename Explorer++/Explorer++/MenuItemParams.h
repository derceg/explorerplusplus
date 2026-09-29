// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuItemCallbacks.h"
#include "MenuItemKey.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>

class IconModel;

enum class MenuItemCheckStyle
{
	Normal,
	Radio
};

class MenuItemParams
{
public:
	MenuItemParams(const std::wstring &text);

	MenuItemParams(const MenuItemParams &) = delete;
	MenuItemParams &operator=(const MenuItemParams &) = delete;
	MenuItemParams(MenuItemParams &&) = default;
	MenuItemParams &operator=(MenuItemParams &&) = default;

	MenuItemParams &SetKey(const MenuItemKey &key) &;
	MenuItemParams &&SetKey(const MenuItemKey &key) &&;

	MenuItemParams &SetCheckStyle(MenuItemCheckStyle checkStyle) &;
	MenuItemParams &&SetCheckStyle(MenuItemCheckStyle checkStyle) &&;
	MenuItemParams &SetIconModel(std::unique_ptr<const IconModel> iconModel) &;
	MenuItemParams &&SetIconModel(std::unique_ptr<const IconModel> iconModel) &&;
	MenuItemParams &SetHelpText(const std::wstring &helpText) &;
	MenuItemParams &&SetHelpText(const std::wstring &helpText) &&;
	MenuItemParams &SetAcceleratorText(const std::optional<std::wstring> &acceleratorText) &;
	MenuItemParams &&SetAcceleratorText(const std::optional<std::wstring> &acceleratorText) &&;

	MenuItemParams &SetEnabledCallback(MenuItemEnabledCallback callback) &;
	MenuItemParams &&SetEnabledCallback(MenuItemEnabledCallback callback) &&;
	MenuItemParams &SetCheckedCallback(MenuItemCheckedCallback callback) &;
	MenuItemParams &&SetCheckedCallback(MenuItemCheckedCallback callback) &&;

	MenuItemParams &SetSelectedCallback(MenuItemSelectedCallback callback) &;
	MenuItemParams &&SetSelectedCallback(MenuItemSelectedCallback callback) &&;
	MenuItemParams &SetSelectedCallback(std::function<void()> callback) &;
	MenuItemParams &&SetSelectedCallback(std::function<void()> callback) &&;
	MenuItemParams &SetMiddleClickedCallback(MenuItemMiddleClickedCallback callback) &;
	MenuItemParams &&SetMiddleClickedCallback(MenuItemMiddleClickedCallback callback) &&;
	MenuItemParams &SetMiddleClickedCallback(std::function<void()> callback) &;
	MenuItemParams &&SetMiddleClickedCallback(std::function<void()> callback) &&;

	const std::optional<MenuItemKey> &GetKey() const;
	const std::wstring &GetText() const;
	MenuItemCheckStyle GetCheckStyle() const;
	std::unique_ptr<const IconModel> TakeIconModel();
	const std::wstring &GetHelpText() const;
	const std::optional<std::wstring> &GetAcceleratorText() const;
	MenuItemEnabledCallback TakeEnabledCallback();
	MenuItemCheckedCallback TakeCheckedCallback();
	MenuItemSelectedCallback TakeSelectedCallback();
	MenuItemMiddleClickedCallback TakeMiddleClickedCallback();

private:
	std::optional<MenuItemKey> m_key;
	std::wstring m_text;
	MenuItemCheckStyle m_checkStyle = MenuItemCheckStyle::Normal;
	std::unique_ptr<const IconModel> m_iconModel;
	std::wstring m_helpText;
	std::optional<std::wstring> m_acceleratorText;
	MenuItemEnabledCallback m_enabledCallback;
	MenuItemCheckedCallback m_checkedCallback;
	MenuItemSelectedCallback m_selectedCallback;
	MenuItemMiddleClickedCallback m_middleClickedCallback;
};
