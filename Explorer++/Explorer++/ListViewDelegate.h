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

	virtual void OnItemsActivated(const std::vector<ListViewItem *> &items) = 0;
	virtual bool OnItemRenamed(ListViewItem *item, const std::wstring &name) = 0;
	virtual void OnItemsRemoved(const std::vector<ListViewItem *> &items,
		RemoveMode removeMode) = 0;
	virtual void OnItemsCopied(const std::vector<ListViewItem *> &items) = 0;
	virtual void OnItemsCut(const std::vector<ListViewItem *> &items) = 0;
	virtual void OnPaste(ListViewItem *lastSelectedItemOpt) = 0;

	virtual void OnShowBackgroundContextMenu(const POINT &ptScreen) = 0;
	virtual void OnShowItemContextMenu(const std::vector<ListViewItem *> &items,
		const POINT &ptScreen) = 0;
	virtual void OnShowHeaderContextMenu(const POINT &ptScreen) = 0;

	virtual void OnBeginDrag(const std::vector<ListViewItem *> &items) = 0;
};
