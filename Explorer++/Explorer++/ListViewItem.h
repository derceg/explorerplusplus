// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ListViewColumn.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <optional>
#include <string>

// Acts as a base class for a single item in a ListView.
class ListViewItem : private boost::noncopyable
{
public:
	using UpdatedSignal = boost::signals2::signal<void()>;

	virtual ~ListViewItem() = default;

	virtual std::wstring GetColumnText(ListViewColumnId columnId) const = 0;
	virtual std::optional<int> GetIconIndex() const = 0;
	virtual bool CanRename() const = 0;
	virtual bool CanRemove() const = 0;

	// If true, indicates that the item represents a file (as opposed to a folder, or some other
	// type of item). In that case, filename cycling will be enabled when the item's text is being
	// edited. That is, F2 will cycle between the name, extension and name + extension.
	//
	// For any other type of item, pressing F2 while editing will have no effect.
	virtual bool IsFile() const = 0;

	[[nodiscard]] boost::signals2::connection AddUpdatedObserver(
		const typename UpdatedSignal::slot_type &observer)
	{
		return m_updatedSignal.connect(observer);
	}

protected:
	void NotifyUpdated()
	{
		m_updatedSignal();
	}

private:
	UpdatedSignal m_updatedSignal;
};
