// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <optional>

class BrowserWindow;
class Tab;

// When adding an observer, this class can be used to indicate when the observer should be triggered
// - on all tab events, or only those that occur in a specific browser window.
class TabEventScope
{
public:
	enum class Scope
	{
		Global,
		Browser
	};

	static TabEventScope Global();
	static TabEventScope ForBrowser(const BrowserWindow &browser);

	Scope GetScope() const;
	std::optional<int> GetBrowserId() const;

private:
	TabEventScope();
	TabEventScope(const BrowserWindow &browser);

	const Scope m_scope;
	const std::optional<int> m_browserId;
};

// When there are multiple browser windows, subscribing to tab events globally becomes more
// difficult. For example, to be notified of tab events in all windows, a class would have to
// subscribe both to tab events in a window, as well as window creation events (to allow
// subscription to tab events in the new window).
//
// It is possible to invert this dependency, so that the tab owner notifies a class whenever a
// particular event occurs. However, that flips the usual dependency relationship.
//
// This class acts as an intermediate. It means that a tab owner can indicate that some event
// occurred, without having an explicit dependency on a subscriber.
//
// It also means that a subscriber can be notified when a tab event happens, without having to know
// exactly where that event came from.
//
// This class is designed to outlive any individual tab. It's up to the owner to ensure that the
// instance is created before any tabs are created and destroyed after all tabs have been closed.
class TabEvents : private boost::noncopyable
{
public:
	using CreatedSignal = boost::signals2::signal<void(const Tab &tab, bool selected)>;
	using SelectedSignal = boost::signals2::signal<void(const Tab &tab)>;
	using MovedSignal = boost::signals2::signal<void(const Tab &tab, int fromIndex, int toIndex)>;
	using PreRemovalSignal = boost::signals2::signal<void(const Tab &tab, int index)>;
	using RemovedSignal = boost::signals2::signal<void(const Tab &tab)>;

	boost::signals2::connection AddCreatedObserver(const CreatedSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddSelectedObserver(const SelectedSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddMovedObserver(const MovedSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddPreRemovalObserver(const PreRemovalSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddRemovedObserver(const RemovedSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);

	void NotifyCreated(const Tab &tab, bool selected);
	void NotifySelected(const Tab &tab);
	void NotifyMoved(const Tab &tab, int fromIndex, int toIndex);
	void NotifyPreRemoval(const Tab &tab, int index);
	void NotifyRemoved(const Tab &tab);

private:
	template <typename Observer>
	static auto MakeFilteredObserver(Observer &&observer, const TabEventScope &scope)
	{
		return [observer = std::forward<Observer>(observer),
				   scope]<typename... Args>(const Tab &tab, Args &&...args)
		{
			if (!DoesEventMatchScope(scope, tab))
			{
				return;
			}

			observer(tab, std::forward<Args>(args)...);
		};
	}

	static bool DoesEventMatchScope(const TabEventScope &scope, const Tab &tab);

	CreatedSignal m_createdSignal;
	SelectedSignal m_selectedSignal;
	MovedSignal m_movedSignal;
	PreRemovalSignal m_preRemovalSignal;
	RemovedSignal m_removedSignal;
};
