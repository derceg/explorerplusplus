// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>

class BrowserWindow;
class Tab;

// When adding an observer, this class can be used to indicate when the observer should be triggered
// - on all tab events, or only those that occur in a specific browser window.
class TabEventScope
{
public:
	static TabEventScope ForBrowser(BrowserWindow *browser);
	static TabEventScope Global();

	const BrowserWindow *GetBrowser() const;

private:
	TabEventScope(BrowserWindow *browser);

	const BrowserWindow *const m_browser;
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
class GlobalTabEventDispatcher : private boost::noncopyable
{
public:
	using CreatedSignal = boost::signals2::signal<void(const Tab &tab, bool selected)>;
	using SelectedSignal = boost::signals2::signal<void(const Tab &tab)>;
	using PreRemovalSignal = boost::signals2::signal<void(const Tab &tab, int index)>;

	boost::signals2::connection AddCreatedObserver(const CreatedSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddSelectedObserver(const SelectedSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddPreRemovalObserver(const PreRemovalSignal::slot_type &observer,
		const TabEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);

	void NotifyCreated(const Tab &tab, bool selected);
	void NotifySelected(const Tab &tab);
	void NotifyPreRemoval(const Tab &tab, int index);

private:
	static std::optional<int> GetIdFromBrowser(const BrowserWindow *browser);
	static bool DoesBrowserMatch(std::optional<int> browserId, const Tab &tab);

	CreatedSignal m_createdSignal;
	SelectedSignal m_selectedSignal;
	PreRemovalSignal m_preRemovalSignal;
};
