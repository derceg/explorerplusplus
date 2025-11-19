// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

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
	SimulatedFileSystemItem(const std::wstring &name, ShellItemType itemType);

	PidlAbsolute GetPidl() const;
	SimulatedFileSystemItem *GetParent();

	void SetName(const std::wstring &name, std::optional<ShellItemType> itemType);

	SimulatedFileSystemItem *AddChild(std::unique_ptr<SimulatedFileSystemItem> item);
	std::unique_ptr<SimulatedFileSystemItem> RemoveChild(SimulatedFileSystemItem *item);
	concurrencpp::generator<const SimulatedFileSystemItem *> GetChildren() const;

	concurrencpp::generator<SimulatedFileSystemItem *> GetItemsDepthFirst();

private:
	std::wstring m_name;
	ShellItemType m_itemType;
	SimulatedFileSystemItem *m_parent = nullptr;
	std::vector<std::unique_ptr<SimulatedFileSystemItem>> m_children;
};
