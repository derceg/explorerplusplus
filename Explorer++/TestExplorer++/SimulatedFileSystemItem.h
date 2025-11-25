// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include "../Helper/ShellHelper.h"
#include <concurrencpp/concurrencpp.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class PidlAbsolute;

class SimulatedFileSystemItem
{
public:
	SimulatedFileSystemItem(const std::wstring &name, ShellItemType itemType,
		ShellItemExtraAttributes extraAttributes = ShellItemExtraAttributes::None);

	PidlAbsolute GetPidl() const;
	SimulatedFileSystemItem *GetParent();

	void SetName(const std::wstring &name, std::optional<ShellItemType> itemType);
	void SetExtraAttributes(ShellItemExtraAttributes extraAttributes);

	SimulatedFileSystemItem *AddChild(std::unique_ptr<SimulatedFileSystemItem> item);
	std::unique_ptr<SimulatedFileSystemItem> RemoveChild(SimulatedFileSystemItem *item);
	concurrencpp::generator<const SimulatedFileSystemItem *> GetChildren() const;

	concurrencpp::generator<SimulatedFileSystemItem *> GetItemsDepthFirst();

private:
	std::wstring m_name;
	ShellItemType m_itemType;
	ShellItemExtraAttributes m_extraAttributes;
	SimulatedFileSystemItem *m_parent = nullptr;
	std::vector<std::unique_ptr<SimulatedFileSystemItem>> m_children;
};
