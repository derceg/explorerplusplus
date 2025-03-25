// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "EventScope.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>

class ShellBrowser;

using ShellBrowserEventScope = EventScope<EventScopeMinimumLevel::ShellBrowser>;

class ShellBrowserEvents : private boost::noncopyable
{
public:
	using Signal = boost::signals2::signal<void(const ShellBrowser *shellBrowser)>;

	// Signaled when the displayed items change. That can occur when the contents of the underlying
	// directory change, as well as when items are filtered/unfiltered.
	boost::signals2::connection AddItemsChangedObserver(const Signal::slot_type &observer,
		const ShellBrowserEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);

	// Signaled when the properties of the directory itself change. For example, when the icon for
	// the directory changes, or the directory is renamed (if the directory is virtual).
	boost::signals2::connection AddDirectoryPropertiesChangedObserver(
		const Signal::slot_type &observer, const ShellBrowserEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);

	// Signaled when the listview selection changes.
	boost::signals2::connection AddSelectionChangedObserver(const Signal::slot_type &observer,
		const ShellBrowserEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);

	void NotifyItemsChanged(const ShellBrowser *shellBrowser);
	void NotifyDirectoryPropertiesChanged(const ShellBrowser *shellBrowser);
	void NotifySelectionChanged(const ShellBrowser *shellBrowser);

private:
	static auto MakeFilteredObserver(const Signal::slot_type &observer,
		const ShellBrowserEventScope &scope)
	{
		return [observer, scope](const ShellBrowser *shellBrowser)
		{
			if (!scope.DoesEventSourceMatch(*shellBrowser))
			{
				return;
			}

			observer(shellBrowser);
		};
	}

	Signal m_itemsChangedSignal;
	Signal m_directoryPropertiesChangedSignal;
	Signal m_selectionChangedSignal;
};
