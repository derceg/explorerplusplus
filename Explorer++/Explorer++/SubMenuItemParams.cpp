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

SubMenuItemParams &SubMenuItemParams::SetEnabledCallback(MenuItemEnabledCallback callback) &
{
	m_enabledCallback = std::move(callback);
	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetEnabledCallback(MenuItemEnabledCallback callback) &&
{
	return std::move(SetEnabledCallback(std::move(callback)));
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

SubMenuItemParams &SubMenuItemParams::SetRightClickedCallback(
	MenuItemRightClickedCallback callback) &
{
	m_rightClickedCallback = std::move(callback);
	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetRightClickedCallback(
	MenuItemRightClickedCallback callback) &&
{
	return std::move(SetRightClickedCallback(std::move(callback)));
}

SubMenuItemParams &SubMenuItemParams::SetDraggedCallback(MenuItemDraggedCallback callback) &
{
	m_draggedCallback = std::move(callback);
	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetDraggedCallback(MenuItemDraggedCallback callback) &&
{
	return std::move(SetDraggedCallback(std::move(callback)));
}

SubMenuItemParams &SubMenuItemParams::SetMaybeGetDropTargetCallback(
	MenuItemMaybeGetDropTargetCallback callback) &
{
	m_maybeGetDropTargetCallback = std::move(callback);
	return *this;
}

SubMenuItemParams &&SubMenuItemParams::SetMaybeGetDropTargetCallback(
	MenuItemMaybeGetDropTargetCallback callback) &&
{
	return std::move(SetMaybeGetDropTargetCallback(std::move(callback)));
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

MenuItemEnabledCallback SubMenuItemParams::TakeEnabledCallback()
{
	return std::move(m_enabledCallback);
}

MenuItemMiddleClickedCallback SubMenuItemParams::TakeMiddleClickedCallback()
{
	return std::move(m_middleClickedCallback);
}

MenuItemRightClickedCallback SubMenuItemParams::TakeRightClickedCallback()
{
	return std::move(m_rightClickedCallback);
}

MenuItemDraggedCallback SubMenuItemParams::TakeDraggedCallback()
{
	return std::move(m_draggedCallback);
}

MenuItemMaybeGetDropTargetCallback SubMenuItemParams::TakeMaybeGetDropTargetCallback()
{
	return std::move(m_maybeGetDropTargetCallback);
}
