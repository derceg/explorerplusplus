// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuItemParams.h"
#include "IconModel.h"

MenuItemParams::MenuItemParams(const std::wstring &text) : m_text(text)
{
}

MenuItemParams &MenuItemParams::SetKey(const MenuItemKey &key) &
{
	m_key = key;
	return *this;
}

MenuItemParams &&MenuItemParams::SetKey(const MenuItemKey &key) &&
{
	return std::move(SetKey(key));
}

MenuItemParams &MenuItemParams::SetCheckStyle(MenuItemCheckStyle checkStyle) &
{
	m_checkStyle = checkStyle;
	return *this;
}

MenuItemParams &&MenuItemParams::SetCheckStyle(MenuItemCheckStyle checkStyle) &&
{
	return std::move(SetCheckStyle(checkStyle));
}

MenuItemParams &MenuItemParams::SetIconModel(std::unique_ptr<const IconModel> iconModel) &
{
	m_iconModel = std::move(iconModel);
	return *this;
}

MenuItemParams &&MenuItemParams::SetIconModel(std::unique_ptr<const IconModel> iconModel) &&
{
	return std::move(SetIconModel(std::move(iconModel)));
}

MenuItemParams &MenuItemParams::SetHelpText(const std::wstring &helpText) &
{
	m_helpText = helpText;
	return *this;
}

MenuItemParams &&MenuItemParams::SetHelpText(const std::wstring &helpText) &&
{
	return std::move(SetHelpText(helpText));
}

MenuItemParams &MenuItemParams::SetAcceleratorText(
	const std::optional<std::wstring> &acceleratorText) &
{
	m_acceleratorText = acceleratorText;
	return *this;
}

MenuItemParams &&MenuItemParams::SetAcceleratorText(
	const std::optional<std::wstring> &acceleratorText) &&
{
	return std::move(SetAcceleratorText(acceleratorText));
}

MenuItemParams &MenuItemParams::SetEnabledCallback(MenuItemEnabledCallback callback) &
{
	m_enabledCallback = std::move(callback);
	return *this;
}

MenuItemParams &&MenuItemParams::SetEnabledCallback(MenuItemEnabledCallback callback) &&
{
	return std::move(SetEnabledCallback(std::move(callback)));
}

MenuItemParams &MenuItemParams::SetCheckedCallback(MenuItemCheckedCallback callback) &
{
	m_checkedCallback = std::move(callback);
	return *this;
}

MenuItemParams &&MenuItemParams::SetCheckedCallback(MenuItemCheckedCallback callback) &&
{
	return std::move(SetCheckedCallback(std::move(callback)));
}

MenuItemParams &MenuItemParams::SetSelectedCallback(MenuItemSelectedCallback callback) &
{
	m_selectedCallback = std::move(callback);
	return *this;
}

MenuItemParams &&MenuItemParams::SetSelectedCallback(MenuItemSelectedCallback callback) &&
{
	return std::move(SetSelectedCallback(std::move(callback)));
}

MenuItemParams &MenuItemParams::SetSelectedCallback(std::function<void()> callback) &
{
	m_selectedCallback = [callback = std::move(callback)](bool, bool)
	{
		callback();
	};

	return *this;
}

MenuItemParams &&MenuItemParams::SetSelectedCallback(std::function<void()> callback) &&
{
	return std::move(SetSelectedCallback(std::move(callback)));
}

MenuItemParams &MenuItemParams::SetMiddleClickedCallback(MenuItemMiddleClickedCallback callback) &
{
	m_middleClickedCallback = std::move(callback);
	return *this;
}

MenuItemParams &&MenuItemParams::SetMiddleClickedCallback(MenuItemMiddleClickedCallback callback) &&
{
	return std::move(SetMiddleClickedCallback(std::move(callback)));
}

MenuItemParams &MenuItemParams::SetMiddleClickedCallback(std::function<void()> callback) &
{
	m_middleClickedCallback = [callback = std::move(callback)](bool, bool)
	{
		callback();
	};

	return *this;
}

MenuItemParams &&MenuItemParams::SetMiddleClickedCallback(std::function<void()> callback) &&
{
	return std::move(SetMiddleClickedCallback(std::move(callback)));
}

const std::optional<MenuItemKey> &MenuItemParams::GetKey() const
{
	return m_key;
}

const std::wstring &MenuItemParams::GetText() const
{
	return m_text;
}

MenuItemCheckStyle MenuItemParams::GetCheckStyle() const
{
	return m_checkStyle;
}

std::unique_ptr<const IconModel> MenuItemParams::TakeIconModel()
{
	return std::move(m_iconModel);
}

const std::wstring &MenuItemParams::GetHelpText() const
{
	return m_helpText;
}

const std::optional<std::wstring> &MenuItemParams::GetAcceleratorText() const
{
	return m_acceleratorText;
}

MenuItemEnabledCallback MenuItemParams::TakeEnabledCallback()
{
	return std::move(m_enabledCallback);
}

MenuItemCheckedCallback MenuItemParams::TakeCheckedCallback()
{
	return std::move(m_checkedCallback);
}

MenuItemSelectedCallback MenuItemParams::TakeSelectedCallback()
{
	return std::move(m_selectedCallback);
}

MenuItemMiddleClickedCallback MenuItemParams::TakeMiddleClickedCallback()
{
	return std::move(m_middleClickedCallback);
}
