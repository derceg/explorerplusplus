// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SubMenuItemParams.h"
#include "IconModel.h"

SubMenuItemParams::SubMenuItemParams(const std::wstring &text) : m_text(text)
{
}

SubMenuItemParams &SubMenuItemParams::SetKey(const MenuItemKey &key) &
{
	m_key = key;
	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetKey(const MenuItemKey &key) &&
{
	return std::move(SetKey(key));
}

SubMenuItemParams &SubMenuItemParams::SetIconModel(std::unique_ptr<const IconModel> iconModel) &
{
	m_iconModel = std::move(iconModel);
	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetIconModel(std::unique_ptr<const IconModel> iconModel) &&
{
	return std::move(SetIconModel(std::move(iconModel)));
}

SubMenuItemParams &SubMenuItemParams::SetMiddleClickedCallback(
	MenuItemMiddleClickedCallback callback) &
{
	m_middleClickedCallback = std::move(callback);
	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetMiddleClickedCallback(
	MenuItemMiddleClickedCallback callback) &&
{
	return std::move(SetMiddleClickedCallback(std::move(callback)));
}

SubMenuItemParams &SubMenuItemParams::SetMiddleClickedCallback(std::function<void()> callback) &
{
	m_middleClickedCallback = [callback = std::move(callback)](bool, bool)
	{
		callback();
	};

	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetMiddleClickedCallback(std::function<void()> callback) &&
{
	return std::move(SetMiddleClickedCallback(std::move(callback)));
}

const std::optional<MenuItemKey> &SubMenuItemParams::GetKey() const
{
	return m_key;
}

const std::wstring &SubMenuItemParams::GetText() const
{
	return m_text;
}

std::unique_ptr<const IconModel> SubMenuItemParams::TakeIconModel()
{
	return std::move(m_iconModel);
}

MenuItemMiddleClickedCallback SubMenuItemParams::TakeMiddleClickedCallback()
{
	return std::move(m_middleClickedCallback);
}
