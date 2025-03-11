// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <optional>
#include <utility>

class BrowserWindow;
class NavigationRequest;
class ShellBrowser;

class NavigationEventScope
{
public:
	enum class Scope
	{
		Global,
		Browser,
		ShellBrowser
	};

	static NavigationEventScope Global();
	static NavigationEventScope ForBrowser(const BrowserWindow &browser);
	static NavigationEventScope ForShellBrowser(const ShellBrowser &shellBrowser);

	Scope GetScope() const;
	std::optional<int> GetBrowserId() const;
	std::optional<int> GetShellBrowserId() const;

private:
	NavigationEventScope();
	NavigationEventScope(const BrowserWindow &browser);
	NavigationEventScope(const ShellBrowser &shellBrowser);

	const Scope m_scope;
	const std::optional<int> m_browserId;
	const std::optional<int> m_shellBrowserId;
};

// Navigations can result in various success or failure states, summarized by the following cases:
//
// 1. Initial Navigation (Success)
//    1.1. The "navigation started" signal is triggered.
//    1.2. Directory enumeration succeeds.
//    1.3. The "navigation will commit" signal is triggered.
//    1.4. The "navigation committed" signal is triggered, making the requested folder the current
//         folder.
//
// 2. Initial Navigation (Failure)
//    2.1. The "navigation started" signal is triggered.
//    2.2. Directory enumeration fails.
//    2.3. The "navigation will commit" signal is triggered.
//    2.4. The "navigation committed" signal is triggered, making the requested folder the current
//         folder.
//
// 3. Initial Navigation (Cancellation)
//    3.1. The "navigation started" signal is triggered.
//    3.2. The navigation is stopped early.
//    3.3. The "navigation will commit" signal is triggered.
//    3.4. The "navigation committed" signal is triggered, making the requested folder the current
//         folder.
//
// 4. Subsequent Navigation (Success)
//    4.1. The "navigation started" signal is triggered.
//    4.2. Directory enumeration succeeds.
//    4.3. The "navigation will commit" signal is triggered.
//    4.4. The "navigation committed" signal is triggered, making the requested folder the current
//         folder.
//
// 5. Subsequent Navigation (Failure)
//    5.1. The "navigation started" signal is triggered.
//    5.2. Directory enumeration fails.
//    5.3. The "navigation failed" signal is triggered.
//
// 6. Subsequent Navigation (Cancellation)
//    6.1. The "navigation started" signal is triggered.
//    6.2. The navigation is stopped early.
//    6.3. The "navigation cancelled" signal is triggered.
class NavigationEvents : private boost::noncopyable
{
public:
	using NavigationSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser,
		const NavigationRequest *request)>;

	using StoppedSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser)>;

	enum class SlotGroup
	{
		HighestPriority = 0,
		HighPriority = 1,
		Default = 2
	};

	// Triggered when a navigation is initiated.
	boost::signals2::connection AddStartedObserver(const NavigationSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when it's known that a navigation will commit, but the commit hasn't yet occurred.
	boost::signals2::connection AddWillCommitObserver(const NavigationSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when the enumeration for a directory successfully finishes. At this point, the
	// requested folder has become the current folder and the enumerated items have been displayed.
	//
	// This can also be triggered if the initial navigation fails. Typically, if a navigation fails,
	// no folder change will occur. Instead, the original folder will continue to be shown. However,
	// when the first navigation occurs, there is no original folder. Because a folder always needs
	// to be displayed, the only effective option is to consider the failed navigation committed. In
	// that case, this event will be triggered, while the failed event won't be triggered.
	//
	// For the same reasons, this can also be triggered if the initial navigation is stopped early.
	boost::signals2::connection AddCommittedObserver(const NavigationSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when the enumeration for a navigation fails. If the initial navigation fails, this
	// won't be triggered, as the navigation will be committed instead.
	boost::signals2::connection AddFailedObserver(const NavigationSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when a navigation is stopped early. As noted above, if the initial navigation is
	// cancelled, this won't be triggered, as the navigation will be committed instead.
	boost::signals2::connection AddCancelledObserver(const NavigationSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when the pending navigations are requested to stop. At the point at which this
	// observer is invoked, the navigations will still be pending and may still be active.
	boost::signals2::connection AddStoppedObserver(const StoppedSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	void NotifyStarted(const ShellBrowser *shellBrowser, const NavigationRequest *request);
	void NotifyWillCommit(const ShellBrowser *shellBrowser, const NavigationRequest *request);
	void NotifyCommitted(const ShellBrowser *shellBrowser, const NavigationRequest *request);
	void NotifyFailed(const ShellBrowser *shellBrowser, const NavigationRequest *request);
	void NotifyCancelled(const ShellBrowser *shellBrowser, const NavigationRequest *request);

	void NotifyStopped(const ShellBrowser *shellBrowser);

private:
	template <typename Observer>
	static auto MakeFilteredObserver(Observer &&observer, const NavigationEventScope &scope)
	{
		return [observer = std::forward<Observer>(observer),
				   scope]<typename... Args>(const ShellBrowser *shellBrowser, Args &&...args)
		{
			if (!DoesEventMatchScope(scope, shellBrowser))
			{
				return;
			}

			observer(shellBrowser, std::forward<Args>(args)...);
		};
	}

	static bool DoesEventMatchScope(const NavigationEventScope &scope,
		const ShellBrowser *shellBrowser);

	NavigationSignal m_startedSignal;
	NavigationSignal m_willCommitSignal;
	NavigationSignal m_committedSignal;
	NavigationSignal m_failedSignal;
	NavigationSignal m_cancelledSignal;

	StoppedSignal m_stoppedSignal;
};
