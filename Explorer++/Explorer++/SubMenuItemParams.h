// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuItemCallbacks.h"
#include "MenuItemKey.h"
#include <optional>
#include <string>

class IconModel;

class SubMenuItemParams
{
public:
	SubMenuItemParams(const std::wstring &text);

	SubMenuItemParams(const SubMenuItemParams &) = delete;
	SubMenuItemParams &operator=(const SubMenuItemParams &) = delete;
	SubMenuItemParams(SubMenuItemParams &&) = default;
	SubMenuItemParams &operator=(SubMenuItemParams &&) = default;

	SubMenuItemParams &SetKey(const MenuItemKey &key) &;
	SubMenuItemParams &&SetKey(const MenuItemKey &key) &&;

	SubMenuItemParams &SetIconModel(std::unique_ptr<const IconModel> iconModel) &;
	SubMenuItemParams &&SetIconModel(std::unique_ptr<const IconModel> iconModel) &&;

	SubMenuItemParams &SetMiddleClickedCallback(MenuItemMiddleClickedCallback callback) &;
	SubMenuItemParams &&SetMiddleClickedCallback(MenuItemMiddleClickedCallback callback) &&;
	SubMenuItemParams &SetMiddleClickedCallback(std::function<void()> callback) &;
	SubMenuItemParams &&SetMiddleClickedCallback(std::function<void()> callback) &&;
	SubMenuItemParams &SetRightClickedCallback(MenuItemRightClickedCallback callback) &;
	SubMenuItemParams &&SetRightClickedCallback(MenuItemRightClickedCallback callback) &&;
	SubMenuItemParams &SetDraggedCallback(MenuItemDraggedCallback callback) &;
	SubMenuItemParams &&SetDraggedCallback(MenuItemDraggedCallback callback) &&;
	SubMenuItemParams &SetMaybeGetDropTargetCallback(MenuItemMaybeGetDropTargetCallback callback) &;
	SubMenuItemParams &&SetMaybeGetDropTargetCallback(
		MenuItemMaybeGetDropTargetCallback callback) &&;

	const std::optional<MenuItemKey> &GetKey() const;
	const std::wstring &GetText() const;
	std::unique_ptr<const IconModel> TakeIconModel();
	MenuItemMiddleClickedCallback TakeMiddleClickedCallback();
	MenuItemRightClickedCallback TakeRightClickedCallback();
	MenuItemDraggedCallback TakeDraggedCallback();
	MenuItemMaybeGetDropTargetCallback TakeMaybeGetDropTargetCallback();

private:
	std::optional<MenuItemKey> m_key;
	std::wstring m_text;
	std::unique_ptr<const IconModel> m_iconModel;
	MenuItemMiddleClickedCallback m_middleClickedCallback;
	MenuItemRightClickedCallback m_rightClickedCallback;
	MenuItemDraggedCallback m_draggedCallback;
	MenuItemMaybeGetDropTargetCallback m_maybeGetDropTargetCallback;
};
