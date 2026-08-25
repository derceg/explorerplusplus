// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/RemoveMode.h"
#include <string>
#include <vector>

class ListViewItem;

// // Allows the ListView controller to be notified of events that occur within the view.
class ListViewDelegate
{
public:
	virtual ~ListViewDelegate() = default;

	virtual void OnItemsActivated(const std::vector<ListViewItem *> &items)
	{
		UNREFERENCED_PARAMETER(items);
	}

	virtual bool OnItemRenamed(ListViewItem *item, const std::wstring &name)
	{
		UNREFERENCED_PARAMETER(item);
		UNREFERENCED_PARAMETER(name);

		return false;
	}

	virtual void OnItemsRemoved(const std::vector<ListViewItem *> &items, RemoveMode removeMode)
	{
		UNREFERENCED_PARAMETER(items);
		UNREFERENCED_PARAMETER(removeMode);
	}

	virtual void OnItemsCopied(const std::vector<ListViewItem *> &items)
	{
		UNREFERENCED_PARAMETER(items);
	}

	virtual void OnItemsCut(const std::vector<ListViewItem *> &items)
	{
		UNREFERENCED_PARAMETER(items);
	}

	virtual void OnPaste(ListViewItem *lastSelectedItemOpt)
	{
		UNREFERENCED_PARAMETER(lastSelectedItemOpt);
	}

	virtual void OnShowBackgroundContextMenu(const POINT &ptScreen)
	{
		UNREFERENCED_PARAMETER(ptScreen);
	}

	virtual void OnShowItemContextMenu(const std::vector<ListViewItem *> &items,
		const POINT &ptScreen)
	{
		UNREFERENCED_PARAMETER(items);
		UNREFERENCED_PARAMETER(ptScreen);
	}

	virtual void OnBeginDrag(const std::vector<ListViewItem *> &items)
	{
		UNREFERENCED_PARAMETER(items);
	}
};
